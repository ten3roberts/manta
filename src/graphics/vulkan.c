#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "window.h"
#include "application.h"
#include "log.h"
#include "vulkan.h"
#include <stdlib.h>

VkInstance instance = VK_NULL_HANDLE;
VkDebugUtilsMessengerEXT debug_messenger = VK_NULL_HANDLE;
Window * window;
int create_instance();

const char * validation_layers[] = {"VK_LAYER_KHRONOS_validation"};
const size_t validation_layers_count = (sizeof(validation_layers) / sizeof(char *));

#if NDEBUG
const int enable_validation_layers = 0;
#else
const int enable_validation_layers = 1;
#endif

static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
													 VkDebugUtilsMessageTypeFlagsEXT messageType,
													 const VkDebugUtilsMessengerCallbackDataEXT * pCallbackData,
													 void * pUserData)
{
	if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
		LOG_E(pCallbackData->pMessage);
	else if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
		LOG_W(pCallbackData->pMessage);
	else
		LOG(pCallbackData->pMessage);
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

int create_instance()
{
	VkApplicationInfo appInfo;
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "crescent";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "crescent";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_0;
	appInfo.pNext = NULL;

	VkInstanceCreateInfo createInfo;
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pNext = NULL;
	createInfo.flags = 0;
	createInfo.pApplicationInfo = &appInfo;

	/* Get the extensions required by glfw and application */
	uint32_t glfw_extension_count = 0;
	const char ** glfw_extensions;

	glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

	/* Create and fill in array containing the extensions required by both glfw and the application */
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

	/* Check the currently supported extensions */
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
				LOG("Enabling extension %s", required_extensions[i]);

				break;
			}
		}
		if (exists == 0)
		{
			LOG_E("System does not provide the required extension %s", required_extensions[i]);
			return -1;
		}
	}

	/* Check for validation layer support */
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
				LOG("Enabling validation layer %s", validation_layers[i]);
				break;
			}
		}
		if (!layer_exists)
		{
			LOG_E("Validation layer %s does not exist", validation_layers[i]);
			return -1;
		}
	}
	/* Setup a temporary debug messenger and enable validation layers */
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
	free(available_layers);

	/* Create the vulkan instance */
	VkResult result = vkCreateInstance(&createInfo, NULL, &instance);
	if (result != VK_SUCCESS)
	{
		LOG_E("Failed to create vulkan instance - error %d", result);
		return result;
	}

	return 0;
}

int create_debug_messenger()
{
	if (!enable_validation_layers)
		return 0;
	/* Enable debug message callback */
	VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
	populate_debug_messenger_create_info(&createInfo);

	VkResult result = create_debug_utils_messenger_ext(instance, &createInfo, NULL, &debug_messenger);
	if (result != VK_SUCCESS)
	{
		LOG_E("Failed to set up debug messenger");
		return result;
	}
	return 0;
}

int vulkan_init()
{
	window = application_get_window();
	/* Create a vulkan instance */
	if (create_instance())
	{
		return -1;
	}
	if (create_debug_messenger())
	{
		return -1;
	}
	LOG_OK("Successfully initialized vulkan");
	return 0;
}

void vulkan_terminate()
{
	if (enable_validation_layers)
	{
		destroy_debug_utils_messenger_ext(instance, debug_messenger, NULL);
	}

	vkDestroyInstance(instance, NULL);
}