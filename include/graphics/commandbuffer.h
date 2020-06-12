#ifndef COMMANDBUFFER_H
#define COMMANDBUFFER_H
#include <vulkan/vulkan.h>
#include "defines.h"
#include <stdbool.h>
#include "handle.h"
#include "framebuffer.h"

DEFINE_HANDLE(Commandbuffer)

// Creates a secondary command buffer for each frame in the swapchain
// Takes in a thread index since recording of command buffers from the same queues can not be done in pararell
// The passed fence should be an existing fence of the primary command buffer the secondary is intended to execute on
// thread index need to be less than RENDERER_MAX_THREADS
Commandbuffer commandbuffer_create_secondary(uint8_t thread_idx, Commandbuffer primary, VkRenderPass renderPass, Framebuffer frameBuffer);

void commandbuffer_set_info(Commandbuffer commandbuffer, Commandbuffer primary, VkRenderPass renderPass, Framebuffer framebuffer);

// Creates a secondary command buffer for each frame in the swapchain
// Takes in a thread index since recording of command buffers from the same queues can not be done in pararell
// thread index need to be less than RENDERER_MAX_THREADS
// Creates a fence for the command buffer
// Return value should not be copied and only be freed with commandbuffer_destroy
Commandbuffer commandbuffer_create_primary(uint8_t thread_idx);

// This will reset and begin recording of a primary or secondary command buffer
// If command buffer is secondary, it will use the inheritance info stored in the command buffer
void commandbuffer_begin(Commandbuffer commandbuffer);

// This will end recording of a primary or secondary command buffer
// Will not submit command buffers
void commandbuffer_end(Commandbuffer commandbuffer);

void commandbuffer_submit(Commandbuffer commandbuffer);

// Returns the raw vulkan command buffer for the current frame
// Is going to be replaced with api-agnostic abstractions§
VkCommandBuffer commandbuffer_vk(Commandbuffer commandbuffer);

VkFence commandbuffer_fence(Commandbuffer commandbuffer);

// Commandbuffer is destroyed immediately if the fence is signaled, I.e; not in use
// If the command buffer is in use, it is queued for destruction and is tried again with commandbuffer_handle_destructions()
// If fence is NULL, the command buffer is assumed to not be in use
// Note: only primary command buffers destory the fence as secondary only inherit it
// Secondary command buffers need to be destroyed before the primary command buffer or have their fence set to VK_NULL_HANDLE
void commandbuffer_destroy(Commandbuffer commandbuffer);

// Destroys any queued command buffers that were in use when destroy was called
// Returns how many command buffers where freed, can safely be ignored
uint32_t commandbuffer_handle_destructions();

// Destroys all command pools at the end of the program
// Called once a frame by renderer
void commandbuffer_destroy_pools();

#endif