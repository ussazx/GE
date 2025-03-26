#pragma once
#include "LuaWrapper.h"
#include <unordered_map>

#define Lua_global_add_cfunc(func) \
static int lua_nop_##func = LuaRegGlobalCollected(nullptr, { #func, Lua_cf(func) });

#define Lua_global_add_cpp_class(cpp_class) \
static int lua_nop_##cpp_class = LuaRegGlobalCollected(nullptr, {}, #cpp_class, \
{cpp_class::LuaGetBaseName(), cpp_class::LuaGetObjectCtor(), LuaObjectDtor<cpp_class>, cpp_class::LuaSetClassTable});

struct LuaCppClassReg
{
	const char* baseClass;
	lua_CFunction objectCtor;
	lua_CFunction objectDtor;
	void(*setClassTable)(const LuaState&, const lua_Idx&);
};

inline int LuaRegGlobalCollected(LuaState* lua, const luaL_Reg& funcReg = {}, const char* className = {}, const LuaCppClassReg& classReg = {})
{
	static std::unordered_map<const char*, lua_CFunction> luaCFunc;
	static std::unordered_map<const char*, LuaCppClassReg> luaCppClass;

	if (funcReg.name)
		luaCFunc[funcReg.name] = funcReg.func;
	if (className)
		luaCppClass[className] = classReg;

	if (lua)
	{
		for (auto it = luaCFunc.begin(); it != luaCFunc.end(); it++)
			lua->SetValue(it->first, it->second);
		
		for (auto it = luaCppClass.begin(); it != luaCppClass.end(); it++)
			LuaRegisterCppClass(*lua, it->first, it->second.baseClass, it->second.objectCtor, it->second.objectDtor, it->second.setClassTable);
	}
	return 0;
}

