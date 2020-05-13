#ifndef MODEL_H
#define MODEL_H

#include "vulkan/vulkan.h"
#include "graphics/commandbuffer.h"
#include "graphics/vertexbuffer.h"

typedef struct Model Model;

// Loads a model from a collada file
// Can be accessed later by name
Model* model_load_collada(const char* filepath);
// Creates and stores a model from given vertices and indices
Model* model_create(const char* name, Vertex* vertices, uint32_t vertex_count, uint32_t* indices, uint32_t index_count);

void model_bind(Model* model, CommandBuffer* commandbuffer);

// Returns the furthest dimenstion of the model
// Useful for bound generation
float model_max_distance(Model* model);

uint32_t model_get_index_count(Model* model);
uint32_t model_get_vertex_count(Model* model);

// Returns a reference to a 1x1 square primitive
Model* model_get_quad();
// Returns a reference to a cube primitive
Model* model_get_cube();

// Retrieves a model by name
Model* model_get(const char* name);

void model_destroy(Model* model);
// Destroys all loaded models including primitives
void model_destroy_all();
#endif