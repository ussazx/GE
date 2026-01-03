#version 450 core

layout(location = 0) in vec2 uv;

layout(set = 1, binding = 0) uniform sampler2D tex;

layout(location = 0) out vec4 fColor;

void main()
{
	fColor = texture(tex, uv);
}