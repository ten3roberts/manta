#include "vulkan_members.h"
#include "vulkan_internal.h"
#include "graphics/graphics.h"
#include "magpie.h"
#include "log.h"
#include "graphics/uniforms.h"
#include "graphics/pipeline.h"

int swapchain_create()
{
	SwapchainSupportDetails support = get_swapchain_support(physical_device);
	VkSurfaceFormatKHR surface_format = pick_swap_surface_format(support.formats, support.format_count);
	VkPresentModeKHR present_mode = pick_swap_present_mode(support.present_modes, support.present_mode_count);
	VkExtent2D extent = pick_swap_extent(&support.capabilities);

	uint32_t image_count = support.capabilities.minImageCount + 1;

	if (support.capabilities.maxImageCount != 0 && image_count > support.capabilities.maxImageCount)
	{
		image_count = support.capabilities.maxImageCount;
	}

	// Fill in the create info
	VkSwapchainCreateInfoKHR createInfo = {0};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = surface;

	createInfo.minImageCount = image_count;
	createInfo.imageFormat = surface_format.format;
	createInfo.imageColorSpace = surface_format.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1;
	// Specifies if we are to render directly to the swapchain image
	// For post processing VK_IMAGE_USAGE_TRANSFER_DST_BIT is to be used with a memcpy
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	QueueFamilies indices = get_queue_families(physical_device);
	uint32_t queueFamilyIndices[] = {indices.graphics, indices.present};

	// If the family indices for graphics and present queue families differ
	if (indices.graphics != indices.present)
	{
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	// If they are the same
	else
	{
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.queueFamilyIndexCount = 0;  // Optional
		createInfo.pQueueFamilyIndices = NULL; // Optional
	}

	createInfo.preTransform = support.capabilities.currentTransform;

	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

	createInfo.presentMode = present_mode;
	// If pixels are obscured by another window, ignore them
	// If we need to read data from those pixels, disable it
	createInfo.clipped = VK_TRUE;

	createInfo.oldSwapchain = VK_NULL_HANDLE;

	VkResult result = vkCreateSwapchainKHR(device, &createInfo, NULL, &swapchain);
	if (result != VK_SUCCESS)
	{
		LOG_E("Failed to create swapchain - code %d", result);
		return -1;
	}
	swapchain_image_format = surface_format.format;
	swapchain_extent = extent;

	// Retrieve swapchain images
	vkGetSwapchainImagesKHR(device, swapchain, &swapchain_image_count, NULL);
	swapchain_images = malloc(swapchain_image_count * sizeof(VkImage));
	vkGetSwapchainImagesKHR(device, swapchain, &swapchain_image_count, swapchain_images);
	LOG("Swapchain contains %d images", swapchain_image_count);
	return 0;
}
int swapchain_recreate()
{
	vkDeviceWaitIdle(device);
	LOG("Recreating swapchain");

	swapchain_destroy();

	swapchain_create();
	create_image_views();
	create_render_pass();

	pipeline_recreate_all();

	create_color_buffer();
	create_depth_buffer();
	create_framebuffers();

	//create_command_buffers();
	return 0;
}
int swapchain_destroy()
{
	// Destroy color buffer
	vkDestroyImageView(device, color_image_view, NULL);
	vkDestroyImage(device, color_image, NULL);
	vkFreeMemory(device, color_image_memory, NULL);
	// Destroy depth buffer
	vkDestroyImageView(device, depth_image_view, NULL);
	vkDestroyImage(device, depth_image, NULL);
	vkFreeMemory(device, depth_image_memory, NULL);

	for (size_t i = 0; i < framebuffer_count; i++)
		vkDestroyFramebuffer(device, framebuffers[i], NULL);

	// Destroy render pass
	vkDestroyRenderPass(device, renderPass, NULL);
	renderPass = NULL;

	// TODO: vkFreeCommandBuffers(device, command_pool, command_buffer_count, command_buffers);

	// Destroy the image views since they were explicitely created
	for (size_t i = 0; i < swapchain_image_view_count; i++)
		vkDestroyImageView(device, swapchain_image_views[i], NULL);

	vkDestroySwapchainKHR(device, swapchain, NULL);

	// Freeing of local resources
	free(framebuffers);
	framebuffers = NULL;
	framebuffer_count = 0;

	/*free(command_buffers);
	command_buffers = NULL;
	command_buffer_count = 0;*/

	free(swapchain_images);
	swapchain_images = NULL;
	swapchain_image_count = 0;

	free(swapchain_image_views);
	swapchain_image_views = NULL;
	swapchain_image_view_count = 0;

	return 0;
}