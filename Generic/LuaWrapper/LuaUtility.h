//***************************************************************************************
// Effects.h by ussa (C) 2026 All Rights Reserved.
// Licensed under the MIT License.
//***************************************************************************************

#include "LuaGlobalReflect.h"
#include <string>
#include <codecvt>
#include <locale>
#include <algorithm>

class LString : public LuaCustomParam<LString>
{
public:
	LString()
	{
		m_text = std::make_shared<std::wstring>();
	}
	LString(LuaIdx idx)
	{
		const char* utf8{};
		if (idx.GetValue(&utf8) == LUA_TSTRING)
		{
			m_cvt = std::make_shared<Cvt>();
			m_text = std::make_shared<std::wstring>(m_cvt->cvt.from_bytes(utf8));
		}
		else
		{
			LString* s = idx.GetCppObj<LString>();
			m_text = s->m_text;
			m_cvt = s->m_cvt;
		}
	}
	LString(const wchar_t* s = L"")
	{
		m_text = std::make_shared<std::wstring>(s);
	}
	LString(const char* utf8)
	{
		m_cvt = std::make_shared<Cvt>();
		m_text = std::make_shared<std::wstring>(m_cvt->cvt.from_bytes(utf8));
	}
	LString(const LString& s)
	{
		m_text = s.m_text;
	}
	LString(LString&& s)
	{
		m_text = s.m_text;
		m_cvt = s.m_cvt;
	}
	LString(const std::string& utf8)
	{
		m_cvt = std::make_shared<Cvt>();
		m_text = std::make_shared<std::wstring>(m_cvt->cvt.from_bytes(utf8));
	}
	LString(const std::wstring& s)
	{
		m_text = std::make_shared<std::wstring>(s);
	}
	LString(std::wstring&& s) : m_text(std::make_shared<std::wstring>(std::forward<std::wstring>(s))) {}

	const LString& operator = (const char* utf8)
	{
		m_cvt = std::make_shared<Cvt>();
		m_text = std::make_shared<std::wstring>(m_cvt->cvt.from_bytes(utf8));
		return *this;
	}
	const LString& operator = (const wchar_t* s)
	{
		m_text = std::make_shared<std::wstring>(s);
		return *this;
	}
	const LString& operator = (const std::string& utf8)
	{
		m_cvt = std::make_shared<Cvt>();
		m_text = std::make_shared<std::wstring>(m_cvt->cvt.from_bytes(utf8));
		return *this;
	}
	const LString& operator = (const std::wstring& s)
	{
		m_text = std::make_shared<std::wstring>(s);
		return *this;
	}
	const LString& operator = (std::wstring&& s)
	{
		m_text = std::make_shared<std::wstring>(std::forward<std::wstring>(s));
		return *this;
	}
	const LString& operator = (const LString& s)
	{
		m_text = s.m_text;
		return *this;
	}
	const LString& operator = (LString&& s)
	{
		m_text = s.m_text;
		if (m_cvt == nullptr)
			m_cvt = s.m_cvt;
		return *this;
	}
	LString operator + (const LString& s) const
	{
		return LString(*m_text + *s.m_text);
	}
	const LString& operator += (const LString& s)
	{
		if (m_text.use_count() > 1)
			m_text = std::make_shared<std::wstring>(*m_text + *s.m_text);
		else
			*m_text += *s.m_text;
		return *this;
	}
	operator const std::wstring& () const
	{
		return *m_text;
	}
	operator const wchar_t* () const
	{
		return m_text->c_str();
	}
	operator const std::string& ()
	{
		if (m_cvt == nullptr)
			m_cvt = std::make_shared<Cvt>();
		m_cvt->utf8 = m_cvt->cvt.to_bytes(*m_text);
		return m_cvt->utf8;
	}
	operator const char* ()
	{
		if (m_cvt == nullptr)
			m_cvt = std::make_shared<Cvt>();
		m_cvt->utf8 = m_cvt->cvt.to_bytes(*m_text);
		return m_cvt->utf8.c_str();
	}

