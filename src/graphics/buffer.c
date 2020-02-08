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

	VkBufferCopy copyRegion = {0};
	copyRegion.srcOffset = src_offset; // Optional
	copyRegion.dstOffset = dst_offset; // Optional
	copyRegion.size = size;
	vkCmdCopyBuffer(commandBuffer, src, dst, 1, &copyRegion);
	vkEndCommandBuffer(commandBuffer);

	// Execute command buffer
	VkSubmitInfo submitInfo = {0};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit(graphics_queue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(graphics_queue);
	vkFreeCommandBuffers(device, command_pool, 1, &commandBuffer);
}
