#include "../Internal.h"
#include "FontAtlas.h"
#include "ft2build.h"
#include FT_FREETYPE_H
#include <memory>
#include <codecvt>

struct GlyphTable
{
	int xAdvance;
	std::vector<uint16_t> indices;
	std::vector<GlyphInfo> glyphs;

	Lua_wrap_cpp_class(GlyphTable, Lua_abstract);
};
Lua_global_add_cpp_class(GlyphTable)

bool AddGlyph(BufferWriter<float3>& pos, BufferWriter<float3>& uvw, const GlyphInfo& g, const Point& pt, const Bound* scissor, float z = 0)
{
	float x = pt.x;
	float y = pt.y;

	float width = g.width;
	float height = g.height;

	float x0 = g.offset;
	float y0 = 0;

	if (scissor)
	{
		if (scissor->left + scissor->right >= g.width || scissor->top + scissor->bottom >= g.height)
			return false;

		if (scissor->left > 0)
		{
			x0 += scissor->left;
			width -= scissor->left;
		}
		if (scissor->right > 0)
			width -= scissor->right;

		if (scissor->top > 0)
		{
			y0 += scissor->top;
			height -= scissor->top;
		}
		if (scissor->bottom > 0)
			height -= scissor->bottom;
	}

	float x1 = x0 + width;
	float y1 = y0 + height;

	uvw[0].x = x0;
	uvw[0].y = y0;
	uvw[0].z = g.width;

	uvw[1].x = x1;
	uvw[1].y = y0;
	uvw[1].z = g.width;

	uvw[2].x = x1;
	uvw[2].y = y1;
	uvw[2].z = g.width;

	uvw[3].x = x0;
	uvw[3].y = y1;
	uvw[3].z = g.width;

	pos[0] = { x, y, z };
	pos[1] = { x + width, y, z };
	pos[2] = { x + width, y + height, z };
	pos[3] = { x, y + height, z };

	return true;
}

bool AddGlyphClip(BufferWriter<float3>& pos, BufferWriter<float3>& uvw, GlyphInfo& g, Point offset, const Rect& clipRect, float z = 0)
{
	if (offset.x + g.width <= 0 || offset.x - clipRect.w >= 0
		|| offset.y + g.height <= 0 || offset.y - clipRect.h >= 0)
		return false;

	Point p{ clipRect.x, clipRect.y };
	Bound scissor{};

	if (offset.x < 0)
		scissor.left = -offset.x;
	else
	{
		p.x += offset.x;
		float right = offset.x + g.width;
		if (right > clipRect.w)
			scissor.right = right - clipRect.w;
	}

	if (offset.y < 0)
		scissor.top = -offset.y;
	else
	{
		p.y += offset.y;
		float bottom = offset.y + g.height;
		if (bottom > clipRect.h)
			scissor.bottom = bottom - clipRect.h;
	}

	return AddGlyph(pos, uvw, g, p, &scissor, z);
}

std::tuple<uint32_t, uint32_t> CMeasureText(LString s, int count, int range, LuacObj<GlyphTable> table)
{
	float x = 0;
	uint32_t i = 0;
	const std::wstring& ss = s;
	for (auto it = ss.begin(); it != ss.end() && (count < 0 || i < count); it++, i++)
	{
		uint32_t index = 0;
		if (*it >= table->indices.size())
			index = L'?' < table->indices.size() ? table->indices[L'?'] : 0xffff;
		else
			index = table->indices[*it];

		float w = (index < table->glyphs.size() ? table->glyphs[index].xAdvance : table->xAdvance);
		float hw = w > 0 ? w / 2 : 0;
		if (range >= 0 && range < x + hw)
			break;
		x += w;
	}
	return { x, i };
}
Lua_global_add_cfunc(CMeasureText);

std::tuple<uint32_t, uint32_t> CMeasureTextR(LString s, int count, int range, LuacObj<GlyphTable> table)
{
	float x = 0;
	uint32_t i = 0;
	const std::wstring& ss = s;
	for (auto it = ss.rbegin(); it != ss.rend() && (count < 0 || i < count); it++, i++)
	{
		uint32_t index = 0;
		if (*it >= table->indices.size())
			index = L'?' < table->indices.size() ? table->indices[L'?'] : 0xffff;
		else
			index = table->indices[*it];

		float w = (index < table->glyphs.size() ? table->glyphs[index].xAdvance : table->xAdvance);
		float hw = w > 0 ? w / 2 : 0;
		if (range >= 0 && range < x + hw)
			break;
		x += w;
	}
	return { x, i };
}
Lua_global_add_cfunc(CMeasureTextR);

