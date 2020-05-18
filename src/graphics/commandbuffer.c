#include "graphics/commandbuffer.h"
#include "graphics/vulkan_members.h"
#include "magpie.h"
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

CommandBuffer* commandbuffer_create_secondary(uint8_t thread_idx, uint32_t frame, VkFence fence, VkRenderPass renderPass, VkFramebuffer frameBuffer)
{
	if (thread_idx >= RENDERER_MAX_THREADS)
	{
		LOG_E("Thread index of %d is greater than the maximum number of threads %d", thread_idx, RENDERER_MAX_THREADS);
		return NULL;
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

	CommandBuffer* commandbuffer = malloc(sizeof(CommandBuffer));
	commandbuffer->thread_idx = thread_idx;
	commandbuffer->recording = false;
	commandbuffer->frame = frame;
	commandbuffer->fence = fence;
	commandbuffer->level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
	commandbuffer->destroy_next = NULL;

	// Fill in inheritance info struct
	commandbuffer->inheritanceInfo = (VkCommandBufferInheritanceInfo){.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO,
																	  .pNext = NULL,
																	  .renderPass = renderPass,
																	  .subpass = 0,
																	  .framebuffer = frameBuffer,
																	  .queryFlags = 0,
																	  .pipelineStatistics = 0};

	VkResult result = vkAllocateCommandBuffers(device, &allocInfo, &commandbuffer->cmd);
	if (result != VK_SUCCESS)
	{
		LOG_E("Failed to create command buffers - code %d", result);
		return NULL;
	}
	return commandbuffer;
}

CommandBuffer* commandbuffer_create_primary(uint8_t thread_idx, uint32_t frame)
{
	if (thread_idx >= RENDERER_MAX_THREADS)
	{
		LOG_E("Thread index of %d is greater than the maximum number of threads %d", thread_idx, RENDERER_MAX_THREADS);
		return NULL;
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

	CommandBuffer* commandbuffer = malloc(sizeof(CommandBuffer));
	commandbuffer->thread_idx = thread_idx;
	commandbuffer->recording = false;
	commandbuffer->frame = frame;
	commandbuffer->level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	commandbuffer->inheritanceInfo = (VkCommandBufferInheritanceInfo){0};
	commandbuffer->destroy_next = NULL;

	VkResult result = vkAllocateCommandBuffers(device, &allocInfo, &commandbuffer->cmd);

	if (result != VK_SUCCESS)
	{
		LOG_E("Failed to create command buffers - code %d", result);
		return NULL;
	}

	// Create fence
	VkFenceCreateInfo fence_info = {0};
	fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	result = vkCreateFence(device, &fence_info, NULL, &commandbuffer->fence);
	if (result != VK_SUCCESS)
	{
		LOG_E("Failed to create command buffer fence - code %d", result);
		return NULL;
	}

	return commandbuffer;
}

void commandbuffer_begin(CommandBuffer* commandbuffer)
{
	vkResetCommandBuffer(commandbuffer->cmd, 0);
	VkCommandBufferBeginInfo begin_info = {0};
	if (commandbuffer->level == VK_COMMAND_BUFFER_LEVEL_SECONDARY)
	{
		begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		begin_info.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
		begin_info.pInheritanceInfo = &commandbuffer->inheritanceInfo;

		// Start recording
	}
	else if (commandbuffer->level == VK_COMMAND_BUFFER_LEVEL_PRIMARY)
	{
		begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		begin_info.flags = 0;
		begin_info.pInheritanceInfo = NULL;
	}

	VkResult result = vkBeginCommandBuffer(commandbuffer->cmd, &begin_info);
	if (result != VK_SUCCESS)
	{
		LOG_E("Failed to start recording of command buffer");
	}
	commandbuffer->recording = true;
}
// This will end recording of a primary or secondary command buffer
void commandbuffer_end(CommandBuffer* commandbuffer)
{
	vkEndCommandBuffer(commandbuffer->cmd);
	commandbuffer->recording = false;
}

void commandbuffer_submit(CommandBuffer* commandbuffer)
{
	VkSubmitInfo submitInfo = {0};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandbuffer->cmd;

	vkQueueSubmit(graphics_queue, 1, &submitInfo, VK_NULL_HANDLE);
}

static CommandBuffer* destroy_queue = NULL;

void commandbuffer_destroy(CommandBuffer* commandbuffer)
{
	// Command buffer is in use
	if (commandbuffer->fence && vkGetFenceStatus(device, commandbuffer->fence) != VK_SUCCESS)
	{
		// Queue for destruction
		LOG_W("Command buffer is still in use, queueing");
		// Insert at head
		commandbuffer->destroy_next = destroy_queue;
		destroy_queue = commandbuffer;
		return;
	}
	// Not in use, destroy immediately

	if (commandbuffer->recording)
		vkEndCommandBuffer(commandbuffer->cmd);

	vkQueueWaitIdle(graphics_queue);

	vkFreeCommandBuffers(device, commandpools[commandbuffer->thread_idx], 1, &commandbuffer->cmd);
	if (commandbuffer->level == VK_COMMAND_BUFFER_LEVEL_PRIMARY)
		vkDestroyFence(device, commandbuffer->fence, NULL);
	free(commandbuffer);
}

void commandbuffer_handle_destructions()
{
	CommandBuffer* prev = NULL;
	CommandBuffer* item = destroy_queue;
	CommandBuffer* next = NULL;
	while (item)
	{
		next = item->destroy_next;
		if (vkGetFenceStatus(device, item->fence) == VK_SUCCESS)
		{
			if (prev)
			{
				prev->destroy_next = next;
			}
			// At head
			else
			{
				destroy_queue = next;
			}
			commandbuffer_destroy(item);
		}
		item = next;
	}
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
