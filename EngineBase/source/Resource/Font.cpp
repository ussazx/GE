#include "../Internal.h"
#include "FontAtlas.h"
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
		uint32_t index = 0;
		if (*it >= table->indices.size())
			index = L'?' < table->indices.size() ? table->indices[L'?'] : 0xffff;
		else
			index = table->indices[*it];

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
		uint32_t index = 0;
		if (*it >= table->indices.size())
			index = L'?' < table->indices.size() ? table->indices[L'?'] : 0xffff;
		else
			index = table->indices[*it];
		
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
		n -= range.firstCode;
		for (size_t j = 0; j < n; j++)
			table->indices[j] = range.startCoordIdx + j;
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
		size_t n = input->Load(src, pitch);
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
		LuaSub("fontSize", atlas.font.fontSize),
		LuaSub("ascender", atlas.font.ascender),
		LuaSub("descender", atlas.font.descender),
		LuaSub("pixels", atlas.numPixels),
		LuaSub("view", atlasView.set));
}
Lua_global_add_cfunc(CLoadFontAtlas);