std::tuple<int, int> CAddText(float x, float y, float z, LString s, LuacObj<GlyphTable> table, LuacObj<CBuffer> vb_pos, int wp_pos, LuacObj<CBuffer> vb_uv, int wp_uv)
{
	if (s.length() == 0)
		return { 0, x };

	BufferWriter<float3> pos(*vb_pos, s.length(), wp_pos);
	BufferWriter<float3> uvw(*vb_uv, s.length(), wp_uv);

	int n = 0;
	const std::wstring& a = s;
	for (auto it = a.begin(); it != a.end(); it++)
	{
		uint16_t index = 0;
		if (*it >= table->indices.size())
			index = table->indices[L'?'];
		else
		{
			index = table->indices[*it];
			if (index == 0xffff)
				index = table->indices[L'?'];
		}
		if (index < table->glyphs.size())
		{
			GlyphInfo& g = table->glyphs[index];
			if (g.hasImage && AddGlyph(pos, uvw, g, { x + g.xOffset, y - g.yOffset }, {}, z))
				n++;
			x += g.xAdvance;

			pos.SkipUsed();
			uvw.SkipUsed();
		}
		else
			x += table->xAdvance;
	}
	return { n, x };
}
Lua_global_add_cfunc(CAddText);

std::tuple<int, int> CAddTextClip(float offset_x, float offset_y, float rect_x, float rect_y, float rect_w, float rect_h, float z, LString s, LuacObj<GlyphTable> table,
	LuacObj<CBuffer> vb_pos, int wp_pos, LuacObj<CBuffer> vb_uv, int wp_uv)
{
	if (s.length() == 0)
		return { 0, offset_x };

	BufferWriter<float3> pos(*vb_pos, s.length(), wp_pos);                                                            
	BufferWriter<float3> uvw(*vb_uv, s.length(), wp_uv);

	int n = 0, x = 0;
	const std::wstring& a = s;
	for (auto it = a.begin(); it != a.end() && x + offset_x < rect_w; it++)
	{
		uint16_t index = 0;
		if (*it >= table->indices.size())
			index = table->indices[L'?'];
		else
		{
			index = table->indices[*it];
			if (index == 0xffff)
				index = table->indices[L'?'];
		}
		if (index < table->glyphs.size())
		{
			GlyphInfo& g = table->glyphs[index];

			Rect clip{ rect_x, rect_y, rect_w, rect_h };
			if (g.hasImage && AddGlyphClip(pos, uvw, g, { offset_x + x + g.xOffset, offset_y - g.yOffset }, { rect_x, rect_y, rect_w, rect_h }, z))
				n++;

			x += g.xAdvance;

			pos.SkipUsed();
			uvw.SkipUsed();
		}
		else
			x += table->xAdvance;
	}
	return { n, x };
}
Lua_global_add_cfunc(CAddTextClip);

LuacObjNew<GlyphTable> CLoadFontAtlas(LuacObj<Engine::StreamInput> input, uint32_t extraWidth)
{
	char c[] = ATLAS_HEADER_MARK;
	input->Read(c, strlen(c));
	if (strcmp(c, ATLAS_HEADER_MARK) != 0)
		return nullptr;

	AtlasInfo atlas{};
	if (!input->Load(atlas))
		return nullptr;

	std::unique_ptr<GlyphTable> table(new GlyphTable);
	table->xAdvance = atlas.font.xAdvance;
	for (size_t i = 0; i < atlas.numCodeRanges; i++)
	{
		CodeRange range{};
		if (!input->Load(range))
			return nullptr;

		size_t n = range.lastCode + 1;
		if (table->indices.size() < n)
			table->indices.resize(n, 0xffff);
		for (size_t j = range.firstCode, k = 0; j < n; j++, k++)
			table->indices[j] = range.startCoordIdx + k;
	}

	table->glyphs.resize(atlas.numGlyphs);
	for (size_t i = 0; i < atlas.numGlyphs; i++)
	{
		auto& g = table->glyphs[i];
		if (!input->Load(g))
			return nullptr;
	}

	uint32_t totalWidth = atlas.numPixels + extraWidth;
	auto atlasView = g_graphic->NewTexelBuffer(totalWidth * 4, g_graphic->GetDefined("FORMAT_R8G8B8A8_UNORM"));
	if (!atlasView.object)
		return nullptr;

	memset(atlasView->GetPtr(), 255, totalWidth * 4);

	if (atlas.pixelType == MONO1)
	{
		size_t pitch = (atlas.numPixels + 7) / 8;
		uint8_t* src = new uint8_t[pitch];
		size_t n = input->Read(src, pitch);
		for (size_t i = 0; i < atlas.numPixels && i < n * 8; i++)
			atlasView->GetPtr()[i * 4 + 3] = (src[i / 8] & (0x80 >> (i % 8))) * 255;
		delete[] src;
	}
	else if (atlas.pixelType == GRAY8)
	{
		uint8_t* src = new uint8_t[atlas.numPixels];
		size_t n = input->Read(src, atlas.numPixels);
		for (size_t i = 0; i < n; i++)
			atlasView->GetPtr()[i * 4 + 3] = src[i];
		delete[] src;
	}
	else
		input->Read(atlasView->GetPtr(), atlas.numPixels * 4);

	return LuacObjNew<GlyphTable>(table.release(),
		LuaSub("maxWidth", atlas.font.maxWidth),
		LuaSub("maxHeight", atlas.font.maxHeight),
		LuaSub("ascender", atlas.font.ascender),
		LuaSub("descender", atlas.font.descender),
		LuaSub("pixels", atlas.numPixels),
		LuaSub("view", atlasView.set));
}
Lua_global_add_cfunc(CLoadFontAtlas);

