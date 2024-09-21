#version 450 core

layout(location = 4) in vec2 pos;
layout(location = 8) in vec3 uvw;
layout(location = 12) in vec4 color;

layout(binding = 0) uniform cb {
    float wd;
	float hd;
} b;

layout(location = 0) out vec3 o_uvw;
layout(location = 1) out vec4 o_color;

void main()
{
	gl_Position.x = pos.x / b.wd;
	gl_Position.y = pos.y / b.hd;
    gl_Position = vec4(gl_Position.xy * 2 - 1, 0, 1);
	o_uvw = uvw;
	o_color = color;
}