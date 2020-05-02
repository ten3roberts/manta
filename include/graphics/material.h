#ifndef MATERIAL_H
#define MATERIAL_H
#include <stdint.h>
#include <vulkan/vulkan.h>
#include "math/mat4.h"

struct ModelPushConstant
{
	mat4 model_matrix;
};

#define PUSH_CONSTANT_SIZE sizeof(struct ModelPushConstant)

// Materials hold textures, shaders, and descriptors

typedef struct Material Material;

// Loads one or more materials from a file and stores them into memory
// Can be accessed later by name
Material* material_load(const char* file);

// Returns a material that has been previosuly loaded into memory
Material* material_get(const char* name);

// Gets a default white material that can be used for whiteboxing
Material* material_get_default();

// Bind the material's pipeline
// Binds a material's descriptors for the specified frame
// If frame is -1, the current frame to render will be used (result of renderer_get_frame)
void material_bind(Material* mat, VkCommandBuffer command_buffer, uint32_t frame);

// Pushed data for the index push constant
// The size and offset of the push constants were defined at material creation
void material_push_constants(Material* mat, VkCommandBuffer command_buffer, uint32_t index, void* data);

// Destroys a single material
void material_destroy(Material* mat);

// Destroys all loaded materials
void material_destroy_all();
#endif