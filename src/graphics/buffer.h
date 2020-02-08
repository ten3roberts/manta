#pragma once

#include <vulkan/vulkan.h>

// Defines a buffer pool that can be used to create pools for different buffer types
typedef struct
{
	VkBufferUsageFlagBits usage;
	uint32_t alignment;
	// Describes the currently used size of the buffer/memory
	uint32_t filled_size;
	// Describes the total size of the buffer/memory that was allocated
	uint32_t alloc_size;
	VkBuffer buffer;
	VkDeviceMemory memory;
} BufferPool;

// An array holding several buffer pools
typedef struct
{
	VkBufferUsageFlagBits usage;
	// Indicates the minimum size of a new pool
	// If smaller than requested size, requested size is use
	// Is used when a buffer is created implictly with 'get'
	// New size of the allocated pool will be requested size * preferred_unit_count
	// This is to avoid allocating one pool per request
	uint32_t preferred_size;
	uint32_t count;
	BufferPool* pools;
} BufferPoolArray;

// Adds another buffer to a BufferPoolArray
// If it is empty, a buffer is created
// Creates the buffer according to type
void buffer_pool_array_add(BufferPoolArray* array, uint32_t size);

// Retrieves an available pool able to hold > size
// Populates buffer, memory, and offset
// If no pool in array is free, the pool array is extended
// Satisfies alignment requirements
void buffer_pool_array_get(BufferPoolArray* array, uint32_t size, VkBuffer* buffer, VkDeviceMemory* memory, uint32_t* offset);

// Destroys and frees all pools and buffers
void buffer_pool_array_destroy(BufferPoolArray* array);

// Creates and allocates memory for a buffer
// If alignment != NULL, alignment will be filled with the required buffer alignment
// If corrected_size != NULL, corrected_size will be filled with the align-corrected size
int buffer_create(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer* buffer,
				  VkDeviceMemory* buffer_memory, uint32_t* alignment, uint32_t* corrected_size);

// Can be used to copy data to staging buffers
void buffer_copy(VkBuffer src, VkBuffer dst, VkDeviceSize size, uint32_t src_offset, uint32_t dst_offset);