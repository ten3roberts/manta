#version 450

struct Camera
{
	vec4 position;
	mat4 view;
	mat4 proj;
};

layout(binding = 0) uniform UniformBufferObject
{
	vec4 background_color;
	Camera cameras[8];
	int camera_count;
}
scene;

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec2 in_uv;

layout(location = 1) out vec2 frag_uv;

// A push constant for +++++++

// Layout push constants
layout(push_constant) uniform ModelMatrix
{
	mat4 matrix;
}
model;

void main()
{
	Camera camera = scene.cameras[0];
	gl_Position = camera.proj * camera.view * model.matrix * vec4(in_position, 1.0);
	//gl_Position = vec4(in_position, 0.0, 1.0);
	frag_uv = in_uv;
}