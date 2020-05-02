#include "transform.h"
#include "graphics/renderer.h"


// Updates the shader data for the currently selected frame
void transform_update(Transform* transform)
{
	int frame_index = renderer_get_frameindex();
	transform->model_matrix[frame_index] = mat4_identity;
	mat4* model = &transform->model_matrix[frame_index];

	mat4 translate = mat4_translate(transform->position);
	mat4 rotate = quat_to_mat4(transform->rotation);
	//mat4 scale = mat4_scale(transform->scale);

	*model = mat4_mul(model, &rotate);
	*model = mat4_mul(model, &translate);
}