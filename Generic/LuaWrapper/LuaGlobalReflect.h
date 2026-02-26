//***************************************************************************************
// Effects.h by ussa (C) 2026 All Rights Reserved.
// Licensed under the MIT License.
//***************************************************************************************

#pragma once
#include "LuaWrapper.h"
#include <unordered_map>

#define Lua_global_add_cfunc(func) \
static int lua_nop_##func = LuaRegGlobalReflected(nullptr, { #func, Lua_cf(func) });

#define Lua_global_add_cpp_class(cpp_class) \
static int lua_nop_##cpp_class = LuaRegGlobalReflected(nullptr, {}, #cpp_class, \
{cpp_class::LuaGetBaseName(), sizeof(cpp_class), cpp_class::LuaGetObjectCtor(), \
	LuaObjectDtor<cpp_class>, typeid(cpp_class).hash_code(), cpp_class::LuaSetClassTable});

struct LuaCppClassReg
{
	const char* baseClass;
	size_t size;
	lua_CFunction objectCtor;
	lua_CFunction objectDtor;
	size_t id;
	void(*setClassTable)(const LuaState&, const lua_Idx&);
};

inline int LuaRegGlobalReflected(lua_State* lua, const luaL_Reg& funcReg = {}, const char* className = {}, const LuaCppClassReg& classReg = {})
{
	static std::unordered_map<const char*, lua_CFunction> luaCFunc;
	static std::unordered_map<const char*, LuaCppClassReg> luaCppClass;

	if (funcReg.name)
		luaCFunc[funcReg.name] = funcReg.func;
	if (className)
		luaCppClass[className] = classReg;

	if (lua)
	{
		LuaState Lua(lua);
		for (auto it = luaCFunc.begin(); it != luaCFunc.end(); it++)
			Lua.SetValue(it->first, it->second);
		
		for (auto it = luaCppClass.begin(); it != luaCppClass.end(); it++)
			LuaRegisterCppClass(Lua, it->first, it->second.baseClass, it->second.size,
				it->second.objectCtor, it->second.objectDtor, it->second.id, it->second.setClassTable);
	}
	return 0;
}

