#version 450 core

layout(location = 0) in vec3 pos;
layout(location = 1) in uint id;

layout(binding = 0) uniform cb {
    mat4 mvp;
} b;

layout(location = 0) out uint o_id;

void main()
{
	gl_Position = vec4(pos, 1) * b.mvp;
	o_id = id;
}