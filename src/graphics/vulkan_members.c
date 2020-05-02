#include "vulkan_members.h"
// Vulkan api members

VkInstance instance = VK_NULL_HANDLE;
VkDebugUtilsMessengerEXT debug_messenger = VK_NULL_HANDLE;

VkPhysicalDevice physical_device = VK_NULL_HANDLE;
VkDevice device = VK_NULL_HANDLE;

VkQueue graphics_queue = VK_NULL_HANDLE;
VkQueue present_queue = VK_NULL_HANDLE;

VkSurfaceKHR surface = VK_NULL_HANDLE;

VkSwapchainKHR swapchain = VK_NULL_HANDLE;

// Swap chain
VkImage* swapchain_images = NULL;
uint32_t swapchain_image_count = 0;

VkImageView* swapchain_image_views = NULL;
uint32_t swapchain_image_view_count = 0;

VkFormat swapchain_image_format;
VkExtent2D swapchain_extent;

VkDescriptorSetLayout global_descriptor_layout = VK_NULL_HANDLE;
DescriptorPack global_descriptors;

VkFramebuffer* framebuffers = NULL;
size_t framebuffer_count = 0;

VkImage color_image = NULL;
VkDeviceMemory color_image_memory = NULL;
VkImageView color_image_view = NULL;

VkImage depth_image = NULL;
VkDeviceMemory depth_image_memory = NULL;
VkImageView depth_image_view = NULL;
VkFormat depth_image_format = VK_FORMAT_UNDEFINED;

VkSampleCountFlagBits msaa_samples = VK_SAMPLE_COUNT_1_BIT;

// Semaphores
VkSemaphore semaphores_image_available[MAX_FRAMES_IN_FLIGHT] = {0};
VkSemaphore semaphores_render_finished[MAX_FRAMES_IN_FLIGHT] = {0};
VkFence in_flight_fences[MAX_FRAMES_IN_FLIGHT] = {0};
VkFence* images_in_flight = NULL;
uint32_t current_frame = 0;

// A pointer to the current window
Window* window = NULL;

// Rendering
VkRenderPass renderPass;

const char* validation_layers[] = {"VK_LAYER_KHRONOS_validation"};
const size_t validation_layers_count = (sizeof(validation_layers) / sizeof(char*));

const char* device_extensions[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
const size_t device_extensions_count = (sizeof(device_extensions) / sizeof(char*));

#ifdef RELEASE
const int enable_validation_layers = 0;
#else
const int enable_validation_layers = 1;
#endif

void* ub = NULL;
void* ub2 = NULL;

void* tex = NULL;