#version 450

layout(binding = 0) uniform UniformBufferObject
{
	mat4 model;
	mat4 view;
	mat4 proj;
}
ubo;

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec2 in_uv;

layout(location = 1) out vec2 frag_uv;

// Layout push constants
layout(push_constant) uniform ModelMatrix
{
	mat4 matrix;
} model;

void main()
{
	gl_Position = ubo.proj * ubo.view * model.matrix * vec4(in_position, 1.0);
	//gl_Position = vec4(in_position, 0.0, 1.0);
	frag_uv = in_uv;
}