//struct GlyphBitmap
//{
//	uint8_t* bitmap;
//	uint32_t width;
//	uint32_t height;
//	uint32_t pitch;
//};

//void GenAtlas()
//{
//	FT_Library ftLib{};
//
//	if (FT_Init_FreeType(&ftLib) != 0)
//		return;
//
//	FT_Face ftFace;
//	if (FT_New_Face(ftLib, "c:\\Windows\\Fonts\\msyh.ttc", 0, &ftFace) != 0)
//		return;
//
//	FT_Select_Charmap(ftFace, FT_ENCODING_UNICODE);
//
//	FT_Set_Pixel_Sizes(ftFace, 0, 18);
//
//	AtlasInfo m{};
//
//	std::vector<GlyphInfo> glyphInfo;
//	std::vector<GlyphBitmap> glyphBmps;
//	std::vector<CodeRange> codeRanges;
//	bool rangeHasBegan = false;
//
//	int loadFlags = FT_LOAD_NO_BITMAP | FT_LOAD_RENDER | FT_LOAD_TARGET_NORMAL;
//	//int loadFlags = FT_LOAD_NO_BITMAP | FT_LOAD_RENDER | FT_LOAD_MONOCHROME | FT_LOAD_TARGET_NORMAL;
//
//	m.pixelType = loadFlags & FT_LOAD_MONOCHROME ? MONO1 : GRAY8;
//
//	uint16_t pixelMode{};
//	uint16_t i = 0;
//	for (; i < UNICODE_MAX; i++)
//	{
//		if (FT_Load_Char(ftFace, i, loadFlags) != 0)
//		{
//			if (rangeHasBegan)
//			{
//				codeRanges.back().lastCode = i - 1;
//				rangeHasBegan = false;
//			}
//			continue;
//		}
//
//		if (!rangeHasBegan)
//		{
//			codeRanges.resize(codeRanges.size() + 1);
//			codeRanges.back().firstCode = i;
//			codeRanges.back().startCoordIdx = glyphInfo.size();
//			rangeHasBegan = true;
//		}
//
//		glyphInfo.resize(glyphInfo.size() + 1);
//		glyphInfo.back().xOffset = ftFace->glyph->bitmap_left;
//		glyphInfo.back().yOffset = ftFace->glyph->bitmap_top;
//		glyphInfo.back().xAdvance = ftFace->glyph->advance.x / 64;
//
//		FT_Bitmap& ftb = ftFace->glyph->bitmap;
//		uint32_t pixels = ftb.width * ftb.rows;
//		if (pixels == 0)
//			continue;
//
//		GlyphBitmap bmp{};
//		bmp.width = ftb.width;
//		bmp.height = ftb.rows;
//
//		pixelMode = ftb.pixel_mode;
//		uint8_t* src = ftb.buffer;
//		int pitch = ftb.pitch > 0 ? ftb.pitch : -ftb.pitch;
//		int n = ftb.pitch > 0 ? 0 : ftb.rows - 1;
//		int s = ftb.pitch > 0 ? 1 : -1;
//		if (pixelMode == FT_PIXEL_MODE_MONO)
//		{
//			bmp.pitch = (ftb.width + 7) / 8;
//			uint8_t* dst = bmp.bitmap = new uint8_t[bmp.pitch * bmp.height];
//
//			for (int row = n, j = 0; j < ftb.rows; row = n + s * ++j, src = ftb.buffer + pitch * row)
//				memcpy(dst + bmp.pitch * j, src, bmp.pitch);
//		}
//		else if (pixelMode == FT_PIXEL_MODE_GRAY)
//		{
//			bmp.pitch = pitch;
//			uint8_t* dst = bmp.bitmap = new uint8_t[pixels];
//
//			for (int row = n, j = 0; j < ftb.rows; row = n + s * ++j, src = ftb.buffer + pitch * row)
//				memcpy(dst + bmp.width * j, src, bmp.width);
//		}
//		else
//			continue;
//
//		glyphBmps.resize(glyphBmps.size() + 1);
//		glyphBmps.back() = bmp;
//
//		glyphInfo.back().hasImage = 1;
//		glyphInfo.back().offset = m.numPixels;
//		glyphInfo.back().width = ftFace->glyph->bitmap.width;
//		glyphInfo.back().height = ftFace->glyph->bitmap.rows;
//
//		m.numPixels += pixels;
//	}
//
//	if (rangeHasBegan)
//		codeRanges.back().lastCode = i - 1;
//
//	//-----Open atlas file
//	std::ofstream ofs("../Demo2/atlas2", std::ios::binary);
//
//	//-----write header mark
//	ofs.write(ATLAS_HEADER_MARK, strlen(ATLAS_HEADER_MARK));
//
//	//-----write meta
//	m.numCodeRanges = codeRanges.size();
//	m.numGlyphs = glyphInfo.size();
//	m.fontSize = FONT_SIZE;
//	m.xAdvance = ftFace->size->metrics.max_advance / 64;
//	m.ascender = ftFace->size->metrics.ascender / 64;
//	m.descender = ftFace->size->metrics.descender / 64;
//	ofs.write((char*)&m, sizeof(m));
//
//	//-----write range(s)
//	for (size_t i = 0; i < codeRanges.size(); i++)
//		ofs.write((char*)&codeRanges[i], sizeof(CodeRange));
//
//	//-----write coords
//	for (size_t i = 0; i < glyphInfo.size(); i++)
//		ofs.write((char*)&glyphInfo[i], sizeof(glyphInfo[i]));
//
//	//-----write pixels
//	if (pixelMode == FT_PIXEL_MODE_MONO)
//	{
//		char c = 0;
//		int rOffset = 0;
//		int lOffset = 8;
//		for (size_t i = 0; i < glyphBmps.size(); i++)
//		{
//			auto& bmp = glyphBmps[i];
//
//			int n = bmp.width % 8;
//			if (n == 0 && rOffset == 0)
//			{
//				ofs.write((char*)bmp.bitmap, bmp.pitch * bmp.height);
//				continue;
//			}
//
//			size_t pitch = n == 0 ? bmp.pitch : bmp.pitch - 1;
//			for (size_t j = 0, k = 0; j < bmp.height; j++, k = 0)
//			{
//				uint8_t* src = bmp.bitmap + bmp.pitch * j;
//				for (; k < pitch; k++)
//				{
//					c |= src[k] >> rOffset;
//					ofs.write(&c, 1);
//					c = src[k] << lOffset;
//				}
//				if (n > 0)
//				{
//					c |= src[k] >> rOffset;
//					rOffset += n;
//					if (rOffset > 7)
//					{
//						ofs.write(&c, 1);
//						rOffset = rOffset - 8;
//						c = src[k] << (n - rOffset);
//					}
//					lOffset = 8 - rOffset;
//				}
//			}
//
//			delete[] glyphBmps[i].bitmap;
//		}
//		if (rOffset > 0)
//			ofs.write(&c, 1);
//	}
//	else if (pixelMode == FT_PIXEL_MODE_GRAY)
//	{
//		for (size_t i = 0; i < glyphBmps.size(); i++)
//		{
//			auto& bmp = glyphBmps[i];
//			ofs.write((char*)bmp.bitmap, bmp.width * bmp.height);
//
//			delete[] glyphBmps[i].bitmap;
//		}
//	}
//
//	ofs.close();
//
//	FT_Done_Face(ftFace);
//	FT_Done_FreeType(ftLib);
//}
