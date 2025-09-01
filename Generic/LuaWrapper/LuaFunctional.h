#pragma once
#include "lua.hpp"
#include <tuple>
#include <assert.h>

struct LuaMeta {};

#ifdef _DEBUG
#ifdef WIN32
#include <Windows.h>
#define Assert(a) if (!(a)) DebugBreak();
#else
#define Assert(a) assert(a)
#endif
#else
#define Assert(a)
#endif

#if LUA_FLOAT_TYPE == LUA_FLOAT_FLOAT
#define Lua_float_type2 double
#else
#define Lua_float_type2 float
#endif

#if LUA_INT_TYPE == LUA_INT_INT
#define Lua_int_type2 long long
#else
#define Lua_int_type2 int
#endif

#define Lua_I(idx, n) ((idx) - ((0x80000000 & (idx)) >> 31) * (n))
#define Lua_T(idx, top) ((idx) >= 0 ? (idx) : (top) + (idx) + 1)

struct lua_Idx
{
	explicit lua_Idx(int n) : idx(n) {}
	int idx;
};

inline void LuaPushValue(lua_State* lua, std::nullptr_t)
{
	lua_pushnil(lua);
}

inline void LuaPushValue(lua_State* lua, const lua_Idx& idx)
{
	lua_pushvalue(lua, idx.idx);
}

inline void LuaPushValue(lua_State* lua, lua_Integer v)
{
	lua_pushinteger(lua, v);
}

inline void LuaPushValue(lua_State* lua, Lua_int_type2 v)
{
	lua_pushinteger(lua, v);
}

inline void LuaPushValue(lua_State* lua, uint32_t v)
{
	lua_pushinteger(lua, v);
}

inline void LuaPushValue(lua_State* lua, long v)
{
	lua_pushinteger(lua, v);
}

inline void LuaPushValue(lua_State* lua, unsigned long v)
{
	lua_pushinteger(lua, v);
}

inline void LuaPushValue(lua_State* lua, uint64_t v)
{
	lua_pushinteger(lua, v);
}

inline void LuaPushValue(lua_State* lua, lua_Number v)
{
	lua_pushnumber(lua, v);
}

inline void LuaPushValue(lua_State* lua, Lua_float_type2 v)
{
	lua_pushnumber(lua, v);
}

inline void LuaPushValue(lua_State* lua, bool v)
{
	lua_pushboolean(lua, v);
}

inline void LuaPushValue(lua_State* lua, const char* v)
{
	lua_pushstring(lua, v);
}

inline void LuaPushValue(lua_State* lua, void* v)
{
	lua_pushlightuserdata(lua, v);
}

inline void LuaPushValue(lua_State* lua, lua_CFunction func)
{
	lua_pushcfunction(lua, func);
}

template<typename ...T>
inline void LuaPushTupleValue(lua_State *L, const std::tuple<T...>& t)
{
	LuaPushValue(L, t._Myfirst._Val);
	LuaPushTupleValue(L, t._Get_rest());
}

inline void LuaPushTupleValue(lua_State *L, const std::tuple<>&) {}

template<typename T>
inline typename std::enable_if<std::_Is_nonbool_integral<T>::value, T>::type LuaGetValue(lua_State* lua, int i)
{
	Assert(abs(i) <= lua_gettop(lua));
	Assert(lua_type(lua, i) == LUA_TNUMBER);
	return lua_tonumber(lua, i);
}

template<typename T>
inline typename std::enable_if<std::is_floating_point<T>::value, T>::type LuaGetValue(lua_State* lua, int i)
{
	Assert(abs(i) <= lua_gettop(lua));
	Assert(lua_type(lua, i) == LUA_TNUMBER);
	return lua_tonumber(lua, i);
}

template<typename T>
inline typename std::enable_if<std::is_same<T, bool>::value, T>::type LuaGetValue(lua_State* lua, int i)
{
	Assert(abs(i) <= lua_gettop(lua));
	Assert(lua_type(lua, i) == LUA_TBOOLEAN);
	return lua_toboolean(lua, i);
}

template<typename T>
inline typename std::enable_if<std::is_same<T, const char*>::value, T>::type LuaGetValue(lua_State* lua, int i)
{
	Assert(abs(i) <= lua_gettop(lua));
	Assert(lua_type(lua, i) == LUA_TSTRING);
	return lua_tostring(lua, i);
}

template<typename T>
inline typename std::enable_if<std::is_pointer<T>::value && !std::is_same<T, const char*>::value, T>::type LuaGetValue(lua_State* lua, int i)
{
	Assert(abs(i) <= lua_gettop(lua));
	Assert(lua_type(lua, i) == LUA_TLIGHTUSERDATA);
	return (T)lua_touserdata(lua, i);
}

