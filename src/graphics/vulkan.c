#include "vulkan_internal.h"
#include "vertexbuffer.h"
#include "indexbuffer.h"
#include "application.h"
#include "log.h"
#include "utils.h"
#include "math/math.h"
#include "vulkan.h"
#include "swapchain.h"
#include "ubo.h"
#include "cr_time.h"
#include <stdlib.h>
#include <stdbool.h>

VertexBuffer* vb = NULL;
VertexBuffer* vb2 = NULL;

IndexBuffer* ib = NULL;

static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
													 VkDebugUtilsMessageTypeFlagsEXT messageType,
													 const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
													 void* pUserData)
{
	if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
		log_call(CONSOLE_RED, "Vulkan debug callback", "(%d)%s", messageSeverity, pCallbackData->pMessage);
	else if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
		log_call(CONSOLE_YELLOW, "Vulkan debug callback", "(%d)%s", messageSeverity, pCallbackData->pMessage);
	else
		log_call(CONSOLE_BLUE, "Vulkan debug callback", "(%d)%s", messageSeverity, pCallbackData->pMessage);
	return VK_FALSE;
}

VkResult create_debug_utils_messenger_ext(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
										  const VkAllocationCallbacks* pAllocator,
										  VkDebugUtilsMessengerEXT* pDebugMessenger)
{
	VkResult (*func)(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
					 const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pMessenger) =
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
									   const VkAllocationCallbacks* pAllocator)
{
	void (*func)(VkInstance instance, VkDebugUtilsMessengerEXT messenger, const VkAllocationCallbacks* pAllocator) =
		(PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");

	if (func != NULL)
	{
		func(instance, debugMessenger, pAllocator);
	}
}

void populate_debug_messenger_create_info(VkDebugUtilsMessengerCreateInfoEXT* createInfo)
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
	const char** glfw_extensions;

	glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

	// Create and fill in array containing the extensions required by both glfw and the application
	uint32_t required_extension_count = glfw_extension_count + enable_validation_layers;
	const char** required_extensions = malloc(required_extension_count * sizeof(char*));

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
	VkExtensionProperties* extensions = malloc(sizeof(VkExtensionProperties) * extension_count);
	vkEnumerateInstanceExtensionProperties(NULL, &extension_count, extensions);

	// Check if the system supports our required extensions
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
	VkLayerProperties* available_layers = malloc(layer_count * sizeof(VkLayerProperties));
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
		createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
	}
	else
	{
		createInfo.enabledLayerCount = 0;
		createInfo.ppEnabledLayerNames = NULL;
	}

	// Create the vulkan instance
	// -9 when creating again and again
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

	LOG_S("Creating debug messenger");
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

