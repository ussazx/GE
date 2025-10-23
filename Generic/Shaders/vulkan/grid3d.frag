#version 450 core

layout(location = 0) in vec4 color;
layout(location = 1) in vec3 pos;
layout(location = 2) in vec3 norm;

layout(location = 0) out vec4 fColor;

void main()
{
	fColor = color;
	fColor.a = abs(dot(normalize(-pos), normalize(norm))) * color.a;
}