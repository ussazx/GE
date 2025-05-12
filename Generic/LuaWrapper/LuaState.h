#pragma once
#include "LuaFunctional.h"
#include <vector>
#include <functional>

#define LuaMultiArg(name) \
template<typename ...T> \
struct name##Arg \
{ \
	name##Arg(T&&... tt) : t(std::forward<T>(tt)...) {} \
	std::tuple<std::_Unrefwrap_t<T>...> t; \
}; \
template<typename ...T> \
name##Arg<T...> name(T&&... args) \
{ \
	return name##Arg<T...>(std::forward<T>(args)...); \
}

LuaMultiArg(LuaGet)

LuaMultiArg(LuaCall)

LuaMultiArg(LuaObjCall)

LuaMultiArg(LuaSetTo)

//template<typename ...T>
//std::tuple<T...> LuaSub(const T&... args)
//{
//	return std::tuple<T...>(args...);
//}

#define LuaSub(...) std::make_tuple(__VA_ARGS__)

struct LuaLoad
{
	LuaLoad(void* c, size_t n = 0) : code((char*)c), len(n == 0 ? strlen((char*)c) : n) {}
	const char* code;
	size_t len;
};

class LuaState;

typedef std::function<void(const LuaState&, const lua_Idx&)> LuaCustomSet;

template<typename ...T>
inline LuaCustomSet LuaDataSet(T&& ...t)
{
	return[t...](const LuaState& state, const lua_Idx& idx)
	{
		state.SetValue(idx, t...);
	};
}

class LuaState
{
public:
	LuaState(lua_State* L = nullptr)
	{ 
		m_lua = L;
		if (m_lua == nullptr)
		{
			m_lua = luaL_newstate();
			luaL_openlibs(m_lua);
			m_closeOnDtor = true;
		}
		else
			m_closeOnDtor = false;
	}

	lua_State* Lua() const
	{
		return m_lua;
	}
	
	LuaState(const LuaState&) = delete;
	const LuaState& operator = (const LuaState&) = delete;

	~LuaState() { if (m_lua && m_closeOnDtor) lua_close(m_lua); };

	static void SetErrorFunc(void(*errFunc)(const char*))
	{
		GetErrorFunc(errFunc);
	}

	static void SetRequireFunc(void(*requireFunc)(LuaState&, const char*))
	{
		GetRequireFunc(requireFunc);
	}

	int GetTop() const
	{
		return lua_gettop(m_lua);
	}

	//SetValue
	template<typename T>
	void SetValue(const char* k, const T& v) const
	{
		LuaPushValue(m_lua, v);
		lua_setglobal(m_lua, k);
	}

	template<typename T>
	void SetValue(const lua_Idx& idx, const T& v) const
	{
		LuaPushValue(m_lua, v);
		lua_copy(m_lua, -1, Lua_I(idx.idx, 1));
		lua_pop(m_lua, 1);
	}

	template<typename ...T>
	void SetValue(const char* k0, const T&... kv) const
	{
		LuaGetTable(m_lua, k0);
		SetValue(lua_Idx(-1), kv...);
		lua_pop(m_lua, 1);
	}

	void SetValue(const char* k, const LuaCustomSet& cs) const
	{
		lua_getglobal(m_lua, k);
		if (lua_type(m_lua, -1) == LUA_TTABLE)
		{
			cs(*this, lua_Idx(GetTop()));
			lua_pop(m_lua, 1);
		}
		else
		{
			cs(*this, lua_Idx(GetTop()));
			lua_setglobal(m_lua, k);
		}
	}

	void SetValue(const lua_Idx& idx, const LuaCustomSet& cs) const
	{
			cs(*this, lua_Idx(Lua_T(idx.idx, GetTop())));
	}

	template<typename T>
	void SetValue(const lua_Idx& idx, const T& k, const LuaCustomSet& cs) const
	{
		int i = idx.idx;
		LuaGetTable(m_lua, i);
		LuaGetTableTable(m_lua, i, k);
		cs(*this, lua_Idx(GetTop()));
		lua_pop(m_lua, 1);
	}

