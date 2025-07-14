#pragma once
#include "LuaState.h"
#include <vector>
#include <typeinfo>

#define Lua_set_cobj(obj) LuaSub(0, (obj)->LuaClassObj(obj)), LuaSub(1, typeid(decltype(obj)).hash_code()), (LuaCustomSet)(obj)->LuaSetClassTable

struct LuaIdx : public lua_Idx
{
	LuaIdx(const LuaState& s, int idx) : lua_Idx(idx), state(s.Lua()) {}
	LuaIdx(LuaIdx&& i) : lua_Idx(i.idx), state(i.state.Lua()) {}
	template<typename ...T>
	void SetValue(const T&... t) const
	{
		state.SetValue(*this, t...);
	}
	template<typename ...T>
	void GetValue(T... t) const
	{
		state.GetValue(*this, t...);
	}
	int Type() const
	{
		return lua_type(state.Lua(), idx);
	}
	template<typename T = void>
	inline T* GetCppObj() const
	{
		auto lua = state.Lua();
		int n = lua_gettop(lua);
		Assert(abs(idx) <= lua_gettop(lua));

		lua_pushinteger(lua, 0);
		lua_gettable(lua, idx);
		Assert(lua_type(lua, -1) == LUA_TLIGHTUSERDATA);

		void* p{};
		p = lua_touserdata(lua, -1);
		lua_pop(lua, 1);
		return (T*)p;
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
			set = LuaDataSet(LuaSub(0, obj), LuaSub(1, typeid(C*).hash_code()), LuaSub(LuaMeta(), LuaGet(C::LuaGetName(), "_class")));
		else
			set = LuaDataSet(nullptr);
	}
	template<typename ...T>
	LuacObjNew(C* obj, T&&... t) : object(obj)
	{
		if (obj)
			set = LuaDataSet(LuaSub(0, obj), LuaSub(1, typeid(C*).hash_code()), LuaSub(LuaMeta(), LuaGet(C::LuaGetName(), "_class")), LuaSub(std::forward<T>(t)...));
		else
			set = LuaDataSet(nullptr);
	}
	C* operator -> () { return object; }
	C* object;
	LuaCustomSet set;
};

#define Lua_cf(func) [](lua_State* L){ return LuaCallCFunc(L, func);}

#define Lua_mf(func) LuaSub(#func, [](lua_State* L){ return LuaCallCFunc(L, &func);})

#define Lua_mt_mf(name, func) LuaSub("_class", name, Lua_cf(func))

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
	lua.SetValue(idxOut, LuaSub(0, c), LuaSub(1, typeid(class_type).hash_code())); \
	lua.GetValue(idxIn, "_class", LuaSetTo(idxOut, LuaMeta())); \
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
static void LuaSetClassTable(const LuaState& s, const lua_Idx& idx) \
{\
	LuaSetClassTable(s, idx, LuaSub(__VA_ARGS__)); \
}

#define Lua_wrap_cpp_class_derived(base_class, cpp_class, ctor, ...) \
Lua_cpp_class_base_def(cpp_class) \
ctor \
void LuaCheckBase(){ base_class::LuaCheckBase(); } \
static const char* LuaGetBaseName() \
{ \
	return #base_class; \
} \
static void LuaSetClassTable(const LuaState& s, const lua_Idx& idx) \
{\
	base_class::LuaSetClassTable(s, idx); \
	LuaSetClassTable(s, idx, LuaSub(__VA_ARGS__)); \
}

template<typename T = void>
inline T* LuaGetCppObj(lua_State* lua, int i)
{
	int n = lua_gettop(lua);
	Assert(abs(i) <= lua_gettop(lua));
	
	lua_pushinteger(lua, 0);
	lua_gettable(lua, i);
	Assert(lua_type(lua, -1) == LUA_TLIGHTUSERDATA);

	void* p{};
	p = lua_touserdata(lua, -1);
	lua_pop(lua, 1);
	return (T*)p;
}

struct LuaReturn
{
	LuaReturn(const LuaState& s) : state(s.Lua()), n(0) {}
	
	template<typename T>
	void Push(const T& t)
	{
		n++;
		LuaPushValue(state.Lua(), t);
	}
	
	template<typename T0, typename ...T1>
	void Push(const T0& t0, const T1&... t1)
	{
		n++;
		lua_pushnil(state.Lua());
		state.SetValue(lua_Idx(-1), t1...);
	}
	
	template<typename ...T>
	void Push(const LuaGetArg<T...>& t)
	{
		n++;
		lua_pushnil(state.Lua());
		state.SetValue(lua_Idx(-1), t);
	}

