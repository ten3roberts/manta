#include "graphics/texture.h"
#include "buffer.h"
#include "log.h"
#include "vulkan_members.h"
#include "magpie.h"
#include "stb_image.h"
#include "hashtable.h"

static hashtable_t* sampler_table;

struct SamplerInfo
{
	SamplerFilterMode filterMode;
	SamplerWrapMode wrapMode;
	int maxAnisotropy;
};

struct Sampler
{
	VkSampler vksampler;
	struct SamplerInfo info;
};

static uint32_t hash_sampler(const void* pkey)
{
	struct SamplerInfo* info = (struct SamplerInfo*)pkey;
	uint32_t result = 0;
	result += hashtable_hashfunc_uint32((uint32_t*)&info->filterMode);
	result += hashtable_hashfunc_uint32((uint32_t*)&info->wrapMode);
	result += hashtable_hashfunc_uint32((uint32_t*)&info->maxAnisotropy);

	return result;
}

static int32_t comp_sampler(const void* pkey1, const void* pkey2)
{
	struct SamplerInfo* info1 = (struct SamplerInfo*)pkey1;
	struct SamplerInfo* info2 = (struct SamplerInfo*)pkey2;

	return (info1->filterMode == info2->filterMode && info1->wrapMode == info2->wrapMode && info1->maxAnisotropy == info2->maxAnisotropy) == 0;
}

// Creates a sampler
Sampler* sampler_get(SamplerFilterMode filterMode, SamplerWrapMode wrapMode, int maxAnisotropy)
{
	// Create table if it doesn't exist
	if (sampler_table == NULL)
		sampler_table = hashtable_create(hash_sampler, comp_sampler);

	struct SamplerInfo samplerInfo = {.filterMode = filterMode, .wrapMode = wrapMode, .maxAnisotropy = maxAnisotropy};

	// Attempt to find sampler by info if it already exists
	Sampler* sampler = hashtable_find(sampler_table, (void*)&samplerInfo);

	if (sampler)
		return sampler;

	// Create sampler
	LOG("Creating new sampler");
	sampler = malloc(sizeof(Sampler));
	sampler->info = samplerInfo;

	// Insert into table
	hashtable_insert(sampler_table, &sampler->info, sampler);

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
	VkResult result = vkCreateSampler(device, &samplerCreateInfo, NULL, &sampler->vksampler);
	if (result != VK_SUCCESS)
	{
		LOG_E("Failed to create image sampler - code %d", result);
		return NULL;
	}
	return sampler;
}

VkSampler sampler_get_vksampler(Sampler* sampler)
{
	return sampler->vksampler;
}

void sampler_destroy(Sampler* sampler)
{
	hashtable_remove(sampler_table, &sampler->info);

	// Last texture was removed
	if (hashtable_get_count(sampler_table) == 0)
	{
		hashtable_destroy(sampler_table);
		sampler_table = NULL;
	}

	vkDestroySampler(device, sampler->vksampler, NULL);

	free(sampler);
}

void sampler_destroy_all()
{
	Sampler* sampler = NULL;
	while (sampler_table && (sampler = hashtable_pop(sampler_table)))
	{
		sampler_destroy(sampler);
	}
}

static hashtable_t* texture_table = NULL;

typedef struct Texture
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
} Texture;

// Loads a texture from a file
// The textures name is the full file path
Texture* texture_load(const char* file)
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
		return NULL;
	}

	// Save the name
	char name[256];
	snprintf(name, sizeof name, "%s", file);
	Texture* tex = texture_create(name, width, height, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_SAMPLE_COUNT_1_BIT,
								  VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);

	texture_update(tex, pixels);

	stbi_image_free(pixels);
	return tex;
}
// Creates a texture with no data
Texture* texture_create(const char* name, int width, int height, VkFormat format, VkImageUsageFlags usage, VkSampleCountFlagBits samples, VkImageLayout layout,
						VkImageAspectFlags imageAspect)
{
	Texture* tex = malloc(sizeof(Texture));

	if (name)
	{
		snprintf(tex->name, sizeof tex->name, "%s", name);

		// Create table if it doesn't exist
		if (texture_table == NULL)
		{
			texture_table = hashtable_create_string();
		}
		// Insert texture into tracking table after name is acquired
		if (hashtable_find(texture_table, tex->name) != NULL)
		{
			LOG_W("Duplicate material %s", tex->name);
			free(tex);
			return NULL;
		}
		// Insert into table
		hashtable_insert(texture_table, tex->name, tex);
	}
	else
	{
		snprintf(tex->name, sizeof name, "%s", "\0");
	}
	tex->width = width;
	tex->height = height;
	tex->layout = layout;
	tex->format = format;

	tex->size = image_create(tex->width, tex->height, format, VK_IMAGE_TILING_OPTIMAL, usage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &tex->vkimage, &tex->memory, samples);

	// Create image view
	tex->view = image_view_create(tex->vkimage, format, imageAspect);

	// Transition image if format is specified
	if (layout != VK_IMAGE_LAYOUT_UNDEFINED)
		transition_image_layout(tex->vkimage, format, VK_IMAGE_LAYOUT_UNDEFINED, layout);

	return tex;
}

