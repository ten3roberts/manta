#ifndef SCENE_H
#define SCENE_H
#include "entity.h"
// A scene contains all entities
// The scene makes sure to render and update all entities
typedef struct Scene Scene;

// Creates an empty scene
Scene* scene_create(const char* name);

// Sets the scene to be the current one to render and add entities to
// Returns the previosuly current scene (can safely be ignored)
Scene* scene_set_current(Scene* scene);
// Gets the current scene
Scene* scene_get_current();

// Handled automatically by entity
void scene_add_entity(Scene* scene, Entity* entity);

// Handled automatically by entity
// Removes an entity from the scene
// Note: this does not destroy the entity
void scene_remove_entity(Scene* scene, Entity* entity);

// Destroys a scene and all entities within not marked with keep
void scene_destroy(Scene* scene);


#endif
