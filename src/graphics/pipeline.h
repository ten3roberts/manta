#ifndef PIPELINE_H
#define PIPELINE_H
#include <vulkan/vulkan.h>
#include "graphics/vertexbuffer.h"

struct PipelineInfo
{
	const char* vertexshader;
	const char* fragmentshader;
	const char* geometryshader;

	VkDescriptorSetLayout* descriptor_layouts;
	uint32_t descriptor_layout_count;

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

#endif