// Checks if supplied device supports all requirements for picking a device
// Returns 0 on success
bool check_device_extension_support(VkPhysicalDevice device)
{
	// Check if all necessary extensions are supported
	uint32_t extension_count;
	VkExtensionProperties* available_extensions = NULL;
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

int pick_physical_device()
{
	// Pick a suitable device

	// Get all physical devices on the system
	uint32_t device_count = 0;
	VkPhysicalDevice* devices = NULL;
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

VkShaderModule create_shader_module(char* code, size_t size)
{
	VkShaderModuleCreateInfo createInfo = {0};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = size;
	createInfo.pCode = (uint32_t*)code;
	VkShaderModule shader_module;
	VkResult result = vkCreateShaderModule(device, &createInfo, NULL, &shader_module);
	if (result != VK_SUCCESS)
	{
		LOG_E("Failed to create shader module - code %d", result);
	}
	return shader_module;
}

int create_render_pass()
{
	VkAttachmentDescription color_attachment = {0};
	color_attachment.format = swapchain_image_format;
	color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
	color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

	// Ignore stencil data since we don't have a depth buffer
	color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

	color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference color_attachment_ref = {0};
	color_attachment_ref.attachment = 0;
	color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = {0};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &color_attachment_ref;

	VkRenderPassCreateInfo renderPassInfo = {0};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = 1;
	renderPassInfo.pAttachments = &color_attachment;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;

	// Subpass dependencies
	VkSubpassDependency dependency = {0};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;

	// Specify what the subpass should wait for
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;

	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;

	VkResult result = vkCreateRenderPass(device, &renderPassInfo, NULL, &renderPass);
	if (result != VK_SUCCESS)
	{
		LOG_E("Failed to create render pass - code %d", result);
		return -1;
	}
	return 0;
}

int create_graphics_pipeline()
{
	// Read vertex shader from SPIR-V
	size_t vert_code_size = read_fileb("./assets/shaders/standard.vert.spv", NULL);
	if (vert_code_size == 0)
	{
		LOG_E("Failed to read vertex shader %s from binary file", "./assets/shaders/standard.vert.spv");
		return -1;
	}
	char* vert_shader_code = malloc(vert_code_size);
	read_fileb("./assets/shaders/standard.vert.spv", vert_shader_code);

	// Read fragment shader from SPIR-V
	size_t frag_code_size = read_fileb("./assets/shaders/standard.frag.spv", NULL);
	if (vert_code_size == 0)
	{
		LOG_E("Failed to read vertex shader %s from binary file", "./assets/shaders/standard.frag.spv");
		return -1;
	}
	char* frag_shader_code = malloc(frag_code_size);
	read_fileb("./assets/shaders/standard.frag.spv", frag_shader_code);

	VkShaderModule vert_shader_module = create_shader_module(vert_shader_code, vert_code_size);
	VkShaderModule frag_shader_module = create_shader_module(frag_shader_code, frag_code_size);

	// Shader stage creation
	// Vertex shader
	VkPipelineShaderStageCreateInfo shader_stage_infos[2] = {{0}, {0}};
	// Create infos for the shaders
	// 0 - vertex shader
	// 1 - fragment shader

	shader_stage_infos[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shader_stage_infos[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	shader_stage_infos[0].module = vert_shader_module;
	shader_stage_infos[0].pName = "main";
	shader_stage_infos[0].pSpecializationInfo = NULL;

	// Fragment shader
	shader_stage_infos[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shader_stage_infos[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	shader_stage_infos[1].module = frag_shader_module;
	shader_stage_infos[1].pName = "main";
	shader_stage_infos[1].pSpecializationInfo = NULL;

	// Vertex input
	// Specify the data the vertex shader takes as input

	VertexInputDescription vertex_description = vertex_get_description();
	VkPipelineVertexInputStateCreateInfo vertex_input_info = {0};
	vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertex_input_info.vertexBindingDescriptionCount = 1;
	vertex_input_info.pVertexBindingDescriptions = &vertex_description.binding_description; // Optional
	vertex_input_info.vertexAttributeDescriptionCount = vertex_description.attribute_count;
	vertex_input_info.pVertexAttributeDescriptions = vertex_description.attributes; // Optional

	// Input assembly
	VkPipelineInputAssemblyStateCreateInfo inputAssembly = {0};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	// Specify viewports and scissors
	VkViewport viewport = {0};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float)swapchain_extent.width;
	viewport.height = (float)swapchain_extent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	// Specify a scissor rectangle that covers the entire frame buffer
	VkRect2D scissor = {0};
	scissor.offset = (VkOffset2D){0, 0};
	scissor.extent = swapchain_extent;

	// Combine viewport and scissor into a viewport state
	VkPipelineViewportStateCreateInfo viewportState = {0};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;

	// Rasterizer
	VkPipelineRasterizationStateCreateInfo rasterizer = {0};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	// Fills the polygons
	// Drawing lines or points requires enabling a GPU feature
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;

	// Cull mode
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;

	rasterizer.depthBiasEnable = VK_FALSE;
	rasterizer.depthBiasConstantFactor = 0.0f; // Optional
	rasterizer.depthBiasClamp = 0.0f;		   // Optional
	rasterizer.depthBiasSlopeFactor = 0.0f;	   // Optional

	// Multisampling
	// Requires GPU feature
	// For now, disable it
	VkPipelineMultisampleStateCreateInfo multisampling = {0};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampling.minSampleShading = 1.0f;			// Optional
	multisampling.pSampleMask = NULL;				// Optional
	multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
	multisampling.alphaToOneEnable = VK_FALSE;		// Optional

	// No depth and stencil testing for now

	// Color blending
	// For now, disabled
	VkPipelineColorBlendAttachmentState colorBlendAttachment = {0};
	colorBlendAttachment.colorWriteMask =
		VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;	 // Optional
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;			 // Optional
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;	 // Optional
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;			 // Optional

	VkPipelineColorBlendStateCreateInfo colorBlending = {0};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;
	colorBlending.blendConstants[0] = 0.0f; // Optional
	colorBlending.blendConstants[1] = 0.0f; // Optional
	colorBlending.blendConstants[2] = 0.0f; // Optional
	colorBlending.blendConstants[3] = 0.0f; // Optional

	// Dynamic states
	VkDynamicState dynamicStates[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_LINE_WIDTH};

	VkPipelineDynamicStateCreateInfo dynamicState = {0};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = 2;
	dynamicState.pDynamicStates = dynamicStates;

	// Pipeline layout
	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {0};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;				   // Optional
	pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout; // Optional
	pipelineLayoutInfo.pushConstantRangeCount = 0;		   // Optional
	pipelineLayoutInfo.pPushConstantRanges = NULL;		   // Optional
	VkResult result = vkCreatePipelineLayout(device, &pipelineLayoutInfo, NULL, &pipeline_layout);
	if (result != VK_SUCCESS)
	{
		LOG_E("Failed to create pipeline layout - code %d", result);
		return -1;
	}
	VkGraphicsPipelineCreateInfo pipelineInfo = {0};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.flags = 0;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shader_stage_infos;

	pipelineInfo.pVertexInputState = &vertex_input_info;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pDepthStencilState = NULL; // Optional
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDynamicState = NULL; // Optional

	// Reference pipelin layout
	pipelineInfo.layout = pipeline_layout;

	// Render passes
	pipelineInfo.renderPass = renderPass;
	pipelineInfo.subpass = 0;

	// Create an entirely new pipeline, not deriving from an old one
	// VK_PIPELINE_CREATE_DERIVATE_BIT needs to be specified in createInfo
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
	pipelineInfo.basePipelineIndex = -1;			  // Optional

	result = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &graphics_pipeline);
	if (result != VK_SUCCESS)
	{
		LOG_E("Failed to create pipeline - code %d", result);
		return -2;
	}
	// Free the temporary resources
	free(vert_shader_code);
	free(frag_shader_code);

	// Modules are not needed after the creaton
	vkDestroyShaderModule(device, vert_shader_module, NULL);
	vkDestroyShaderModule(device, frag_shader_module, NULL);
	return 0;
}

int create_framebuffers()
{
	framebuffer_count = swapchain_image_view_count;
	framebuffers = malloc(framebuffer_count * sizeof(VkFramebuffer));

	for (size_t i = 0; i < swapchain_image_view_count; i++)
	{
		VkImageView attachments[] = {swapchain_image_views[i]};

		VkFramebufferCreateInfo framebufferInfo = {0};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = renderPass;
		framebufferInfo.attachmentCount = 1;
		framebufferInfo.pAttachments = attachments;
		framebufferInfo.width = swapchain_extent.width;
		framebufferInfo.height = swapchain_extent.height;
		framebufferInfo.layers = 1;

		VkResult result = vkCreateFramebuffer(device, &framebufferInfo, NULL, &framebuffers[i]);
		if (result != VK_SUCCESS)
		{
			LOG_E("Failed to create frambuffer %d - code %d", i, result);
			return -i;
		}
	}
	return 0;
}

int create_command_pool()
{
	QueueFamilies queueFamilyIndices = get_queue_families(physical_device);

	VkCommandPoolCreateInfo poolInfo = {0};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = queueFamilyIndices.graphics;
	poolInfo.flags = 0; // Optional
	VkResult result = vkCreateCommandPool(device, &poolInfo, NULL, &command_pool);
	if (result != VK_SUCCESS)
	{
		LOG_E("Failed to create command pool - code %d", result);
		return -1;
	}
	return 0;
}

int create_command_buffers()
{
	command_buffer_count = framebuffer_count;
	command_buffers = malloc(command_buffer_count * sizeof(VkCommandBuffer));

	VkCommandBufferAllocateInfo allocInfo = {0};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = command_pool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = (uint32_t)command_buffer_count;

	VkResult result = vkAllocateCommandBuffers(device, &allocInfo, command_buffers);

	if (result != VK_SUCCESS)
	{
		LOG_E("Failed to create command buffers - code %d", result);
		return -1;
	}

	// Prerecord command buffers
	for (size_t i = 0; i < command_buffer_count; i++)
	{
		VkCommandBufferBeginInfo begin_info = {0};
		begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		begin_info.flags = 0;
		begin_info.pInheritanceInfo = NULL; // Optional

		// Start recording
		// If something was already recorded, it is reset
		// It is not possible to append recordings to a command buffer
		result = vkBeginCommandBuffer(command_buffers[i], &begin_info);
		if (result != VK_SUCCESS)
		{
			LOG_E("Failed to begin recording command buffer %d - code %d", i, result);
			return -2;
		}
		VkRenderPassBeginInfo render_pass_info = {0};
		render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		render_pass_info.renderPass = renderPass;
		render_pass_info.framebuffer = framebuffers[i];
		render_pass_info.renderArea.offset = (VkOffset2D){0, 0};
		render_pass_info.renderArea.extent = swapchain_extent;
		VkClearValue clearColor = {{{0.0f, 0.0f, 0.1f, 1.0f}}};
		render_pass_info.clearValueCount = 1;
		render_pass_info.pClearValues = &clearColor;
		vkCmdBeginRenderPass(command_buffers[i], &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
		vkCmdBindPipeline(command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline);
		ub_bind(ub, command_buffers[i], i);
		vb_bind(vb, command_buffers[i]);
		vb_bind(vb2, command_buffers[i]);
		ib_bind(ib, command_buffers[i]);
		vkCmdDrawIndexed(command_buffers[i], ib->index_count, 1, 0, 0, 0);
		vkCmdEndRenderPass(command_buffers[i]);
		result = vkEndCommandBuffer(command_buffers[i]);
		if (result != VK_SUCCESS)
		{
			LOG_E("Failed to record command buffer");
			return -3;
		}
	}
	return 0;
}

int create_sync_objects()
{
	images_in_flight = calloc(swapchain_image_count, sizeof *images_in_flight);

	VkSemaphoreCreateInfo semaphore_info = {0};
	semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fence_info = {0};
	fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	VkResult result;
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		result = vkCreateSemaphore(device, &semaphore_info, NULL, &semaphores_image_available[i]);
		if (result != VK_SUCCESS)
		{
			LOG_E("Failed to create image available semaphore for frame %d - code %d", i, result);
			return -1;
		}
		result = vkCreateSemaphore(device, &semaphore_info, NULL, &semaphores_render_finished[i]);
		if (result != VK_SUCCESS)
		{
			LOG_E("Failed to create render finished semaphore for frame %d - code %d", i, result);
			return -2;
		}
		result = vkCreateFence(device, &fence_info, NULL, &in_flight_fences[i]);
		if (result != VK_SUCCESS)
		{
			LOG_E("Failed to create in flight fence %d - code %d", i, result);
			return -3;
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
	if (swapchain_create())
	{
		return -6;
	}
	if (create_image_views())
	{
		return -7;
	}
	if (create_render_pass())
	{
		return -8;
	}
	if (ub_create_descriptor_set_layout(0))
	{
		return -9;
	}
	ub = ub_create(sizeof(TransformType));
	if (ub_create_descriptor_pool())
	{
		return -10;
	}
	if (ub_create_descriptor_sets(ub, sizeof(TransformType)))
	{
		return -11;
	}
	if (create_graphics_pipeline())
	{
		return -10;
	}
	if (create_framebuffers())
	{
		return -11;
	}
	if (create_command_pool())
	{
		return -12;
	}
	vb = vb_generate_triangle();
	vb2 = vb_generate_square();
	ib = ib_create();
	if (create_command_buffers())
	{
		return -13;
	}
	if (create_sync_objects())
	{
		return -14;
	}
	LOG_OK("Successfully initialized vulkan");
	return 0;
}

void vulkan_terminate()
{
	LOG_S("Terminating vulkan");

	vkDeviceWaitIdle(device);
	swapchain_destroy();
	//free(descriptor_sets);
	vkDestroyDescriptorSetLayout(device, descriptorSetLayout, NULL);
	vb_destroy(vb);
	vb_destroy(vb2);
	ib_destroy(ib);
	// Wait for device to finish operations before cleaning up
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		vkDestroySemaphore(device, semaphores_render_finished[i], NULL);
		vkDestroySemaphore(device, semaphores_image_available[i], NULL);
		vkDestroyFence(device, in_flight_fences[i], NULL);
	}
	vkDestroyCommandPool(device, command_pool, NULL);

	vkDestroyDevice(device, NULL);
	if (enable_validation_layers)
	{
		destroy_debug_utils_messenger_ext(instance, debug_messenger, NULL);
	}
	vkDestroySurfaceKHR(instance, surface, NULL);
	vkDestroyInstance(instance, NULL);
	free(images_in_flight);
	images_in_flight = NULL;
}