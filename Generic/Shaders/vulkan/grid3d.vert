#version 450 core

layout(location = 0) in vec3 pos;
layout(location = 1) in vec4 color;

layout(binding = 0) uniform cb {
    mat4 view;
	mat4 proj;
} b;

layout(location = 0) out vec4 o_color;
layout(location = 1) out vec3 o_pos;
layout(location = 2) out vec3 o_norm;

void main()
{
	gl_Position = vec4(pos, 1) * b.view;
	o_pos = gl_Position.xyz;
	o_pos.x = 0;
	o_norm = (vec4(0, 1, 0, 0) * b.view).xyz;
	gl_Position *= b.proj;
	o_color = color;
}