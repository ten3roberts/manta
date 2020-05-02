#include "entity.h"
#include "magpie.h"
#include "graphics/material.h"
#include "log.h"
#include "mempool.h"
#include "scene.h"

// Pool entity creation to allow for faster allocations and reduce memory fragmentation
static mempool_t* entity_pool = NULL;

struct Entity
{
	char name[256];
	Transform transform;
	Material* material;
};

Entity* entity_create(const char* name, const char* material_name, Transform transform)
{
	// Create the memory pool
	if(entity_pool == NULL)
	{
		entity_pool = mempool_create(sizeof(Entity), 1024);
	}

	Entity* entity = mempool_alloc(entity_pool);
	snprintf(entity->name, sizeof entity->name, "%s", name);

	entity->transform = transform;
	entity->material = material_get(material_name);

	if(entity->material == NULL)
	{
		LOG_E("Unknown material %s. Using default material", material_name);
	}
	entity->material = material_get_default();

	// Add to scene
	scene_add_entity(scene_get_current(), entity);
	return entity;
}

const char* entity_get_name(Entity* entity)
{
	return entity->name;
}

void entity_destroy(Entity* entity)
{
	scene_remove_entity(scene_get_current(), entity);
	free(entity);
}