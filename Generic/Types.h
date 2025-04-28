#pragma once

typedef unsigned char byte;

typedef float float1;

typedef int int1;

typedef unsigned int uint1;

//struct float2
//{
//	float x;
//	float y;
//};

struct Point
{
	float x;
	float y;
};

struct Rect
{
	float x;
	float y;
	float w;
	float h;
};

struct Bound
{
	float left;
	float right;
	float top;
	float bottom;
};

