#pragma once
#include "math/mat4.h"

#include <stdint.h>


typedef struct
{
	mat4 model;
	mat4 view;
	mat4 proj;
} TransformType;

typedef void UniformBuffer;

int ub_create_descriptor_set_layout(uint32_t binding);
int ub_create_descriptor_pool();

VkDescriptorSetLayout* ub_get_layouts();

UniformBuffer* ub_create(uint32_t size);
// Updates a uniform buffer with data
// If size is -1, the whole buffer will be updated according to the internal size
// The size defines the amount of bytes to copy after the offset
void ub_update(UniformBuffer* ub, void* data, uint32_t i);
void ub_bind(UniformBuffer* ub, VkCommandBuffer command_buffer, int i);
void ub_destroy(UniformBuffer* ub);

// Destroys all UniformBuffer pools in the end of the programs
// The pool was first created implicitly when a UniformBuffer was created
void ub_pools_destroy();