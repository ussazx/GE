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

	struct TerminalNotification
	{
		void(*addEvent)(const char* name, int id);
		void(*flushStdout)(void);
		void(*flushStderr)(void);
	};

	DECLSPEC void LuaRegister(lua_State* L, const TerminalNotification& n);

	DECLSPEC bool Initialize(const InitParam& param);
	DECLSPEC void CleanUp();
}