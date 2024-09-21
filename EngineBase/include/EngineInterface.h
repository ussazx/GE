#pragma once
#include <stdint.h>

#ifdef  DLL_EXPORTS
#define DECLSPEC extern __declspec(dllexport)
#define DECLSPEC_C extern "C" __declspec(dllexport)
#else
#define DECLSPEC extern __declspec(dllimport)
#define DECLSPEC_C extern "C" __declspec(dllimport)
#endif

namespace Engine
{
	enum SeekPos
	{
		CUR,
		BEG,
		END
	};

#ifndef INTERFACE_IMPLEMENT
	DECLSPEC class StreamInput
	{
	public:
		virtual ~StreamInput() {};
		virtual bool IsValid() = 0;
		virtual int64_t GetSize() = 0;
		virtual int64_t GetOffset() = 0;
		virtual void* GetData() = 0;
		virtual int64_t Seek(int64_t offset, SeekPos start = CUR) = 0;
		virtual int64_t Read(void* buff, int64_t size) = 0;

		template<class T>
		__int64 Load(T& out, bool missingFail = true)
		{
			__int64 n = Read(&out, sizeof(T));
			return missingFail && n < sizeof(T) ? 0 : n;
		}
	};
#else
	class StreamInput;
#endif

	enum ValueType
	{
		None,
		Integer,
		Float,
		String,
		Bool,
		Pointer,
		StreamIn
	};

	DECLSPEC class CallStack
	{
	public:
		virtual ~CallStack() {};

		virtual uint32_t GetCount() = 0;

		virtual ValueType PopInteger(int64_t& value) = 0;
		virtual ValueType PopFloat(float& value) = 0;
		virtual ValueType PopBool(bool& value) = 0;
		virtual ValueType PopString(const char*& value) = 0;
		virtual ValueType PopPointer(void*& value) = 0;
		virtual ValueType PopStream(StreamInput*& value) = 0;

		virtual void PushInteger(int64_t value) = 0;
		virtual void PushFloat(float value) = 0;
		virtual void PushBool(bool value) = 0;
		virtual void PushString(const char* value) = 0;
		virtual void PushPointer(void* value) = 0;
	};

	DECLSPEC class Registry
	{
	public:
		virtual ~Registry() {};
		virtual void Register(const char* name, void(*function)(CallStack&)) = 0;
	};

	struct ImageInfo
	{

	};

	class ImageLoader
	{
	public:
		virtual ~ImageLoader() {}
		virtual bool Load(StreamInput& input, ImageInfo& info) = 0;
	};

	class Importer
	{
	public:
	};

	struct GlyphField
	{
		uint32_t offset;
		uint16_t lineNo;
		uint16_t hasImage;
		uint16_t width;
		uint16_t height;
		int xOffset;
		int yOffset;
		float xAdvance;
	};

	DECLSPEC class Font
	{
	public:
		virtual ~Font() {};
		virtual GlyphField& GetGlyphField(uint16_t code) = 0;
	};
};