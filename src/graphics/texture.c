#include "graphics/texture.h"
#include "buffer.h"
#include "log.h"
#include "vulkan_members.h"
#include "magpie.h"
#include "stb_image.h"
#include "handlepool.h"

struct SamplerInfo
{
	SamplerFilterMode filterMode;
	SamplerWrapMode wrapMode;
	int maxAnisotropy;
};

typedef struct Sampler_raw
{
	VkSampler vksampler;
	struct SamplerInfo info;
} Sampler_raw;

static int32_t comp_sampler(const struct SamplerInfo* info1, const struct SamplerInfo* info2)
{
	return (info1->filterMode == info2->filterMode && info1->wrapMode == info2->wrapMode && info1->maxAnisotropy == info2->maxAnisotropy) == 0;
}

static handlepool_t sampler_pool = HANDLEPOOL_INIT(sizeof(Sampler_raw));

// Creates a sampler
Sampler sampler_get(SamplerFilterMode filterMode, SamplerWrapMode wrapMode, int maxAnisotropy)
{
	// Look if sampler already exists
	struct SamplerInfo samplerInfo = {.filterMode = filterMode, .wrapMode = wrapMode, .maxAnisotropy = maxAnisotropy};

	for (uint32_t i = 0; i < sampler_pool.size; i++)
	{
		if (HANDLEPOOL_INDEX((&sampler_pool), i)->next != NULL)
			continue;

		Sampler_raw* found = (Sampler_raw*)HANDLEPOOL_INDEX((&sampler_pool), i)->data;
		if (comp_sampler(&samplerInfo, &found->info) == 0)
		{
			return PUN_HANDLE(HANDLEPOOL_INDEX((&sampler_pool), i)->handle, Sampler);
		}
	}

	// Create new sampler
	const struct handle_wrapper* wrapper = handlepool_alloc(&sampler_pool);
	Sampler_raw* raw = (Sampler_raw*)wrapper->data;
	Sampler handle = PUN_HANDLE(wrapper->handle, Sampler);

	// Create sampler
	LOG("Creating new sampler");
	raw->info = samplerInfo;

	VkSamplerCreateInfo samplerCreateInfo = {0};

	samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;

	// Filtering
	switch (filterMode)
	{
	case SAMPLER_FILTER_LINEAR:
		samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
		break;
	case SAMPLER_FILTER_NEAREST:
		samplerCreateInfo.magFilter = VK_FILTER_NEAREST;
		break;
	default:
		LOG_E("Invalid sampler filter mode %d", filterMode);
	}

	samplerCreateInfo.minFilter = samplerCreateInfo.magFilter;

	// Tiling
	switch (wrapMode)
	{
	case SAMPLER_WRAP_REPEAT:
		samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		break;
	case SAMPLER_WRAP_REPEAT_MIRROR:
		samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
		break;
	case SAMPLER_WRAP_CLAMP_EDGE:
		samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		break;
	case SAMPLER_WRAP_CLAMP_BORDER:
		samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
		break;
	default:
		LOG_E("Invalid sampler wrap mode %d", wrapMode);
	}

	samplerCreateInfo.addressModeV = samplerCreateInfo.addressModeU;
	samplerCreateInfo.addressModeW = samplerCreateInfo.addressModeU;

	// Anisotropic filtering
	samplerCreateInfo.anisotropyEnable = maxAnisotropy > 1 ? VK_TRUE : VK_FALSE;
	samplerCreateInfo.maxAnisotropy = maxAnisotropy;

	samplerCreateInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;

	// Texels are adrees using 0 - 1, not 0 - texWidth
	samplerCreateInfo.unnormalizedCoordinates = VK_FALSE;

	// Can be used for precentage-closer filtering
	samplerCreateInfo.compareEnable = VK_FALSE;
	samplerCreateInfo.compareOp = VK_COMPARE_OP_ALWAYS;

	// Mipmapping
	samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerCreateInfo.mipLodBias = 0.0f;
	samplerCreateInfo.minLod = 0.0f;
	samplerCreateInfo.maxLod = 0.0f;

	// Samplers are not combined with one specific image
	VkResult result = vkCreateSampler(device, &samplerCreateInfo, NULL, &raw->vksampler);
	if (result != VK_SUCCESS)
	{
		LOG_E("Failed to create image sampler - code %d", result);
		return INVALID(Sampler);
	}
	return handle;
}

