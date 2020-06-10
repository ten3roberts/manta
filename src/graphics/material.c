#include "graphics/material.h"
#include "graphics/texture.h"
#include "graphics/uniforms.h"
#include "utils.h"
#include "graphics/vulkan_internal.h"
#include "graphics/renderer.h"
#include "log.h"
#include "libjson.h"
#include "handletable.h"
#include "graphics/pipeline.h"
#include "magpie.h"
#include "handlepool.h"
#include "handletable.h"

// A linked list tracking all loaded materials
static handletable_t* material_table = NULL;
static Material material_default = INVALID(Material);

#define GLOBAL_DESCRIPTOR_INDEX	  0
#define MATERIAL_DESCRIPTOR_INDEX 1
#define ENTITY_DESCRIPTOR_INDEX	  2

typedef struct Material_raw
{
	// Name should not be modified after creation
	char name[256];

	// An array to all descriptor layouts of the pipeline
	// 0 : global descriptor layout
	// 1 : per material descriptor layout
	// 2 : per rendertree node layout
	VkDescriptorSetLayout descriptor_layouts[3];
	DescriptorPack* material_descriptors;
	Pipeline* pipeline;

	VkPushConstantRange push_constants[4];
	uint32_t push_constant_count;

	// The material indexes are specified by the json bindings
	uint32_t texture_count;
	Texture textures[7];
	uint32_t sampler_count;
	Sampler samplers[7];
	// The resource management part, inaccessible for the use
	struct Material *prev, *next;
} Material_raw;

static handlepool_t material_pool = HANDLEPOOL_INIT(sizeof(Material_raw));

