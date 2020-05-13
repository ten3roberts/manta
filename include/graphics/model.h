#ifndef MODEL_H
#define MODEL_H

#include "vulkan/vulkan.h"
#include "graphics/commandbuffer.h"
#include "graphics/vertexbuffer.h"

// A model contains several meshes loaded from collada
typedef struct Model Model;
// Represents a vertex and index buffer
typedef struct Mesh Mesh;
//@Models@
// Loads a model from a collada file
// Can be accessed later by name
void model_load_collada(const char* filepath);

// Retrieves a model by name
Model* model_get(const char* name);

// Gets a mesh by index
// Returns NULL if out of range
Mesh* model_get_mesh(Model* model, uint32_t index);

// Finds a mesh with given name in model
// Returns NULL if it doesn't exist
Mesh* model_find_mesh(Model* model, const char* name);

// Adds a mesh to a model
// Mesh is then owned and destroyed by model
void model_add_mesh(Model* model, Mesh* mesh);

// Destroys a model and all contained meshes
void model_destroy(Model* model);
// Destroys all loaded models including primitives
void model_destroy_all();

// @Meshes@
// Binds a mesh
void mesh_bind(Mesh* mesh, CommandBuffer* commandbuffer);
// Returns the furthest dimenstion of the mesh
// Useful for bound generation
float mesh_max_distance(Mesh* mesh);
uint32_t mesh_get_index_count(Mesh* mesh);
uint32_t mesh_get_vertex_count(Mesh* mesh);

// Gets a mesh by 'modelname:meshname' if only modelname is supplied, the first mesh of the model is returned
Mesh* mesh_find(const char* name);
Mesh* mesh_create(const char* name, Vertex* vertices, uint32_t vertex_count, uint32_t* indices, uint32_t index_count);
// Destroys a mesh
// Note: mesh should not be owned by a model when calling destroy
// Meshes in models are autmatically destroyed
void mesh_destroy(Mesh* mesh);
#endif