VkSampler sampler_get_vksampler(Sampler sampler)
{
	return ((Sampler_raw*)handlepool_get_raw(&sampler_pool, PUN_HANDLE(sampler, GenericHandle)))->vksampler;
}

void sampler_destroy(Sampler sampler)
{
	Sampler_raw* raw = handlepool_get_raw(&sampler_pool, PUN_HANDLE(sampler, GenericHandle));

	vkDestroySampler(device, raw->vksampler, NULL);

	handlepool_free(&sampler_pool, PUN_HANDLE(sampler, GenericHandle));
}

void sampler_destroy_all()
{
	for (uint32_t i = 0; i < sampler_pool.size; i++)
	{
		if (HANDLEPOOL_INDEX((&sampler_pool), i)->next != NULL)
			continue;

		sampler_destroy(PUN_HANDLE(HANDLEPOOL_INDEX((&sampler_pool), i)->handle, Sampler));
	}
}

typedef struct Texture_raw
{
	// Name should not be modified after creation
	char name[256];
	int width;
	int height;
	// The allocated size of the image
	// May be slightly larger than width*height*channels due to alignment requirements
	VkDeviceSize size;
	VkImageLayout layout;
	VkFormat format;
	VkImage vkimage;
	VkDeviceMemory memory;
	VkImageView view;
	// If set to true, the texture owns the vkimage and will free it on destruction
	bool owns_image;
} Texture_raw;

static handlepool_t texture_pool = HANDLEPOOL_INIT(sizeof(Texture_raw));

// Loads a texture from a file
// The textures name is the full file path
Texture texture_load(const char* file)
{
	LOG_S("Loading texture %s", file);

	uint8_t* pixels;
	int width = 0, height = 0, channels = 0;

	// Solid color requested
	// Create a solid white 256*256 texture
	if (strcmp(file, "col:white") == 0)
	{
		width = 256;
		height = 256;
		pixels = calloc(4 * width * height, sizeof(*pixels));
		memset(pixels, 255, 4 * width * height);
	}
	else
	{
		pixels = stbi_load(file, &width, &height, &channels, STBI_rgb_alpha);
	}

	if (pixels == NULL)
	{
		LOG_E("Failed to load texture %s", file);
		stbi_image_free(pixels);
		return (Texture){.index = -1, .pattern = -1};
	}

	// Save the name
	char name[256];
	snprintf(name, sizeof name, "%s", file);
	Texture tex = texture_create(name, width, height, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_SAMPLE_COUNT_1_BIT,
								 VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);

	texture_update(tex, pixels);

	stbi_image_free(pixels);
	return tex;
}

// Creates a texture with no data
Texture texture_create(const char* name, int width, int height, VkFormat format, VkImageUsageFlags usage, VkSampleCountFlagBits samples, VkImageLayout layout,
					   VkImageAspectFlags imageAspect)
{
	const struct handle_wrapper* wrapper = handlepool_alloc(&texture_pool);
	Texture_raw* raw = (Texture_raw*)wrapper->data;
	Texture handle = PUN_HANDLE(wrapper->handle, Texture);

	if (name)
	{
		snprintf(raw->name, sizeof raw->name, "%s", name);
	}
	else
	{
		snprintf(raw->name, sizeof raw->name, "%s", "unnamed");
	}

	raw->width = width;
	raw->height = height;
	raw->layout = layout;
	raw->format = format;
	raw->owns_image = true;

	raw->size = image_create(raw->width, raw->height, format, VK_IMAGE_TILING_OPTIMAL, usage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &raw->vkimage, &raw->memory, samples);

	// Create image view
	raw->view = image_view_create(raw->vkimage, format, imageAspect);

	// Transition image if format is specified
	if (layout != VK_IMAGE_LAYOUT_UNDEFINED)
		transition_image_layout(raw->vkimage, format, VK_IMAGE_LAYOUT_UNDEFINED, layout);

	return handle;
}