	template<typename ...T>
	void SetValue(const char* k, const LuaGetArg<T...>& get) const
	{
		lua_pushnil(m_lua);
		GetValue(std::tuple_cat(get.t, std::make_tuple(LuaSetTo(lua_Idx(GetTop())))));
		lua_setglobal(m_lua, k);
	}

	template<typename ...T>
	void SetValue(const lua_Idx& idx, const LuaGetArg<T...>& get) const
	{
		GetValue(std::tuple_cat(get.t, std::make_tuple(LuaSetTo(lua_Idx(Lua_T(idx.idx, GetTop()))))));
	}

	template<typename T0, typename ...T1>
	void SetValue(const lua_Idx& idx, const T0& k, const LuaGetArg<T1...>& get) const
	{
		int i = idx.idx;
		LuaGetTable(m_lua, i);
		LuaPushValue(m_lua, k);
		lua_pushnil(m_lua);
		GetValue(std::tuple_cat(get.t, std::make_tuple(LuaSetTo(lua_Idx(GetTop())))));
		lua_settable(m_lua, Lua_I(i, 2));
	}

	template<typename T0, typename T1>
	void SetValue(const lua_Idx& idx, const T0& k, const T1& v) const
	{
		int i = idx.idx;
		LuaSetField(m_lua, i, k, v);
	}

	template<typename T0, typename ...T1>
	void SetValue(const lua_Idx& idx, const T0& k0, const T1&... kv) const
	{
		int i = idx.idx;
		LuaGetTable(m_lua, i);
		LuaGetTableTable(m_lua, i, k0);
		SetValue(lua_Idx(-1), kv...);
		lua_pop(m_lua, 1);
	}

	void SetValue(const lua_Idx& dst, const LuaMeta&, const lua_Idx& src) const
	{
		int srci = src.idx;
		if (lua_type(m_lua, srci) == LUA_TTABLE)
		{	
			LuaGetTable(m_lua, dst.idx);
			lua_pushvalue(m_lua, srci);
			lua_setmetatable(m_lua, Lua_I(dst.idx, 1));
		}
	}

	void SetValue(const lua_Idx& dst, const LuaMeta&, const char* src) const
	{
		lua_getglobal(m_lua, src);
		if (lua_type(m_lua, -1) == LUA_TTABLE)
		{
			int dsti = Lua_I(dst.idx, 1);
			LuaGetTable(m_lua, dsti);
			lua_setmetatable(m_lua, dsti);
		}
		else
			lua_pop(m_lua, 1);
	}

	template<typename ...T>
	void SetValue(const lua_Idx& dst, const LuaMeta&, const LuaGetArg<T...>& get) const
	{
		lua_pushnil(m_lua);
		GetValue(std::tuple_cat(get.t, std::make_tuple(LuaSetTo(lua_Idx(GetTop())))));
		if (lua_type(m_lua, -1) == LUA_TTABLE)
		{
			int dsti = Lua_I(dst.idx, 1);
			LuaGetTable(m_lua, dsti);
			lua_setmetatable(m_lua, dsti);
		}
		else
			lua_pop(m_lua, 1);
	}

	void SetValue(const lua_Idx& dst, const LuaMeta&, const LuaCustomSet& cs) const
	{
		lua_pushnil(m_lua);
		cs(*this, lua_Idx(GetTop()));
		if (lua_type(m_lua, -1) == LUA_TTABLE)
		{
			int dsti = Lua_I(dst.idx, 1);
			LuaGetTable(m_lua, dsti);
			lua_setmetatable(m_lua, dsti);
		}
		else
			lua_pop(m_lua, 1);
	}

	void SetValue(const char* k, luaL_Reg* v) const
	{
		LuaGetTable(m_lua, k);
		luaL_setfuncs(m_lua, v, 0);
		lua_pop(m_lua, 1);
	}

	void SetValue(const lua_Idx& idx, luaL_Reg* v) const
	{
		int i = idx.idx;
		LuaGetTable(m_lua, i);
		lua_pushvalue(m_lua, i);
		luaL_setfuncs(m_lua, v, 0);
		lua_pop(m_lua, 1);
	}

	template<typename ...T>
	void SetValue(const lua_Idx& idx, luaL_Reg* k0, const T&... kv) const 
	{
		SetValue(idx, k0);
		SetValue(idx, kv...);
	}

