#version 450 core

layout(location = 0) in vec3 uvw;
layout(location = 1) in vec4 in_color;

layout(set = 1, binding = 0) uniform samplerBuffer tex;

layout(location = 0) out vec4 fColor;

void main()
{
	fColor = texelFetch(tex, int(floor(uvw.x) + floor(uvw.y) * uvw.z)) * in_color;
}