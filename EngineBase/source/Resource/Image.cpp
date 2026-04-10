#include "../Internal.h"
#include "png/src/png.h"
#include "../Graphic/Graphic.h"

static void OnPngWarning(png_structp png_ptr, png_const_charp message) {}

static void OnPngError(png_structp png_ptr, png_const_charp message)
{
	throw std::exception();
}

static void ReadStreamPng(png_structp png_ptr, png_bytep data, png_size_t length)
{
	Engine::StreamInput* input = (Engine::StreamInput*)png_get_io_ptr(png_ptr);
	if (data == nullptr)
		input->Seek(length);
	input->Read(data, length);
}

static LuacObjNew<Texture> CNewEmptyImage(uint32_t width, uint32_t height, byte r, byte g, byte b, byte a)
{
	LuacObjNew<Texture> texture = g_graphic->NewTexture(
		g_graphic->GetDefined("IMAGE_TYPE_2D"),
		g_graphic->GetDefined("FORMAT_R8G8B8A8_UNORM"),
		width, height);

	unsigned char* buffer = (unsigned char*)texture->GetData();
	short bytesPerRow = texture->GetRowPitch();
	for (uint32_t i = 0; i < height; i++)
	{
		unsigned char* p = buffer + i * bytesPerRow;
		for (uint32_t j = 0, k0 = 0, k1 = 0; j < width; j++, k0 = 4 * j, k1 = 3 * j)
		{
			p[k0++] = r;
			p[k0++] = g;
			p[k0++] = b;
			p[k0] = a;
		}
	}
	return texture;
}
Lua_global_add_cfunc(CNewEmptyImage)

static LuacObjNew<Texture> CLoadImagePng(LuacObj<Engine::StreamInput> input)
{
	//unsigned char c[4]{};
	//if (!input->Read(c, 4) == 4 && memcmp(c, "\211PNG", 4))
	//	return nullptr;
	
	png_structp psp = png_create_read_struct
	(
		PNG_LIBPNG_VER_STRING,
		NULL,
		OnPngError,
		OnPngWarning
	);
	if (!psp)
		return false;
	png_infop pip = png_create_info_struct(psp);
	if (!pip)
	{
		png_destroy_read_struct(&psp, nullptr, nullptr);
		return false;
	}

	uint32_t width = 0, height = 0;
	int bitDepth = 0, colorType = 0;
	LuacObjNew<Texture> texture = nullptr;
	try
	{
		png_set_read_fn(psp, (void*)input, ReadStreamPng);
		png_read_info(psp, pip);
		png_get_IHDR(psp, pip, &width, &height, &bitDepth, &colorType, nullptr, nullptr, nullptr);

		png_set_expand(psp);
		png_set_gray_to_rgb(psp);
		png_set_strip_16(psp);
		png_set_packing(psp);

		bool hasAlpha = (colorType & PNG_COLOR_MASK_ALPHA) ||
		png_get_valid(psp, pip, PNG_INFO_tRNS);

		texture = g_graphic->NewTexture(
			g_graphic->GetDefined("IMAGE_TYPE_2D"),
			g_graphic->GetDefined("FORMAT_R8G8B8A8_UNORM"),
			width, height);

		unsigned char* buffer = (unsigned char*)texture->GetData();
		short bytesPerRow = texture->GetRowPitch();

		if (hasAlpha)
			for (uint32_t i = 0; i < height; i++)
				png_read_row(psp, buffer + bytesPerRow * i, nullptr);
		else
		{
			unsigned char* c = (unsigned char*)alloca(width * 3);
			for (uint32_t i = 0; i < height; i++)
			{
				png_read_row(psp, c, nullptr);
				unsigned char* p = buffer + i * bytesPerRow;
				for (uint32_t j = 0, k0 = 0, k1 = 0; j < width; j++, k0 = 4 * j, k1 = 3 * j)
				{
					p[k0++] = c[k1++];
					p[k0++] = c[k1++];
					p[k0++] = c[k1];
					p[k0] = 255;
				}
			}
		}
		
		png_read_end(psp, pip);
		png_destroy_read_struct(&psp, &pip, nullptr);
	}
	catch (const std::exception&)
	{
		if (texture.object)
			delete texture.object;
		return nullptr;
	}
	return texture;
}
Lua_global_add_cfunc(CLoadImagePng)