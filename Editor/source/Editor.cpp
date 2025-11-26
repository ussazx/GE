#include "wx/app.h"
#include "EditorUI/TestFrame.h"
#include "EditorUI/EntranceDlg.h"
#include "EditorUI/TestDlg.h"
#include "EditorUI/Entrance.h"
#include "EditorUI/MainFrame.h"
#include "Generic/utilities/DataReader.h"

#define DEFAULT_LAYOUT "notebook_layout0/1<panel>0|2<page0>0|3<page1>0|4<page2>0|*|layout2|name=dummy;caption=;state=2098174;dir=3;layer=0;row=0;pos=0;prop=100000;bestw=225;besth=225;minw=225;minh=225;maxw=-1;maxh=-1;floatx=-1;floaty=-1;floatw=-1;floath=-1|name=1;caption=;state=2098172;dir=5;layer=0;row=0;pos=0;prop=100000;bestw=250;besth=250;minw=-1;minh=-1;maxw=-1;maxh=-1;floatx=-1;floaty=-1;floatw=-1;floath=-1|name=2;caption=;state=2098172;dir=4;layer=1;row=0;pos=0;prop=100000;bestw=490;besth=338;minw=-1;minh=-1;maxw=-1;maxh=-1;floatx=-1;floaty=-1;floatw=-1;floath=-1|name=3;caption=;state=2098172;dir=3;layer=2;row=0;pos=0;prop=100000;bestw=225;besth=225;minw=-1;minh=-1;maxw=-1;maxh=-1;floatx=-1;floaty=-1;floatw=-1;floath=-1|name=4;caption=;state=2098172;dir=2;layer=3;row=0;pos=0;prop=100000;bestw=225;besth=225;minw=-1;minh=-1;maxw=-1;maxh=-1;floatx=-1;floaty=-1;floatw=-1;floath=-1|dock_size(5,0,0)=20|dock_size(4,1,0)=182|dock_size(3,2,0)=203|dock_size(2,3,0)=227|/"

char g_stdout[BUFSIZ];

void Terminal::OnLuaError(const char* s)
{
	OutputDebugStringA(s);
	OutputDebugStringA("\n");
#ifdef _DEBUG
	DebugBreak();
#endif
}

Lua_global_add_cfunc(GetTickCount)

void Terminal::OnRequired(LuaState& lua, const char* requiredName)
{
	std::string ss = std::string("Resources/lua-program/") + requiredName;
	_strlwr((char*)ss.c_str());
	if (ss.rfind(".lua") == std::string::npos)
		ss += ".lua";

	FileData fd(ss.c_str(), true);
	assert(fd.IsLoaded());
	lua.LoadRequired(requiredName, (char*)fd.GetData(), fd.GetSize());
}

wxLocale gLocale;

class MyApp : public wxApp
{
public:
	bool OnInit() wxOVERRIDE;
	void CleanUp() wxOVERRIDE;
};

wxDECLARE_APP(MyApp);
wxIMPLEMENT_APP(MyApp);

struct AA
{
	AA() {}
	AA(AA&&)
	{
		int a = 5;
	}
	AA(const AA&)
	{
		int a = 5;
	}
	void f() {}
	Lua_wrap_cpp_class(AA, Lua_ctor(), Lua_mf(f))
};
Lua_global_add_cpp_class(AA)

struct BB : public AA
{
	BB(int a, bool b) {}
	
	std::tuple<int, int> f() { return { 1, true }; }

	static void ff(LuaIdx i, int n) 
	{
		n = 1;
	}

	Lua_wrap_cpp_class_derived(AA, BB, Lua_ctor(int, bool), Lua_mf(f), Lua_mf(ff));
};
Lua_global_add_cpp_class(BB)

void ff(int a, int b)
{

}
Lua_global_add_cfunc(ff)

