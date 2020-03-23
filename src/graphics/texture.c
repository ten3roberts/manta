#include "texture.h"
#include "buffer.h"
#include "log.h"
#include "vulkan_members.h"
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <stdlib.h>

typedef struct Texture
{
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

Texture* texture_create(const char* file)
{
	LOG_S("Loading texture %s", file);
	Texture* tex = malloc(sizeof(Texture));
	tex->pixels = stbi_load(file, &tex->width, &tex->height, &tex->channels, STBI_rgb_alpha);
	uint32_t image_size = tex->width * tex->height * 4;
	if (tex->pixels == NULL)
	{
		LOG_E("Failed to load texture %s", file);
		free(tex);
		return NULL;
	}

	// Create a staging bufer
	VkBuffer staging_buffer;
	VkDeviceMemory staging_buffer_memory;
	buffer_create(image_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &staging_buffer,
				  &staging_buffer_memory, NULL, NULL);

	// Transfer image data to host visible staging buffer
	void* data;
	vkMapMemory(device, staging_buffer_memory, 0, image_size, 0, &data);
	memcpy(data, tex->pixels, image_size);
	vkUnmapMemory(device, staging_buffer_memory);

	image_create(tex->width, tex->height, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL,
				 VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				 &tex->image, &tex->memory, VK_SAMPLE_COUNT_1_BIT);

	transition_image_layout(tex->image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED,
							VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	copy_buffer_to_image(staging_buffer, tex->image, tex->width, tex->height);

	transition_image_layout(tex->image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
							VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	// Clean up staging buffers
	vkDestroyBuffer(device, staging_buffer, NULL);
	vkFreeMemory(device, staging_buffer_memory, NULL);

	// Create image view
	tex->view = image_view_create(tex->image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);

	tex->sampler = sampler_create();

	// Delete raw pixels
	stbi_image_free(tex->pixels);
	tex->pixels = NULL;

	// ub_create_descriptor_sets(tex->descriptors, 1, NULL, NULL, 0, tex->view, tex->sampler);
	return tex;
}

void texture_destroy(Texture* tex)
{
	vkDestroyImage(device, tex->image, NULL);
	vkFreeMemory(device, tex->memory, NULL);
	vkDestroyImageView(device, tex->view, NULL);
	vkDestroySampler(device, tex->sampler, NULL);
	free(tex);
}

void* texture_get_image_view(Texture* tex)
{
	return tex->view;
}

void* texture_get_sampler(Texture* tex)
{
	return tex->sampler;
}