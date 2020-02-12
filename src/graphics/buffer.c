#include "buffer.h"
#include "vulkan_internal.h"
#include "log.h"
#include <stdlib.h>
#include <math.h>

void buffer_pool_array_add(BufferPoolArray* array, uint32_t size)
{
	LOG_S("Creating new buffer pool with usage %d", array->usage);
	array->pools = realloc(array->pools, ++array->count * sizeof(BufferPool));
	BufferPool* pool = &array->pools[array->count - 1];
	if (array->pools == NULL)
	{
		LOG_E("Failed to allocate uniform buffer pool");
		return;
	}
	buffer_create(size, array->usage, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				  &pool->buffer, &pool->memory, &pool->alignment, NULL);
	pool->filled_size = 0;
	pool->alloc_size = size;
}

void buffer_pool_array_get(BufferPoolArray* array, uint32_t size, VkBuffer* buffer, VkDeviceMemory* memory,
						   uint32_t* offset)
{
	// No pool has been created yet
	if (array->count == 0)
	{
		buffer_pool_array_add(array, (array->preferred_size > size ? array->preferred_size : size));
	}

	for (int i = 0; i < array->count; i++)
	{
		// Pool is full or not large enough
		if (array->pools[i].filled_size + size > array->pools[i].alloc_size)
		{
			// At last pool
			if (i == array->count - 1)
			{
				buffer_pool_array_add(array, size * (array->preferred_size > size ? array->preferred_size : size));
			}
			continue;
		};

		// Occupy space
		*buffer = array->pools[i].buffer;
		*memory = array->pools[i].memory;
		*offset = array->pools[i].filled_size;
		array->pools[i].filled_size += ceil(size / (float)array->pools[i].alignment) * array->pools[i].alignment;
	}
}

void buffer_pool_array_destroy(BufferPoolArray* array)
{
	LOG_S("Destroy buffer pool array");
	for (int i = 0; i < array->count; i++)
	{
		vkDestroyBuffer(device, array->pools[i].buffer, NULL);
		vkFreeMemory(device, array->pools[i].memory, NULL);
	}
	free(array->pools);
	array->pools = NULL;
	array->count = 0;
}

// Buffer creation
uint32_t find_memory_type(uint32_t type_filter, VkMemoryPropertyFlags properties)
{
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(physical_device, &memProperties);
	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
	{
		if (type_filter & (1 << i) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
		{
			return i;
		}
	}
	LOG_E("Could not find a suitable memory type");
	return 0;
}

int buffer_create(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer* buffer,
				  VkDeviceMemory* buffer_memory, uint32_t* alignment, uint32_t* corrected_size)
{
	// Create the vertex buffer
	VkBufferCreateInfo bufferInfo = {0};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	bufferInfo.flags = 0;

	VkResult result = vkCreateBuffer(device, &bufferInfo, NULL, buffer);
	if (result != VK_SUCCESS)
	{
		LOG_E("Failed to create buffer");
		return -1;
	}
	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(device, *buffer, &memRequirements);
	if (alignment)
		*alignment = memRequirements.alignment;
	if (corrected_size)
		*corrected_size = memRequirements.size;

	VkMemoryAllocateInfo allocInfo = {0};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = find_memory_type(memRequirements.memoryTypeBits, properties);

	result = vkAllocateMemory(device, &allocInfo, NULL, buffer_memory);
	if (result != VK_SUCCESS)
	{
		LOG_E("Failed to allocate buffer memory");
		return -2;
	}

	// Associate the memory with the buffer
	vkBindBufferMemory(device, *buffer, *buffer_memory, 0);
	return 0;
}

void buffer_copy(VkBuffer src, VkBuffer dst, VkDeviceSize size, uint32_t src_offset, uint32_t dst_offset)
{
	VkCommandBuffer command_buffer = single_use_commands_begin();

	VkBufferCopy copyRegion = {0};
	copyRegion.srcOffset = src_offset;
	copyRegion.dstOffset = dst_offset;
	copyRegion.size = size;
	vkCmdCopyBuffer(command_buffer, src, dst, 1, &copyRegion);

	single_use_commands_end(command_buffer);
}

// Command Buffers
VkCommandBuffer single_use_commands_begin()
{
	VkCommandBufferAllocateInfo allocInfo = {0};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = command_pool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo = {0};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	return commandBuffer;
}

void single_use_commands_end(VkCommandBuffer command_buffer)
{
	vkEndCommandBuffer(command_buffer);

	VkSubmitInfo submitInfo = {0};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &command_buffer;

	vkQueueSubmit(graphics_queue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(graphics_queue);

	vkFreeCommandBuffers(device, command_pool, 1, &command_buffer);
}

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

VkImageView image_view_create(VkImage image, VkFormat format, VkImageAspectFlags aspect_flags)
{
	VkImageViewCreateInfo viewInfo = {0};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = image;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = format;
	viewInfo.subresourceRange.aspectMask = aspect_flags;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;
	VkImageView view;
	VkResult result = vkCreateImageView(device, &viewInfo, NULL, &view);
	if (result != VK_SUCCESS)
	{
		LOG_E("Failed to create texture image view - code %d", result);
		return NULL;
	}
	return view;
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

	// Automatically use the right image aspect bit
	if (new_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
	{
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

		if (has_stencil_component(format))
		{
			barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
		}
	}
	else
	{
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	}
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

	// Depth buffer
	else if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask =
			VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	}
	else
	{
		LOG_E("Unsupported layout transition");
		return;
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
