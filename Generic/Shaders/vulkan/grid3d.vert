#version 450 core

layout(location = 0) in vec4 lineInfo;
layout(location = 1) in vec4 fadeInfo;
layout(location = 2) in float range;
layout(location = 3) in vec4 color;
layout(location = 4) in int inst_seq;

layout(binding = 0) uniform cb {
    mat4 view;
	mat4 proj;
} b;

layout(location = 0) out vec4 o_color;
layout(location = 1) out vec3 o_wpos;
layout(location = 2) out vec3 o_vpos;
layout(location = 3) out vec3 o_norm;
layout(location = 4) out vec3 o_origin;

void main()
{
	float isXLine = lineInfo.w;
	float space = fadeInfo.x;
	
	uint fadeCount = uint(fadeInfo.y);
	uint noneFadeCount = uint(fadeInfo.z);
	float fade = fadeInfo.w;
	
	float lenH = abs(lineInfo.x);
	float len = lenH * 2;
	
	float cx = lineInfo.y;
	float cz = lineInfo.z;
	float left = cx - lenH;
	float right = cx + lenH;
	float front = cz + lenH;
	float back = cz - lenH;
	
	o_wpos.y = 0;
	float d = space * inst_seq;
	if (isXLine != 0)
	{
		if (back > d)
		{
			d = back - d;
			d = front - (d - floor(d / len) * len);
		}
		else if (front < d)
		{
			d = d - front;
			d = back + d - floor(d / len) * len;
		}
		o_wpos.x = lineInfo.x + right - lenH;
		o_wpos.z = d;
	}
	else
	{
		if (left > d)
		{
			d = left - d;
			d = right - (d - floor(d / len) * len);
		}
		else if (right < d)
		{
			d = d - right;
			d = left + d - floor(d / len) * len;
		}
		o_wpos.x = d;
		o_wpos.z = lineInfo.x + front - lenH;
	}
	uint i = uint(round(abs(d) / space));
	if (fadeCount == 0 || i % noneFadeCount == 0)
		fade = 1;
	else if (i % fadeCount != 0)
		fade = 0;
	
	o_origin = vec3(cx, cz, range);
	gl_Position = vec4(o_wpos, 1) * b.view;
	o_vpos = gl_Position.xyz;
	gl_Position *= b.proj;
	o_norm = (vec4(0, 1, 0, 0) * b.view).xyz;
	o_color = color;
	o_color.a *= fade;
}