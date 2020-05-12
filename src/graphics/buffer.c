#include "buffer.h"
#include "vulkan_internal.h"
#include "log.h"
#include "magpie.h"
#include <math.h>

#define B_MILLION 1048576

void buffer_pool_add(BufferPool* pool, uint32_t size)
{
	LOG_S("Creating new buffer pool with usage %d", pool->usage);
	pool->blocks = realloc(pool->blocks, ++pool->block_count * sizeof(struct BufferPoolBlock));
	// Check for allocation errors
	if (pool->blocks == NULL)
	{
		LOG_E("Failed to allocate uniform buffer pool");
		return;
	}
	// Get a pointer to the new pool
	struct BufferPoolBlock* new_block = &pool->blocks[pool->block_count - 1];
	// Create the buffer and memory for vulkan
	buffer_create(size, pool->usage, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &new_block->buffer,
				  &new_block->memory, &pool->alignment, NULL);

	// No blocks have been allocated nor freed, so initialize to 0
	new_block->free_blocks = NULL;
	new_block->end = 0;
	new_block->alloc_size = size;
}

void buffer_pool_malloc(BufferPool* pool, uint32_t size, VkBuffer* buffer, VkDeviceMemory* memory, uint32_t* offset)
{
	// No pool has been created yet
	if (pool->block_count == 0)
	{
		// Create a new pool 64 times larger than requested size to support multiple allocation if < 1MB
		buffer_pool_add(pool, size < B_MILLION ? size * 64 : size);
	}

	// Iterate all blocks
	for (uint32_t i = 0; i < pool->block_count; i++)
	{
		// Check if it will fit in a free block
		// Will search for the closest fitting block to reoccupy small freed blocks first
		struct BufferPoolFree* best_fit = NULL;
		struct BufferPoolFree* best_fit_prev = NULL;
		uint32_t best_fit_margin = -1;
		struct BufferPoolFree* free_it = pool->blocks[i].free_blocks;
		struct BufferPoolFree* free_prev = NULL;
		while (free_it)
		{
			// Check if fits and is better than the best fit
			if (free_it->size - size < best_fit_margin)
			{
				best_fit = free_it;
				best_fit_prev = free_prev;
			}

			free_prev = free_it;
			free_it = free_it->next;
		}

		if (best_fit)
		{
			best_fit->size -= size;

			// Occupy space
			*buffer = best_fit->buffer;
			*memory = best_fit->memory;
			*offset = best_fit->offset;
			// Does not count to freed size since it was freed

			// Remove the freed block completely
			if (best_fit->size == 0)
			{
				best_fit_prev->next = best_fit->next;
				free(best_fit);
			}
		}

		// Pool is full or not large enough
		if (pool->blocks[i].end + size > pool->blocks[i].alloc_size)
		{
			// At last pool
			if (i == pool->block_count - 1)
			{
				buffer_pool_add(pool, size < B_MILLION ? size * 64 : size);
			}
			continue;
		};

		// Occupy space
		*buffer = pool->blocks[i].buffer;
		*memory = pool->blocks[i].memory;
		*offset = pool->blocks[i].end;
		// Satisfy alignment requirements
		pool->blocks[i].end += ceil(size / (float)pool->alignment) * pool->alignment;
	}
}

// Will iterate and merge adjacent free blocks
// This will only work if the blocks are sorted, which they are
int buffer_pool_merge_free(struct BufferPoolBlock* block)
{
	struct BufferPoolFree* cur = block->free_blocks;
	struct BufferPoolFree* next = cur->next;
	while (cur->next)
	{
		// Check if contiguous
		if (cur->offset + cur->size == cur->next->offset)
		{
			LOG_S("Merging adjacent buffer blocks");
			cur->size += cur->next->size;

			// Remove the next block since it was merged
			cur->next = next->next;
			free(next);
			return EXIT_SUCCESS;
		}
		cur = cur->next;
		next = cur->next;
	}
	return EXIT_FAILURE;
}