inline void LuaGetTable(lua_State* lua, const char* name, bool nilOnly = false)
{
	lua_getglobal(lua, name);
	if (lua_type(lua, -1) != LUA_TTABLE && (lua_type(lua, -1) == LUA_TNIL || !nilOnly))
	{
		lua_pop(lua, 1);
		lua_newtable(lua);
		lua_setglobal(lua, name);
		lua_getglobal(lua, name);
	}
}

inline void LuaGetTable(lua_State* lua, int idx, bool nilOnly = true)
{
	if (lua_type(lua, idx) != LUA_TTABLE && (lua_type(lua, idx) == LUA_TNIL || !nilOnly))
	{
		lua_newtable(lua);
		lua_copy(lua, -1, Lua_I(idx, 1));
		lua_pop(lua, 1);
	}
}

inline void LuaGetTableTable(lua_State* lua, int tableIdx, const char* key)
{
	lua_getfield(lua, tableIdx, key);
	if (lua_type(lua, -1) != LUA_TTABLE)
	{
		lua_pop(lua, 1);
		lua_newtable(lua);
		lua_setfield(lua, Lua_I(tableIdx, 1), key);
		lua_getfield(lua, tableIdx, key);
	}
}

template<typename T>
inline void LuaGetTableTable(lua_State* lua, int tableIdx, const T& key)
{
	LuaPushValue(lua, key);
	lua_gettable(lua, Lua_I(tableIdx, 1));
	if (lua_type(lua, -1) != LUA_TTABLE)
	{
		lua_pop(lua, 1);
		LuaPushValue(lua, key);
		lua_newtable(lua);
		lua_settable(lua, Lua_I(tableIdx, 2));
		LuaPushValue(lua, key);
		lua_gettable(lua, Lua_I(tableIdx, 1));
	}
}

inline void LuaGetTableTable(lua_State* lua, int tableIdx, const LuaMeta&)
{
	if (lua_type(lua, tableIdx) == LUA_TNIL)
	{
		lua_newtable(lua);
		lua_copy(lua, -1, Lua_I(tableIdx, 1));
		lua_pop(lua, 1);
	}
	if (lua_getmetatable(lua, tableIdx) == 0)
	{
		lua_newtable(lua);
		lua_setmetatable(lua, Lua_I(tableIdx, 1));
		lua_getmetatable(lua, tableIdx);
	}
}

template<typename T0, typename T1>
inline void LuaSetField(lua_State* lua, int tableIdx, const T0& k, const T1& v)
{
	LuaGetTable(lua, tableIdx);
	LuaPushValue(lua, k);
	LuaPushValue(lua, v);
	lua_settable(lua, Lua_I(tableIdx, 2));
}

template<typename T>
inline void LuaSetField(lua_State* lua, int tableIdx, const T& k, luaL_Reg* v)
{
	LuaGetTable(lua, tableIdx);
	LuaGetTableTable(lua, tableIdx, k);
	luaL_setfuncs(lua, v, 0);
	lua_pop(lua, 1);
}

template<typename T>
inline void LuaSetField(lua_State* lua, int tableIdx, const T& k, const luaL_Reg* v)
{
	LuaGetTable(lua, tableIdx);
	LuaGetTableTable(lua, tableIdx, k);
	luaL_setfuncs(lua, v, 0);
	lua_pop(lua, 1);
}

inline void LuaSetField(lua_State* lua, int tableIdx, const char* k, luaL_Reg* v)
{
	LuaGetTable(lua, tableIdx);
	LuaGetTableTable(lua, tableIdx, k);
	luaL_setfuncs(lua, v, 0);
	lua_pop(lua, 1);
}

inline void LuaSetField(lua_State* lua, int tableIdx, const char* k, const luaL_Reg* v)
{
	LuaGetTable(lua, tableIdx);
	LuaGetTableTable(lua, tableIdx, k);
	luaL_setfuncs(lua, v, 0);
	lua_pop(lua, 1);
}

template<typename T>
inline void LuaSetField(lua_State* lua, int tableIdx, const char* k, const T& v)
{
	LuaGetTable(lua, tableIdx);
	LuaPushValue(lua, v);
	lua_setfield(lua, Lua_I(tableIdx, 1), k);
}

template<typename T>
inline void LuaGetField(lua_State* lua, int tableIdx, T k)
{
	LuaPushValue(lua, k);
	lua_gettable(lua, Lua_I(tableIdx, 1));
}

inline void LuaGetField(lua_State* lua, int tableIdx, const char* k)
{
	lua_getfield(lua, tableIdx, k);
}