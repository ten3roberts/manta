#ifndef COMMANDBUFFER_H
#define COMMANDBUFFER_H
#include <vulkan/vulkan.h>
#include "defines.h"
#include <stdbool.h>

typedef struct
{
	VkCommandBuffer buffer;
	VkCommandBufferLevel level;
	// The frame in the swapchain the commandbuffer corresponds to
	// Set by the user
	uint8_t frame;
	// Signifies which pool it was allocated from
	// Should not be changed
	uint8_t thread_idx;
	bool recording;
	VkCommandBufferInheritanceInfo inheritanceInfo;
} CommandBuffer;

// Creates a secondary command buffer
// Takes in a thread index since recording of command buffers from the same queues can not be done in pararell
// thread index need to be less than RENDERER_MAX_THREADS
// Passed by copy but should be passes by reference to other functions for performance
CommandBuffer commandbuffer_create_secondary(uint8_t thread_idx, uint32_t frame, VkRenderPass renderPass, VkFramebuffer frameBuffer);

CommandBuffer commandbuffer_create_primary(uint8_t thread_idx, uint32_t frame);

// This will reset and begin recording of a primary or secondary command buffer
// If command buffer is secondary, it will use the inheritance info stored in the command buffer
void commandbuffer_begin(CommandBuffer* commandbuffer);

// This will end recording of a primary or secondary command buffer
// Will not submit command buffers
void commandbuffer_end(CommandBuffer* commandbuffer);

void commandbuffer_submit(CommandBuffer* commandbuffer);

void commandbuffer_destroy(CommandBuffer* commandbuffer);

// Destroys all command pools at the end of the program
void commandbuffer_destroy_pools();

#endif