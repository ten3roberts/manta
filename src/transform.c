#include "transform.h"
#include "graphics/renderer.h"

// Updates the shader data for the currently selected frame
void transform_update(Transform* transform)
{
	transform->model_matrix = mat4_identity;

	mat4 translate = mat4_translate(transform->position);
	mat4 rotate = quat_to_mat4(transform->rotation);
	//mat4 scale = mat4_scale(transform->scale);

	transform->model_matrix = mat4_mul(&transform->model_matrix, &rotate);
	transform->model_matrix = mat4_mul(&transform->model_matrix, &translate);
}