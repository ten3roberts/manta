#include "vulkan_members.h"
#include "graphics/vertexbuffer.h"

// Holds internal function prototypes and definitions that should be shared across several compilation units using
// vulkan Defined in vulkan_internal.c
SwapchainSupportDetails get_swapchain_support(VkPhysicalDevice device);
VkSurfaceFormatKHR pick_swap_surface_format(VkSurfaceFormatKHR* formats, size_t count);
VkPresentModeKHR pick_swap_present_mode(VkPresentModeKHR* modes, size_t count);
VkExtent2D pick_swap_extent(VkSurfaceCapabilitiesKHR* capabilities);
QueueFamilies get_queue_families(VkPhysicalDevice device);

int has_stencil_component(VkFormat format);

VkFormat find_supported_format(VkFormat* formats, uint32_t format_count, VkImageTiling tiling, VkFormatFeatureFlags features);
VkFormat find_depth_format();

int create_color_buffer();
int create_depth_buffer();

VkSampleCountFlagBits get_max_sample_count(VkPhysicalDevice device);

// Defined in vulkan.c
int create_image_views();
int create_render_pass();
int create_graphics_pipeline();
int create_framebuffers();
int create_command_buffers();
int create_descriptor_pool();
