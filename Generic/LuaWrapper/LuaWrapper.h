#pragma once
#include "LuaState.h"
#include <vector>
#include <typeinfo>
#include <array>

#define Lua_set_cobj(obj) LuaSub(0, (obj)->LuaClassObj(obj)), (obj)->LuaGetMemberFuncs()

struct LuaIdx : public lua_Idx
{
	LuaIdx(const LuaState& s, int idx) : lua_Idx(idx), state(s.Lua()) {}
	LuaIdx(lua_State* L, int idx) : lua_Idx(idx), state(L) {}
	LuaIdx(LuaIdx&& i) : lua_Idx(i.idx), state(i.state.Lua()) {}
	LuaState state;
};

template<typename T>
struct LuacObj
{
	LuacObj(T* p = {}) : ptr(p) {}
	operator T* () { return ptr; }
	template<typename T1>
	operator T1 () { return (T1)ptr; }
	T* operator -> () { return ptr; }
	T* ptr;
	typedef T* cObj;
};

template<class T>
struct LuacObjNew
{
	LuacObjNew(T* obj) : object(obj) {}
	T* object;
};

#define Lua_cf(func) [](lua_State* L){ return LuaCallCFunc(L, &func);}

#define Lua_mf(func) {#func, [](lua_State* L){ return LuaCallCFunc(L, &func);}}

#define Lua_smf(func) {#func, [](lua_State* L){ return LuaCallCFunc(L, func);}}

#define Lua_cpp_class_base_def(cpp_class) \
typedef cpp_class class_type; \
template<typename T> \
static cpp_class* LuaClassObj(T p) \
{ \
	return (cpp_class*)p; \
}\
static const char* LuaGetName() \
{ \
	return #cpp_class; \
}

#define Lua_abstract \
static lua_CFunction LuaGetObjectCtor() { return {}; }

#define Lua_ctor(...) \
static lua_CFunction LuaGetObjectCtor() { return LuaObjectCtor; } \
static int LuaObjectCtor(lua_State *L) \
{ \
	static const size_t size = std::tuple_size<std::tuple<__VA_ARGS__>>::value; \
	LuaState lua(L); \
	int t = lua.GetTop() - size; \
	lua_Idx idxIn(t++); \
	class_type* c = LuaCallConstructor<class_type, __VA_ARGS__>(L, t, std::make_index_sequence<size>()); \
	lua_pushnil(L); \
	lua_Idx idxOut(lua.GetTop()); \
	lua.SetValue(idxOut, 0, c); \
	lua.GetValue(idxIn, LuaMeta(), "class", 0, LuaSetTo(idxOut, 1)); \
	lua.GetValue(idxIn, LuaMeta(), "class", LuaSetTo(idxOut, LuaMeta())); \
	return 1; \
}

#define Lua_wrap_cpp_class(cpp_class, ctor, ...) \
Lua_cpp_class_base_def(cpp_class) \
ctor \
void LuaCheckBase(){} \
static const char* LuaGetBaseName() \
{ \
	return {}; \
} \
static const luaL_Reg* LuaGetMemberFuncs(size_t* n = {}) \
{ \
	static std::vector<luaL_Reg> regOut = { __VA_ARGS__ }; \
	static bool loaded = false; \
	if (!loaded) \
	{ \
		regOut.push_back({}); \
		loaded = true; \
	} \
	if (n) *n = regOut.size() - 1; \
	return regOut.data(); \
}

#define Lua_wrap_cpp_class_derived(base_class, cpp_class, ctor, ...) \
Lua_cpp_class_base_def(cpp_class) \
ctor \
void LuaCheckBase(){ base_class::LuaCheckBase(); } \
static const char* LuaGetBaseName() \
{ \
	return #base_class; \
} \
static const luaL_Reg* LuaGetMemberFuncs(size_t* n = {}) \
{ \
	static std::vector<luaL_Reg> regOut = { __VA_ARGS__ }; \
	static bool loaded = false; \
	if (!loaded) \
	{ \
		size_t n = 0; \
		const luaL_Reg* regBase = base_class::LuaGetMemberFuncs(&n); \
		for (size_t i = 0; i < n; i++) \
			regOut.insert(regOut.begin(), regBase[i]); \
		\
		regOut.push_back({}); \
		loaded = true; \
	} \
	if (n) *n = regOut.size() - 1; \
	return regOut.data(); \
}

template<typename T = void>
inline T* LuaGetCppObj(lua_State* lua, int i)
{
	int n = lua_gettop(lua);
	Assert(abs(i) <= lua_gettop(lua));
	Assert(lua_type(lua, i) == LUA_TTABLE);
	
	lua_pushinteger(lua, 0);
	lua_gettable(lua, i);
	Assert(lua_type(lua, -1) == LUA_TLIGHTUSERDATA);
	
	void* p{};
	p = lua_touserdata(lua, -1);
	lua_pop(lua, 1);
	return (T*)p;
}

