#version 450

struct Camera
{
	vec4 position;
	mat4 view;
	mat4 proj;
};

struct Entity
{
	mat4 model;
	vec4 color;
};

layout(binding = 0) uniform UniformBufferObject
{
	vec4 background_color;
	Camera cameras[8];
	int camera_count;
}
scene;

layout(binding = 0, set = 2) uniform Entities
{
	Entity entities[64];
}
entities;

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec2 in_uv;

layout(location = 1) out vec2 frag_uv;

// A push constant for +++++++

// Layout push constants
layout(push_constant) uniform ModelMatrix
{
	int index;
}
entity;

void main()
{
	Entity entity = entities.entities[entity.index];
	Camera camera = scene.cameras[0];
	gl_Position = camera.proj * camera.view * entity.model * vec4(in_position, 1.0);
	//gl_Position = vec4(in_position, 0.0, 1.0);
	frag_uv = in_uv;
}