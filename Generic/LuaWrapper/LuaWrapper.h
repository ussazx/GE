//***************************************************************************************
// Effects.h by ussa (C) 2026 All Rights Reserved.
// Licensed under the MIT License.
//***************************************************************************************

#pragma once
#include "LuaState.h"
#include <vector>
#include <typeinfo>

#define Lua_set_cobj(obj) LuaObjCustomSet<std::remove_reference<decltype(*(obj))>::type> \
	(*obj, &std::remove_reference<decltype(*(obj))>::type::LuaSetObj), (LuaCustomSet)(obj)->LuaSetClassTable

template<typename ...T>
inline LuaCustomSet LuaDataSet(T&& ...t)
{
	return[t...](const LuaState& state, const lua_Idx& idx)
	{
		state.SetValue(idx, t...);
	};
}

struct LuaIdx : public lua_Idx
{
	LuaIdx(const LuaState& s, int idx) : lua_Idx(idx), state(s.Lua()) {}
	LuaIdx(LuaIdx&& i) : lua_Idx(i.idx), state(i.state.Lua()) {}
	template<typename ...T>
	void SetValue(const T&... t) const
	{
		state.SetValue(Idx(), t...);
	}
	template<typename ...T>
	int GetValue(T... t) const
	{
		return state.GetValue(Idx(), t...);
	}
	template<typename T = void>
	inline T* GetCppObj() const
	{
		auto lua = state.Lua();
		int n = lua_gettop(lua);
		Assert(abs(idx) <= lua_gettop(lua));

		Assert(lua_type(lua, idx) == LUA_TTABLE);
		static const auto id = typeid(T).hash_code();
		lua_pushinteger(lua, id);
		lua_gettable(lua, Lua_I(idx, 1));
		Assert(lua_type(lua, -1) == LUA_TLIGHTUSERDATA);

		void* p{};
		p = lua_touserdata(lua, -1);
		lua_pop(lua, 1);
		return (T*)p;
	}
	lua_Idx& Idx()
	{
		return *this;
	}
	const lua_Idx& Idx() const
	{
		return *this;
	}
private:
	LuaState state;
};

template<typename T>
struct LuacObj
{
	LuacObj(T* p = {}) : ptr(p) {}
	operator T* () { return ptr; }
	template<typename T1>
	operator T1 () { return (T1)ptr; }
	T& operator * () { return *ptr; }
	T* operator -> () { return ptr; }
	T* ptr;
	typedef T cObj;
};

template<typename T>
struct LuaCustomParam
{
	static T GetValue(const LuaIdx&);
	typedef T LuaCustomType;
};

template<class C>
struct LuacObjNew
{
	LuacObjNew(C* obj) : object(obj)
	{
		if (obj)
			set = LuaDataSet(LuaObjCustomSet<typename std::remove_reference<decltype(*(obj))>::type>
			(*obj, &std::remove_reference<decltype(*(obj))>::type::LuaSetObj), LuaSub(LuaMeta(), LuaGet(C::LuaGetName(), "_class")));
		else
			set = LuaDataSet(nullptr);
	}
	template<typename ...T>
	LuacObjNew(C* obj, T&&... t) : object(obj)
	{
		if (obj)
			set = LuaDataSet(LuaObjCustomSet<typename std::remove_reference<decltype(*(obj))>::type>
			(*obj, &std::remove_reference<decltype(*(obj))>::type::LuaSetObj), LuaSub(LuaMeta(), LuaGet(C::LuaGetName(), "_class")),
				LuaSub(std::forward<T>(t)...));
		else
			set = LuaDataSet(nullptr);
	}
	C* operator -> () { return object; }
	C* object;
	LuaCustomSet set;
};

#define Lua_cf(func) [](lua_State* L){ return LuaCallCFunc(L, func);}

#define Lua_mf(func) LuaSub(#func, [](lua_State* L){ return LuaCallCFunc<class_type>(L, &class_type::func);})

#define Lua_mt_mf(name, func) LuaSub("_class", name, [](lua_State* L){ return LuaCallCFunc<class_type>(L, &class_type::func);})

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
}\
static void LuaSetClassTable(const LuaState&, const lua_Idx&, const std::tuple<>&){} \
template<typename T> \
static void LuaSetClassTable(const LuaState& s, const lua_Idx& idx, const std::tuple<T>& t) \
{\
	s.SetValue(idx, std::get<0>(t)); \
}\
template<typename ...T> \
static void LuaSetClassTable(const LuaState& s, const lua_Idx& idx, const std::tuple<T...>& t) \
{\
	s.SetValue(idx, t); \
}

#define Lua_abstract \
static lua_CFunction LuaGetObjectCtor() { return {}; }

