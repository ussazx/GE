#pragma once
#include "lua.hpp"
#include <stdint.h>

#ifdef WIN32
#include <Windows.h>
#elif defined ANDROID
#include <native_activity.h>
#endif

#ifdef  DLL_EXPORTS
#define DECLSPEC extern __declspec(dllexport)
#define DECLSPEC_C extern "C" __declspec(dllexport)
#else
#define DECLSPEC extern __declspec(dllimport)
#define DECLSPEC_C extern "C" __declspec(dllimport)
#endif

namespace Engine
{
	struct InitParam
	{
#ifdef WIN32
		HINSTANCE hInst;
#elif defined ANDROID
		AAssetManager* assetManager;
#endif
	};

	struct TerminalImpl
	{
		struct FileParser
		{
			virtual ~FileParser() {}
			virtual bool FindFirst(const wchar_t*) = 0;
			virtual bool FindNext() = 0;
		};
		void(*addEvent)(const char* name, int id);
		void(*flushStdout)(void);
		void(*flushStderr)(void);
		void(*setClipboardText)(const wchar_t*);
		const wchar_t*(*getClipboardText)();
		FileParser*(*newFileParser)();
		void(*newDirectory)(const wchar_t*);
		void(*setCurrentDir)(const wchar_t*);
		const wchar_t*(*newFileDialog)(const wchar_t* title, const wchar_t* defName, const wchar_t* filter);
	};

	DECLSPEC void LuaRegister(lua_State* L, const TerminalImpl& n);

	DECLSPEC bool Initialize(const InitParam& param);
	DECLSPEC void CleanUp();
}