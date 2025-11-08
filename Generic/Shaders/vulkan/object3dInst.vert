#version 450 core

layout(location = 0) in vec3 pos;
layout(location = 1) in vec2 uv;
layout(location = 2) in vec4 color;
layout(location = 3) in mat4 model;

layout(binding = 0) uniform cb {
    mat4 view;
	mat4 proj;
} b;

layout(location = 0) out vec2 o_uv;
layout(location = 1) out vec4 o_color;

void main()
{
	gl_Position = vec4(pos, 1) * model * b.view * b.proj;
	o_uv = uv;
	o_color = color;
}