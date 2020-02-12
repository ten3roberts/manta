#include "vulkan_internal.h"
#include "math/math.h"
#include "settings.h"
#include "log.h"
#include "stdlib.h"

SwapchainSupportDetails get_swapchain_support(VkPhysicalDevice device)
{
	SwapchainSupportDetails details;

	// Get the device capabilites for the swapchain
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

	// Get the surface formats
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &details.format_count, NULL);
	if (details.format_count)
	{
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &details.format_count, details.formats);
	}

	// Get the present formats
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &details.present_mode_count, NULL);
	if (details.present_mode_count)
	{
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &details.present_mode_count, details.present_modes);
	}

	return details;
}

// Picks the best swapchain surface format and returns it from those supplied
VkSurfaceFormatKHR pick_swap_surface_format(VkSurfaceFormatKHR* formats, size_t count)
{
	for (size_t i = 0; i < count; i++)
	{
		// The best format exists and is returned
		if (formats[i].format == VK_FORMAT_B8G8R8A8_SRGB && formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{
			LOG_OK("The best swapchain surface format was available");
			return formats[i];
		}
	}

	// If the best one wasn't available
	return formats[0];
}

// Picks the best swapchain present mode and returns it from those supplied
VkPresentModeKHR pick_swap_present_mode(VkPresentModeKHR* modes, size_t count)
{
	VkPresentModeKHR preferred_mode = VSYNC_NONE;
	VsyncMode mode = settings_get_vsync();

	if(mode == VSYNC_NONE)
		preferred_mode = VK_PRESENT_MODE_IMMEDIATE_KHR;
	else if(mode == VSYNC_DOUBLE)
		preferred_mode = VK_PRESENT_MODE_FIFO_KHR;
	else if(mode == VSYNC_TRIPLE)
		preferred_mode = VK_PRESENT_MODE_MAILBOX_KHR;

	for (size_t i = 0; i < count; i++)
	{
		// Triple buffered present mode is preferred
		if (modes[i] == preferred_mode)
		{
			LOG_OK("The best swapchain present mode was available");
			return modes[i];
		}
	}

	// Only FIFO/double buffered vsync is guranteed to be available
	// Use it as a fallback
	LOG_OK("Choosing double buffered vsync");
	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D pick_swap_extent(VkSurfaceCapabilitiesKHR* capabilities)
{
	if (capabilities->currentExtent.width != UINT32_MAX)
	{
		return capabilities->currentExtent;
	}
	else
	{
		VkExtent2D actual_extent = {window_get_width(window), window_get_height(window)};
		actual_extent.width =
			max(capabilities->minImageExtent.width, min(capabilities->maxImageExtent.width, actual_extent.width));
		actual_extent.height =
			max(capabilities->minImageExtent.height, min(capabilities->maxImageExtent.height, actual_extent.height));

		return actual_extent;
	}
}

// Function shared across different compilation units
QueueFamilies get_queue_families(VkPhysicalDevice device)
{
	QueueFamilies indices = {0, 0};

	uint32_t queue_family_count = 0;
	VkQueueFamilyProperties* queue_families = NULL;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, NULL);
	queue_families = malloc(queue_family_count * sizeof(VkQueueFamilyProperties));

	vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families);

	size_t i = 0;
	for (i = 0; i < queue_family_count; i++)
	{
		// Check if index belongs to graphics queue family
		if (queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
			indices.graphics = i;
			indices.queue_validity |= GRAPHICS_FAMILY_VALID_BIT;
		}

		// Check if index belongs to the present queue family
		VkBool32 present_support = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &present_support);
		if (present_support)
		{
			indices.present = i;
			indices.queue_validity |= PRESENT_FAMILY_VALID_BIT;
		}
	}

	free(queue_families);
	queue_families = NULL;

	return indices;
}
