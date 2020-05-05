#ifndef SCENE_H
#define SCENE_H
#include "entity.h"
#include "graphics/camera.h"
// A scene contains all entities
// The scene makes sure to render and update all entities
typedef struct Scene Scene;

// Creates an empty scene
// If no scene is set as current, the newly created scene will be set as current
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

// Finds an entity in the scene by name
// Returns NULL if it doesn't exist
Entity* scene_find_entity(Scene* scene, const char* name);

// Get the entity at index
// Returns NULL if out of bounds
Entity* scene_get_entity(Scene* scene, uint32_t index);

// Adds a camera to the scene
// Note: there cannot be more than CAMERA_MAX cameras in a scene due to shaderdata constraints
void scene_add_camera(Scene* scene, Camera* camera);

// Get the entity at index
// Returns NULL if out of bounds
Camera* scene_get_camera(Scene* scene, uint32_t index);

// Updates all entities and cameras in the scene
void scene_update(Scene* scene);

// Will destroy all entities in the scene
void scene_destroy_entities(Scene* scene);

// Destroys a scene and all entities within not marked with keep
void scene_destroy(Scene* scene);

#endif