	void SetValue(const char* k, const luaL_Reg* v) const
	{
		LuaGetTable(m_lua, k);
		luaL_setfuncs(m_lua, v, 0);
		lua_pop(m_lua, 1);
	}

	void SetValue(const lua_Idx& idx, const luaL_Reg* v) const
	{
		int i = idx.idx;
		LuaGetTable(m_lua, i);
		lua_pushvalue(m_lua, i);
		luaL_setfuncs(m_lua, v, 0);
		lua_pop(m_lua, 1);
	}

	template<typename ...T>
	void SetValue(const lua_Idx& idx, const luaL_Reg* k0, const T&... kv) const 
	{
		SetValue(idx, k0);
		SetValue(idx, kv...);
	}

	template<typename ...T>
	void SetValue(const char* name, const std::tuple<T...>& t) const
	{
		LuaGetTable(m_lua, name);
		SetValue(lua_Idx(-1), t);
		lua_pop(m_lua, 1);
	}

	template<typename T0, typename T1>
	void SetValue(const lua_Idx& idx, const std::tuple<T0, T1>& t) const
	{
		SetValue(idx, std::get<0>(t), std::get<1>(t));
	}

	template<typename ...T>
	void SetValue(const lua_Idx& idx, const std::tuple<T...>& t) const
	{
		SetValue(idx, t._Myfirst._Val, t._Get_rest());
	}

	template<typename T0, typename ...T1>
	void SetValue(const lua_Idx& idx, const T0& k, const std::tuple<T1...>& t) const
	{
		int i = idx.idx;
		LuaGetTable(m_lua, i);
		LuaGetTableTable(m_lua, i, k);
		SetValue(lua_Idx(-1), t);
		lua_pop(m_lua, 1);
	}

	template<typename ...T0, typename ...T1>
	void SetValue(const lua_Idx& idx, const std::tuple<T0...>& t0, const std::tuple<T1...>& t1) const
	{
		SetValue(idx, t0);
		SetValue(idx, t1);
	}
	template<typename ...T>
	void SetValue(const lua_Idx& idx, const std::tuple<T...>& t, const LuaCustomSet& cs) const
	{
		SetValue(idx, t);
		cs(*this, idx);
	}

	template<typename ...T>
	void SetValue(const lua_Idx& idx, const std::tuple<T...>& t0, luaL_Reg* t1) const
	{
		SetValue(idx, t0);
		SetValue(idx, t1);
	}

	template<typename ...T>
	void SetValue(const lua_Idx& idx, const std::tuple<T...>& t0, const luaL_Reg* t1) const
	{
		SetValue(idx, t0);
		SetValue(idx, t1);
	}

	template<typename ...T0, typename ...T1>
	void SetValue(const lua_Idx& idx, const std::tuple<T0...>& t0, const T1&...t1) const
	{
		SetValue(idx,t0);
		SetValue(idx,t1...);
	}

	void SetValue(const char* k, const LuaLoad& v) const
	{
		if (luaL_loadbuffer(m_lua, v.code, v.len, v.code) != 0)
		{
			lua_pop(m_lua, 1);
			lua_pushnil(m_lua);
		}
		lua_setglobal(m_lua, k);
	}

	void SetValue(const lua_Idx& idx, const LuaLoad& v) const
	{
		if (luaL_loadbuffer(m_lua, v.code, v.len, v.code) != 0)
		{
			lua_pop(m_lua, 1);
			lua_pushnil(m_lua);
		}
		lua_copy(m_lua, -1, Lua_I(idx.idx, 1));
		lua_pop(m_lua, 1);
	}

	template<typename T>
	void SetValue(const lua_Idx& idx, const T& k, const LuaLoad& v) const
	{
		luaL_loadbuffer(m_lua, v.code, v.len, v.code);
		LuaSetField(m_lua, Lua_I(idx.idx, 1), k, lua_Idx(-1));
		lua_pop(m_lua, 1);
	}

	template<typename T0, typename ...T1>
	void SetValue(const lua_Idx& idx, const T0& k, const LuaLoad& v0, const T1&... v1) const
	{
		SetValue(idx, k, v0);
		SetValue(idx, v1...);
	}

