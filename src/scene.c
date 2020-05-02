#include "scene.h"
#include "magpie.h"
#include "hashtable.h"
#include "log.h"
#include "graphics/renderer.h"

struct Scene
{
	char name[256];

	// The number of entities
	uint32_t entity_count;
	// The size of the array
	uint32_t entities_size;
	Entity** entities;
};

static Scene* scene_current = NULL;

// Creates an empty scene
Scene* scene_create(const char* name)
{
	Scene* scene = calloc(1, sizeof(Scene));
	snprintf(scene->name, sizeof scene->name, "%s", name);

	if (scene_current == NULL)
		(void)scene_set_current(scene);
	return scene;
}

Scene* scene_set_current(Scene* scene)
{
	Scene* old = scene_current;
	scene_current = scene;
	return old;
}

Scene* scene_get_current()
{
	return scene_current;
}

void scene_add_entity(Scene* scene, Entity* entity)
{
	// Allocate list
	if (scene->entities == NULL)
	{
		scene->entities_size = 4;
		scene->entities = malloc(scene->entities_size * sizeof(Entity*));
	}
	// Resize array up
	if (scene->entity_count + 1 >= scene->entities_size)
	{
		scene->entities_size = scene->entities_size << 1;
		scene->entities = realloc(scene->entities, scene->entities_size);
	}
	// Add at end of array
	scene->entities[scene->entity_count] = entity;
	++scene->entity_count;
	renderer_flag_rebuild();
}

void scene_remove_entity(Scene* scene, Entity* entity)
{
	for (uint32_t i = 0; i < scene->entity_count; i++)
	{
		// Found
		if (scene->entities[i] == entity)
		{
			// Fill gap
			memmove(scene->entities + i, scene->entities + i + 1, scene->entity_count - i - 1);
			--scene->entity_count;

			// Resize array down
			if (scene->entity_count < scene->entities_size / 2)
			{
				scene->entities_size = scene->entities_size >> 1;
				scene->entities = realloc(scene->entities, scene->entities_size);
			}
			renderer_flag_rebuild();
			return;
		}
	}
}

Entity* scene_find_entity(Scene* scene, const char* name)
{
	for (uint32_t i = 0; i < scene->entity_count; i++)
	{
		// Found
		if (strcmp(entity_get_name(scene->entities[i]), name) == 0)
		{
			return scene->entities[i];
		}
	}
	return NULL;
}
// Get the entity at index
// Returns NULL if out of bounds
Entity* scene_get_entity(Scene* scene, uint32_t index)
{
	if (index >= scene->entity_count)
		return NULL;
	return scene->entities[index];
}

void scene_update(Scene* scene)
{
	for (uint32_t i = 0; i < scene->entity_count; i++)
	{
		entity_update(scene->entities[i]);
	}
}

void scene_destroy_entities(Scene* scene)
{
	for (uint32_t i = 0; i < scene->entity_count; i++)
	{
		entity_destroy(scene->entities[i]);
	}
}

// Destroys a scene and all entities within not marked with keep
void scene_destroy(Scene* scene)
{
	free(scene->entities);
	free(scene);
}