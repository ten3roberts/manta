#ifndef ENTITY_H
#define ENTITY_H
#include "transform.h"
#include "graphics/model.h"
#include "graphics/material.h"
#include "colliders.h"

typedef struct Entity Entity;

// Creates an entity and adds it to the scene
Entity* entity_create(const char* name, const char* material_name, const char* model_name, Transform transform);

const char* entity_get_name(Entity* entity);

Transform* entity_get_transform(Entity* entity);
Material* entity_get_material(Entity* entity);
Model* entity_get_model(Entity* entity);
const SphereCollider* entity_get_boundingsphere(Entity* entity);

// Is called once a frame
void entity_update(Entity* entity);

// Is only called irreguraly when command buffers are rebuilt
// data_write refers to a mapped pointer to which the entity writes its specific data to
void entity_render(Entity* entity, CommandBuffer* commandbuffer, void* data_write, uint32_t index, VkDescriptorSet data_descriptors);

// Destroys and entity and removes it from the scene
void entity_destroy(Entity* entity);
#endif