	//GetValue
	template<typename ...T>
	int GetValue(const char* n0, T... n1) const
	{
		lua_getglobal(m_lua, n0);
		int type = GetValue(lua_Idx(GetTop()), n1...);
		lua_pop(m_lua, 1);
		return type;
	}

	int GetValue(const lua_Idx& idx, lua_CFunction* v) const
	{
		int i = idx.idx;
		if (lua_type(m_lua, i) == LUA_TFUNCTION)
			*v = lua_tocfunction(m_lua, i);
		return lua_type(m_lua, i);
	}

	int GetValue(const lua_Idx& idx, lua_Integer* v) const
	{
		int i = idx.idx;
		if (lua_type(m_lua, i) == LUA_TNUMBER)
			*v = lua_tonumber(m_lua, i);
		return lua_type(m_lua, i);
	}

	int GetValue(const lua_Idx& idx, Lua_int_type2* v) const
	{
		int i = idx.idx;
		if (lua_type(m_lua, i) == LUA_TNUMBER)
			*v = lua_tonumber(m_lua, i);
		return lua_type(m_lua, i);
	}

	int GetValue(const lua_Idx& idx, uint32_t* v) const
	{
		int i = idx.idx;
		if (lua_type(m_lua, i) == LUA_TNUMBER)
			*v = lua_tonumber(m_lua, i);
		return lua_type(m_lua, i);
	}

	int GetValue(const lua_Idx& idx, uint64_t* v) const
	{
		int i = idx.idx;
		if (lua_type(m_lua, i) == LUA_TNUMBER)
			*v = lua_tonumber(m_lua, i);
		return lua_type(m_lua, i);
	}

	int GetValue(const lua_Idx& idx, lua_Number* v) const
	{
		int i = idx.idx;
		if (lua_type(m_lua, i) == LUA_TNUMBER)
			*v = lua_tonumber(m_lua, i);
		return lua_type(m_lua, i);
	}

	int GetValue(const lua_Idx& idx, Lua_float_type2* v) const
	{
		int i = idx.idx;
		if (lua_type(m_lua, i) == LUA_TNUMBER)
			*v = lua_tonumber(m_lua, i);
		return lua_type(m_lua, i);
	}

	int GetValue(const lua_Idx& idx, bool* v) const
	{
		int i = idx.idx;
		if (lua_type(m_lua, i) == LUA_TBOOLEAN)
			*v = lua_toboolean(m_lua, i);
		return lua_type(m_lua, i);
	}

	int GetValue(const lua_Idx& idx, const char** v) const
	{
		int i = idx.idx;
		if (lua_type(m_lua, i) == LUA_TSTRING)
			*v = lua_tostring(m_lua, i);
		return lua_type(m_lua, i);
	}

	int GetValue(const lua_Idx& idx, void** v) const
	{
		int i = idx.idx;
		if (lua_type(m_lua, i) == LUA_TLIGHTUSERDATA)
			*v = lua_touserdata(m_lua, i);
		return lua_type(m_lua, i);
	}

	int GetValue(const lua_Idx& idx, nullptr_t t) const
	{
		int i = idx.idx;
		return lua_type(m_lua, i);
	}

	template<typename T>
	int GetValue(const lua_Idx& idx, T k, nullptr_t v) const
	{
		int i = idx.idx;
		if (lua_type(m_lua, i) != LUA_TTABLE)
			return 0;

		LuaGetField(m_lua, i, k);
		int t = GetValue(lua_Idx(GetTop()), v);
		lua_pop(m_lua, 1);
		return t;
	}

	template<typename T0, typename ...T1>
	int GetValue(const lua_Idx& idx, const T0& k0, T1... kn) const
	{
		int i = idx.idx;
		if (lua_type(m_lua, i) != LUA_TTABLE)
			return 0;

		LuaGetField(m_lua, i, k0);
		int t = GetValue(lua_Idx(GetTop()), kn...);
		lua_pop(m_lua, 1);
		return t;
	}

