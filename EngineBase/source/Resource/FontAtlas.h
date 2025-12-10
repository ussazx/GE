#pragma once

#define ATLAS_HEADER_MARK "Atlas"

enum AtlasPixelType
{
	MONO1,
	GRAY8,
	R8G8B8A8,
	B8G8R8A8,
	A8R8G8B8,
	A8B8G8R8
};

struct CodeRange
{
	uint16_t firstCode;
	uint16_t lastCode;
	uint32_t startCoordIdx;
};

struct GlyphInfo
{
	uint32_t offset;
	uint16_t hasImage;
	uint16_t width;
	uint16_t height;
	int xOffset;
	int yOffset;
	float xAdvance;
};

struct AtlasInfo
{
	AtlasPixelType pixelType;
	uint16_t numCodeRanges;
	uint16_t numGlyphs;
	uint32_t numPixels;
	struct Font
	{
		uint32_t maxWidth;
		uint32_t maxHeight;
		float xAdvance;
		int ascender;
		int descender;
	} font;
};

struct AtlasPixel
{
	unsigned char r;
	unsigned char g;
	unsigned char b;
	unsigned char a;
};