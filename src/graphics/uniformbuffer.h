#pragma once
#include "math/mat4.h"
#include <vulkan/vulkan.h>
#include "graphics/texture.h"
#include <stdint.h>

typedef struct
{
	mat4 model;
	mat4 view;
	mat4 proj;
} TransformType;

typedef void UniformBuffer;

// Creates a descriptor set layout from the specified bindings
// Used when creating descriptors and during pipeline creation
int descriptorlayout_create(VkDescriptorSetLayoutBinding* bindings, uint32_t binding_count, VkDescriptorSetLayout* dst_layout);

// Creates multiple descriptors, one for each frame in flight (swapchain_image_count)
// Writes the buffers and samplers to each frame's descriptor as specified in bindings
// The number of uniformbuffers should match the bindings
// The number of textures should match the bindings
// dst_descriptors should be an array of swapchain_image_count length. Arrays data will be overwritten
int descriptorset_create(VkDescriptorSetLayout layout, VkDescriptorSetLayoutBinding* bindings, uint32_t binding_count, UniformBuffer** uniformbuffers,
						 Texture** textures, VkDescriptorSet* dst_descriptors);

UniformBuffer* ub_create(uint32_t size, uint32_t binding);
// Updates a uniform buffer with data
// If size is -1, the whole buffer will be updated according to the internal size
// The size defines the amount of bytes to copy after the offset
void ub_update(UniformBuffer* ub, void* data, uint32_t i);
void ub_destroy(UniformBuffer* ub);

// Destroys all UniformBuffer pools in the end of the programs
// The pool were first created implicitly when a UniformBuffer was created
void ub_pools_destroy();