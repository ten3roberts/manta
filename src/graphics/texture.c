#include "texture.h"
#include "buffer.h"
#include "log.h"
#include "vulkan_members.h"
#include "uniformbuffer.h"
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

void image_create(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,
				  VkMemoryPropertyFlags properties, VkImage* image, VkDeviceMemory* memory)
{
	// Create the VKImage
	VkImageCreateInfo imageInfo = {0};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = width;
	imageInfo.extent.height = height;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;

	imageInfo.format = format;

	imageInfo.tiling = tiling;

	// Initial image layout is undefined
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	imageInfo.usage = usage;

	// Image will only be used by one queue
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	// Samples is irrelevant for images not used as frambuffer attachments
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.flags = 0; // Optional

	VkResult result = vkCreateImage(device, &imageInfo, NULL, image);
	if (result != VK_SUCCESS)
	{
		LOG_E("Failed to create image - code %d", result);
	}

	// Allocate memory for the image buffer
	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(device, *image, &memRequirements);

	VkMemoryAllocateInfo allocInfo = {0};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = find_memory_type(memRequirements.memoryTypeBits, properties);

	if (vkAllocateMemory(device, &allocInfo, NULL, memory) != VK_SUCCESS)
	{
		LOG_E("Failed to allocate image memory");
	}

	vkBindImageMemory(device, *image, *memory, 0);
}

void transition_image_layout(VkImage image, VkFormat format, VkImageLayout old_layout, VkImageLayout new_layout)
{
	VkCommandBuffer command_buffer = single_use_commands_begin();

	VkImageMemoryBarrier barrier = {0};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = old_layout;
	barrier.newLayout = new_layout;

	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

	barrier.image = image;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;

	VkPipelineStageFlags sourceStage;
	VkPipelineStageFlags destinationStage;
	if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
			 new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else
	{
		LOG_E("Unsupported layout transition");
	}

	vkCmdPipelineBarrier(command_buffer, sourceStage, destinationStage, 0, 0, NULL, 0, NULL, 1, &barrier);

	single_use_commands_end(command_buffer);
}

void copy_buffer_to_image(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
{
	VkCommandBuffer command_buffer = single_use_commands_begin();

	VkBufferImageCopy region = {0};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;

	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;

	region.imageOffset = (VkOffset3D){0, 0, 0};
	region.imageExtent = (VkExtent3D){width, height, 1};

	vkCmdCopyBufferToImage(command_buffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

	single_use_commands_end(command_buffer);
}

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
	const stbi_uc* p = tex->pixels + (4 * (10 * tex->width + 15));
	int r = p[0];
	int g = p[1];
	int b = p[2];
	int a = p[3];

	LOG("%d, %d, %d", r, g, b);

	image_create(tex->width, tex->height, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL,
				 VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				 &tex->image, &tex->memory);


	transition_image_layout(tex->image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED,
							VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	copy_buffer_to_image(staging_buffer, tex->image, tex->width, tex->height);

	transition_image_layout(tex->image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
							VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	// Clean up staging buffers
	vkDestroyBuffer(device, staging_buffer, NULL);
	vkFreeMemory(device, staging_buffer_memory, NULL);

	// Create image view
	tex->view = image_view_create(tex->image, VK_FORMAT_R8G8B8A8_SRGB);

	tex->sampler = sampler_create();

	// Delete raw pixels
	stbi_image_free(tex->pixels);
	tex->pixels = NULL;

	// ub_create_descriptor_sets(tex->descriptors, 1, NULL, NULL, 0, tex->view, tex->sampler);
	return tex;
}

void texture_bind(Texture* tex, VkCommandBuffer command_buffer, int i)
{
	// vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1,
	// &tex->descriptors[i], 0, NULL);
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