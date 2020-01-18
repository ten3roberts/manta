#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "window.h"
#include "application.h"
#include "log.h"
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

VkInstance instance = VK_NULL_HANDLE;
VkDebugUtilsMessengerEXT debug_messenger = VK_NULL_HANDLE;
VkPhysicalDevice physical_device = VK_NULL_HANDLE;
VkDevice device = VK_NULL_HANDLE;
VkQueue graphics_queue = VK_NULL_HANDLE;

// A pointer to the
Window * window;

const char * validation_layers[] = {"VK_LAYER_KHRONOS_validation"};
const size_t validation_layers_count = (sizeof(validation_layers) / sizeof(char *));

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
			// return -1;
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

#define GRAPHICS_QUEUE_VALID_BIT 0b1

// Specifies the indices for the different queue families
// Is filled in by get_queue_families function
typedef struct
{
	// The index for the graphics queue family
	uint32_t graphics;

	// Defines a bitfield of the valid queues
	// graphics queue 0b1
	int queue_validity;
} QueueFamilies;

QueueFamilies get_queue_families(VkPhysicalDevice device)
{
	QueueFamilies indices = {0, 0};

	uint32_t queue_family_count = 0;
	VkQueueFamilyProperties * queue_families = NULL;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, NULL);
	queue_families = malloc(queue_family_count * sizeof (VkQueueFamilyProperties));

	vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families);

	size_t i = 0;
	for (i = 0; i < queue_family_count; i++)
	{
		if (queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
			indices.graphics = i;
			indices.queue_validity |= GRAPHICS_QUEUE_VALID_BIT;
		}
	}

	free(queue_families);
	queue_families = NULL;

	return indices;
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

		// Application can't function without support for geometry shader
		if (!deviceFeatures.geometryShader)
			continue;

		// Checks to see if the required families are supported
		QueueFamilies indices = get_queue_families(devices[i]);
		if (!indices.queue_validity & 0b1)
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

	// Specify to create them along with the logical device
	VkDeviceQueueCreateInfo queueCreateInfo = {0};

	queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueCreateInfo.queueFamilyIndex = indices.graphics;
	queueCreateInfo.queueCount = 1;
	float queue_priority = 1.0f;
	queueCreateInfo.pQueuePriorities = &queue_priority;

	VkPhysicalDeviceFeatures deviceFeatures = {0};

	VkDeviceCreateInfo createInfo = {0};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.pQueueCreateInfos = &queueCreateInfo;
	createInfo.queueCreateInfoCount = 1;

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
	if(result != VK_SUCCESS)
	{
		LOG_E("Failed to create logical device - code %d", result);
		return -1;
	}

	// Get the handles for the newly created queues
	vkGetDeviceQueue(device, indices.graphics, 0, &graphics_queue);

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
	if (pick_physical_device())
	{
		return -3;
	}
	if (create_logical_device())
	{
		return -4;
	}
	LOG_OK("Successfully initialized vulkan");
	return 0;
}

void vulkan_terminate()
{
	vkDestroyDevice(device, NULL);
	if (enable_validation_layers)
	{
		destroy_debug_utils_messenger_ext(instance, debug_messenger, NULL);
	}

	vkDestroyInstance(instance, NULL);
}