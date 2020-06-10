#include "graphics/commandbuffer.h"
#include "graphics/vulkan_members.h"
#include "mempool.h"
#include "magpie.h"
#include "log.h"
#include "handlepool.h"

static VkCommandPool commandpools[RENDERER_MAX_THREADS] = {0};

typedef struct
{
	// One command buffer for each frame
	VkCommandBuffer cmd;
	VkCommandBufferLevel level;

	// The fence that gets signaled when command buffer is complete
	VkFence fence;
	// Signifies which pool it was allocated from
	// Should not be changed
	uint8_t thread_idx;
	bool recording;
	VkCommandBufferInheritanceInfo inheritanceInfo;
	// If in destroy queue
	Commandbuffer next;
} Commandbuffer_raw;

static handlepool_t handlepool = HANDLEPOOL_INIT(sizeof(Commandbuffer_raw));

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

Commandbuffer commandbuffer_create_secondary(uint8_t thread_idx, Commandbuffer primary, VkRenderPass renderPass, VkFramebuffer frameBuffer)
{
	if (thread_idx >= RENDERER_MAX_THREADS)
	{
		LOG_E("Thread index of %d is greater than the maximum number of threads %d", thread_idx, RENDERER_MAX_THREADS);
		return INVALID(Commandbuffer);
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

	const struct handle_wrapper* wrapper = handlepool_alloc(&handlepool);
	Commandbuffer_raw* raw = (Commandbuffer_raw*)wrapper->data;
	Commandbuffer handle = wrapper->handle;

	raw->thread_idx = thread_idx;
	raw->recording = false;
	raw->level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
	raw->next = INVALID(Commandbuffer);

	// Fill in inheritance info struct
	commandbuffer_set_info(handle, primary, renderPass, frameBuffer);

	VkResult result = vkAllocateCommandBuffers(device, &allocInfo, &raw->cmd);
	if (result != VK_SUCCESS)
	{
		LOG_E("Failed to create command buffers - code %d", result);
		return INVALID(Commandbuffer);
	}

	return handle;
}

Commandbuffer commandbuffer_create_primary(uint8_t thread_idx)
{
	if (thread_idx >= RENDERER_MAX_THREADS)
	{
		LOG_E("Thread index of %d is greater than the maximum number of threads %d", thread_idx, RENDERER_MAX_THREADS);
		return INVALID(Commandbuffer);
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

	const struct handle_wrapper* wrapper = handlepool_alloc(&handlepool);
	Commandbuffer_raw* raw = (Commandbuffer_raw*)wrapper->data;
	Commandbuffer handle = wrapper->handle;
	raw->thread_idx = thread_idx;
	raw->recording = false;
	raw->level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	raw->inheritanceInfo = (VkCommandBufferInheritanceInfo){0};
	raw->next = INVALID(Commandbuffer);

	VkResult result = vkAllocateCommandBuffers(device, &allocInfo, &raw->cmd);

	if (result != VK_SUCCESS)
	{
		LOG_E("Failed to create command buffers - code %d", result);
		return INVALID(Commandbuffer);
	}

	// Create fence
	VkFenceCreateInfo fence_info = {0};
	fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	result = vkCreateFence(device, &fence_info, NULL, &raw->fence);
	if (result != VK_SUCCESS)
	{
		LOG_E("Failed to create command buffer fence - code %d", result);
		return INVALID(Commandbuffer);
	}

	return handle;
}

void commandbuffer_set_info(Commandbuffer commandbuffer, Commandbuffer primary, VkRenderPass renderPass, VkFramebuffer frameBuffer)
{
	Commandbuffer_raw* raw = handlepool_get_raw(&handlepool, commandbuffer);
	if (HANDLE_VALID(primary))
		raw->fence = commandbuffer_fence(primary);
	else
		raw->fence = VK_NULL_HANDLE;

	raw->inheritanceInfo = (VkCommandBufferInheritanceInfo){
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO,
		.pNext = NULL,
		.renderPass = renderPass,
		.subpass = 0,
		.framebuffer = frameBuffer,
		.queryFlags = 0,
		.pipelineStatistics = 0};
}

void commandbuffer_begin(Commandbuffer commandbuffer)
{
	Commandbuffer_raw* raw = handlepool_get_raw(&handlepool, commandbuffer);

	vkResetCommandBuffer(raw->cmd, 0);
	VkCommandBufferBeginInfo begin_info = {0};
	if (raw->level == VK_COMMAND_BUFFER_LEVEL_SECONDARY)
	{
		begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		begin_info.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
		begin_info.pInheritanceInfo = &raw->inheritanceInfo;

		// Start recording
	}
	else if (raw->level == VK_COMMAND_BUFFER_LEVEL_PRIMARY)
	{
		begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		begin_info.flags = 0;
		begin_info.pInheritanceInfo = NULL;
	}

	VkResult result = vkBeginCommandBuffer(raw->cmd, &begin_info);
	if (result != VK_SUCCESS)
	{
		LOG_E("Failed to start recording of command buffer");
	}
	raw->recording = true;
}
// This will end recording of a primary or secondary command buffer
void commandbuffer_end(Commandbuffer commandbuffer)
{
	Commandbuffer_raw* raw = handlepool_get_raw(&handlepool, commandbuffer);

	vkEndCommandBuffer(raw->cmd);
	raw->recording = false;
}

void commandbuffer_submit(Commandbuffer commandbuffer)
{
	Commandbuffer_raw* raw = handlepool_get_raw(&handlepool, commandbuffer);

	VkSubmitInfo submitInfo = {0};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &raw->cmd;

	vkQueueSubmit(graphics_queue, 1, &submitInfo, VK_NULL_HANDLE);
}

VkCommandBuffer commandbuffer_vk(Commandbuffer commandbuffer)
{
	Commandbuffer_raw* raw = handlepool_get_raw(&handlepool, commandbuffer);
	return raw->cmd;
}
VkFence commandbuffer_fence(Commandbuffer commandbuffer)
{
	Commandbuffer_raw* raw = handlepool_get_raw(&handlepool, commandbuffer);
	return raw->fence;
}

static Commandbuffer destroy_queue = INVALID(Commandbuffer);

void commandbuffer_destroy(Commandbuffer commandbuffer)
{
	Commandbuffer_raw* raw = handlepool_get_raw(&handlepool, commandbuffer);

	// Command buffer is in use
	if (raw->fence && vkGetFenceStatus(device, raw->fence) != VK_SUCCESS)
	{
		// Queue for destruction
		LOG_S("Command buffer is still in use, queueing");
		// Insert at head
		raw->next = destroy_queue;
		destroy_queue = commandbuffer;
		return;
	}

	// Not in use, destroy immediately
	if (raw->recording)
		vkEndCommandBuffer(raw->cmd);

	vkQueueWaitIdle(graphics_queue);

	vkFreeCommandBuffers(device, commandpools[raw->thread_idx], 1, &raw->cmd);
	if (raw->level == VK_COMMAND_BUFFER_LEVEL_PRIMARY)
		vkDestroyFence(device, raw->fence, NULL);

	handlepool_free(&handlepool, commandbuffer);
}

uint32_t commandbuffer_handle_destructions()
{
	Commandbuffer item = destroy_queue;
	Commandbuffer prev = INVALID(Commandbuffer);
	Commandbuffer next = INVALID(Commandbuffer);
	uint32_t destroyed = 0;
	while (HANDLE_VALID(item))
	{
		Commandbuffer_raw* raw = handlepool_get_raw(&handlepool, item);
		next = raw->next;
		if (vkGetFenceStatus(device, raw->fence) == VK_SUCCESS)
		{
			if (HANDLE_VALID(prev))
			{
				((Commandbuffer_raw*)handlepool_get_raw(&handlepool, prev))->next = next;
			}
			// At head
			else
			{
				destroy_queue = next;
			}
			commandbuffer_destroy(item);
			++destroyed;
		}
		item = next;
	}
	return destroyed;
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