static const void* keyfunc_material(GenericHandle handle)
{
	return ((Material_raw*)handlepool_get_raw(&material_pool, handle))->name;
}
// Loads a material from a json struct
Material material_load_internal(JSON* object)
{
	const struct handle_wrapper* wrapper = handlepool_alloc(&material_pool);
	Material_raw* raw = (Material_raw*)wrapper->data;
	Material handle = PUN_HANDLE(wrapper->handle, Material);

	// Set name if it exists, if it doesn't, use the filename without extension
	JSON* jname = json_get_member(object, "name");
	if (jname)
	{
		snprintf(raw->name, sizeof raw->name, "%s", json_get_string(jname));
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
		// If not / succeeds, try \\ as manta supports both (Win,Unix)
		const char* path_pos = NULL;
		path_pos = strrchr(tmp_name, '/');
		if (path_pos == NULL)
			path_pos = strrchr(tmp_name, '\\');
		// If no dir separator is found, the filepath is the filename
		if (path_pos == NULL)
			path_pos = tmp_name;

		size_t lname = ext_pos - path_pos;
		if (lname > sizeof raw->name)
			lname = sizeof raw->name;
		// Allocate memory for the name
		memcpy(raw->name, path_pos + 1, lname - 1);
		raw->name[lname - 1] = '\0';
	}

	// Create table if it doesn't exist
	if (material_table == NULL)
	{
		material_table = handletable_create(keyfunc_material, handletable_hashfunc_string, handletable_comp_string);
	}

	// Insert material into tracking table after name is acquired
	if (HANDLE_VALID(handletable_find(material_table, raw->name)))
	{
		LOG_W("Duplicate material %s", raw->name);
		material_destroy(handle);
		return INVALID(Material);
	}

	// Insert into table
	handletable_insert(material_table, PUN_HANDLE(handle, GenericHandle));

	// Fill in global layout, assumes global descriptors exist and does not create them
	raw->descriptor_layouts[GLOBAL_DESCRIPTOR_INDEX] = global_descriptor_layout;
	raw->descriptor_layouts[ENTITY_DESCRIPTOR_INDEX] = rendertree_get_descriptor_layout();
	// Fill in per material layout
	JSON* jbindings = json_get_member(object, "bindings");
	if (json_type(jbindings) != JSON_TARRAY)
	{
		LOG_E("Failed to load material %s - bindings should be of type array", raw->name);
		material_destroy(handle);
		return INVALID(Material);
	}

	// Since samplers and images take up two layout bindings allocate twice the amount of children
	const int material_binding_count = json_get_count(jbindings);

	// Freed on destruction of pipeline
	VkDescriptorSetLayoutBinding* material_bindings = malloc(material_binding_count * sizeof(VkDescriptorSetLayoutBinding));
	// Iterate and fill out the bindings
	JSON* bindcur = json_get_elements(jbindings);

	raw->texture_count = 0;
	raw->sampler_count = 0;
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
		if (strcmp(type, "uniform") == 0)
		{
			material_bindings[i].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			// [TODO]: create uniform buffer
		}

		else if (strcmp(type, "texture") == 0)
		{
			material_bindings[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			// Load the texture it specifies
			const char* texture_name = json_get_member_string(bindcur, "texture");
			if (texture_name == NULL)
			{
				LOG_E("Failed to load material %s - missing value for \"texture\" in binding %d", raw->name, i);
				free(material_bindings);
				material_destroy(handle);
				return INVALID(Material);
			}
			const char* texture_path = json_get_member_string(object, texture_name);
			if (texture_path == NULL)
			{
				LOG_E("Failed to load material %s - missing texture \"%s\", required by binding %d", raw->name, texture_name, i);
				free(material_bindings);
				material_destroy(handle);
				return INVALID(Material);
			}
			raw->textures[raw->texture_count++] = texture_get(texture_path);

			// Sampler options

			const char* j_filterMode = json_get_member_string(bindcur, "filterMode");
			const char* j_wrapMode = json_get_member_string(bindcur, "wrapMode");

			VkFilter filterMode = VK_FILTER_LINEAR;
			VkSamplerAddressMode wrapMode = VK_SAMPLER_ADDRESS_MODE_REPEAT;

			if (j_filterMode == NULL)
			{
			}
			else if (strcmp(j_filterMode, "linear") == 0)
			{
				filterMode = VK_FILTER_LINEAR;
			}
			else if (strcmp(j_filterMode, "point") == 0 || strcmp(j_filterMode, "nearest") == 0)
			{
				filterMode = VK_FILTER_NEAREST;
			}
			if (j_wrapMode == NULL)
			{
			}
			else if (strcmp(j_wrapMode, "repeat") == 0)
			{
				wrapMode = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			}
			else if (strcmp(j_wrapMode, "clamp_edge") == 0)
			{
				wrapMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			}

			else if (strcmp(j_wrapMode, "clamp_border") == 0)
			{
				wrapMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
			}

			JSON* j_aniso = json_get_member(bindcur, "anisotropy");
			int anisotropy = 16;
			if (j_aniso && json_type(j_aniso) == JSON_TNUMBER)
				anisotropy = json_get_number(j_aniso);

			raw->samplers[raw->sampler_count++] = sampler_get(filterMode, wrapMode, anisotropy);
		}
		else
		{
			LOG_E("Failed to load material %s - unknown binding type \"%s\"", raw->name, type);
			free(material_bindings);
			material_destroy(handle);
			return INVALID(Material);
		}

		// Read the shader stage of the resource
		const char* stage = json_get_member_string(bindcur, "stage");
		if (stage == NULL)
		{
			LOG_E("Missing stage for binding %d in material %s", material_bindings[i].binding, raw->name);
		}
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
			LOG_E("Failed to load material %s - unknown shader stage \"%s\"", raw->name, stage);
			free(material_bindings);
			material_destroy(handle);
			return INVALID(Material);
		}

		bindcur = json_next(bindcur);
	}

	descriptorlayout_create(material_bindings, material_binding_count, &raw->descriptor_layouts[MATERIAL_DESCRIPTOR_INDEX]);

	// Create the material descriptors
	raw->material_descriptors = descriptorpack_create(raw->descriptor_layouts[MATERIAL_DESCRIPTOR_INDEX], material_bindings, material_binding_count);

	// Write the descriptors
	descriptorpack_write(raw->material_descriptors, material_bindings, material_binding_count, NULL, raw->textures, raw->samplers);

	free(material_bindings);
	// Load the shaders
	// Get the shader names temporarily
	const char* vertexshader = json_get_member_string(object, "vertexshader");
	const char* fragmentshader = json_get_member_string(object, "fragmentshader");
	if (vertexshader == NULL || fragmentshader == NULL)
	{
		LOG_E("Failed to read shaders from material %s", raw->name);
		material_destroy(handle);
		return INVALID(Material);
	}

	// Create the graphics pipeline
	// Create the pipeline
	struct PipelineInfo pipeline_info = {0};
	pipeline_info.descriptor_layout_count = 3;
	pipeline_info.descriptor_layouts = raw->descriptor_layouts;

	snprintf(pipeline_info.vertexshader, sizeof pipeline_info.vertexshader, "%s", vertexshader);
	snprintf(pipeline_info.fragmentshader, sizeof pipeline_info.fragmentshader, "%s", fragmentshader);
	snprintf(pipeline_info.geometryshader, sizeof pipeline_info.geometryshader, "%s", "");

	// Define one push constant for model matrix
	raw->push_constant_count = 1;
	raw->push_constants[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	raw->push_constants[0].offset = 0;
	raw->push_constants[0].size = PUSH_CONSTANT_SIZE;

	pipeline_info.push_constant_count = 1;
	pipeline_info.push_constants = malloc(pipeline_info.push_constant_count * sizeof(VkPushConstantRange));
	memcpy(pipeline_info.push_constants, raw->push_constants, raw->push_constant_count * sizeof(VkPushConstantRange));

	// Passed to pipeline. Freeing handled inside pipeline
	pipeline_info.vertex_description = vertex_get_description();
	raw->pipeline = pipeline_get(&pipeline_info);
	if (raw->pipeline == NULL)
	{
		LOG_E("Failed to create graphics pipeline for material %s", raw->name);
		material_destroy(handle);
		return INVALID(Material);
	}

	return handle;
}