#define Lua_ctor_void \
static lua_CFunction LuaGetObjectCtor() { return LuaObjectCtor; } \
static int LuaObjectCtor(lua_State *L) \
{ \
	LuaState lua(L); \
	int t = lua.GetTop(); \
	lua_Idx idxIn(t++); \
	class_type* c = LuaCallConstructor<class_type>(L, t, std::make_index_sequence<0>()); \
	lua_pushnil(L); \
	lua_Idx idxOut(lua.GetTop()); \
	c->LuaSetObj(lua, idxOut); \
	lua.SetValue(idxOut, LuaMeta(), LuaGet(idxIn, "_class")); \
	return 1; \
}

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
	c->LuaSetObj(lua, idxOut); \
	lua.SetValue(idxOut, LuaMeta(), LuaGet(idxIn, "_class")); \
	return 1; \
}

#define Lua_wrap_cpp_class(cpp_class, ctor, ...) \
Lua_cpp_class_base_def(cpp_class) \
ctor \
void LuaSetObj(const LuaState& s, const lua_Idx& idx) \
{ \
	static const auto id = typeid(cpp_class).hash_code(); \
	s.SetValue(idx, id, this); \
	s.SetValue(idx, #cpp_class, this); \
} \
static const char* LuaGetBaseName() \
{ \
	return {}; \
} \
static void LuaSetClassTable(const LuaState& s, const lua_Idx& idx) \
{\
	LuaSetClassTable(s, idx, LuaSub(__VA_ARGS__)); \
}

#define Lua_wrap_cpp_class_derived(base_class, cpp_class, ctor, ...) \
Lua_cpp_class_base_def(cpp_class) \
ctor \
void LuaSetObj(const LuaState& s, const lua_Idx& idx) \
{ \
	base_class::LuaSetObj(s, idx); \
	static const auto id = typeid(cpp_class).hash_code(); \
	s.SetValue(idx, id, this); \
	s.SetValue(idx, #cpp_class, this); \
} \
static const char* LuaGetBaseName() \
{ \
	return #base_class; \
} \
static void LuaSetClassTable(const LuaState& s, const lua_Idx& idx) \
{\
	base_class::LuaSetClassTable(s, idx); \
	LuaSetClassTable(s, idx, LuaSub(__VA_ARGS__)); \
}

template<class T = void>
inline T* LuaGetCppObj(lua_State* lua, int i)
{
	int n = lua_gettop(lua);
	Assert(abs(i) <= lua_gettop(lua));

	Assert(lua_type(lua, i) == LUA_TTABLE);
	static const auto id = typeid(T).hash_code();
	lua_pushinteger(lua, id);
	lua_gettable(lua, Lua_I(i, 1));
	Assert(lua_type(lua, -1) == LUA_TLIGHTUSERDATA);

	void* p{};
	p = lua_touserdata(lua, -1);
	lua_pop(lua, 1);
	return (T*)p;
}

template<typename T = void>
inline T* LuaGetCppObj(const LuaIdx& idx)
{
	static const auto id = typeid(T).hash_code();
	void* p{};
	idx.GetValue(id, &p);
	return (T*)p;
}

struct LuaReturn
{
	LuaReturn(const LuaState& s) : state(s.Lua()), n(0) {}

	template<typename ...T>
	void Push(const T&... t)
	{
		n++;
		lua_pushnil(state.Lua());
		state.SetValue(lua_Idx(-1), t...);
	}

	size_t Count()
	{
		return n;
	}
private:
	LuaState state;
	size_t n;
};

template<class C>
inline int LuaPushRetValue(lua_State *L, const LuacObjNew<C>& obj)
{
	return LuaPushRetValue(L, obj.set);
}

inline int LuaPushRetValue(lua_State *L, const LuaCustomSet& cs)
{
	lua_pushnil(L);
	cs(L, lua_Idx(lua_gettop(L)));
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
	Assert(abs(i) <= lua_gettop(L));
	return LuaIdx(L, i);
}

template<class T>
inline typename T::cObj* LuaGetValue(lua_State* L, int i)
{
	Assert(abs(i) <= lua_gettop(L));
	return LuaGetCppObj<T::cObj>(L, i);
}

template<class T>
inline typename T::LuaCustomType LuaGetValue(lua_State* L, int i)
{
	Assert(abs(i) <= lua_gettop(L));
	return T::GetValue(LuaIdx(L, i));
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

template<class C = void, typename R, typename ...T>
inline int LuaCallCFunc(lua_State *L, R(*f)(T...))
{
	const int n = sizeof ...(T);
	return LuaCallCFunc(L, f, lua_gettop(L) - n + 1, std::make_index_sequence<n>());
}

template<typename ...T, size_t... I>
inline int LuaCallCFunc(lua_State *L, void(*f)(LuaReturn&, T...), int t, std::index_sequence<I...>)
{
	LuaReturn ret(L);
	f(ret, LuaGetValue<T>(L, t + I)...);
	return ret.Count();
}

template<class C = void, typename ...T>
inline int LuaCallCFunc(lua_State *L, void(*f)(LuaReturn&, T...))
{
	const int n = sizeof ...(T);
	return LuaCallCFunc(L, f, lua_gettop(L) - n + 1, std::make_index_sequence<n>());
}

template<class C0, class C1, typename ...T, size_t... I>
inline int LuaCallCFunc(lua_State *L, C0* c, void(C1::*f)(T...), int t, std::index_sequence<I...>)
{
	(c->*f)(LuaGetValue<T>(L, t + I)...);
	return 0;
}

template<class C0, class C1, typename ...T, size_t... I>
inline int LuaCallCFunc(lua_State *L, C0* c, void(C1::*f)(T...)const, int t, std::index_sequence<I...>)
{
	(c->*f)(LuaGetValue<T>(L, t + I)...);
	return 0;
}

template<class C0, class C1, typename R, typename ...T, size_t... I>
inline int LuaCallCFunc(lua_State *L, C0* c, R(C1::*f)(T...), int t, std::index_sequence<I...>)
{
	return LuaPushRetValue(L, (c->*f)(LuaGetValue<T>(L, t + I)...));
}

template<class C0, class C1, typename R, typename ...T, size_t... I>
inline int LuaCallCFunc(lua_State *L, C0* c, R(C1::*f)(T...)const, int t, std::index_sequence<I...>)
{
	return LuaPushRetValue(L, (c->*f)(LuaGetValue<T>(L, t + I)...));
}

template<class C0, class C1, typename R, typename ...T>
inline int LuaCallCFunc(lua_State *L, R(C1::*f)(T...))
{
	const int n = sizeof ...(T);
	int t = lua_gettop(L) - n;
	C0* c = LuaGetCppObj<C0>(L, t++);
	assert(c);
	return LuaCallCFunc(L, c, f, t, std::make_index_sequence<n>());
}

template<class C0, class C1, typename R, typename ...T>
inline int LuaCallCFunc(lua_State *L, R(C1::*f)(T...)const)
{
	const int n = sizeof ...(T);
	int t = lua_gettop(L) - n;
	C0* c = LuaGetCppObj<C0>(L, t++);
	assert(c);
	return LuaCallCFunc(L, c, f, t, std::make_index_sequence<n>());
}

template<class C0, class C1, typename ...T, size_t... I>
inline int LuaCallCFunc(lua_State *L, C0* c, void(C1::*f)(LuaReturn&, T...), int t, std::index_sequence<I...>)
{
	LuaReturn ret(L);
	(c->*f)(ret, LuaGetValue<T>(L, t + I)...);
	return ret.Count();
}

template<class C0, class C1, typename ...T, size_t... I>
inline int LuaCallCFunc(lua_State *L, C0* c, void(C1::*f)(LuaReturn&, T...)const, int t, std::index_sequence<I...>)
{
	LuaReturn ret(L);
	(c->*f)(ret, LuaGetValue<T>(L, t + I)...);
	return ret.Count();
}

template<class C0, class C1, typename ...T>
inline int LuaCallCFunc(lua_State *L, void(C1::*f)(LuaReturn&, T...))
{
	const int n = sizeof ...(T);
	int t = lua_gettop(L) - n;
	C0* c = LuaGetCppObj<C0>(L, t++);
	assert(c);
	return LuaCallCFunc(L, c, f, t, std::make_index_sequence<n>());
}

template<class C0, class C1, typename ...T>
inline int LuaCallCFunc(lua_State *L, void(C1::*f)(LuaReturn&, T...)const)
{
	const int n = sizeof ...(T);
	int t = lua_gettop(L) - n;
	C0* c = LuaGetCppObj<C0>(L, t++);
	assert(c);
	return LuaCallCFunc(L, c, f, t, std::make_index_sequence<n>());
}

template<class C, typename ...T, size_t... I>
inline C* LuaCallConstructor(lua_State *L, int t, std::index_sequence<I...>)
{
	return new C(LuaGetValue<T>(L, t + I)...);
}

template<class T>
inline int LuaObjectDtor(lua_State *L)
{
	void* p{};
	static const auto id = typeid(T).hash_code();
	LuaState(L).GetValue(lua_Idx(1), id, &p);
	delete (T*)p;
	return 0;
}

inline void LuaRegisterCppClass(LuaState& lua, const char* name, const char* baseName, size_t size,
	lua_CFunction objectCtor, lua_CFunction objectDtor, size_t id, const LuaCustomSet& setClassTable)
{
	if (baseName)
		lua.GetValue(baseName, LuaSetTo(name, "_base"), LuaSetTo(name, "_class", "__index"));
	if (objectCtor)
		lua.SetValue(name, LuaMeta(), "__call", objectCtor);
	lua.SetValue(name, setClassTable);
	lua.SetValue(name, "_size", size);
	lua.SetValue(name, "_class", "__gc", objectDtor);
	lua.SetValue(name, "_class", "__index", LuaGet(name));
	lua.SetValue(name, LuaGet(name), id);
}

template<class T>
inline void LuaRegisterCppClass(lua_State *L)
{
	LuaState lua(L);
	LuaRegisterCppClass(lua, T::LuaGetName(), T::LuaGetBaseName(), sizeof(T),
		T::LuaGetObjectCtor(), LuaObjectDtor<T>, typeid(T).hash_code(), T::LuaSetClassTable);
}
