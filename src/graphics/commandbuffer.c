#include "graphics/commandbuffer.h"
#include "graphics/vulkan_members.h"
#include "log.h"

static VkCommandPool commandpools[RENDERER_MAX_THREADS] = {0};

static int commandpool_create(uint8_t thread_idx)
{
	QueueFamilies queueFamilyIndices = get_queue_families(physical_device);

	VkCommandPoolCreateInfo poolInfo = {0};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = queueFamilyIndices.graphics;
	// Enables the renderer to individually reset command buffers
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; // Optional
	VkResult result = vkCreateCommandPool(device, &poolInfo, NULL, &commandpools[thread_idx]);
	if (result != VK_SUCCESS)
	{
		LOG_E("Failed to create command pool - code %d", result);
		return -1;
	}
	return 0;
}

CommandBuffer commandbuffer_create_secondary(uint8_t thread_idx)
{
	if (thread_idx >= RENDERER_MAX_THREADS)
	{
		LOG_E("Thread index of %d is greater than the maximum number of threads %d", thread_idx, RENDERER_MAX_THREADS);
		return (CommandBuffer){0};
	}

	// Create command pool if it doesn't exist
	if (commandpools[thread_idx] == NULL)
	{
		commandpool_create(thread_idx);
	}

	// Create command buffer

	// Allocate primary command buffer
	VkCommandBufferAllocateInfo allocInfo = {0};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = commandpools[thread_idx];
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
	allocInfo.commandBufferCount = 1;

	CommandBuffer commandbuffer = {.thread_idx = thread_idx, .frame = 0, .level = VK_COMMAND_BUFFER_LEVEL_SECONDARY};
	VkResult result = vkAllocateCommandBuffers(device, &allocInfo, &commandbuffer.buffer);

	if (result != VK_SUCCESS)
	{
		LOG_E("Failed to create command buffers - code %d", result);
		return (CommandBuffer){0};
	}
	return commandbuffer;
}

CommandBuffer commandbuffer_create_primary(uint8_t thread_idx)
{
	if (thread_idx >= RENDERER_MAX_THREADS)
	{
		LOG_E("Thread index of %d is greater than the maximum number of threads %d", thread_idx, RENDERER_MAX_THREADS);
		return (CommandBuffer){0};
	}

	// Create command pool if it doesn't exist
	if (commandpools[thread_idx] == NULL)
	{
		commandpool_create(thread_idx);
	}

	// Create command buffer

	// Allocate primary command buffer
	VkCommandBufferAllocateInfo allocInfo = {0};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = commandpools[thread_idx];
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = 1;

	CommandBuffer commandbuffer = {.thread_idx = thread_idx, .frame = 0, .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY};
	VkResult result = vkAllocateCommandBuffers(device, &allocInfo, &commandbuffer.buffer);

	if (result != VK_SUCCESS)
	{
		LOG_E("Failed to create command buffers - code %d", result);
		return (CommandBuffer){0};
	}
	return commandbuffer;
}

void commandbuffer_destroy(CommandBuffer* commandbuffer)
{
	vkEndCommandBuffer(commandbuffer->buffer);

	VkSubmitInfo submitInfo = {0};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandbuffer->buffer;

	vkQueueSubmit(graphics_queue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(graphics_queue);

	vkFreeCommandBuffers(device, commandpools[commandbuffer->thread_idx], 1, &commandbuffer->buffer);
}

void commandbuffer_destroy_pools()
{
	for (uint32_t i = 0; i < RENDERER_MAX_THREADS; i++)
	{
		if (commandpools[i])
		{
			vkDestroyCommandPool(device, commandpools[i], NULL);
			commandpools[i] = NULL;
		}
	}
}