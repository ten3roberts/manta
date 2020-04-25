#include "vulkan_internal.h"
#include "vertexbuffer.h"
#include "indexbuffer.h"
#include "application.h"
#include "log.h"
#include "utils.h"
#include "math/math.h"
#include "vulkan.h"
#include "swapchain.h"
#include "uniforms.h"
#include "settings.h"
#include "cr_time.h"
#include "texture.h"
#include "buffer.h"
#include "magpie.h"
#include <stdbool.h>
#include "model.h"
#include "material.h"

Model* model_cube;
Model* model;
Material* material;
VkDescriptorSetLayoutBinding bindings[2];

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

	if (enable_validation_layers)
	{ // Check for validation layer support
		uint32_t layer_count;
		vkEnumerateInstanceLayerProperties(&layer_count, NULL);
		VkLayerProperties* available_layers = malloc(layer_count * sizeof(VkLayerProperties));
		vkEnumerateInstanceLayerProperties(&layer_count, available_layers);

		for (i = 0; i < validation_layers_count; i++)
		{
			int layer_exists = 0;
			for (size_t j = 0; j < layer_count; j++)
			{
				if (strcmp(validation_layers[i], available_layers[j].layerName) == 0)
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
		free(available_layers);
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

		// Does not support anisotropic filtering
		if (deviceFeatures.samplerAnisotropy == VK_FALSE)
		{
			continue;
		}

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
	VkSampleCountFlagBits wanted_samples = VK_SAMPLE_COUNT_1_BIT;
	switch (settings_get_msaa())
	{
	case 1:
		wanted_samples = VK_SAMPLE_COUNT_1_BIT;
		break;
	case 4:
		wanted_samples = VK_SAMPLE_COUNT_4_BIT;
		break;
	case 8:
		wanted_samples = VK_SAMPLE_COUNT_8_BIT;
		break;
	case 16:
		wanted_samples = VK_SAMPLE_COUNT_16_BIT;
		break;
	case 32:
		wanted_samples = VK_SAMPLE_COUNT_32_BIT;
		break;
	case 64:
		wanted_samples = VK_SAMPLE_COUNT_64_BIT;
		break;

	default:
		break;
	}
	msaa_samples = min(wanted_samples, get_max_sample_count(physical_device));

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
	deviceFeatures.samplerAnisotropy = VK_TRUE;

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
		swapchain_image_views[i] =
			image_view_create(swapchain_images[i], swapchain_image_format, VK_IMAGE_ASPECT_COLOR_BIT);
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
	// Frame buffer
	VkAttachmentDescription color_attachment = {0};
	color_attachment.format = swapchain_image_format;
	color_attachment.samples = msaa_samples;
	color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

	// Resolve msaa
	VkAttachmentDescription colorAttachmentResolve = {0};
	colorAttachmentResolve.format = swapchain_image_format;
	colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference colorAttachmentResolveRef = {0};
	colorAttachmentResolveRef.attachment = 2;
	colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	// Ignore stencil data since we don't have a depth buffer
	color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

	color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	color_attachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference color_attachment_ref = {0};
	color_attachment_ref.attachment = 0;
	color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	//  Depth buffer
	VkAttachmentDescription depth_attachment = {0};
	depth_attachment.format = find_depth_format();
	depth_attachment.samples = msaa_samples;
	depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depth_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depth_attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depth_attachment_ref = {0};
	depth_attachment_ref.attachment = 1;
	depth_attachment_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	// Subpasses
	VkSubpassDescription subpass = {0};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &color_attachment_ref;
	subpass.pDepthStencilAttachment = &depth_attachment_ref;
	subpass.pResolveAttachments = &colorAttachmentResolveRef;

	// Subpass dependencies
	VkSubpassDependency dependency = {0};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;

	// Specify what the subpass should wait for
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;

	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	// Render pass info
	VkAttachmentDescription attachments[] = {color_attachment, depth_attachment, colorAttachmentResolve};
	VkRenderPassCreateInfo renderPassInfo = {0};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = sizeof(attachments) / sizeof(*attachments);
	renderPassInfo.pAttachments = attachments;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
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

int create_graphics_pipeline(struct PipelineCreateInfo* createinfo, VkPipelineLayout* dst_pipeline_layout,
							 VkPipeline* dst_pipeline)
{
	// Read vertex shader from SPIR-V
	size_t vert_code_size = read_fileb(createinfo->vertexshader, NULL);
	if (vert_code_size == 0)
	{
		LOG_E("Failed to read vertex shader %s from binary file", createinfo->vertexshader);
		return -1;
	}
	char* vert_shader_code = malloc(vert_code_size);
	read_fileb(createinfo->vertexshader, vert_shader_code);

	// Read fragment shader from SPIR-V
	size_t frag_code_size = read_fileb(createinfo->fragmentshader, NULL);
	if (vert_code_size == 0)
	{
		LOG_E("Failed to read vertex shader %s from binary file", "./assets/shaders/standard.frag.spv");
		return -1;
	}
	char* frag_shader_code = malloc(frag_code_size);
	read_fileb(createinfo->fragmentshader, frag_shader_code);

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
	VertexInputDescription vertex_description = createinfo->vertex_description;
	VkPipelineVertexInputStateCreateInfo vertex_input_info = {0};
	vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertex_input_info.vertexBindingDescriptionCount = 1;
	vertex_input_info.pVertexBindingDescriptions = &vertex_description.binding_description;
	vertex_input_info.vertexAttributeDescriptionCount = vertex_description.attribute_count;
	vertex_input_info.pVertexAttributeDescriptions = vertex_description.attributes;

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
	rasterizer.cullMode = VK_CULL_MODE_NONE;
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
	multisampling.rasterizationSamples = msaa_samples;
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

	// Depth buffer
	VkPipelineDepthStencilStateCreateInfo depthStencil = {0};
	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable = VK_TRUE;
	depthStencil.depthWriteEnable = VK_TRUE;
	depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;

	// Don't use bounds
	depthStencil.depthBoundsTestEnable = VK_FALSE;
	depthStencil.minDepthBounds = 0.0f; // Optional
	depthStencil.maxDepthBounds = 1.0f; // Optional

	// Disable stencil
	depthStencil.stencilTestEnable = VK_FALSE;
	depthStencil.front = (VkStencilOpState){0}; // Optional
	depthStencil.back = (VkStencilOpState){0};	// Optional

	// Dynamic states
	VkDynamicState dynamicStates[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_LINE_WIDTH};

	VkPipelineDynamicStateCreateInfo dynamicState = {0};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = 2;
	dynamicState.pDynamicStates = dynamicStates;

	// Pipeline layout
	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {0};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = createinfo->descriptor_layout_count;
	pipelineLayoutInfo.pSetLayouts = createinfo->descriptor_layouts;
	pipelineLayoutInfo.pushConstantRangeCount = 0; // TODO
	pipelineLayoutInfo.pPushConstantRanges = NULL; // TODO
	VkResult result = vkCreatePipelineLayout(device, &pipelineLayoutInfo, NULL, dst_pipeline_layout);
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
	pipelineInfo.pDepthStencilState = &depthStencil;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDynamicState = NULL; // Optional

	// Reference pipeline layout
	pipelineInfo.layout = *dst_pipeline_layout;

	// Render passes
	pipelineInfo.renderPass = renderPass;
	pipelineInfo.subpass = 0;

	// Create an entirely new pipeline, not deriving from an old one
	// VK_PIPELINE_CREATE_DERIVATE_BIT needs to be specified in createInfo
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
	pipelineInfo.basePipelineIndex = -1;			  // Optional

	result = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, dst_pipeline);
	if (result != VK_SUCCESS)
	{
		LOG_E("Failed to create graphics - code %d", result);
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

int create_color_buffer()
{
	VkFormat colorFormat = swapchain_image_format;

	image_create(swapchain_extent.width, swapchain_extent.height, colorFormat, VK_IMAGE_TILING_OPTIMAL,
				 VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
				 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &color_image, &color_image_memory, msaa_samples);
	color_image_view = image_view_create(color_image, colorFormat, VK_IMAGE_ASPECT_COLOR_BIT);
	return 0;
}

int create_depth_buffer()
{
	depth_image_format = find_depth_format();

	image_create(swapchain_extent.width, swapchain_extent.height, depth_image_format, VK_IMAGE_TILING_OPTIMAL,
				 VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &depth_image,
				 &depth_image_memory, msaa_samples);

	depth_image_view = image_view_create(depth_image, depth_image_format, VK_IMAGE_ASPECT_DEPTH_BIT);

	// Transition image layout explicitely
	transition_image_layout(depth_image, depth_image_format, VK_IMAGE_LAYOUT_UNDEFINED,
							VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
	return 0;
}

int create_framebuffers()
{
	framebuffer_count = swapchain_image_view_count;
	framebuffers = malloc(framebuffer_count * sizeof(VkFramebuffer));

	for (size_t i = 0; i < swapchain_image_view_count; i++)
	{
		VkImageView attachments[] = {color_image_view, depth_image_view, swapchain_image_views[i]};

		VkFramebufferCreateInfo framebufferInfo = {0};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = renderPass;
		framebufferInfo.attachmentCount = sizeof(attachments) / sizeof(*attachments);
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
		VkClearValue clear_values[2] = {{.color = {.float32 = {0.0f, 0.0f, 0.0f, 1.0f}}},
										{.depthStencil = {1.0f, 0.0f}}};
		render_pass_info.clearValueCount = 2;
		render_pass_info.pClearValues = clear_values;
		vkCmdBeginRenderPass(command_buffers[i], &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
		if (material == NULL || model == NULL)
			continue;
		material_bind(material, command_buffers[i], i);

		model_bind(model, command_buffers[i]);
		vkCmdDrawIndexed(command_buffers[i], model_get_index_count(model), 1, 0, 0, 0);

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
	if (create_command_pool())
	{
		return -12;
	}

	ub = ub_create(sizeof(TransformType), 0);

	// model_cube = model_load_collada("./assets/models/cube.dae");
	model = model_load_collada("./assets/models/plane.dae");

	// Create uniform buffer layout

	bindings[0].binding = 0;
	bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	bindings[0].descriptorCount = 1;
	bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	bindings[0].pImmutableSamplers = NULL; // Optional

	descriptorlayout_create(bindings, 1, &global_descriptor_layout);
	descriptorpack_create(global_descriptor_layout, bindings, 1,
						  (UniformBuffer**)&ub, (Texture**)&tex, &global_descriptors);

	material = material_load("./assets/materials/grid.json");

	if (create_color_buffer())
	{
		return -13;
	}
	if (create_depth_buffer())
	{
		return -13;
	}
	if (create_framebuffers())
	{
		return -11;
	}

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

	if (model)
		model_destroy(model);

	vkDestroyDescriptorSetLayout(device, global_descriptor_layout, NULL);

	// Free textures and materials
	material_destroy_all();
	texture_destroy_all();

	if (global_descriptors.count)
		descriptorpack_destroy(&global_descriptors);

	ub_destroy(ub);
	ub_pools_destroy();

	vb_pools_destroy();




	// Wait for device to finish operations before cleaning up
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		vkDestroySemaphore(device, semaphores_render_finished[i], NULL);
		vkDestroySemaphore(device, semaphores_image_available[i], NULL);
		vkDestroyFence(device, in_flight_fences[i], NULL);
	}
	vkDestroyRenderPass(device, renderPass, NULL);
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