bool MyApp::OnInit()
{
	if (!wxApp::OnInit())
		return false;

	wxImage::AddHandler(new wxJPEGHandler);
	wxImage::AddHandler(new wxPNGHandler);
	wxImage::AddHandler(new wxTGAHandler);

	int lang = wxLocale::GetLanguageInfo(wxLANGUAGE_DEFAULT)->Language;
	gLocale.Init(lang);
	wxLocale::AddCatalogLookupPathPrefix("./locale");
#ifdef _DEBUG
	wxLocale::AddCatalogLookupPathPrefix("../debug/locale");
#else
	wxLocale::AddCatalogLookupPathPrefix("../release/locale");
#endif 
	gLocale.AddCatalog("locale");

	//wxFrame* f = new TestDlg(NULL,
	//wxID_ANY, 
	//"wxAUI Sample Application");
	//f->Show();
	//return true;

	//wxFrame* f = new TestFrame(NULL,
	//wxID_ANY,
	//"wxAUI Sample Application",
	//wxDefaultPosition,
	//wxWindow::FromDIP(wxSize(1100, 700), NULL));
	//f->Show();
	//return true;

	setbuf(stdout, g_stdout);
	setbuf(stderr, g_stdout);

	vmWindow::InitEventNameMap();

	Engine::InitParam eip{};
	eip.hInst = wxGetInstance();
	assert(Engine::Initialize(eip));

	Engine::LuaRegister(Terminal::Lua().Lua());

	LuaRegGlobalReflected(&Terminal::Lua());


	Terminal::Lua().Run("a = 1.1");
	uint32_t n;
	Terminal::Lua().GetValue("a", &n);

	Terminal::Lua().Run("BB.ff(nil, 1)");

	auto t = GetTickCount();
	Terminal::Lua().Run("for i = 1, 100000 do ff(1, 2) end");
	DebugLog(L"%d", GetTickCount() - t);

	Terminal::Lua().Run("function ff1(a, b) return a + b end");
	t = GetTickCount();
	for (size_t i = 0; i < 100000; i++)
	{
		Terminal::Lua().GetValue("ff1", LuaCall(1, 2));
		//lua_getglobal(l, "ff1");
		//lua_pushinteger(l, 1);
		//lua_pushinteger(l, 2);
		//lua_call(l, 2, 1);
		//lua_pop(l, 1);
	}
	DebugLog(L"%d", GetTickCount() - t);

	Terminal::Lua().SetValue("SYS", "CURSOR_IBEAM", wxCURSOR_IBEAM);
	Terminal::Lua().SetValue("SYS", "CURSOR_ARROW", wxCURSOR_ARROW);
	Terminal::Lua().SetValue("SYS", "CURSOR_SIZEWE", wxCURSOR_SIZEWE);
	Terminal::Lua().SetValue("SYS", "CURSOR_SIZENS", wxCURSOR_SIZENS);
	Terminal::Lua().SetValue("SYS", "MOUSE_BTN_LEFT", wxMOUSE_BTN_LEFT);
	Terminal::Lua().SetValue("SYS", "MOUSE_BTN_MIDDLE", wxMOUSE_BTN_MIDDLE);
	Terminal::Lua().SetValue("EDITOR", true);

	wchar_t s[256]{};
	GetCurrentDirectory(256, s);
	//FileData fd("../../Resources/lua-program/..Test.lua", true);
	//FileData fd("Resources/lua-program/utility.lua", true);
	FileData fd("Resources/lua-program/editor.lua", true);
	assert(fd.IsLoaded());
	Terminal::Lua().Run((char*)fd.GetData(), fd.GetSize());
	fd.Release();

	Terminal::Lua().Run("function z() return a, b end zz = {a = 'abc', b = '123'} w = {z = z} o = {zz = zz}");
	Terminal::Lua().SetValue("w", "z", LuaFEnv(), LuaGet("o", "zz"));
	const char* a{};
	const char* b{};
	Terminal::Lua().GetValue("z", LuaCall(), &a, &b);

	Terminal::Lua().Run("a = {} b = {}");
	Terminal::Lua().SetValue("a", 1, LuaGet("b"), "z", 1);
	Terminal::Lua().Run("Print(a[1][b].z)");

	Terminal::Lua().Run("A = class() function A:f() end a = A() B = class(A) b = B() C = class(B) c = C()");

	t = GetTickCount();
	Terminal::Lua().Run("for i = 1, 100000 do A:f() end");
	DebugLog(L"%d", GetTickCount() - t);

	t = GetTickCount();
	Terminal::Lua().Run("for i = 1, 100000 do a:f() end");
	DebugLog(L"%d", GetTickCount() - t);

	t = GetTickCount();
	Terminal::Lua().Run("for i = 1, 100000 do b:f() end");
	DebugLog(L"%d", GetTickCount() - t);

	t = GetTickCount();
	Terminal::Lua().Run("for i = 1, 100000 do c:f() end");
	DebugLog(L"%d", GetTickCount() - t);

	t = GetTickCount();
	Terminal::Lua().Run("local o function fz() if (not o) then o = {} end return o end for i = 1, 100000 do local x = o or fz() end");
	DebugLog(L"%d", GetTickCount() - t);

	t = GetTickCount();
	Terminal::Lua().Run("local o for i = 1, 100000 do if (not o) then o = {} end local x = o end");
	DebugLog(L"%d", GetTickCount() - t);

	Entrance en(nullptr, "");
	Terminal::Lua().SetValue("cEntrance", Lua_set_cobj(&en));
	Terminal::Lua().GetValue("LoadEntrance", LuaCall());

	assert(Terminal::Lua().GetTop() == 0);
	
	if (en.ShowModal() != wxID_OK)
		return false;

	//Terminal::Lua().GetValue("LoadProject", LuaCall());

	MainFrame* mf = new MainFrame(nullptr, wxID_ANY, "", "cMainFrame");
	Terminal::Lua().SetValue("cMainFrame", Lua_set_cobj(mf));

	Terminal::Lua().GetValue("LoadMainFrame", LuaCall());
	//mf->m_nb->LoadPerspective(DEFAULT_LAYOUT);

	mf->Show();

	return true;
}

