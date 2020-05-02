#ifndef ENTITY_H
#define ENTITY_H
#include "transform.h"
#include "graphics/model.h"
#include "graphics/material.h"

typedef struct Entity Entity;

// Creates an entity and adds it to the scene
Entity* entity_create(const char* name, const char* material_name, const char* model_name, Transform transform);

const char* entity_get_name(Entity* entity);

Transform* entity_get_transform(Entity* entity);
Material* entity_get_material(Entity* entity);
Model* entity_get_model(Entity* entity);

// Is called once a frame
void entity_update(Entity* entity);

// Is only called irreguraly when command buffers are rebuilt
void entity_render(Entity* entity, VkCommandBuffer command_buffer, uint32_t frame);



// Destroys and entity and removes it from the scene
void entity_destroy(Entity* entity);
#endif