	template<typename ...T>
	int GetValue(const lua_Idx& idx, const LuaMeta&, T... kn) const
	{
		lua_getmetatable(m_lua, idx.idx);
		if (lua_type(m_lua, -1) != LUA_TTABLE)
			return 0;

		int t = GetValue(lua_Idx(GetTop()), kn...);
		lua_pop(m_lua, 1);
		return t;
	}

	template<typename ...T>
	int GetValue(const std::tuple<T...>& t) const
	{
		return GetValue(t, std::make_index_sequence<std::tuple_size<std::tuple<T...>>::value>());
	}

	template<typename ...T, size_t ...I>
	int GetValue(const std::tuple<T...>& t, std::index_sequence<I...>) const
	{
		return GetValue(std::get<I>(t)...);
	}

	template<typename T>
	int GetValue(const lua_Idx& idx, const LuaSetToArg<T>& t) const
	{
		SetValue(t.t._Myfirst._Val, idx);
		return lua_type(m_lua, idx.idx);
	}

	template<typename ...T>
	int GetValue(const lua_Idx& idx, const LuaSetToArg<T...>& t) const
	{
		SetValue(t.t._Myfirst._Val, std::tuple_cat(t.t._Get_rest(), std::make_tuple(idx)));
		return lua_type(m_lua, idx.idx);
	}

	template<typename ...T0, typename ...T1>
	int GetValue(const lua_Idx& idx, const LuaSetToArg<T0...>& t0, T1... t1) const
	{
		GetValue(idx, t0);
		return GetValue(idx, t1...);
	}

	template<typename ...T>
	int GetValue(const lua_Idx& func, const LuaCallArg<T...>& t) const
	{
		int i = func.idx;
		int type = lua_type(m_lua, i);

		ErrFunc errFunc = GetErrorFunc(nullptr, false);
		if (errFunc) lua_pushcfunction(m_lua, ErrorFunc), i = Lua_I(i, 1);
		
		lua_pushvalue(m_lua, i);
		
		LuaPushTupleValue(m_lua, t.t);
		static int n = (int)std::tuple_size<std::tuple<T...>>::value;
		lua_pcall(m_lua, n, 0, errFunc ? -2 - n : 0);

		if (errFunc) lua_pop(m_lua, 1);

		return type;
	}

	template<typename ...T0, typename ...T1>
	int GetValue(const lua_Idx& func, const LuaCallArg<T0...>& t, T1... ret) const
	{
		int i = func.idx;
		int type = lua_type(m_lua, i);

		ErrFunc errFunc = GetErrorFunc(nullptr, false);
		if (errFunc) lua_pushcfunction(m_lua, ErrorFunc), i = Lua_I(i, 1);

		lua_pushvalue(m_lua, i);
		
		LuaPushTupleValue(m_lua, t.t);
		static int n0 = (int)std::tuple_size<std::tuple<T0...>>::value;
		static int n1 = (int)std::tuple_size<std::tuple<T1...>>::value;
		lua_pcall(m_lua, n0, n1, errFunc ? -2 - n0 : 0);

		int j = -n1;
		int v[] = { (GetValue(lua_Idx(j++), ret), 0)... };

		lua_pop(m_lua, n1 + (errFunc ? 1 : 0));

		return type;
	}

	template<typename T0, typename ...T1>
	int GetValue(const lua_Idx& table, T0 func, const LuaObjCallArg<T1...>& t) const
	{
		int i = table.idx;
		int type = lua_type(m_lua, i);

		ErrFunc errFunc = GetErrorFunc(nullptr, false);
		if (errFunc) lua_pushcfunction(m_lua, ErrorFunc), i = Lua_I(i, 1);

		LuaGetField(m_lua, i, func);

		lua_pushvalue(m_lua, Lua_I(i, 1));
		LuaPushTupleValue(m_lua, t.t);
		static int n = (int)std::tuple_size<std::tuple<T1...>>::value + 1;
		lua_pcall(m_lua, n, 0, errFunc ? -2 - n: 0);

		if (errFunc) lua_pop(m_lua, 1);

		return type;
	}

