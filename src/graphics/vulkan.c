#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "window.h"
#include "application.h"
#include "log.h"
#include "vulkan.h"
#include <stdlib.h>

VkInstance instance;
Window * window;
int create_instance();

int vulkan_init()
{
	window = application_get_window();
	/* Create a vulkan instance */
	if (create_instance())
	{
		return -1;
	}
	LOG_OK("Successfully initialized vulkan");
	return 0;
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

	/* Get the extensions required by glfw */
	uint32_t glfw_extension_count = 0;
	const char ** glfw_extensions;

	glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

	createInfo.enabledExtensionCount = glfw_extension_count;
	createInfo.ppEnabledExtensionNames = glfw_extensions;

	/* Check the currently supported extensions */
	uint32_t extension_count = 0;
	vkEnumerateInstanceExtensionProperties(NULL, &extension_count, NULL);
	VkExtensionProperties * extensions = malloc(sizeof(VkExtensionProperties) * extension_count);
	vkEnumerateInstanceExtensionProperties(NULL, &extension_count, extensions);

	/* Check if the system supports those extensions */
	for (size_t i = 0; i < glfw_extension_count; i++)
	{
		int exists = 0;
		for (size_t j = 0; j < extension_count; j++)
		{
			if (strcmp(glfw_extensions[i], extensions[j].extensionName) == 0)
			{
				exists = 1;
				break;
			}
		}
		if (exists == 0)
		{
			LOG_E("System does not provide the required extension %sb", glfw_extensions[i]);
			return -1;
		}
	}

	createInfo.enabledLayerCount = 0;

	/* Create the vulkan instance */
	if (vkCreateInstance(&createInfo, NULL, &instance) != VK_SUCCESS)
	{
		LOG_E("Failed to create vulkan instance");
		return -1;
	}
	return 0;
}

void vulkan_terminate()
{
	 vkDestroyInstance(instance, NULL);
}