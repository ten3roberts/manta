#include "graphics/texture.h"
#include "buffer.h"
#include "log.h"
#include "vulkan_members.h"
#include "magpie.h"
#include "stb_image.h"
#include "hashtable.h"

static hashtable_t* texture_table = NULL;

typedef struct Texture
{
	// Name should not be modified after creation
	char name[256];
	int width;
	int height;
	int channels;
	stbi_uc* pixels;
	VkImage image;
	VkDeviceMemory memory;
	VkImageView view;
	VkSampler sampler;
	VkDescriptorSet descriptors[3];
} Texture;

VkSampler sampler_create()
{
	VkSamplerCreateInfo samplerInfo = {0};

	// Linear of pixel art, downscaling; upscaling
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;

	// Tiling
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

	// Anisotropic filtering
	samplerInfo.anisotropyEnable = VK_TRUE;
	samplerInfo.maxAnisotropy = 16;

	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;

	// Texels are adrees using 0 - 1, not 0 - texWidth
	samplerInfo.unnormalizedCoordinates = VK_FALSE;

	// Can be used for precentage-closer filtering
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

	// Mipmapping
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = 0.0f;
	VkSampler sampler;
	// Samplers are not combined with one specific image
	VkResult result = vkCreateSampler(device, &samplerInfo, NULL, &sampler);
	if (result != VK_SUCCESS)
	{
		LOG_E("Failed to create image sampler - code %d", result);
		return NULL;
	}
	return sampler;
}

// Only used internally since textures are only loaded once
static Texture* texture_load(const char* file)
{
	LOG_S("Loading texture %s", file);
	Texture* tex = malloc(sizeof(Texture));
	// Solid color requested
	// Create a solid white 256*256 texture
	if (strcmp(file, "col:white") == 0)
	{
		tex->width = 256;
		tex->height = 256;
		tex->pixels = calloc(4 * tex->width * tex->height, sizeof(tex->pixels));
		memset(tex->pixels, 255, 4 * tex->width * tex->height);
	}
	else
	{
		tex->pixels = stbi_load(file, &tex->width, &tex->height, &tex->channels, STBI_rgb_alpha);
	}
	uint32_t image_size = tex->width * tex->height * 4;
	if (tex->pixels == NULL)
	{
		LOG_E("Failed to load texture %s", file);
		free(tex);
		return NULL;
	}

	// Save the name
	snprintf(tex->name, sizeof tex->name, "%s", file);

	// Create a staging bufer
	VkBuffer staging_buffer;
	VkDeviceMemory staging_buffer_memory;
	buffer_create(image_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &staging_buffer, &staging_buffer_memory,
				  NULL, NULL);

	// Transfer image data to host visible staging buffer
	void* data;
	vkMapMemory(device, staging_buffer_memory, 0, image_size, 0, &data);
	memcpy(data, tex->pixels, image_size);
	vkUnmapMemory(device, staging_buffer_memory);

	image_create(tex->width, tex->height, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
				 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &tex->image, &tex->memory, VK_SAMPLE_COUNT_1_BIT);

	transition_image_layout(tex->image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	copy_buffer_to_image(staging_buffer, tex->image, tex->width, tex->height);

	transition_image_layout(tex->image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	// Clean up staging buffers
	vkDestroyBuffer(device, staging_buffer, NULL);
	vkFreeMemory(device, staging_buffer_memory, NULL);

	// Create image view
	tex->view = image_view_create(tex->image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);

	tex->sampler = sampler_create();

	// Delete raw pixels
	// Since this is just free(), it doesn't matter how the texture was created
	stbi_image_free(tex->pixels);

	tex->pixels = NULL;

	// ub_create_descriptor_sets(tex->descriptors, 1, NULL, NULL, 0, tex->view, tex->sampler);
	return tex;
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

	hashtable_insert(texture_table, (void*)tex->name, tex);
	return tex;
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

	vkDestroyImage(device, tex->image, NULL);
	vkFreeMemory(device, tex->memory, NULL);
	vkDestroyImageView(device, tex->view, NULL);
	vkDestroySampler(device, tex->sampler, NULL);

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

void* texture_get_sampler(Texture* tex)
{
	return tex->sampler;
}