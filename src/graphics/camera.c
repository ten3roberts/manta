#include "graphics/camera.h"
#include "graphics/renderer.h"
#include "scene.h"
#include "magpie.h"
#include <stdio.h>

struct Camera
{
	char name[256];
	Transform transform;
	// View matrix is the inverse of the model matrix in transform
	// Since the camera doesn't use push constants for the data, there is only one instance of the view matrix
	// It is instead handled internally by copy in uniform buffers
	mat4 view;
	// The projection matrix of the camera
	mat4 proj;
};

Camera* camera_create_perspective(const char* name, Transform transform, float aspect, float fov, float near, float far)
{
	Camera* camera = malloc(sizeof(Camera));

	snprintf(camera->name, sizeof camera->name, "%s", name);

	camera->transform = transform;
	camera->proj = mat4_perspective(aspect, fov, near, far);
	camera->view = mat4_identity;
	scene_add_camera(scene_get_current(), camera);
	return camera;
}

const char* camera_get_name(Camera* camera)
{
	return camera->name;
}

Transform* camera_get_transform(Camera* camera)
{
	return &camera->transform;
}

mat4 camera_get_view_matrix(Camera* camera)
{
	return camera->view;
}
mat4 camera_get_projection_matrix(Camera* camera)
{
	return camera->proj;
}

// Is called once a frame
void camera_update(Camera* camera)
{
	transform_update(&camera->transform);
	camera->view = mat4_inverse(&camera->transform.model_matrix[renderer_get_frameindex()]);
}

// Destroys and camera and removes it from the scene
void camera_destroy(Camera* camera)
{
	free(camera);
}