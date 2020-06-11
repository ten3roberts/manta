#ifndef TRANSFORM_H
#define TRANSFORM_H
#include "math/vec.h"
#include "math/quaternion.h"
#include "math/mat4.h"

// Contains position, rotation and scale
typedef struct Transform
{
	vec3 position;
	quaternion rotation;
	vec3 scale;

	// Shader data
	// One model matrix containing the transform rotation for each swapchain image
	mat4 model_matrix;
} Transform;

void transform_update(Transform* transform);

// Moves a transform relative to its rotation
// Z is forwards
void transform_translate(Transform* transform, vec3 translation);

// Moves a transform relative to the global axis
void transform_translate_global(Transform* transform, vec3 translation);


#endif