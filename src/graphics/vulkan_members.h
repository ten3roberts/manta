#ifndef VULKAN_MEMBERS_H
#define VULKAN_MEMBERS_H
#include <stdint.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "window.h"
#include "uniforms.h"


#define MAX_FRAMES_IN_FLIGHT 3

#define GRAPHICS_FAMILY_VALID_BIT 0b1
#define PRESENT_FAMILY_VALID_BIT  0b01
#define QUEUE_FAMILIES_COMLPLETE  0b11

#define QUEUE_FAMILY_COUNT 2

// Specifies the indices for the different queue families
// Is filled in by get_queue_families function
typedef struct
{
	// The index for the graphics queue family
	uint32_t graphics;
	uint32_t present;

	// Defines a bitfield of the valid queues
	// graphics queue 0b1
	int queue_validity;
} QueueFamilies;

QueueFamilies get_queue_families(VkPhysicalDevice device);

typedef struct
{
	VkSurfaceCapabilitiesKHR capabilities;
	VkSurfaceFormatKHR formats[64];
	uint32_t format_count;
	VkPresentModeKHR present_modes[64];
	uint32_t present_mode_count;
} SwapchainSupportDetails;

// Vulkan api members

extern VkInstance instance;
extern VkDebugUtilsMessengerEXT debug_messenger;

extern VkPhysicalDevice physical_device;
extern VkDevice device;

extern VkQueue graphics_queue;
extern VkQueue present_queue;

extern VkSurfaceKHR surface;

extern VkSwapchainKHR swapchain;

// Swap chain
extern VkImage* swapchain_images;
extern uint32_t swapchain_image_count;

extern VkImageView* swapchain_image_views;
extern uint32_t swapchain_image_view_count;

extern VkFormat swapchain_image_format;
extern VkExtent2D swapchain_extent;

extern VkDescriptorSetLayout global_descriptor_layout;
extern DescriptorPack global_descriptors;
extern VkFramebuffer* framebuffers;
extern size_t framebuffer_count;

extern VkImage color_image;
extern VkDeviceMemory color_image_memory;
extern VkImageView color_image_view;

extern VkImage depth_image;
extern VkDeviceMemory depth_image_memory;
extern VkImageView depth_image_view;
extern VkFormat depth_image_format;

extern VkSampleCountFlagBits msaa_samples;

extern VkCommandPool command_pool;

// Semaphores
extern VkSemaphore semaphores_image_available[MAX_FRAMES_IN_FLIGHT];
extern VkSemaphore semaphores_render_finished[MAX_FRAMES_IN_FLIGHT];
extern VkFence in_flight_fences[MAX_FRAMES_IN_FLIGHT];
extern VkFence* images_in_flight;

// Keep track of frames in advance, I.e; frames in flight
extern uint32_t current_frame;

// Rendering
extern VkRenderPass renderPass;

extern const char* validation_layers[];
extern const size_t validation_layers_count;

extern const char* device_extensions[];
extern const size_t device_extensions_count;

extern const int enable_validation_layers;

extern void* ub;
extern void* ub2;
#endif