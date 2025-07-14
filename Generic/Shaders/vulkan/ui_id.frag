#version 450 core

layout(location = 0) flat in uint in_id;

layout(location = 0) out uint out_id;

void main()
{
	out_id = in_id;
}