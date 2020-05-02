#ifndef ENTITY_H
#define ENTITY_H
#include "transform.h"

typedef struct Entity Entity;

// Creates an entity and adds it to the scene
Entity* entity_create(const char* name, const char* material_name, Transform transform);

const char* entity_get_name(Entity* entity);

// Destroys and entity and removes it from the scene
void entity_destroy(Entity* entity);
#endif