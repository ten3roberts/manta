#include "vulkan_members.h"

// Holds internal functions that should be shared across several compilation units using vulkan
SwapchainSupportDetails get_swapchain_support(VkPhysicalDevice device);
VkSurfaceFormatKHR pick_swap_surface_format(VkSurfaceFormatKHR* formats, size_t count);
VkPresentModeKHR pick_swap_present_mode(VkPresentModeKHR* modes, size_t count);
VkExtent2D pick_swap_extent(VkSurfaceCapabilitiesKHR* capabilities);
QueueFamilies get_queue_families(VkPhysicalDevice device);