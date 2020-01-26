#include "log.h"
#include "vulkan_members.h"

void renderer_draw()
{
	uint32_t image_index;
	vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, semaphore_image_available, VK_NULL_HANDLE, &image_index);

	// Submit render queue
	// Specifies which semaphores to wait for before execution
	// Specify to wait for image available before writing to swapchain
	VkSubmitInfo submit_info = {0};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	VkSemaphore wait_semaphores[] = {semaphore_image_available};
	VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
	submit_info.waitSemaphoreCount = sizeof wait_semaphores / sizeof *wait_semaphores;
	submit_info.pWaitSemaphores = wait_semaphores;
	submit_info.pWaitDstStageMask = wait_stages;

	// Specify which command buffers to submit for execution
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &command_buffers[image_index];

	// Specify which semaphores to signal on completion
	VkSemaphore signal_semaphores[] = {semaphore_render_finished};
	submit_info.signalSemaphoreCount = sizeof signal_semaphores / sizeof *signal_semaphores;
	submit_info.pSignalSemaphores = signal_semaphores;

	VkResult result = vkQueueSubmit(graphics_queue, 1, &submit_info, VK_NULL_HANDLE);
	if (result != VK_SUCCESS)
	{
		LOG_E("Failed to submit draw command buffer - code %d", result);
		return;
	}

	// Presentation
	VkPresentInfoKHR present_info = {0};
	present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	present_info.waitSemaphoreCount = sizeof signal_semaphores / sizeof *signal_semaphores;
	present_info.pWaitSemaphores = signal_semaphores;

	VkSwapchainKHR swapchains[] = {swapchain};
	present_info.swapchainCount = sizeof swapchains / sizeof *swapchains;
	present_info.pSwapchains = swapchains;
	present_info.pImageIndices = &image_index;

	present_info.pResults = NULL; // Optional

	vkQueuePresentKHR(present_queue, &present_info);
}