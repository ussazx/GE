#version 450 core

layout(location = 0) in vec3 pos;
layout(location = 1) in uint id;

layout(binding = 0) uniform cb {
    float wd;
	float hd;
} b;

layout(location = 0) out uint o_id;

void main()
{
	gl_Position.x = pos.x / b.wd;
	gl_Position.y = pos.y / b.hd;
    gl_Position = vec4(gl_Position.xy * 2 - 1, pos.z, 1);
	o_id = id;
}