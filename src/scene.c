#include "scene.h"
#include "magpie.h"
#include "hashtable.h"
#include "log.h"
#include "graphics/renderer.h"

struct Scene
{
	char name[256];
	// Entities are stored in a table based on their name
	hashtable_t* entity_table;
};

static Scene* scene_current = NULL;

// Creates an empty scene
Scene* scene_create(const char* name)
{
	Scene* scene = calloc(1, sizeof(Scene));
	snprintf(scene->name, sizeof scene->name, "%s", name);
	scene->entity_table = hashtable_create_string() return scene;
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
	if (hashtable_find(scene->entity_table, entity_get_name(entity)))
	{
		LOG_W("Entity %s is already added in scene %s", entity_get_name(entity), scene->name);
		return;
	}
	hashtable_insert(scene->entity_table, entity_get_name(entity), entity);
	renderer_flag_rebuild();
}

void scene_remove_entity(Scene* scene, Entity* entity)
{
	if (hashtable_remove(scene->entity_table, entity_get_name(entity)))
		renderer_flag_rebuild();
}

// Destroys a scene and all entities within not marked with keep
void scene_destroy(Scene* scene)
{
	hashtable_destroy(scene->entity_table);
}