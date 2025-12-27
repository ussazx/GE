#version 450

layout(location = 0) flat in uint id;

layout (set = 1, binding = 0) uniform usampler2D idView;

layout(location = 0) out vec4 color;

void main()
{
	vec2 uv = gl_FragCoord.xy / textureSize(idView, 0).xy;
	
	vec2 d = 2.0f / textureSize(idView, 0);
	
	if (uv.x - d.x <= 0 || uv.x + d.x >= 1 ||  uv.y - d.y <= 0 || uv.y + d.y >= 1 ||
		texture(idView, vec2(uv.x - d.x, uv.y)).x != id ||
		texture(idView, vec2(uv.x + d.x, uv.y)).x != id ||
		texture(idView, vec2(uv.x, uv.y - d.y)).x != id ||
		texture(idView, vec2(uv.x, uv.y + d.y)).x != id ||
		texture(idView, vec2(uv.x - d.x, uv.y - d.y)).x != id ||
		texture(idView, vec2(uv.x + d.x, uv.y + d.y)).x != id ||
		texture(idView, vec2(uv.x + d.x, uv.y - d.y)).x != id ||
		texture(idView, vec2(uv.x - d.x, uv.y + d.y)).x != id)
		color = vec4(1, 1, 1, 1);
	else
		discard;
}