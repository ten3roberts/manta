#ifndef PIPELINE_H
#define PIPELINE_H
#include "graphics/vertexbuffer.h"
#include <vulkan/vulkan.h>

struct PipelineInfo
{
	char vertexshader[4096];
	char fragmentshader[4096];
	char geometryshader[4096];

	VkDescriptorSetLayout* descriptor_layouts;
	uint32_t descriptor_layout_count;
	VkPushConstantRange* push_constants;
	uint32_t push_constant_count;

	VertexInputDescription vertex_description;
};

typedef struct Pipeline Pipeline;

// Creates a pipeline from info
// If a pipeline using that info already exists, it is returned instead
Pipeline* pipeline_get(struct PipelineInfo* info);
void pipeline_destroy(Pipeline* pipeline);
// Destroys all loaded pipelines
void pipeline_destroy_all();

void pipeline_bind(Pipeline* pipeline, VkCommandBuffer command_buffer);
// Returns the internal pipeline layout
VkPipelineLayout pipeline_get_layout(Pipeline* pipeline);

// Recreates a pipeline from the same info as creation but with updated swapchain data
void pipeline_recreate(Pipeline* pipeline);

// Recreates all pipelines
void pipeline_recreate_all();

#endif