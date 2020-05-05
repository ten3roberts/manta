#ifndef CAMERA_H
#define CAMERA_H

#include "math/vec.h"
#include "math/mat4.h"
#include "transform.h"

typedef struct Camera Camera;

Camera* camera_create_perspective(const char* name, Transform transform, float aspect, float fov, float near, float far);

const char* camera_get_name(Camera* camera);

Transform* camera_get_transform(Camera* camera);
mat4 camera_get_view_matrix(Camera* camera);
mat4 camera_get_projection_matrix(Camera* camera);

// Is called once a frame
void camera_update(Camera* camera);

// Destroys and camera and removes it from the scene
void camera_destroy(Camera* camera);
#endif