	void Push(const LuaLoad& t)
	{
		n++;
		lua_pushnil(state.Lua());
		state.SetValue(lua_Idx(-1), t);
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

template<typename R, typename ...T>
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

template<typename ...T>
inline int LuaCallCFunc(lua_State *L, void(*f)(LuaReturn&, T...))
{
	const int n = sizeof ...(T);
	return LuaCallCFunc(L, f, lua_gettop(L) - n + 1, std::make_index_sequence<n>());
}

template<class C, typename ...T, size_t... I>
inline int LuaCallCFunc(lua_State *L, C* c, void(C::*f)(T...), int t, std::index_sequence<I...>)
{
	(c->*f)(LuaGetValue<T>(L, t + I)...);
	return 0;
}

template<class C, typename ...T, size_t... I>
inline int LuaCallCFunc(lua_State *L, C* c, void(C::*f)(T...)const, int t, std::index_sequence<I...>)
{
	(c->*f)(LuaGetValue<T>(L, t + I)...);
	return 0;
}

template<class C, typename R, typename ...T, size_t... I>
inline int LuaCallCFunc(lua_State *L, C* c, R(C::*f)(T...), int t, std::index_sequence<I...>)
{
	return LuaPushRetValue(L, (c->*f)(LuaGetValue<T>(L, t + I)...));
}

template<class C, typename R, typename ...T, size_t... I>
inline int LuaCallCFunc(lua_State *L, C* c, R(C::*f)(T...)const, int t, std::index_sequence<I...>)
{
	return LuaPushRetValue(L, (c->*f)(LuaGetValue<T>(L, t + I)...));
}

template<class C, typename R, typename ...T>
inline int LuaCallCFunc(lua_State *L, R(C::*f)(T...))
{
	const int n = sizeof ...(T);
	int t = lua_gettop(L) - n;
	C* c = LuaGetCppObj<C>(L, t++);
	assert(c);
	return LuaCallCFunc(L, c, f, t, std::make_index_sequence<n>());
}

template<class C, typename R, typename ...T>
inline int LuaCallCFunc(lua_State *L, R(C::*f)(T...)const)
{
	const int n = sizeof ...(T);
	int t = lua_gettop(L) - n;
	C* c = LuaGetCppObj<C>(L, t++);
	assert(c);
	return LuaCallCFunc(L, c, f, t, std::make_index_sequence<n>());
}

template<class C, typename ...T, size_t... I>
inline int LuaCallCFunc(lua_State *L, void(C::*f)(LuaReturn&, T...), int t, std::index_sequence<I...>)
{
	LuaReturn ret(L);
	f(ret, LuaGetValue<T>(L, t + I)...);
	return ret.Count();
}

template<class C, typename ...T, size_t... I>
inline int LuaCallCFunc(lua_State *L, void(C::*f)(LuaReturn&, T...)const, int t, std::index_sequence<I...>)
{
	LuaReturn ret(L);
	f(ret, LuaGetValue<T>(L, t + I)...);
	return ret.Count();
}

template<class C, typename ...T>
inline int LuaCallCFunc(lua_State *L, void(C::*f)(LuaReturn&, T...))
{
	const int n = sizeof ...(T);
	return LuaCallCFunc(L, f, lua_gettop(L) - n + 1, std::make_index_sequence<n>());
}

template<class C, typename ...T>
inline int LuaCallCFunc(lua_State *L, void(C::*f)(LuaReturn&, T...)const)
{
	const int n = sizeof ...(T);
	return LuaCallCFunc(L, f, lua_gettop(L) - n + 1, std::make_index_sequence<n>());
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
	LuaState(L).GetValue(lua_Idx(1), 0, &p);
	delete (T*)p;
	return 0;
}

inline void LuaRegisterCppClass(LuaState& lua, const char* name, const char* baseName, size_t size,
	lua_CFunction objectCtor, lua_CFunction objectDtor, const LuaCustomSet& setClassTable)
{
	if (baseName)
		lua.GetValue(baseName, LuaSetTo(name, "_base"), LuaSetTo(name, "_class", "__index"));
	if (objectCtor)
		lua.SetValue(name, LuaMeta(), "__call", objectCtor);
	lua.SetValue(name, setClassTable);
	lua.SetValue(name, "_size", size);
	lua.SetValue(name, "_class", "__gc", objectDtor);
	lua.GetValue(name, LuaSetTo(name, "_class", "__index"));
}

template<class T>
inline void LuaRegisterCppClass(LuaState& lua)
{
	LuaRegisterCppClass(lua, T::LuaGetName(), T::LuaGetBaseName(), sizeof(T), T::LuaGetObjectCtor(), LuaObjectDtor<T>, T::LuaSetClassTable);
}
