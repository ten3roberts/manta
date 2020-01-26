#pragma once
#include <stdint.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "window.h"


#define GRAPHICS_FAMILY_VALID_BIT 0b1
#define PRESENT_FAMILY_VALID_BIT 0b01
#define QUEUE_FAMILIES_COMLPLETE 0b11

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

typedef struct
{
	VkSurfaceCapabilitiesKHR capabilities;
	VkSurfaceFormatKHR formats[64];
	uint32_t format_count;
	VkPresentModeKHR present_modes[64];
	uint32_t present_mode_count;
} SwapchainSupportDetails;

// Vulkan api members

static VkInstance instance = VK_NULL_HANDLE;
static VkDebugUtilsMessengerEXT debug_messenger = VK_NULL_HANDLE;

static VkPhysicalDevice physical_device = VK_NULL_HANDLE;
static VkDevice device = VK_NULL_HANDLE;

static VkQueue graphics_queue = VK_NULL_HANDLE;
static VkQueue present_queue = VK_NULL_HANDLE;

static VkSurfaceKHR surface = VK_NULL_HANDLE;

static VkSwapchainKHR swapchain = VK_NULL_HANDLE;

// Swap chain
static VkImage* swapchain_images = NULL;
static uint32_t swapchain_image_count = 0;

static VkImageView* swapchain_image_views = NULL;
static uint32_t swapchain_image_view_count = 0;

static VkFormat swapchain_image_format;
static VkExtent2D swapchain_extent;

// Pipeline layout
static VkPipelineLayout pipeline_layout = VK_NULL_HANDLE;

static VkPipeline graphics_pipeline = VK_NULL_HANDLE;

static VkFramebuffer* framebuffers = NULL;
static size_t framebuffer_count = 0;

static VkCommandPool command_pool;

static VkCommandBuffer* command_buffers = NULL;
static size_t command_buffer_count = 0;

// Semaphores
VkSemaphore semaphore_image_available;
VkSemaphore semaphore_render_finished;

// A pointer to the current window
static Window* window = NULL;

// Rendering
static VkRenderPass renderPass;

static const char* validation_layers[] = {"VK_LAYER_KHRONOS_validation"};
static const size_t validation_layers_count = (sizeof(validation_layers) / sizeof(char*));

static const char* device_extensions[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
static const size_t device_extensions_count = (sizeof(device_extensions) / sizeof(char*));

#if NDEBUG
statuc const int enable_validation_layers = 0;
#else
static const int enable_validation_layers = 1;
#endif