#include "transform.h"
#include "graphics/renderer.h"
#include "log.h"

// Updates the shader data for the currently selected frame
void transform_update(Transform* transform)
{
	transform->model_matrix = mat4_identity;

	mat4 translate = mat4_translate(transform->position);
	mat4 rotate = quat_to_mat4(transform->rotation);
	mat4 scale = mat4_scale(transform->scale);
	//scale = mat4_transpose(&scale);

	transform->model_matrix = mat4_mul(&transform->model_matrix, &scale);
	transform->model_matrix = mat4_mul(&transform->model_matrix, &rotate);
	transform->model_matrix = mat4_mul(&transform->model_matrix, &translate);
}

void transform_translate(Transform* transform, vec3 translation)
{
	transform->position = vec3_add(transform->position, quat_transform_vec3(transform->rotation, translation));
}

void transform_translate_global(Transform* transform, vec3 translation)
{
	transform->position = vec3_add(transform->position, translation);
}