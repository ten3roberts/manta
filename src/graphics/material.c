#include "graphics/material.h"
#include "graphics/texture.h"
#include "graphics/uniforms.h"
#include "utils.h"
#include "graphics/vulkan_internal.h"
#include "graphics/renderer.h"
#include "log.h"
#include "libjson.h"
#include "hashtable.h"
#include "graphics/pipeline.h"
#include "magpie.h"

// A linked list tracking all loaded materials
static hashtable_t* material_table = NULL;

#define GLOBAL_DESCRIPTOR_INDEX	  0
#define MATERIAL_DESCRIPTOR_INDEX 1

struct Material
{
	// Name should not be modified after creation
	char name[256];

	// An array to all descriptor layouts of the pipeline
	// 0 : global descriptor layout
	// 1 : per material descriptor layout
	VkDescriptorSetLayout descriptor_layouts[2];
	DescriptorPack material_descriptors;
	Pipeline* pipeline;

	// The material indexes are specified by the json bindings
	uint32_t texture_count;
	Texture* textures[7];
	// The resource management part, inaccessible for the use
	struct Material *prev, *next;
};

// Loads a material from a json struct
Material* material_load_internal(JSON* object)
{
	Material* mat = calloc(1, sizeof(Material));

	// Set name if it exists, if it doesn't, use the filename without extension
	JSON* jname = json_get_member(object, "name");
	if (jname)
	{
		snprintf(mat->name, sizeof mat->name, "%s", json_get_string(jname));
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
		if (lname > sizeof mat->name)
			lname = sizeof mat->name;
		// Allocate memory for the name
		memcpy(mat->name, path_pos + 1, lname);
		mat->name[lname] = '\0';
	}

	// Create table if it doesn't exist
	if (material_table == NULL)
	{
		material_table = hashtable_create_string();
	}
	// Insert material into tracking table after name is acquired
	if (hashtable_find(material_table, mat->name) != NULL)
	{
		LOG_W("Duplicate material %s", mat->name);
		material_destroy(mat);
		return NULL;
	}
	// Insert into table
	hashtable_insert(material_table, mat->name, mat);

	// Fill in global layout, assumes global descriptors exist and does not create them
	mat->descriptor_layouts[GLOBAL_DESCRIPTOR_INDEX] = global_descriptor_layout;
	// Fill in per material layout
	JSON* jbindings = json_get_member(object, "bindings");
	if (json_type(jbindings) != JSON_TARRAY)
	{
		LOG_E("Failed to load material %s - bindings should be of type array", mat->name);
		material_destroy(mat);
		return NULL;
	}
	const int material_binding_count = json_get_count(jbindings);

	VkDescriptorSetLayoutBinding* material_bindings = malloc(material_binding_count * sizeof(VkDescriptorSetLayoutBinding));
	// Iterate and fill out the bindings
	JSON* bindcur = json_get_elements(jbindings);
	// The current iterator on where to fill the next used texture slot
	for (int i = 0; i < material_binding_count; i++)
	{
		// Initialize
		material_bindings[i].binding = 0;
		material_bindings[i].descriptorCount = 1;
		material_bindings[i].descriptorType = 0;
		material_bindings[i].pImmutableSamplers = 0;
		material_bindings[i].stageFlags = 0;

		// Read the binding info
		material_bindings[i].binding = json_get_member_number(bindcur, "binding");

		// Read the descriptor type
		const char* type = json_get_member_string(bindcur, "type");
		if (strcmp(type, "uniformbuffer") == 0)
		{
			material_bindings[i].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			// [TODO]: create uniform buffer
		}
		else if (strcmp(type, "sampler") == 0)
		{
			material_bindings[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			// Load the texture it specifies
			const char* texture_name = json_get_member_string(bindcur, "texture");
			if (texture_name == NULL)
			{
				LOG_E("Failed to load material %s - missing value for \"texture\" in binding %d", mat->name, i);
				free(material_bindings);
				material_destroy(mat);
				return NULL;
			}
			const char* texture_path = json_get_member_string(object, texture_name);
			if (texture_path == NULL)
			{
				LOG_E("Failed to load material %s - missing texture \"%s\", required by binding %d", mat->name, texture_name, i);
				free(material_bindings);
				material_destroy(mat);
				return NULL;
			}
			mat->textures[mat->texture_count++] = texture_get(texture_path);
		}
		else
		{
			LOG_E("Failed to load material %s - unknown binding type \"%s\"", mat->name, type);
			free(material_bindings);
			material_destroy(mat);
			return NULL;
		}
		// Read the shader stage of the resource
		const char* stage = json_get_member_string(bindcur, "stage");
		if (strcmp(stage, "vertex") == 0)
		{
			material_bindings[i].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		}
		else if (strcmp(stage, "fragment") == 0)
		{
			material_bindings[i].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		}
		else
		{
			LOG_E("Failed to load material %s - unknown shader stage \"%s\"", mat->name, stage);
			free(material_bindings);
			material_destroy(mat);
			return NULL;
		}

		bindcur = json_next(bindcur);
	}

	descriptorlayout_create(material_bindings, material_binding_count, &mat->descriptor_layouts[MATERIAL_DESCRIPTOR_INDEX]);

	// Create the material descriptors
	descriptorpack_create(mat->descriptor_layouts[MATERIAL_DESCRIPTOR_INDEX], material_bindings, material_binding_count, NULL, mat->textures, &mat->material_descriptors);

	free(material_bindings);
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
	struct PipelineInfo pipeline_info = {0};
	pipeline_info.descriptor_layout_count = 2;
	pipeline_info.descriptor_layouts = mat->descriptor_layouts;
	pipeline_info.vertexshader = string_dup(vertexshader);
	pipeline_info.fragmentshader = string_dup(fragmentshader);
	pipeline_info.geometryshader = NULL;
	pipeline_info.vertex_description = vertex_get_description();
	mat->pipeline = pipeline_get(&pipeline_info);
	if (mat->pipeline == NULL)
	{
		LOG_E("Failed to create graphics pipeline for material %s", mat->name);
		material_destroy(mat);
		return NULL;
	}

	return mat;
}

Material* material_load(const char* file)
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
		LOG_E("Failed to load material from json %s - root schema should be of either object or array type", file);
		return NULL;
	}

	json_destroy(root);

	return mat;
}