Material material_load(const char* file)
{
	JSON* root = json_loadfile(file);
	int jtype = json_type(root);

	if (root == NULL)
	{
		LOG_E("Failed to load material from json file %s", file);
		return INVALID(Material);
	}

	Material mat = INVALID(Material);
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
		return INVALID(Material);
	}

	json_destroy(root);

	return mat;
}

Material material_get(const char* name)
{
	// No materials loaded
	if (material_table == NULL)
		return INVALID(Material);

	GenericHandle handle = handletable_find(material_table, name);
	return PUN_HANDLE(handle, Material);
}

Material material_get_default()
{
	if (material_default.index != INVALID(Material).index)
		return material_default;

	// Create the default material
	JSON* root = json_create_object();

	json_add_member(root, "name", json_create_string("default"));
	json_add_member(root, "albedo", json_create_string("col:white"));
	json_add_member(root, "vertexshader", json_create_string("./assets/shaders/default.vert.spv"));
	json_add_member(root, "fragmentshader", json_create_string("./assets/shaders/default.frag.spv"));
	JSON* bindings = json_create_array();
	JSON* binding = json_create_object();
	json_add_member(binding, "binding", json_create_number(0));
	json_add_member(binding, "texture", json_create_string("albedo"));
	json_add_member(binding, "type", json_create_string("texture"));
	json_add_member(binding, "stage", json_create_string("fragment"));
	json_add_element(bindings, binding);
	json_add_member(root, "bindings", bindings);

	material_default = material_load_internal(root);

	json_destroy(root);
	return material_default;
}

void material_bind(Material mat, Commandbuffer commandbuffer, VkDescriptorSet data_descriptors)
{
	Material_raw* raw = handlepool_get_raw(&material_pool, PUN_HANDLE(mat, GenericHandle));
	pipeline_bind(raw->pipeline, commandbuffer_vk(commandbuffer));

	// Get the layout from the pipeline
	VkPipelineLayout pipeline_layout = pipeline_get_layout(raw->pipeline);

	// Bind global set 0
	vkCmdBindDescriptorSets(commandbuffer_vk(commandbuffer), VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, GLOBAL_DESCRIPTOR_INDEX, 1, &global_descriptors->sets[renderer_get_frameindex()], 0,
							NULL);
	// Bind material set 1
	vkCmdBindDescriptorSets(commandbuffer_vk(commandbuffer), VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, MATERIAL_DESCRIPTOR_INDEX, 1,
							&raw->material_descriptors->sets[renderer_get_frameindex()], 0, NULL);

	// Per entity data set 2
	vkCmdBindDescriptorSets(commandbuffer_vk(commandbuffer), VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, ENTITY_DESCRIPTOR_INDEX, 1, &data_descriptors, 0, NULL);
}

void material_push_constants(Material mat, Commandbuffer commandbuffer, uint32_t index, void* data)
{
	Material_raw* raw = handlepool_get_raw(&material_pool, PUN_HANDLE(mat, GenericHandle));

	vkCmdPushConstants(commandbuffer_vk(commandbuffer), pipeline_get_layout(raw->pipeline), raw->push_constants[index].stageFlags, raw->push_constants[index].offset,
					   raw->push_constants[index].size, data);
}

void material_destroy(Material mat)
{
	Material_raw* raw = handlepool_get_raw(&material_pool, PUN_HANDLE(mat, GenericHandle));

	// Remove from table if it exists
	handletable_remove(material_table, raw->name);
	// Last texture was removed
	if (handletable_get_count(material_table) == 0)
	{
		handletable_destroy(material_table);
		material_table = NULL;
	}

	vkDeviceWaitIdle(device);

	// Destroy all textures
	for (uint32_t i = 0; i < raw->texture_count; i++)
	{
		texture_destroy(raw->textures[i]);
	}

	descriptorpack_destroy(raw->material_descriptors);
	vkDestroyDescriptorSetLayout(device, raw->descriptor_layouts[MATERIAL_DESCRIPTOR_INDEX], NULL);

	handlepool_free(&material_pool, PUN_HANDLE(mat, GenericHandle));
}

void material_destroy_all()
{
	for (uint32_t i = 0; i < material_pool.size; i++)
	{
		if (HANDLEPOOL_INDEX((&material_pool), i)->next != NULL)
			continue;

		material_destroy(PUN_HANDLE(HANDLEPOOL_INDEX((&material_pool), i)->handle, Material));
	}
}