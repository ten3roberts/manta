#ifndef BUFFER_H
#define BUFFER_H

#include <vulkan/vulkan.h>
#include "graphics/commandbuffer.h"


// Defines a buffer pool that can be used to create pools for different buffer types
struct BufferPoolBlock
{
	// Describes the currently used offset to the last used bytes of the buffer/memory
	uint32_t end;
	// Describes the total size of the buffer/memory that was allocated
	uint32_t alloc_size;
	VkBuffer buffer;
	VkDeviceMemory memory;
	struct BufferPoolFree* free_blocks;
};

// Describes a freed part of memory for a pool
struct BufferPoolFree
{
	VkBuffer buffer;
	VkDeviceMemory memory;
	// The offset to the start of the freed space
	uint32_t offset;
	uint32_t size;
	// A pointer to the next freed block
	struct BufferPoolFree* next;
};

// An array holding several buffer pools
typedef struct
{
	VkBufferUsageFlagBits usage;
	uint32_t alignment;

	// How many blocks are in the pool
	uint32_t block_count;
	struct BufferPoolBlock* blocks;
} BufferPool;

// Adds another buffer pool to a BufferPoolArray
// If it is empty, a buffer is created
// Creates the buffer according to type
// void buffer_pool_array_add(BufferPoolArray* array, uint32_t size);

// Retrieves an available pool able to hold > size
// Populates buffer, memory, and offset
// If no pool in array is free, the pool array is extended
// Satisfies alignment requirements
void buffer_pool_malloc(BufferPool* pool, uint32_t size, VkBuffer* buffer, VkDeviceMemory* memory, uint32_t* offset);

void buffer_pool_free(BufferPool* pool, uint32_t size, VkBuffer buffer, VkDeviceMemory memory, uint32_t offset);

// Destroys and frees all pools and buffers
void buffer_pool_array_destroy(BufferPool* pool);

uint32_t find_memory_type(uint32_t type_filter, VkMemoryPropertyFlags properties);

// Creates and allocates memory for a buffer
// If alignment != NULL, alignment will be filled with the required buffer alignment
// If corrected_size != NULL, corrected_size will be filled with the align-corrected size
int buffer_create(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer* buffer,
				  VkDeviceMemory* buffer_memory, uint32_t* alignment, uint32_t* corrected_size);

// Can be used to copy data to staging buffers
void buffer_copy(VkBuffer src, VkBuffer dst, VkDeviceSize size, uint32_t src_offset, uint32_t dst_offset);

// Command buffers
// Allocates and starts a single use command buffer
CommandBuffer single_use_commands_begin();

// Ends and frees a single time command buffer
void single_use_commands_end(CommandBuffer* command_buffer);

void image_create(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,
				  VkMemoryPropertyFlags properties, VkImage* image, VkDeviceMemory* memory,
				  VkSampleCountFlagBits num_samples);

VkImageView image_view_create(VkImage image, VkFormat format, VkImageAspectFlags aspect_flags);

void copy_buffer_to_image(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

void transition_image_layout(VkImage image, VkFormat format, VkImageLayout old_layout, VkImageLayout new_layout);
#endif