	const char* ch(size_t n)
	{
		if (m_cvt == nullptr)
			m_cvt = std::make_shared<Cvt>();
		m_cvt->utf8 = m_cvt->cvt.to_bytes(m_text->substr(n, 1));
		return m_cvt->utf8.c_str();
	}

	size_t length() const
	{
		return m_text->length();
	}

	void set(LString s)
	{
		if (m_text.use_count() > 1)
			m_text = std::make_shared<std::wstring>(*s.m_text);
		else
			m_text = s.m_text;
		if (m_cvt == nullptr and s.m_cvt != nullptr)
			m_cvt = s.m_cvt;
	}
	const wchar_t* c_str() const
	{
		return m_text->c_str();
	}
	const char* utf8()
	{
		return *this;
	}

	bool same(LString s)
	{
		return *m_text == *s.m_text;
	}

	bool ncsame(LString s)
	{
		const std::wstring& str1 = *m_text;
		const std::wstring& str2 = *s.m_text;
		if (str1.size() != str2.size()) 
			return false;
		return std::equal(str1.begin(), str1.end(), str2.begin(),
			[&](wchar_t c1, wchar_t c2) { return std::tolower(c1, std::locale()) == std::tolower(c2, std::locale()); });
	}

	const char* lower_utf8()
	{
		std::wstring s = *m_text;
		std::transform(s.begin(), s.end(), s.begin(), [&](wchar_t c) {return std::tolower(c, std::locale()); });
		if (m_cvt == nullptr)
			m_cvt = std::make_shared<Cvt>();
		m_cvt->utf8 = m_cvt->cvt.to_bytes(s);
		return m_cvt->utf8.c_str();
	}

	size_t insert(size_t pos, LString s)
	{
		if (m_text.use_count() > 1)
			m_text = std::make_shared<std::wstring>(*m_text);
		m_text->insert(pos, s.c_str());
		if (m_cvt == nullptr and s.m_cvt != nullptr)
			m_cvt = s.m_cvt;
		return s.length();
	}

	void erase(size_t pos, int count)
	{
		if (m_text.use_count() > 1)
			m_text = std::make_shared<std::wstring>(*m_text);
		m_text->erase(pos, count);
	}

	LuacObjNew<LString> substr(size_t pos, size_t count)
	{
		LString* s = new LString(m_text->substr(pos, count));
		s->m_cvt = m_cvt;
		return s;
	}

	static LuacObjNew<LString> concat(LString s0, LString s1)
	{
		LString* s = new LString(*s0.m_text + *s1.m_text);
		s->m_cvt = s0.m_cvt != nullptr ? s0.m_cvt : s1.m_cvt;
		return s;
	}

	static bool equal(LString s0, LString s1)
	{
		return *s0.m_text == *s1.m_text;
	}

	int find(LString s)
	{
		return m_text->find(*s.m_text);
	}

	int rfind(LString s)
	{
		return m_text->rfind(*s.m_text);
	}

	Lua_wrap_cpp_class(LString, Lua_ctor(LuaIdx), Lua_mf(set), Lua_mf(utf8), Lua_mf(length), Lua_mf(ch),
		Lua_mf(same), Lua_mf(ncsame), Lua_mf(lower_utf8),
		Lua_mf(insert), Lua_mf(erase), Lua_mf(substr), Lua_mt_mf("__concat", concat), Lua_mt_mf("__eq", equal), Lua_mf(find), Lua_mf(rfind))
protected:
	struct Cvt
	{
		std::wstring_convert<std::codecvt_utf8<wchar_t>> cvt;
		std::string utf8;
	};
	std::shared_ptr<Cvt> m_cvt;
	std::shared_ptr<std::wstring> m_text;
};
template<> inline LString LuaCustomParam<LString>::GetValue(const LuaIdx& idx)
{
	const char* s{};
	if (idx.GetValue(&s) == LUA_TSTRING)
		return s;
	return *idx.GetCppObj<LString>();
}
Lua_global_add_cpp_class(LString)