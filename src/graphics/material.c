#include "material.h"
#include "graphics/texture.h"
#include "graphics/uniforms.h"
#include "utils.h"
#include "graphics/vulkan_internal.h"
#include "graphics/renderer.h"
#include "log.h"
#include "libjson.h"

#define GLOBAL_DESCRIPTOR_INDEX	  0
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

// Loads a material from a json struct
Material* material_load_internal(JSON* object)
{
	Material* mat = calloc(1, sizeof(Material));

	// Set name if it exists, if it doesn't, use the filename without extension
	JSON* jname = json_get_member(object, "name");
	if (jname)
	{
		mat->name = string_dup(json_get_string(jname));
	}
	// If name doesn't exist, use the filename
	else
	{
		const char* tmp_name = json_get_name(object);
		const char* ext_pos = strrchr(tmp_name, '.');
		if (ext_pos == NULL)
		{
			ext_pos = tmp_name + strlen(tmp_name);
		}
		// Get the start of the fileNAME
		// If not / succeeds, try \\ as crescent supports both (Win,Unix)
		const char* path_pos = NULL;
		path_pos = strrchr(tmp_name, '/');
		if (path_pos == NULL)
			path_pos = strrchr(tmp_name, '\\');
		// If no dir separator is found, the filepath is the filename
		if (path_pos == NULL)
			path_pos = tmp_name;

		size_t lname = ext_pos - path_pos;
		mat->name = malloc(lname);
		// Allocate memory for the name
		memcpy(mat->name, tmp_name, lname + 1);
		mat->name[lname] = '\0';
	}

	// Load the albedo texture
	JSON* jalbedo = json_get_member(object, "albedo");
	if (jalbedo && json_type(jalbedo) == JSON_TSTRING)
	{
		mat->textures[0] = texture_create(json_get_string(jalbedo));
	}
	else
	{
		LOG_E("Failed to read albedo from material %s", mat->name);
		material_destroy(mat);
		return NULL;
	}

	// Fill in global layout, assumes global descriptors exist and does not create them
	mat->descriptor_layouts[GLOBAL_DESCRIPTOR_INDEX] = global_descriptor_layout;

	// Fill in per material layout
	const uint32_t material_binding_count = 1;
	VkDescriptorSetLayoutBinding material_bindings[material_binding_count];
	material_bindings[0].binding = 0;
	material_bindings[0].descriptorCount = 1;
	material_bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	material_bindings[0].pImmutableSamplers = NULL;
	material_bindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	descriptorlayout_create(material_bindings, material_binding_count,
							&mat->descriptor_layouts[MATERIAL_DESCRIPTOR_INDEX]);

	// Create the material descriptors
	descriptorset_create(mat->descriptor_layouts[MATERIAL_DESCRIPTOR_INDEX], material_bindings, material_binding_count,
						 NULL, mat->textures, mat->material_descriptors);

	// Load the shaders
	// Get the shader names temporarily
	const char* vertexshader = json_get_member_string(object, "vertexshader");
	const char* fragmentshader = json_get_member_string(object, "fragmentshader");
	if (vertexshader == NULL || fragmentshader == NULL)
	{
		LOG_E("Failed to read shaders from material %s", mat->name);
		material_destroy(mat);
		return NULL;
	}

	// Create the graphics pipeline
	// Create the pipeline
	struct PipelineCreateInfo pipeline_create_info = {0};
	pipeline_create_info.descriptor_layout_count = 2;
	pipeline_create_info.descriptor_layouts = mat->descriptor_layouts;
	pipeline_create_info.vertexshader = vertexshader;
	pipeline_create_info.fragmentshader = fragmentshader;
	pipeline_create_info.geometryshader = NULL;
	pipeline_create_info.vertex_description = vertex_get_description();
	int result = create_graphics_pipeline(&pipeline_create_info, &mat->pipeline_layout, &mat->pipeline);
	if (result != 0)
	{
		LOG_E("Failed to create graphics pipeline for material %s - code %d", mat->name, result);
		material_destroy(mat);
		return NULL;
	}

	return mat;
}

Material* material_create(const char* file)
{
	JSON* root = json_loadfile(file);
	int jtype = json_type(root);

	if (root == NULL)
	{
		LOG_E("Failed to load material from json file %s", file);
		return NULL;
	}

	Material* mat = NULL;
	// Load the one root material
	if (jtype == JSON_TOBJECT)
	{
		mat = material_load_internal(root);
	}
	// Load several materials in linked lists [TODO]
	else if (jtype == JSON_TARRAY)
	{
		JSON* cur = json_get_elements(root);
		while (cur)
		{
			material_load_internal(cur);
			cur = json_next(cur);
		}
	}
	else
	{
		LOG_E("Failed to load material from json %s, root schema should be of either object or array type", file);
		return NULL;
	}
	
	json_destroy(root);

	return mat;
}

void material_bind(Material* mat, VkCommandBuffer command_buffer, uint32_t frame)
{
	if (frame == -1)
		frame = renderer_get_frameindex();

	vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mat->pipeline);

	// Bind global set 0
	vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mat->pipeline_layout, 0, 1,
							&global_descriptors[frame], 0, NULL);
	// Bind material set 1
	vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mat->pipeline_layout, 1, 1,
							&mat->material_descriptors[frame], 0, NULL);
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