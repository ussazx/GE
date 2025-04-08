#pragma once
#include "LuaWrapper/LuaUtility.h"
#include "../EngineBase/include/EngineBase.h"

struct Terminal
{
	struct CFileFinder
	{
		virtual ~CFileFinder() {}
		virtual bool FindFirst(LString path) = 0;
		virtual bool FindNext() = 0;
		virtual bool IsDirectory() = 0;
		virtual LuacObjNew<LString> GetName() = 0;
		Lua_wrap_cpp_class(CFileFinder, Lua_abstract, Lua_mf(FindFirst), Lua_mf(FindNext), Lua_mf(IsDirectory), Lua_mf(GetName));
	};

	struct CTimer
	{
		virtual ~CTimer() {}
		virtual void Start(int, bool) = 0;
		virtual void Stop() = 0;
		Lua_wrap_cpp_class(CTimer, Lua_abstract, Lua_mf(Start), Lua_mf(Stop))
	};

	static void AddEvent(const char* name, int id);
	static void FlushStdout();
	static void FlushStderr();
	static LuacObjNew<CTimer> NewTimer(uint32_t id);
	static LuacObjNew<CFileFinder> NewFileFinder();
	static void SetClipboardText(LString s);
	static LuacObjNew<LString> GetClipboardText();
	static LuacObjNew<LString> NewFileDialog(LString title, LString defName, LString filter);
	static void NewDirectory(LString path);
	static void SetCurrentDir(LString path);

	static void OnLuaError(const char* err);
	static void OnRequired(LuaState& lua, const char* requiredName);

	static LuaState& Lua()
	{
		return *GetLua();
	}

	static void CleanUp()
	{
		Lua().GetValue("AppCleanUp", LuaCall());
		GetLua(true);
	}

	Lua_wrap_cpp_class(Terminal, Lua_abstract, Lua_mf(AddEvent), Lua_mf(FlushStdout), Lua_mf(FlushStderr), Lua_mf(NewTimer),
		Lua_mf(NewFileFinder), Lua_mf(GetClipboardText), Lua_mf(SetClipboardText), Lua_mf(NewFileDialog),
		Lua_mf(NewDirectory), Lua_mf(SetCurrentDir))

private:
	static void LuaRegister()
	{
		LuaState::SetErrorFunc(OnLuaError);
		LuaState::SetRequireFunc(OnRequired);
		LuaRegisterCppClass<CFileFinder>(Lua());
		LuaRegisterCppClass<CTimer>(Lua());
		static Terminal t;
		Lua().SetValue("cTerminal", Lua_set_cobj(&t));
	}

	static LuaState* GetLua(bool free = false)
	{
		static std::unique_ptr<LuaState> lua;
		if (free)
			delete lua.release();
		else if (lua == nullptr)
		{
			lua = std::make_unique<LuaState>();
			LuaRegister();
		}
		return lua.get();
	}
};
