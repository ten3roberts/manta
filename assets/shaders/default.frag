#version 450
#extension GL_ARB_separate_shader_objects : enable

struct Entity
{
	mat4 model;
	vec4 color;
};

layout(binding = 0, set = 2) uniform Entities
{
	Entity entities[64];
}
entities;

// Layout push constants
layout(push_constant) uniform ModelMatrix
{
	int index;
}
entity;

layout(location = 0) out vec4 out_color;
layout(location = 1) in vec2 frag_uv;

layout(binding = 0, set = 1) uniform sampler2D texSampler;

void main()
{
	Entity entity = entities.entities[entity.index];
	out_color = texture(texSampler, frag_uv) * entity.color;
}