void MyApp::CleanUp()
{
	Terminal::CleanUp();
	Engine::CleanUp();
	wxApp::CleanUp();
}

void Terminal::AddEvent(const char* name, int id)
{
	vmWindow::AddEvent(name, id);
}

void Terminal::FlushStdout()
{
	fflush(stdout);
	static std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;
	OutputDebugString(conv.from_bytes(g_stdout).c_str());
	memset(g_stdout, 0, strlen(g_stdout));
}

void Terminal::FlushStderr()
{
	FlushStdout();
}

void Terminal::SetClipboardText(LString s)
{
	wxTheClipboard->SetData(new wxTextDataObject(s.c_str()));
}

LuacObjNew<LString> Terminal::GetClipboardText()
{
	static wxTextDataObject data;
	static std::wstring s;
	if (wxTheClipboard->GetData(data))
		return new LString(data.GetText().wc_str());
	return nullptr;
}

void Terminal::NewDirectory(LString path)
{
	::CreateDirectory(path, NULL);
}

void Terminal::SetCurrentDir(LString path)
{
	::SetCurrentDirectory(path);
}

LuacObjNew<LString> Terminal::NewFileDialog(LString title, LString defName, LString filters)
{
	wxFileDialog dialog(nullptr,
		title.c_str(),
		wxEmptyString,
		defName.c_str(),
		filters.c_str(),
		wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
	dialog.ShowModal();
	return new LString(dialog.GetPath().wc_str());
}

LuacObjNew<LString> Terminal::OpenFileDialog(LString title, LString defName, LString filters)
{
	wxFileDialog dialog(nullptr,
		title.c_str(),
		wxEmptyString,
		defName.c_str(),
		filters.c_str(),
		wxFD_OPEN);
	dialog.ShowModal();
	return new LString(dialog.GetPath().wc_str());
}

LuacObjNew<LString> Terminal::ChooseDirDialog(LString title, LString home, bool mustExist)
{
	wxDirDialog dialog(nullptr, title.c_str(), home.c_str(), wxDD_DEFAULT_STYLE | (mustExist ? wxDD_DIR_MUST_EXIST : 0));
	dialog.ShowModal();
	return new LString(dialog.GetPath().wc_str());
}

void Terminal::MessageDialog(LuaReturn& ret, LString caption, LString message, LString yes, LString no, LString cancel)
{
	wxMessageDialog dialog({},
		message.c_str(),
		caption.c_str(),
		wxCENTER |
		wxNO_DEFAULT | wxYES_NO | wxCANCEL |
		wxICON_INFORMATION);
	dialog.SetYesNoCancelLabels(yes.c_str(), no.c_str(), cancel.c_str());
	switch (dialog.ShowModal())
	{
	case wxID_YES:
		ret.Push(true);
	case wxID_NO:
		ret.Push(false);
	default:
		ret.Push(nullptr);
	}
}

class Timer : public Terminal::CTimer
{
public:
	Timer(uint32_t id)
	{
		m_id = id;
		m_timer.Bind(wxEVT_TIMER, &Timer::OnTimer, this);
	}
	void Start(int t, bool oneShot) override
	{
		m_timer.Start(t, oneShot);
	}
	void Stop() override
	{
		m_timer.Stop();
	}
	void OnTimer(wxTimerEvent&)
	{
		Terminal::Lua().GetValue("Timer", "OnTimer", LuaCall(m_id));
	}

	wxTimer m_timer;
	uint32_t m_id;
};
LuacObjNew<Terminal::CTimer> Terminal::NewTimer(uint32_t id)
{
	return new Timer(id);
}

class FileFinder : public Terminal::CFileFinder
{
public:
	bool FindFirst(LString path) override
	{
		m_wfd = {};
		if (m_hFind != INVALID_HANDLE_VALUE)
			FindClose(m_hFind);
		m_hFind = ::FindFirstFile(path, &m_wfd);
		return m_hFind != INVALID_HANDLE_VALUE;
	}

	bool FindNext() override
	{
		return ::FindNextFile(m_hFind, &m_wfd) == TRUE;
	}

	bool IsDirectory() override
	{
		return (m_wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY;
	}

	LuacObjNew<LString> GetName() override
	{
		return new LString(m_wfd.cFileName);
	}

	~FileFinder()
	{
		if (m_hFind != INVALID_HANDLE_VALUE)
			FindClose(m_hFind);
	}

private:
	WIN32_FIND_DATA m_wfd = {};
	HANDLE m_hFind = INVALID_HANDLE_VALUE;
};

LuacObjNew<Terminal::CFileFinder> Terminal::NewFileFinder()
{
	return new FileFinder;
}