#ifndef SHADERTYPES_H
#define SHADERTYPES_H
#include "math/mat4.h"
#include "math/vec.h"
#include <vulkan/vulkan.h>
#include <stdint.h>

// Defines the maximum amount of cameras in a scene
#define CAMERA_MAX 8

//@Shader Types@ Defines several structs to match shader uniform structs

struct LayoutInfo
{
	VkDescriptorSetLayoutBinding* bindings;
	uint32_t binding_count;
	uint32_t* buffer_sizes;
};

// Defines a camera in shader

struct CameraData
{
	vec4 position;
	mat4 view;
	mat4 proj;
};

// Defines scene data in the shader
struct SceneData
{
	vec4 background_color;
	struct CameraData cameras[CAMERA_MAX];
	uint32_t camera_count;
};

struct EntityData
{
	mat4 model_matrix;
	vec4 color;
};

#endif