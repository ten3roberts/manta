#include "log.h"
#include "vulkan_internal.h"
#include "graphics/graphics.h"
#include "cr_time.h"
#include "graphics/uniforms.h"
#include "math/quaternion.h"
#include "graphics/pipeline.h"
#include "scene.h"
#include "graphics/commandbuffer.h"
#include "graphics/rendertree.h"
#include "graphics/framebuffer.h"
#include "utils.h"
#include "magpie.h"
#include "defines.h"

#define ONE_FRAME_LIMIT 512

static uint32_t image_index;

// 0: No resize
// 1: Resize event
static int resize_event;

static uint8_t flag_rebuild = 0;

static CommandBuffer* primarybuffers[3];

static CommandBuffer* oneframe_commands[3];
static UniformBuffer* oneframe_buffer = NULL;
static DescriptorPack* oneframe_descriptors = NULL;
static int oneframe_draw_index = 0;

// The framebuffer with the swapchain images as attachments
// The last framebuffer and renderer to the window
static Framebuffer* framebuffer_main = NULL;

// Rebuilds command buffers for the current frame
// Needs to be called after renderer_begin
static void renderer_rebuild(Scene* scene)
{
	CommandBuffer* commandbuffer = primarybuffers[image_index];
	commandbuffer_begin(commandbuffer);

	// Begin render pass
	VkRenderPassBeginInfo render_pass_info = {0};
	render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	render_pass_info.renderPass = renderPass;
	render_pass_info.framebuffer = framebuffer_main->vkFramebuffers[image_index];
	render_pass_info.renderArea.offset = (VkOffset2D){0, 0};
	render_pass_info.renderArea.extent = swapchain_extent;
	VkClearValue clear_values[2] = {{.color = {.float32 = {0.0f, 0.0f, 0.0f, 1.0f}}}, {.depthStencil = {1.0f, 0.0f}}};
	render_pass_info.clearValueCount = 2;
	render_pass_info.pClearValues = clear_values;
	vkCmdBeginRenderPass(commandbuffer->cmd, &render_pass_info, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);

	// Iterate all entities
	RenderTreeNode* rendertree = scene_get_rendertree(scene);
	Camera* camera = scene_get_camera(scene, 0);
	rendertree_render(rendertree, commandbuffer, camera, image_index);

	// One frame draws
	commandbuffer_end(oneframe_commands[image_index]);
	//oneframe_buffer_mapped = NULL;
	vkCmdExecuteCommands(commandbuffer->cmd, 1, &oneframe_commands[image_index]->cmd);
	vkCmdEndRenderPass(commandbuffer->cmd);
	commandbuffer_end(commandbuffer);
}

