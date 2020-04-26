#ifndef MODEL_H
#define MODEL_H

// TODO: remove vulkan header dependency
#include "vulkan/vulkan.h"
typedef struct Model Model;

Model* model_load_collada(const char* filepath);

void model_bind(Model* model, VkCommandBuffer command_buffer);

uint32_t model_get_index_count(Model* model);
uint32_t model_get_vertex_count(Model* model);

void model_destroy(Model* model);
#endif