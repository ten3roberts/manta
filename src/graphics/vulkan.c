#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "window.h"
#include "application.h"
#include "log.h"
#include "math/math_extra.h"
#include "vulkan.h"
#include <stdlib.h>
#include <stdbool.h>

static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
													 VkDebugUtilsMessageTypeFlagsEXT messageType,
													 const VkDebugUtilsMessengerCallbackDataEXT * pCallbackData,
													 void * pUserData)
{
	if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
		log_call(CONSOLE_RED, "Vulkan debug callback", "(%d)%s", messageSeverity, pCallbackData->pMessage);
	else if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
		log_call(CONSOLE_YELLOW, "Vulkan debug callback", "(%d)%s", messageSeverity, pCallbackData->pMessage);
	else
		log_call(CONSOLE_BLUE, "Vulkan debug callback", "(%d)%s", messageSeverity, pCallbackData->pMessage);
	return VK_FALSE;
}

VkResult create_debug_utils_messenger_ext(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT * pCreateInfo,
										  const VkAllocationCallbacks * pAllocator,
										  VkDebugUtilsMessengerEXT * pDebugMessenger)
{
	VkResult (*func)(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT * pCreateInfo,
					 const VkAllocationCallbacks * pAllocator, VkDebugUtilsMessengerEXT * pMessenger) =
		(PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");

	if (func != NULL)
	{
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	}
	else
	{
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

void destroy_debug_utils_messenger_ext(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger,
									   const VkAllocationCallbacks * pAllocator)
{
	void (*func)(VkInstance instance, VkDebugUtilsMessengerEXT messenger, const VkAllocationCallbacks * pAllocator) =
		(PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");

	if (func != NULL)
	{
		func(instance, debugMessenger, pAllocator);
	}
}

void populate_debug_messenger_create_info(VkDebugUtilsMessengerCreateInfoEXT * createInfo)
{
	createInfo->pUserData = NULL;
	createInfo->pNext = NULL;
	createInfo->flags = 0;
	createInfo->sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	createInfo->messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
								  VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
								  VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo->messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
							  VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
							  VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	createInfo->pfnUserCallback = debug_callback;
}

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

VkInstance instance = VK_NULL_HANDLE;
VkDebugUtilsMessengerEXT debug_messenger = VK_NULL_HANDLE;

VkPhysicalDevice physical_device = VK_NULL_HANDLE;
VkDevice device = VK_NULL_HANDLE;

VkQueue graphics_queue = VK_NULL_HANDLE;
VkQueue present_queue = VK_NULL_HANDLE;

VkSurfaceKHR surface = VK_NULL_HANDLE;

VkSwapchainKHR swapchain = VK_NULL_HANDLE;

// Swap chain
VkImage * swapchain_images = NULL;
uint32_t swapchain_image_count = 0;

VkImageView * swapchain_image_views = NULL;
uint32_t swapchain_image_view_count = 0;

VkFormat swapchain_image_format;
VkExtent2D swapchain_extent;

// A pointer to the current window
Window * window;

const char * validation_layers[] = {"VK_LAYER_KHRONOS_validation"};
const size_t validation_layers_count = (sizeof(validation_layers) / sizeof(char *));

const char * device_extensions[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
const size_t device_extensions_count = (sizeof(device_extensions) / sizeof(char *));

#if NDEBUG
const int enable_validation_layers = 0;
#else
const int enable_validation_layers = 1;
#endif

int create_instance()
{
	VkApplicationInfo appInfo = {0};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "crescent";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "crescent";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_0;

	VkInstanceCreateInfo createInfo = {0};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;

	// Get the extensions required by glfw and application
	uint32_t glfw_extension_count = 0;
	const char ** glfw_extensions;

	glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

	// Create and fill in array containing the extensions required by both glfw and the application
	uint32_t required_extension_count = glfw_extension_count + enable_validation_layers;
	const char ** required_extensions = malloc(required_extension_count * sizeof(char *));

	size_t i = 0;
	for (i = 0; i < glfw_extension_count; i++)
	{
		required_extensions[i] = glfw_extensions[i];
	}
	required_extensions[i++] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;

	createInfo.enabledExtensionCount = required_extension_count;
	createInfo.ppEnabledExtensionNames = required_extensions;

	// Check the currently supported extensions
	uint32_t extension_count = 0;
	vkEnumerateInstanceExtensionProperties(NULL, &extension_count, NULL);
	VkExtensionProperties * extensions = malloc(sizeof(VkExtensionProperties) * extension_count);
	vkEnumerateInstanceExtensionProperties(NULL, &extension_count, extensions);

	/* Check if the system supports our required extensions */
	for (i = 0; i < required_extension_count; i++)
	{
		int exists = 0;
		for (size_t j = 0; j < extension_count; j++)
		{
			if (strcmp(required_extensions[i], extensions[j].extensionName) == 0)
			{
				exists = 1;
				LOG("Enabling extension '%s'", required_extensions[i]);

				break;
			}
		}
		if (exists == 0)
		{
			LOG_E("System does not provide the required extension '%s'", required_extensions[i]);
			return -1;
		}
	}

	// Check for validation layer support
	uint32_t layer_count;
	vkEnumerateInstanceLayerProperties(&layer_count, NULL);
	VkLayerProperties * available_layers = malloc(layer_count * sizeof(VkLayerProperties));
	vkEnumerateInstanceLayerProperties(&layer_count, available_layers);

	for (i = 0; i < validation_layers_count; i++)
	{
		int layer_exists = 0;
		for (size_t j = 0; j < layer_count; j++)
		{
			if (strcmp(validation_layers[0], available_layers[j].layerName) == 0)
			{
				layer_exists = 1;
				LOG("Enabling validation layer '%s'", validation_layers[i]);
				break;
			}
		}
		if (!layer_exists)
		{
			LOG_E("Validation layer '%s' does not exist", validation_layers[i]);
			return -1;
		}
	}

	// Setup a temporary debug messenger and enable validation layers
	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
	if (enable_validation_layers)
	{
		createInfo.enabledLayerCount = validation_layers_count;
		createInfo.ppEnabledLayerNames = validation_layers;
		populate_debug_messenger_create_info(&debugCreateInfo);
		createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT *)&debugCreateInfo;
	}
	else
	{
		createInfo.enabledLayerCount = 0;
		createInfo.ppEnabledLayerNames = NULL;
	}

	// Create the vulkan instance
	VkResult result = vkCreateInstance(&createInfo, NULL, &instance);
	if (result != VK_SUCCESS)
	{
		LOG_E("Failed to create vulkan instance - code %d", result);
		return result;
	}

	free(required_extensions);
	free(extensions);
	free(available_layers);

	return 0;
}

int create_debug_messenger()
{
	if (!enable_validation_layers)
		return 0;
	// Enable debug message callback
	VkDebugUtilsMessengerCreateInfoEXT createInfo = {0};
	populate_debug_messenger_create_info(&createInfo);

	VkResult result = create_debug_utils_messenger_ext(instance, &createInfo, NULL, &debug_messenger);
	if (result != VK_SUCCESS)
	{
		LOG_E("Failed to set up debug messenger");
		return result;
	}
	return 0;
}

QueueFamilies get_queue_families(VkPhysicalDevice device)
{
	QueueFamilies indices = {0, 0};

	uint32_t queue_family_count = 0;
	VkQueueFamilyProperties * queue_families = NULL;
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

// Checks if supplied device supports all requirements for picking a device
// Returns 0 on success
bool check_device_extension_support(VkPhysicalDevice device)
{
	// Check if all necessary extensions are supported
	uint32_t extension_count;
	VkExtensionProperties * available_extensions = NULL;
	vkEnumerateDeviceExtensionProperties(device, NULL, &extension_count, NULL);
	available_extensions = malloc(extension_count * sizeof(VkExtensionProperties));
	vkEnumerateDeviceExtensionProperties(device, NULL, &extension_count, available_extensions);

	/* Check if the system supports our required extensions */
	size_t i = 0;
	for (i = 0; i < device_extensions_count; i++)
	{
		int exists = 0;
		for (size_t j = 0; j < extension_count; j++)
		{
			if (strcmp(device_extensions[i], available_extensions[j].extensionName) == 0)
			{
				exists = 1;
				break;
			}
		}
		if (exists == 0)
		{
			return -1;
		}
	}

	free(available_extensions);
	return 0;
}

int create_surface()
{
	VkResult result = glfwCreateWindowSurface(instance, window_get_raw(window), NULL, &surface);
	if (result != VK_SUCCESS)
	{
		LOG_E("Failed to create KHR surface - code %d", result);
	}
	return 0;
}

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

int pick_physical_device()
{
	// Pick a suitable device

	// Get all physical devices on the system
	uint32_t device_count = 0;
	VkPhysicalDevice * devices = NULL;
	vkEnumeratePhysicalDevices(instance, &device_count, NULL);
	if (device_count == 0)
	{
		LOG_E("Unable to retrieve graphical devices");
		return -1;
	}

	devices = malloc(device_count * sizeof(VkPhysicalDevice));
	vkEnumeratePhysicalDevices(instance, &device_count, devices);
	size_t i = 0;
	int max_score = 0;
	VkPhysicalDevice best_device = NULL;

	// Enumerate them and select the best one by score
	for (i = 0; i < device_count; i++)
	{
		int score = -1;
		// Check suitability of device
		VkPhysicalDeviceProperties deviceProperties;
		vkGetPhysicalDeviceProperties(devices[i], &deviceProperties);

		VkPhysicalDeviceFeatures deviceFeatures;
		vkGetPhysicalDeviceFeatures(devices[i], &deviceFeatures);

		if (check_device_extension_support(devices[i]))
			continue;

		// Application can't function without support for geometry shader
		if (!deviceFeatures.geometryShader)
			continue;

		// Checks to see if the required families are supported
		QueueFamilies indices = get_queue_families(devices[i]);
		if (!(indices.queue_validity & QUEUE_FAMILIES_COMLPLETE))
			continue;

		// Check to see if swap chain support is adequate
		bool swapchain_adequate = false;
		SwapchainSupportDetails swapchain_support = get_swapchain_support(devices[i]);
		swapchain_adequate = swapchain_support.format_count && swapchain_support.present_mode_count;
		if (!swapchain_adequate)
			continue;

		// Discrete GPUs have a significant performance advantage
		if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
		{
			score += 2000;
		}

		// Maximum possible size of textures affects graphics quality
		score += deviceProperties.limits.maxImageDimension2D;
		if (score > max_score)
		{
			max_score = score;
			best_device = devices[i];
		}
	}

	if (best_device == NULL)
	{
		LOG_E("Unable to find suitable graphical device");
		return -2;
	}

	// Print the best one and set physical_device to it
	VkPhysicalDeviceProperties deviceProperties;
	vkGetPhysicalDeviceProperties(best_device, &deviceProperties);
	LOG_OK("Picking graphical device '%s'", deviceProperties.deviceName);
	physical_device = best_device;

	free(devices);
	return 0;
}

int create_logical_device()
{
	// Get the queues we require
	QueueFamilies indices = get_queue_families(physical_device);

	// Calculate the number of unique queues
	uint32_t unique_queue_families[QUEUE_FAMILY_COUNT];
	uint32_t unique_queue_family_count = 0;
	for (size_t i = 0; i < QUEUE_FAMILY_COUNT; i++)
	{
		int exists = false;
		for (size_t j = 0; j < unique_queue_family_count; j++)
		{
			if (*(&indices.graphics + i) == unique_queue_families[j])
			{
				exists = true;
			}
		}
		if (!exists)
		{
			unique_queue_families[i] = *(&indices.graphics + i);
			unique_queue_family_count++;
		}
	}

	// Specify to create them along with the logical device
	VkDeviceQueueCreateInfo queueCreateInfos[QUEUE_FAMILY_COUNT] = {0};
	for (size_t i = 0; i < unique_queue_family_count; i++)
	{
		queueCreateInfos[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfos[i].queueFamilyIndex = *(&indices.graphics + i);
		queueCreateInfos[i].queueCount = 1;

		float queue_priority = 1.0f;
		queueCreateInfos[i].pQueuePriorities = &queue_priority;
	}

	VkPhysicalDeviceFeatures deviceFeatures = {0};

	VkDeviceCreateInfo createInfo = {0};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.pQueueCreateInfos = queueCreateInfos;
	createInfo.queueCreateInfoCount = unique_queue_family_count;

	// The device specific extension
	// The physical device extension support has been checked before
	createInfo.enabledExtensionCount = device_extensions_count;
	createInfo.ppEnabledExtensionNames = device_extensions;

	createInfo.pEnabledFeatures = &deviceFeatures;

	// Specify device validation layers for support with older implementations
	if (enable_validation_layers)
	{
		createInfo.enabledLayerCount = validation_layers_count;
		createInfo.ppEnabledLayerNames = validation_layers;
	}
	else
	{
		createInfo.enabledLayerCount = 0;
	}

	// Create a logical device to interface with the previously picked physical device
	VkResult result = vkCreateDevice(physical_device, &createInfo, NULL, &device);
	if (result != VK_SUCCESS)
	{
		LOG_E("Failed to create logical device - code %d", result);
		return -1;
	}

	// Get the handles for the newly created queues
	vkGetDeviceQueue(device, indices.graphics, 0, &graphics_queue);
	vkGetDeviceQueue(device, indices.present, 0, &present_queue);

	return 0;
}

// Picks the best swapchain surface format and returns it from those supplied
VkSurfaceFormatKHR pick_swap_surface_format(VkSurfaceFormatKHR * formats, size_t count)
{
	for (size_t i = 0; i < count; i++)
	{
		// The best format exists and is returned
		if (formats[i].format == VK_FORMAT_B8G8R8_UNORM && formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{
			LOG_OK("The best swapchain surface format was available");
			return formats[i];
		}
	}

	// If the best one wasn't available
	return formats[0];
}

// Picks the best swapchain present mode and returns it from those supplied
VkPresentModeKHR pick_swap_present_mode(VkPresentModeKHR * modes, size_t count)
{
	for (size_t i = 0; i < count; i++)
	{
		// Triple buffered present mode is preferred
		if (modes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
		{
			LOG_OK("The best swapchain present mode was available");
			return modes[i];
		}
	}

	// Only FIFO/double buffered vsync is guranteed to be available
	// Use it as a fallback
	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D pick_swap_extent(VkSurfaceCapabilitiesKHR * capabilities)
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

int create_swapchain()
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
		LOG_E("Failed to create swapchaing - code %d", result);
		return -1;
	}
	swapchain_image_format = surface_format.format;
	swapchain_extent = extent;

	// Retrieve swapchain images
	vkGetSwapchainImagesKHR(device, swapchain, &swapchain_image_count, NULL);
	swapchain_images = malloc(swapchain_image_count * sizeof(VkImage));
	vkGetSwapchainImagesKHR(device, swapchain, &swapchain_image_count, swapchain_images);
	return 0;
}

int create_image_views()
{
	swapchain_image_views = malloc(swapchain_image_count * sizeof(VkImageView));
	swapchain_image_view_count = swapchain_image_count;
	size_t i = 0;
	for (i = 0; i < swapchain_image_count; i++)
	{
		VkImageViewCreateInfo createInfo = {0};
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.image = swapchain_images[i];

		// What type of image we are storing
		createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		createInfo.format = swapchain_image_format;

		createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

		// How the images should be accessed
		createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		createInfo.subresourceRange.baseMipLevel = 0;
		createInfo.subresourceRange.levelCount = 1;
		createInfo.subresourceRange.baseArrayLayer = 0;
		createInfo.subresourceRange.layerCount = 1;

		VkResult result = vkCreateImageView(device, &createInfo, NULL, &swapchain_image_views[i]);
		if (result != VK_SUCCESS)
		{
			LOG_E("Failed to create image view for swapchain image %d - code %d", i, result);
			return -1;
		}
	}
	return 0;
}

int vulkan_init()
{
	window = application_get_window();
	// Create a vulkan instance
	if (create_instance())
	{
		return -1;
	}
	if (create_debug_messenger())
	{
		return -2;
	}
	if (create_surface())
	{
		return -3;
	}
	if (pick_physical_device())
	{
		return -4;
	}
	if (create_logical_device())
	{
		return -5;
	}
	if (create_swapchain())
	{
		return -6;
	}
	LOG_OK("Successfully initialized vulkan");
	return 0;
}

void vulkan_terminate()
{
	// Destroy the image views since they were explicitely created
	for (size_t i = 0; i < swapchain_image_view_count; i++)
		vkDestroyImageView(device, swapchain_image_views[i], NULL);

	free(swapchain_images);
	free(swapchain_image_views);
	vkDestroySwapchainKHR(device, swapchain, NULL);
	vkDestroyDevice(device, NULL);
	if (enable_validation_layers)
	{
		destroy_debug_utils_messenger_ext(instance, debug_messenger, NULL);
	}
	vkDestroySurfaceKHR(instance, surface, NULL);
	vkDestroyInstance(instance, NULL);
}