int renderer_init()
{
	// Load primitive models
	model_load_collada("./assets/models/primitive.dae");

	oneframe_buffer = ub_create(sizeof(struct EntityData) * ONE_FRAME_LIMIT, 0, 0);

	oneframe_descriptors = descriptorpack_create(rendertree_get_descriptor_layout(), rendertree_get_descriptor_bindings(), rendertree_get_descriptor_binding_count());

	descriptorpack_write(oneframe_descriptors, rendertree_get_descriptor_bindings(), rendertree_get_descriptor_binding_count(), &oneframe_buffer, NULL, NULL);

	// Create main framebuffer

	// Color
	// Depth
	// Swapchain image
	static const int attachment_count = 3;

	Texture color_attachment = texture_create("color_attachment", swapchain_extent.width, swapchain_extent.height, swapchain_image_format, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, msaa_samples, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_ASPECT_COLOR_BIT);

	Texture depth_attachment = texture_create("depth_attachment", swapchain_extent.width, swapchain_extent.height, find_depth_format(), VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, msaa_samples, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT);

	// Get the swapchain images
	uint32_t swapchain_image_count = 0;
	VkImage* swapchain_images = NULL;
	vkGetSwapchainImagesKHR(device, swapchain, &swapchain_image_count, NULL);
	swapchain_images = malloc(swapchain_image_count * sizeof(VkImage));
	vkGetSwapchainImagesKHR(device, swapchain, &swapchain_image_count, swapchain_images);

	Texture swapchain_attachments[3] = {0};
	for (uint32_t i = 0; i < swapchain_image_count; i++)
	{
		char name[512];
		string_format(name, sizeof name, "swapchain_image_%d", i);
		swapchain_attachments[i] = texture_create_existing(name, swapchain_extent.width, swapchain_extent.height, swapchain_image_format, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_LAYOUT_UNDEFINED, swapchain_images[i], VK_IMAGE_ASPECT_COLOR_BIT);
	}

	Texture* attachments[3] = {0};

	for (uint32_t i = 0; i < swapchain_image_count; i++)
	{
		attachments[i] = malloc(3 * sizeof *attachments);

		attachments[i][0] = color_attachment;
		attachments[i][1] = depth_attachment;
		attachments[i][2] = swapchain_attachments[i];
	}

	framebuffer_main = framebuffer_create(attachments, attachment_count);
	for (uint32_t i = 0; i < swapchain_image_count; i++)
		free(attachments[i]);
	free(swapchain_images);

	// Create primary command buffers
	for (int i = 0; i < 3; i++)
	{
		primarybuffers[i] = commandbuffer_create_primary(0, i);
		oneframe_commands[i] = commandbuffer_create_secondary(0, i, primarybuffers[i]->fence, renderPass, framebuffer_main->vkFramebuffers[i]);
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
	//--flag_rebuild;
	renderer_rebuild(scene);

	// Submit render queue
	// Check if a previous frame is using this image (i.e. there is its fence to wait on)
	if (primarybuffers[image_index]->fence != VK_NULL_HANDLE)
	{
		vkWaitForFences(device, 1, &primarybuffers[image_index]->fence, VK_TRUE, UINT64_MAX);
	}

	// Submit render queue
	// Specifies which semaphores to wait for before execution
	// Specify to wait for image available before writing to swapchain
	VkSubmitInfo submit_info = {0};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	VkSemaphore wait_semaphores[] = {semaphores_image_available[current_frame]};
	VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
	submit_info.waitSemaphoreCount = LENOF(wait_semaphores);
	submit_info.pWaitSemaphores = wait_semaphores;
	submit_info.pWaitDstStageMask = wait_stages;

	// Specify which command buffers to submit for execution
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &primarybuffers[image_index]->cmd;

	// Specify which semaphores to signal on completion
	VkSemaphore signal_semaphores[] = {semaphores_render_finished[current_frame]};
	submit_info.signalSemaphoreCount = LENOF(signal_semaphores);
	submit_info.pSignalSemaphores = signal_semaphores;

	// Synchronise CPU-GPU
	vkResetFences(device, 1, &primarybuffers[current_frame]->fence);

	VkResult result = vkQueueSubmit(graphics_queue, 1, &submit_info, primarybuffers[current_frame]->fence);
	if (result != VK_SUCCESS)
	{
		LOG_E("Failed to submit draw command buffer - code %d", result);
		return;
	}

	// Presentation
	VkPresentInfoKHR present_info = {0};
	present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	present_info.waitSemaphoreCount = LENOF(signal_semaphores);
	present_info.pWaitSemaphores = signal_semaphores;

	VkSwapchainKHR swapchains[] = {swapchain};
	present_info.swapchainCount = LENOF(swapchains);
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

	// Destroy command buffers not in used queued for destruction
	commandbuffer_handle_destructions();

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

	vkWaitForFences(device, 1, &primarybuffers[current_frame]->fence, VK_TRUE, UINT64_MAX);

	vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, semaphores_image_available[current_frame], VK_NULL_HANDLE, &image_index);

	// Begin one frame draws
	commandbuffer_begin(oneframe_commands[image_index]);
	material_bind(material_get_default(), oneframe_commands[image_index], oneframe_descriptors->sets[image_index]);
	oneframe_draw_index = 0;
}

void renderer_resize()
{
	resize_event = 1;
}

void renderer_flag_rebuild()
{
	flag_rebuild = swapchain_image_count;
}

int renderer_get_frameindex()
{
	return image_index;
}

Framebuffer* renderer_get_framebuffer()
{
	return framebuffer_main;
}

void renderer_draw_custom(Mesh* mesh, vec3 position, quaternion rotation, vec3 scale, vec4 color)
{
	if (oneframe_draw_index >= ONE_FRAME_LIMIT)
	{
		return;
	}
	Transform transform = (Transform){.position = position, .rotation = rotation, .scale = scale};
	transform_update(&transform);
	struct EntityData data = {0};
	data.model_matrix = transform.model_matrix;
	data.color = color;

	// Binding is done by renderer
	mesh_bind(mesh, oneframe_commands[image_index]);
	ub_update(oneframe_buffer, &data, sizeof(struct EntityData) * oneframe_draw_index, sizeof(struct EntityData), image_index);
	// Set push constant for model matrix
	material_push_constants(material_get_default(), oneframe_commands[image_index], 0, &oneframe_draw_index);
	mesh_draw(mesh, oneframe_commands[image_index]);
	oneframe_draw_index++;
}

void renderer_draw_cube(vec3 position, quaternion rotation, vec3 scale, vec4 color)
{
	static Mesh* mesh = NULL;
	if (mesh == NULL)
		mesh = mesh_find("primitive:cube");
	renderer_draw_custom(mesh, position, rotation, scale, color);
}
void renderer_draw_cube_wire(vec3 position, quaternion rotation, vec3 scale, vec4 color)
{
	static Mesh* mesh = NULL;
	if (mesh == NULL)
		mesh = mesh_find("primitive:cube_wire");
	renderer_draw_custom(mesh, position, rotation, scale, color);
}

void renderer_terminate()
{
	vkDeviceWaitIdle(device);
	// Free all remaining command buffers in destroy queue
	commandbuffer_handle_destructions();
	ub_destroy(oneframe_buffer);
	for (int i = 0; i < 3; i++)
	{
		commandbuffer_destroy(oneframe_commands[i]);
		commandbuffer_destroy(primarybuffers[i]);
	}

	framebuffer_destroy(framebuffer_main);

	descriptorpack_destroy(oneframe_descriptors);

	vkDeviceWaitIdle(device);
}