Material* material_get(const char* name)
{
	// No materials loaded
	if (material_table == NULL)
		return NULL;
	return hashtable_find(material_table, name);
}

void material_bind(Material* mat, VkCommandBuffer command_buffer, uint32_t frame)
{
	if (frame == -1)
		frame = renderer_get_frameindex();

	pipeline_bind(mat->pipeline, command_buffer);

	// Get the layout from the pipeline
	VkPipelineLayout pipeline_layout = pipeline_get_layout(mat->pipeline);
	
	// Bind global set 0
	vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1, &global_descriptors.sets[frame], 0, NULL);
	// Bind material set 1
	vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 1, 1, &mat->material_descriptors.sets[frame], 0, NULL);
}

void material_destroy(Material* mat)
{
	// Remove from table if it exists
	hashtable_remove(material_table, mat->name);
	// Last texture was removed
	if (hashtable_get_count(material_table) == 0)
	{
		hashtable_destroy(material_table);
		material_table = NULL;
	}

	vkDeviceWaitIdle(device);

	// Destroy all textures
	for (uint32_t i = 0; i < mat->texture_count; i++)
	{
		texture_destroy(mat->textures[i]);
	}

	pipeline_destroy(mat->pipeline);

	descriptorpack_destroy(&mat->material_descriptors);
	vkDestroyDescriptorSetLayout(device, mat->descriptor_layouts[MATERIAL_DESCRIPTOR_INDEX], NULL);

	free(mat);
}

void material_destroy_all()
{
	Material* mat = NULL;
	while (material_table && (mat = hashtable_pop(material_table)))
	{
		material_destroy(mat);
	}
}