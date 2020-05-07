#ifndef MODEL_H
#define MODEL_H

#include "vulkan/vulkan.h"
typedef struct Model Model;

// Loads a model from a collada file
// Can be accessed later by name
Model* model_load_collada(const char* filepath);

void model_bind(Model* model, VkCommandBuffer command_buffer);

// Returns the furthest dimenstion of the model
// Useful for bound generation
float model_max_distance(Model* model);

uint32_t model_get_index_count(Model* model);
uint32_t model_get_vertex_count(Model* model);

// Retrieves a model by name
Model* model_get(const char* name);

void model_destroy(Model* model);
// Destroys all loaded models
void model_destroy_all();
#endif