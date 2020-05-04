#include "entity.h"
#include "magpie.h"
#include "graphics/material.h"
#include "log.h"
#include "mempool.h"
#include "scene.h"
#include "graphics/model.h"
#include <stdio.h>

// Pool entity creation to allow for faster allocations and reduce memory fragmentation
static mempool_t* entity_pool = NULL;

struct Entity
{
	char name[256];
	Transform transform;
	Material* material;
	Model* model;
};

Entity* entity_create(const char* name, const char* material_name, const char* model_name, Transform transform)
{
	// Create the memory pool
	if (entity_pool == NULL)
	{
		entity_pool = mempool_create(sizeof(Entity), 8);
	}

	Entity* entity = mempool_alloc(entity_pool);
	snprintf(entity->name, sizeof entity->name, "%s", name);

	entity->transform = transform;
	entity->material = material_get(material_name);

	if (entity->material == NULL)
	{
		LOG_W("Unknown material %s. Using default material", material_name);
		entity->material = material_get_default();
	}

	entity->model = model_get(model_name);
	if (entity->model == NULL)
	{
		LOG_E("Unknown model %s", model_name);
	}

	// Add to scene
	scene_add_entity(scene_get_current(), entity);
	return entity;
}

const char* entity_get_name(Entity* entity)
{
	return entity->name;
}
Transform* entity_get_transform(Entity* entity)
{
	return &entity->transform;
}
Material* entity_get_material(Entity* entity)
{
	return entity->material;
}
Model* entity_get_model(Entity* entity)
{
	return entity->model;
}

void entity_update(Entity* entity)
{
	transform_update(&entity->transform);
}

// Is only called irreguraly when command buffers are rebuilt
void entity_render(Entity* entity, VkCommandBuffer command_buffer, uint32_t frame)
{
	// Binding is done by renderer
	material_bind(entity->material, command_buffer, frame);
	model_bind(entity->model, command_buffer);

	// Set push constant for model matrix
	material_push_constants(entity->material, command_buffer, 0, &entity->transform.model_matrix[frame]);
	
	vkCmdDrawIndexed(command_buffer, model_get_index_count(entity->model), 1, 0, 0, 0);
}

void entity_destroy(Entity* entity)
{
	mempool_free(entity_pool, entity);
	if(mempool_get_count(entity_pool) == 0)
	{
		mempool_destroy(entity_pool);
		entity_pool = NULL;
	}
	//scene_remove_entity(scene_get_current(), entity);
}