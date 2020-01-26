#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec4 out_color;
layout(location = 0) in vec3 frag_color;

void main() {
    out_color = vec4(1.0, 0.0, 0.0, 1.0);
}
