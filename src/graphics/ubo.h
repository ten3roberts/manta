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

UniformBuffer* ub_create(uint32_t size);
// Updates a uniform buffer with data
// If size is -1, the whole buffer will be updated according to the internal size
// The size defines the amount of bytes to copy after the offset
void ub_update(UniformBuffer* ub, void* data, uint32_t size, uint32_t offset, uint32_t i);
void ub_destroy(UniformBuffer* ub);

void** ub_get_buffer(UniformBuffer* ub);