void buffer_pool_free(BufferPool* pool, uint32_t size, VkBuffer buffer, VkDeviceMemory memory, uint32_t offset)
{
	// Find the block
	struct BufferPoolBlock* block = NULL;
	for (uint32_t i = 0; i < pool->block_count; i++)
	{
		if (pool->blocks[i].buffer == buffer && pool->blocks[i].memory == memory)
		{
			block = &pool->blocks[i];
		}
	}
	if (block == NULL)
	{
		LOG_E("Failed to find the block buffer with offset %d and size %d was allocated from", offset, size);
		return;
	}

	struct BufferPoolFree* cur = block->free_blocks;

	// Handle beginning of list
	if (block->free_blocks == NULL)
	{
		struct BufferPoolFree* free_block = malloc(sizeof(struct BufferPoolFree));
		free_block->buffer = buffer;
		free_block->memory = memory;
		free_block->offset = offset;
		free_block->size = size;
		free_block->next = NULL;
		block->free_blocks = free_block;
		return;
	}
	// Loop through to end and insert or merge
	while (cur)
	{
		// Reached sorted insert point
		if (cur->offset < offset)
		{
			// Insert a new block at tail
			struct BufferPoolFree* free_block = malloc(sizeof(struct BufferPoolFree));
			free_block->buffer = buffer;
			free_block->memory = memory;
			free_block->offset = offset;
			free_block->size = size;

			// Insert
			free_block->next = cur->next;
			cur->next = free_block;

			// Merge if necessary
			buffer_pool_merge_free(block);
			return;
		}
		cur = cur->next;
	}
}

void buffer_pool_array_destroy(BufferPool* pool)
{
	LOG_S("Destroying buffer pool array");
	for (int i = 0; i < pool->block_count; i++)
	{
		// Remove all freed spaces
		struct BufferPoolFree* cur = pool->blocks[i].free_blocks;
		struct BufferPoolFree* next = NULL;
		while (cur)
		{
			next = cur->next;

			free(cur);

			cur = next;
		}

		vkDestroyBuffer(device, pool->blocks[i].buffer, NULL);
		vkFreeMemory(device, pool->blocks[i].memory, NULL);
	}
	free(pool->blocks);
	pool->blocks = NULL;
	pool->block_count = 0;
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

int buffer_create(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer* buffer, VkDeviceMemory* buffer_memory,
				  uint32_t* alignment, uint32_t* corrected_size)
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
	CommandBuffer commandbuffer = single_use_commands_begin();

	VkBufferCopy copyRegion = {0};
	copyRegion.srcOffset = src_offset;
	copyRegion.dstOffset = dst_offset;
	copyRegion.size = size;
	vkCmdCopyBuffer(commandbuffer.buffer, src, dst, 1, &copyRegion);

	single_use_commands_end(&commandbuffer);
}

// Command Buffers
CommandBuffer single_use_commands_begin()
{
	CommandBuffer commandbuffer = commandbuffer_create_primary(0, 0);

	commandbuffer_begin(&commandbuffer);

	return commandbuffer;
}

void single_use_commands_end(CommandBuffer* commandbuffer)
{
	commandbuffer_end(commandbuffer);
	commandbuffer_destroy(commandbuffer);
}

void image_create(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties,
				  VkImage* image, VkDeviceMemory* memory, VkSampleCountFlagBits num_samples)
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
	imageInfo.samples = num_samples;
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
	CommandBuffer commandbuffer = single_use_commands_begin();

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
	else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
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
		barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	}
	else
	{
		LOG_E("Unsupported layout transition");
		return;
	}

	vkCmdPipelineBarrier(commandbuffer.buffer, sourceStage, destinationStage, 0, 0, NULL, 0, NULL, 1, &barrier);

	single_use_commands_end(&commandbuffer);
}

void copy_buffer_to_image(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
{
	CommandBuffer commandbuffer = single_use_commands_begin();

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

	vkCmdCopyBufferToImage(commandbuffer.buffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

	single_use_commands_end(&commandbuffer);
}
