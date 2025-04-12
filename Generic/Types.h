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
	int x;
	int y;
};

struct Rect
{
	int x;
	int y;
	int w;
	int h;
};

struct Bound
{
	uint32_t left;
	uint32_t right;
	uint32_t top;
	uint32_t bottom;
};

