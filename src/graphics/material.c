#include "material.h"
#include "graphics/texture.h"
#include "graphics/uniforms.h"
#include "utils.h"
#include "graphics/vulkan_internal.h"
#include "graphics/renderer.h"
#include "log.h"

#define GLOBAL_DESCRIPTOR__INDEX  0
#define MATERIAL_DESCRIPTOR_INDEX 1

struct Material
{
	char* name;

	// An array to all descriptor layouts of the pipeline
	// 0 : global descriptor layout
	// 1 : per material descriptor layout
	VkDescriptorSetLayout descriptor_layouts[2];
	VkDescriptorSet material_descriptors[3];
	VkPipeline pipeline;
	VkPipelineLayout pipeline_layout;

	// 0 : albedo
	Texture* textures[1];
};

Material* material_create(const char* file)
{
	Material* mat = malloc(sizeof(Material));
	mat->name = string_dup("Placeholder");

	// Fill in global layout
	mat->descriptor_layouts[GLOBAL_DESCRIPTOR__INDEX] = global_descriptor_layout;

	// Load textures
	mat->textures[0] = texture_create("./assets/textures/statue.jpg");

	// Fill in per material layout
	const uint32_t material_binding_count = 1;
	VkDescriptorSetLayoutBinding material_bindings[material_binding_count];
	material_bindings[0].binding = 0;
	material_bindings[0].descriptorCount = 1;
	material_bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	material_bindings[0].pImmutableSamplers = NULL;
	material_bindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	descriptorlayout_create(material_bindings, material_binding_count, &mat->descriptor_layouts[MATERIAL_DESCRIPTOR_INDEX]);

	// Create the material descriptors
	descriptorset_create(mat->descriptor_layouts[MATERIAL_DESCRIPTOR_INDEX], material_bindings, material_binding_count, NULL, mat->textures, mat->material_descriptors);

	// Create the pipeline
	struct PipelineCreateInfo pipeline_create_info = {0};
	pipeline_create_info.descriptor_layout_count = 2;
	pipeline_create_info.descriptor_layouts = mat->descriptor_layouts;
	pipeline_create_info.vertexshader = "./assets/shaders/standard.vert.spv";
	pipeline_create_info.fragmentshader = "./assets/shaders/standard.frag.spv";
	pipeline_create_info.geometryshader = NULL;
	pipeline_create_info.vertex_description = vertex_get_description();
	int result = create_graphics_pipeline(&pipeline_create_info, &mat->pipeline_layout, &mat->pipeline);
	if (result != 0)
	{
		LOG_E("Failed to create graphics pipeline for material %s - code %d", mat->name, result);
		free(mat->name);
		free(mat);
		return NULL;
	}

	return mat;
}

void material_bind(Material* mat, VkCommandBuffer command_buffer, uint32_t frame)
{
	if(frame == -1) frame = renderer_get_frameindex();

	vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mat->pipeline);

	// Bind global set 0
	vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mat->pipeline_layout, 0, 1, &global_descriptors[frame], 0, NULL);
	// Bind material set 1
	vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mat->pipeline_layout, 1, 1, &mat->material_descriptors[frame], 0, NULL);
}

void material_destroy(Material* mat)
{
	vkDeviceWaitIdle(device);

	free(mat->name);
	texture_destroy(mat->textures[0]);

	vkDestroyPipelineLayout(device, mat->pipeline_layout, NULL);
	vkDestroyPipeline(device, mat->pipeline, NULL);

	vkDestroyDescriptorSetLayout(device, mat->descriptor_layouts[MATERIAL_DESCRIPTOR_INDEX], NULL);
	free(mat);
}