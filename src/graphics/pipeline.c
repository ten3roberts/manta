#include "graphics/pipeline.h"
#include "graphics/vulkan_members.h"
#include "hashtable.h"
#include "log.h"
#include "utils.h"
#include <vulkan/vulkan.h>
#include <string.h>
#include <magpie.h>

// Holds all currently loaded pipelines
// The hash function will hash the info struct
static hashtable_t* pipeline_table = NULL;

// The hash checks if the
static uint32_t hash_pipelineinfo(const void* key)
{
	struct PipelineInfo* info = (struct PipelineInfo*)key;
	uint32_t result = 0;
	if (info->vertexshader)
		result += hashtable_hashfunc_string(info->vertexshader);
	if (info->geometryshader)
		result += hashtable_hashfunc_string(info->geometryshader);
	if (info->fragmentshader)
		result += hashtable_hashfunc_string(info->fragmentshader);

	return result;
}

static int32_t comp_pipelineinfo(const void* pkey1, const void* pkey2)
{
	struct PipelineInfo* info1 = (struct PipelineInfo*)pkey1;
	struct PipelineInfo* info2 = (struct PipelineInfo*)pkey2;

	// Check to see if the shaders are identical
	if (strcmp_s(info1->vertexshader, info2->vertexshader))
		return 1;
	if (strcmp_s(info1->fragmentshader, info2->fragmentshader))
		return 1;
	if (strcmp_s(info1->geometryshader, info2->geometryshader))
		return 1;

	// Match
	return 0;
}

static int pipeline_create(struct PipelineInfo* info, VkPipeline* pipeline, VkPipelineLayout* layout);

struct Pipeline
{
	VkPipeline pipeline;
	VkPipelineLayout layout;
	struct PipelineInfo info;
};

Pipeline* pipeline_get(struct PipelineInfo* info)
{
	// Create the hashtable with the custom types
	if (pipeline_table == NULL)
		pipeline_table = hashtable_create(hash_pipelineinfo, comp_pipelineinfo);

	Pipeline* pipeline = hashtable_find(pipeline_table, info);
	if (pipeline)
	{
		return pipeline;
	}

	// Pipeline does not exist
	pipeline = malloc(sizeof(Pipeline));
	pipeline->info = *info;
	int result = pipeline_create(info, &pipeline->pipeline, &pipeline->layout);
	if (result != 0)
	{
		LOG_E("Pipeline creation using shaders %s, %s, and %s failed with code - %d", info->vertexshader, info->geometryshader, info->fragmentshader, result);
		return NULL;
	}

	// Insert into table
	hashtable_insert(pipeline_table, &pipeline->info, pipeline);
	return pipeline;
}
void pipeline_destroy(Pipeline* pipeline)
{
	hashtable_remove(pipeline_table, &pipeline->info);

	// Last texture was removed
	if (hashtable_get_count(pipeline_table) == 0)
	{
		hashtable_destroy(pipeline_table);
		pipeline_table = NULL;
	}

	// Allocate on the material size
	free(pipeline->info.descriptor_layouts);
	free(pipeline->info.push_constants);

	// Destroy vulkan objects
	vkDestroyPipeline(device, pipeline->pipeline, NULL);
	vkDestroyPipelineLayout(device, pipeline->layout, NULL);

	free(pipeline);
}
// Destroys all loaded pipelines
void pipeline_destroy_all()
{
	Pipeline* pipeline = NULL;
	while (pipeline_table && (tex = hashtable_pop(pipeline_table)))
	{
		pipeline_destroy(pipeline);
	}
}

void pipeline_bind(Pipeline* pipeline, VkCommandBuffer command_buffer)
{
	vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->pipeline);
}

VkPipelineLayout pipeline_get_layout(Pipeline* pipeline)
{
	return pipeline->layout;
}

void pipeline_recreate(Pipeline* pipeline)
{
	// Destroy old
	vkDestroyPipeline(device, pipeline->pipeline, NULL);
	vkDestroyPipelineLayout(device, pipeline->layout, NULL);

	int result = pipeline_create(&pipeline->info, &pipeline->pipeline, &pipeline->layout);
	if (result != 0)
	{
		LOG_E("Pipeline recreation using shaders %s, %s, and %s failed with code - %d", pipeline->info.vertexshader, pipeline->info.geometryshader, pipeline->info.fragmentshader,
			  result);
		return;
	}
}

void pipeline_recreate_all()
{
	LOG_S("Recreating all pipelines");
	hashtable_iterator* it = hashtable_iterator_begin(pipeline_table);
	Pipeline* pipeline = NULL;
	while ((pipeline = hashtable_iterator_next(it)))
	{
		pipeline_recreate(pipeline);
	}
	hashtable_iterator_end(it);
}

// Vulkan implementation of pipeline creation
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

static int pipeline_create(struct PipelineInfo* info, VkPipeline* pipeline, VkPipelineLayout* layout)
{
	// Read vertex shader from SPIR-V
	size_t vert_code_size = read_fileb(info->vertexshader, NULL);
	if (vert_code_size == 0)
	{
		LOG_E("Failed to read vertex shader %s from binary file", info->vertexshader);
		return -1;
	}
	char* vert_shader_code = malloc(vert_code_size);
	read_fileb(info->vertexshader, vert_shader_code);

	// Read fragment shader from SPIR-V
	size_t frag_code_size = read_fileb(info->fragmentshader, NULL);
	if (vert_code_size == 0)
	{
		LOG_E("Failed to read vertex shader %s from binary file", info->fragmentshader);
		return -2;
	}
	char* frag_shader_code = malloc(frag_code_size);
	read_fileb(info->fragmentshader, frag_shader_code);

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
	VertexInputDescription vertex_description = info->vertex_description;
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
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
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

	// TODO: Dynamic states
	/*VkDynamicState dynamicStates[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_LINE_WIDTH};

	VkPipelineDynamicStateCreateInfo dynamicState = {0};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = 2;
	dynamicState.pDynamicStates = dynamicStates;*/

	// Pipeline layout
	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {0};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = info->descriptor_layout_count;
	pipelineLayoutInfo.pSetLayouts = info->descriptor_layouts;
	pipelineLayoutInfo.pushConstantRangeCount = info->push_constant_count; // TODO
	pipelineLayoutInfo.pPushConstantRanges = info->push_constants; // TODO
	
	VkResult result = vkCreatePipelineLayout(device, &pipelineLayoutInfo, NULL, layout);
	if (result != VK_SUCCESS)
	{
		LOG_E("Failed to create pipeline layout - code %d", result);
		return -3;
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
	pipelineInfo.layout = *layout;

	// Render passes
	pipelineInfo.renderPass = renderPass;
	pipelineInfo.subpass = 0;

	// Create an entirely new pipeline, not deriving from an old one
	// VK_PIPELINE_CREATE_DERIVATE_BIT needs to be specified in createInfo
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
	pipelineInfo.basePipelineIndex = -1;			  // Optional

	result = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, pipeline);
	if (result != VK_SUCCESS)
	{
		LOG_E("Failed to create graphics - code %d", result);
		return -4;
	}
	// Free the temporary resources
	free(vert_shader_code);
	free(frag_shader_code);

	// Modules are not needed after the creaton
	vkDestroyShaderModule(device, vert_shader_module, NULL);
	vkDestroyShaderModule(device, frag_shader_module, NULL);
	return 0;
}