template<typename T>
inline int LuaPushRetValue(lua_State *L, LuacObjNew<T>& t)
{
	lua_pushnil(L);
	if (t.object)
	{
		LuaIdx idxOut(L, lua_gettop(L));
		LuaSetCppObjRegistered(t.object, idxOut.state, idxOut);
	}
	return 1;
}

template<typename T>
inline int LuaPushRetValue(lua_State *L, const LuacObjNew<T>& t)
{
	lua_pushnil(L);
	if (t.object)
	{
		LuaIdx idxOut(L, lua_gettop(L));
		idxOut.state.SetValue(idxOut, 0, t.object);
		idxOut.state.GetValue(T::LuaGetName(), LuaMeta(), "class", LuaSetTo(idxOut, LuaMeta()));
	}
	return 1;
}

template<typename T>
inline int LuaPushRetValue(lua_State *L, T t)
{
	LuaPushValue(L, t);
	return 1;
}

template<typename ...T>
inline int LuaPushRetValue(lua_State *L, const std::tuple<T...>& t)
{
	LuaPushRetValue(L, t._Myfirst._Val);
	LuaPushRetValue(L, t._Get_rest());
	return std::tuple_size<std::tuple<T...>>::value;
}

inline void LuaPushRetValue(lua_State *L, const std::tuple<>&) {}

template<typename T>
inline typename std::enable_if<std::is_same<LuaIdx, T>::value, T>::type LuaGetValue(lua_State* L, int i)
{
	return LuaIdx(L, i);
}

template<class T>
inline typename T::cObj LuaGetValue(lua_State* L, int i)
{
	return (typename T::cObj)LuaGetCppObj(L, i);
}

template<typename ...T, size_t... I>
inline int LuaCallCFunc(lua_State *L, void(*f)(T...), int t, std::index_sequence<I...>)
{
	f(LuaGetValue<T>(L, t + I)...);
	return 0;
}

template<typename R, typename ...T, size_t... I>
inline int LuaCallCFunc(lua_State *L, R(*f)(T...), int t, std::index_sequence<I...>)
{
	return LuaPushRetValue(L, f(LuaGetValue<T>(L, t + I)...));
}

template<typename R, typename ...T>
inline int LuaCallCFunc(lua_State *L, R(*f)(T...))
{
	const int n = sizeof ...(T);
	return LuaCallCFunc(L, f, lua_gettop(L) - n + 1, std::make_index_sequence<n>());
}

template<class C, typename ...T, size_t... I>
static int LuaCallCFunc(lua_State *L, C* c, void(C::*f)(T...), int t, std::index_sequence<I...>)
{
	(c->*f)(LuaGetValue<T>(L, t + I)...);
	return 0;
}

template<class C, typename R, typename ...T, size_t... I>
static int LuaCallCFunc(lua_State *L, C* c, R(C::*f)(T...), int t, std::index_sequence<I...>)
{
	return LuaPushRetValue(L, (c->*f)(LuaGetValue<T>(L, t + I)...));
}

template<class C, typename R, typename ...T>
static int LuaCallCFunc(lua_State *L, R(C::*f)(T...))
{
	const int n = sizeof ...(T);
	int t = lua_gettop(L) - n;
	C* c = LuaGetCppObj<C>(L, t++);
	assert(c);
	return LuaCallCFunc(L, c, f, t, std::make_index_sequence<n>());
}

template<class C, typename ...T, size_t... I>
static C* LuaCallConstructor(lua_State *L, int t, std::index_sequence<I...>)
{
	return new C(LuaGetValue<T>(L, t + I)...);
}

template<typename T0, class ...T1>
inline void LuaSetCppObjRegistered(T0* c, const LuaState& lua, const T1& ...t)
{
	lua.SetValue(t..., 0, c);
	lua.SetValue(t..., 1, typeid(T0).hash_code());
	lua.GetValue(T0::LuaGetName(), LuaMeta(), "class", LuaSetTo(t..., LuaMeta()));
}

template<class T>
static int LuaObjectDtor(lua_State *L)
{
	void* p{};
	LuaState(L).GetValue(lua_Idx(1), 0, &p);
	delete (T*)p;
	return 0;
}

inline void LuaRegisterCppClass(LuaState& lua, const char* name, const char* baseName, 
	lua_CFunction objectCtor, lua_CFunction objectDtor, const luaL_Reg* memberFuncs)
{
	if (baseName)
		lua.GetValue(baseName, LuaSetTo(name, "__base"), LuaSetTo(name, LuaMeta(), "__index"));
	if (objectCtor)
		lua.SetValue(name, LuaMeta(), "__call", objectCtor);
	lua.SetValue(name, memberFuncs);
	lua.SetValue(name, LuaMeta(), "class", "__gc", objectDtor);
	lua.GetValue(name, LuaSetTo(name, LuaMeta(), "class", "__index"));
}

template<class T>
inline void LuaRegisterCppClass(LuaState& lua)
{
	LuaRegisterCppClass(lua, T::LuaGetName(), T::LuaGetBaseName(), T::LuaGetObjectCtor(), LuaObjectDtor<T>, T::LuaGetMemberFuncs());
}