Texture texture_create_existing(const char* name, int width, int height, VkFormat format, VkSampleCountFlagBits samples, VkImageLayout layout, VkImage image, VkImageAspectFlags imageAspect)
{
	const struct handle_wrapper* wrapper = handlepool_alloc(&texture_pool);
	Texture_raw* raw = (Texture_raw*)wrapper->data;
	Texture handle = PUN_HANDLE(wrapper->handle, Texture);

	if (name)
	{
		snprintf(raw->name, sizeof raw->name, "%s", name);
	}
	else
	{
		snprintf(raw->name, sizeof raw->name, "%s", "unnamed");
	}

	raw->width = width;
	raw->height = height;
	raw->layout = layout;
	raw->format = format;
	raw->owns_image = false;

	// Get image size
	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(device, image, &memRequirements);

	raw->size = memRequirements.size;
	raw->vkimage = image;

	// Create image view
	raw->view = image_view_create(raw->vkimage, format, imageAspect);

	return handle;
}

// Updates the texture data from host memory
void texture_update(Texture tex, uint8_t* pixeldata)
{ // Create a staging bufer
	VkBuffer staging_buffer;
	VkDeviceMemory staging_buffer_memory;
	Texture_raw* raw = (Texture_raw*)handlepool_get_raw(&texture_pool, PUN_HANDLE(tex, GenericHandle));
	VkDeviceSize image_size = raw->size;

	buffer_create(image_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &staging_buffer, &staging_buffer_memory,
				  NULL, NULL);

	// Transfer image data to host visible staging buffer
	void* data;
	vkMapMemory(device, staging_buffer_memory, 0, image_size, 0, &data);
	memcpy(data, pixeldata, image_size);
	vkUnmapMemory(device, staging_buffer_memory);

	// Transition image layout
	transition_image_layout(raw->vkimage, raw->format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

	copy_buffer_to_image(staging_buffer, raw->vkimage, raw->width, raw->height);

	// Transition image layout
	transition_image_layout(raw->vkimage, raw->format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, raw->layout);

	// Clean up staging buffer
	vkDestroyBuffer(device, staging_buffer, NULL);
	vkFreeMemory(device, staging_buffer_memory, NULL);
}

Texture texture_get(const char* name)
{
	for (uint32_t i = 0; i < texture_pool.size; i++)
	{
		if (HANDLEPOOL_INDEX((&texture_pool), i)->next != NULL)
			continue;

		Texture_raw* raw = (Texture_raw*)HANDLEPOOL_INDEX((&texture_pool), i)->data;
		if (strcmp(name, raw->name) == 0)
		{
			return PUN_HANDLE(HANDLEPOOL_INDEX((&texture_pool), i)->handle, Texture);
		}
	}

	// Load from file
	return texture_load(name);
}

void texture_destroy(Texture tex)
{
	Texture_raw* raw = handlepool_get_raw(&texture_pool, tex);

	if (raw->owns_image)
	{
		vkDestroyImage(device, raw->vkimage, NULL);
		vkFreeMemory(device, raw->memory, NULL);
	}
	vkDestroyImageView(device, raw->view, NULL);
	handlepool_free(&texture_pool, tex);
}

void texture_destroy_all()
{
	for (uint32_t i = 0; i < texture_pool.size; i++)
	{
		if (HANDLEPOOL_INDEX((&texture_pool), i)->next != NULL)
			continue;

		texture_destroy(PUN_HANDLE(HANDLEPOOL_INDEX((&texture_pool), i)->handle, Texture));
	}
}

void* texture_get_image_view(Texture tex)
{
	return ((Texture_raw*)handlepool_get_raw(&texture_pool, tex))->view;
}