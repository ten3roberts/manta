#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec4 out_color;
layout(location = 1) in vec2 frag_uv;

layout(binding = 0, set = 1) uniform sampler2D texSampler1;
layout(binding = 1, set = 1) uniform sampler2D texSampler2;

void main()
{
	out_color = texture(texSampler1, frag_uv) + texture(texSampler2, frag_uv);
	//out_color = vec4(0.39, 0.39, 0.39, 1);
}