Texture* texture_create_existing(const char* name, int width, int height, VkFormat format, VkSampleCountFlagBits samples, VkImageLayout layout, VkImage image, VkImageAspectFlags imageAspect)
{
	Texture* tex = malloc(sizeof(Texture));

	if (name)
	{
		snprintf(tex->name, sizeof tex->name, "%s", name);

		// Create table if it doesn't exist
		if (texture_table == NULL)
		{
			texture_table = hashtable_create_string();
		}
		// Insert texture into tracking table after name is acquired
		if (hashtable_find(texture_table, tex->name) != NULL)
		{
			LOG_W("Duplicate material %s", tex->name);
			free(tex);
			return NULL;
		}
		// Insert into table
		hashtable_insert(texture_table, tex->name, tex);
	}
	else
	{
		snprintf(tex->name, sizeof name, "%s", "\0");
	}
	tex->width = width;
	tex->height = height;
	tex->layout = layout;
	tex->format = format;

	// Get image size
	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(device, image, &memRequirements);

	tex->size = memRequirements.size;
	tex->vkimage = image;

	// Create image view
	tex->view = image_view_create(tex->vkimage, format, imageAspect);

	return tex;
}

// Updates the texture data from host memory
void texture_update(Texture* tex, uint8_t* pixeldata)
{ // Create a staging bufer
	VkBuffer staging_buffer;
	VkDeviceMemory staging_buffer_memory;
	VkDeviceSize image_size = tex->size;

	buffer_create(image_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &staging_buffer, &staging_buffer_memory,
				  NULL, NULL);

	// Transfer image data to host visible staging buffer
	void* data;
	vkMapMemory(device, staging_buffer_memory, 0, image_size, 0, &data);
	memcpy(data, pixeldata, image_size);
	vkUnmapMemory(device, staging_buffer_memory);

	// Transition image layout
	transition_image_layout(tex->vkimage, tex->format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

	copy_buffer_to_image(staging_buffer, tex->vkimage, tex->width, tex->height);

	// Transition image layout
	transition_image_layout(tex->vkimage, tex->format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, tex->layout);

	// Clean up staging buffer
	vkDestroyBuffer(device, staging_buffer, NULL);
	vkFreeMemory(device, staging_buffer_memory, NULL);
}

Texture* texture_get(const char* name)
{
	// Create table if it doesn't exist
	if (texture_table == NULL)
		texture_table = hashtable_create_string();

	// Attempt to find texture if it already exists
	Texture* tex = hashtable_find(texture_table, (void*)name);

	if (tex)
		return tex;

	// Load from file
	tex = texture_load(name);
	// File was not found
	if (tex == NULL)
		return NULL;

	return tex;
}

void texture_disown(Texture* tex)
{
	hashtable_remove(texture_table, tex->name);

	// Last texture was removed
	if (hashtable_get_count(texture_table) == 0)
	{
		hashtable_destroy(texture_table);
		texture_table = NULL;
	}
}

void texture_destroy(Texture* tex)
{
	hashtable_remove(texture_table, tex->name);

	// Last texture was removed
	if (hashtable_get_count(texture_table) == 0)
	{
		hashtable_destroy(texture_table);
		texture_table = NULL;
	}

	vkDestroyImage(device, tex->vkimage, NULL);
	vkFreeMemory(device, tex->memory, NULL);
	vkDestroyImageView(device, tex->view, NULL);

	free(tex);
}

void texture_destroy_all()
{
	Texture* tex = NULL;
	while (texture_table && (tex = hashtable_pop(texture_table)))
	{
		texture_destroy(tex);
	}
}

void* texture_get_image_view(Texture* tex)
{
	return tex->view;
}