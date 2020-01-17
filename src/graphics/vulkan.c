#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>


#include "window.h"
#include "application.h"
#include "log.h"
#include "vulkan.h"

VkInstance instance;
Window * window;
int create_instance();

int init_vulkan()
{
	window = application_get_window();
	/*Create a vulkan instance*/
	if(create_instance())
	{
		return -1;
	}
	LOG_OK("Successfully initialized vulkan");
	return 0;
}

int create_instance()
{
	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "crescent";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "crescent";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_0;

	VkInstanceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;

	/* Get the extensions required by glfw */
	uint32_t glfwExtensionCount = 0;
	const char ** glfwExtensions;

	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	createInfo.enabledExtensionCount = glfwExtensionCount;
	createInfo.ppEnabledExtensionNames = glfwExtensions;

	createInfo.enabledLayerCount = 0;

	/* Create the vulkan instance */
	VkResult result = vkCreateInstance(&createInfo, NULL, &instance);
	if (vkCreateInstance(&createInfo, NULL, &instance) != VK_SUCCESS)
	{
		LOG_E("Failed to create vulkan instance");
		return -1;
	}
	return 0;
}