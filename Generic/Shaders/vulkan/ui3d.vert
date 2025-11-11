#version 450 core

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 uvw;
layout(location = 2) in vec4 color;

layout(binding = 0) uniform cb {
    float wd;
	float hd;
} b;

layout(set = 2, binding = 0) uniform cb1 {
	mat4 model;
} b1;

layout(set = 3, binding = 0) uniform cb2 {
	mat4 view;
	mat4 proj;
} b2;

layout(location = 0) out vec3 o_uvw;
layout(location = 1) out vec4 o_color;

void main()
{
	gl_Position.x = pos.x / b.wd;
	gl_Position.y = pos.y / b.hd;
    gl_Position = vec4(gl_Position.xy * 2 - 1, 0, 1);
	gl_Position.x /= b2.proj[0][0];
	gl_Position.y /= b2.proj[1][1];
	gl_Position *= b1.model * b2.view * b2.proj;
	o_uvw = uvw;
	o_color = color;
}