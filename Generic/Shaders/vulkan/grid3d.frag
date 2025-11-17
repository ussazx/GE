#version 450 core

layout(location = 0) in vec4 color;
layout(location = 1) in vec3 wpos;
layout(location = 2) in vec3 vpos;
layout(location = 3) in vec3 norm;
layout(location = 4) in vec3 origin;

layout(location = 0) out vec4 fColor;

void main()
{
	fColor = color;
	//fColor.a = abs(dot(normalize(-pos), normalize(norm))) * color.a;
	vec3 v = wpos - vec3(origin.x, 0, origin.y);
	//float n = max(abs(v.x), abs(v.z));
	float n = sqrt(v.x * v.x + v.z * v.z);
	fColor.a = max(0, (origin.z - n) / origin.z) * color.a;
}