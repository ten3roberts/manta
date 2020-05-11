#include "log.h"
#include "vulkan_members.h"
#include "graphics/graphics.h"
#include "cr_time.h"
#include "graphics/uniforms.h"
#include "math/quaternion.h"
#include "graphics/pipeline.h"
#include "scene.h"
#include "graphics/commandbuffer.h"

static uint32_t image_index;

// 0: No resize
// 1: Resize event
static int resize_event;

static uint8_t flag_rebuild = 0;

CommandBuffer commandbuffers[3];


// Rebuilds command buffers for the current frame
// Needs to be called after renderer_begin
static void renderer_rebuild()
{
	VkCommandBuffer command_buffer = commandbuffers[image_index].buffer;
	vkResetCommandBuffer(command_buffer, 0);
	VkCommandBufferBeginInfo begin_info = {0};
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin_info.flags = 0;
	begin_info.pInheritanceInfo = NULL;

	// Start recording
	VkResult result = vkBeginCommandBuffer(command_buffer, &begin_info);
	if (result != VK_SUCCESS)
	{
		LOG_E("Failed to record command buffer %d", image_index);
		return;
	}
	// Begin render pass
	VkRenderPassBeginInfo render_pass_info = {0};
	render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	render_pass_info.renderPass = renderPass;
	render_pass_info.framebuffer = framebuffers[image_index];
	render_pass_info.renderArea.offset = (VkOffset2D){0, 0};
	render_pass_info.renderArea.extent = swapchain_extent;
	VkClearValue clear_values[2] = {{.color = {.float32 = {0.0f, 0.0f, 0.0f, 1.0f}}}, {.depthStencil = {1.0f, 0.0f}}};
	render_pass_info.clearValueCount = 2;
	render_pass_info.pClearValues = clear_values;
	vkCmdBeginRenderPass(command_buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);

	// Iterate all entities
	uint32_t index = 0;
	Entity* entity = NULL;
	Scene* scene = scene_get_current();
	while ((entity = scene_get_entity(scene, index)))
	{
		entity_render(entity, command_buffer, image_index);
		++index;
	}
	vkCmdEndRenderPass(command_buffer);
	result = vkEndCommandBuffer(command_buffer);
	if (result != VK_SUCCESS)
	{
		LOG_E("Failed to record command buffer");
		return;
	}
}

int renderer_init()
{
	// Create primary command buffers
	for (int i = 0; i < 3; i++)
	{
		commandbuffers[i] = commandbuffer_create_primary(0);
	}

	return 0;
}

void renderer_submit(Scene* scene)
{
	// Don't render while user is resizing window
	if (resize_event)
	{
		return;
	}

	// Rebuild command buffers if required
	if (flag_rebuild == 1)
	{
		renderer_rebuild();
	}

	// Update uniform buffer
	//TransformType transform_buffer;
	//quaternion rotation = quat_axis_angle((vec3){0, 0.5, 1}, time_elapsed());

	//mat4 rot = quat_to_mat4(rotation);
	//mat4 pos = mat4_translate((vec3){0, sinf(time_elapsed()) * 0.5, -time_elapsed() * 0.2 + -2});
	//transform_buffer.model = mat4_mul(&rot, &pos);

	//transform_buffer.view = mat4_identity;
	// transform_buffer.proj = mat4_perspective(window_get_width(window) / window_get_height(window), 1, 0, 10);
	//transform_buffer.proj = mat4_ortho(window_get_aspect(window), 1, 0, 10);
	//ub_update(ub, &transform_buffer, 0, CS_WHOLE_SIZE, -1);

	// Submit render queue
	// Check if a previous frame is using this image (i.e. there is its fence to wait on)
	if (images_in_flight[image_index] != VK_NULL_HANDLE)
	{
		vkWaitForFences(device, 1, &images_in_flight[image_index], VK_TRUE, UINT64_MAX);
	}

	// Mark the image as now being in use by this frame
	images_in_flight[image_index] = in_flight_fences[current_frame];

	// Submit render queue
	// Specifies which semaphores to wait for before execution
	// Specify to wait for image available before writing to swapchain
	VkSubmitInfo submit_info = {0};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	VkSemaphore wait_semaphores[] = {semaphores_image_available[current_frame]};
	VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
	submit_info.waitSemaphoreCount = sizeof wait_semaphores / sizeof *wait_semaphores;
	submit_info.pWaitSemaphores = wait_semaphores;
	submit_info.pWaitDstStageMask = wait_stages;

	// Specify which command buffers to submit for execution
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &commandbuffers[image_index].buffer;

	// Specify which semaphores to signal on completion
	VkSemaphore signal_semaphores[] = {semaphores_render_finished[current_frame]};
	submit_info.signalSemaphoreCount = sizeof signal_semaphores / sizeof *signal_semaphores;
	submit_info.pSignalSemaphores = signal_semaphores;

	// Synchronise CPU-GPU
	vkResetFences(device, 1, &in_flight_fences[current_frame]);

	VkResult result = vkQueueSubmit(graphics_queue, 1, &submit_info, in_flight_fences[current_frame]);
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

	result = vkQueuePresentKHR(present_queue, &present_info);

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
	{
		LOG_S("Suboptimal swapchain");
		swapchain_recreate();
		return;
	}
	if (result != VK_SUCCESS)
	{
		LOG_E("Failed to present swapchain image");
		return;
	}

	current_frame = (current_frame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void renderer_begin()
{
	// Skip rendering if window is minimized
	if (window_get_minimized(graphics_get_window()))
	{
		SLEEP(0.1f);
		return;
	}

	// Handle resize event
	if (resize_event == 1)
	{
		resize_event = 2;
		// Don't get next image since swapchain is outdated
		return;
	}
	// There has been one frame clear of resize events
	else if (resize_event == 2)
	{
		swapchain_recreate();
		resize_event = 0;
	}

	vkWaitForFences(device, 1, &in_flight_fences[current_frame], VK_TRUE, UINT64_MAX);

	vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, semaphores_image_available[current_frame], VK_NULL_HANDLE, &image_index);
}

void renderer_resize()
{
	resize_event = 1;
}

void renderer_flag_rebuild()
{
	flag_rebuild = 1;
}

int renderer_get_frameindex()
{
	return image_index;
}

void renderer_terminate()
{
	vkDeviceWaitIdle(device);
}