	template<typename T0, typename ...T1, typename ...T2>
	int GetValue(const lua_Idx& table, T0 func, const LuaObjCallArg<T1...>& t, T2... ret) const
	{
		int i = table.idx;
		int type = lua_type(m_lua, i);

		ErrFunc errFunc = GetErrorFunc(nullptr, false);
		if (errFunc) lua_pushcfunction(m_lua, ErrorFunc), i = Lua_I(i, 1);

		LuaGetField(m_lua, i, func);

		lua_pushvalue(m_lua, Lua_I(i, 1));
		LuaPushTupleValue(m_lua, t.t);
		static int n0 = (int)std::tuple_size<std::tuple<T1...>>::value + 1;
		static int n1 = (int)std::tuple_size<std::tuple<T2...>>::value;
		lua_pcall(m_lua, n0, n1, errFunc ? -2 - n0 : 0);

		int j = -n1;
		int v[] = { (GetValue(lua_Idx(j++), ret), 0)... };

		lua_pop(m_lua, n1 + (errFunc ? 1 : 0));

		return type;
	}

	template<typename T0, typename T1>
	int GetValue(const lua_Idx& idx, const std::tuple<T0, T1>& t) const
	{
		return GetValue(idx, std::get<0>(t), std::get<1>(t));
	}

	template<typename ...T>
	int GetValue(const lua_Idx& idx, const std::tuple<T...>& t) const
	{
		return GetValue(lua_Idx(GetTop()), t._Myfirst._Val, t._Get_rest());
	}

	template<typename ...T0, typename ...T1>
	int GetValue(const lua_Idx& idx, const std::tuple<T0...>& t0, T1...t1) const
	{
		GetValue(idx, t0);
		return GetValue(idx, t1...);
	}

	int Run(const char* code, size_t len = 0) const
	{
		return LoadRequired(nullptr, code, len);
	}
	
	int LoadRequired(const char* name, const char* code, size_t len = 0) const
	{
		if (name && strlen(name) > 0)
			SetValue(lua_Idx(LUA_REGISTRYINDEX), LUA_LOADED_TABLE, name, true);
	
		RequireFunc reqFunc = GetRequireFunc(nullptr, false);
		if (reqFunc)
			SetValue("require", Require);

		ErrFunc errFunc = GetErrorFunc(nullptr, false);
		if (errFunc)
			lua_pushcfunction(m_lua, ErrorFunc);

		int n = luaL_loadbuffer(m_lua, code, len == 0 ? strlen(code) : len, code);
		if (n != 0)
		{
			if (errFunc)
			{
				const char* s{};
				GetValue(lua_Idx(-1), &s);
				errFunc(s);
			}
			return n;
		}

		n = lua_pcall(m_lua, 0, LUA_MULTRET, errFunc ? -2 : 0);

		if (errFunc)
			lua_pop(m_lua, 1);
		return n;
	}

	const char* GetErrMsg() const
	{
		return lua_type(m_lua, -1) == LUA_TSTRING ? lua_tostring(m_lua, -1) : "empty msg";
	}

protected:
	typedef void(*ErrFunc)(const char*);
	static ErrFunc GetErrorFunc(ErrFunc errFunc, bool acceptNull = true)
	{
		static ErrFunc m_errFunc;
		if (errFunc || acceptNull)
			m_errFunc = errFunc;
		return m_errFunc;
	}

	typedef void(*RequireFunc)(LuaState&, const char*);
	static RequireFunc GetRequireFunc(RequireFunc requireFunc, bool acceptNull = true)
	{
		static RequireFunc m_requireFunc;
		if (requireFunc || acceptNull)
			m_requireFunc = requireFunc;
		return m_requireFunc;
	}

	static int Require(lua_State* L)
	{
		LuaState lua(L);
		const char* s{};
		lua.GetValue(lua_Idx(-1), &s);
		bool b{};
		lua.GetValue(lua_Idx(LUA_REGISTRYINDEX), LUA_LOADED_TABLE, s, &b);
		if (!b)
			GetRequireFunc(nullptr, false)(lua, s);
		return 0;
	}

	static int ErrorFunc(lua_State* L)
	{
		ErrFunc errFunc = GetErrorFunc(nullptr, false);
		LuaState lua(L);
		const char* s{};
		lua.GetValue(lua_Idx(-1), &s);
		errFunc(s);
		return 0;
	}

	lua_State* m_lua;
	bool m_closeOnDtor;
};