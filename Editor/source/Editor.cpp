#include "wx/app.h"
#include "EditorUI/TestFrame.h"
#include "EditorUI/EntranceDlg.h"
#include "EditorUI/TestDlg.h"
#include "EditorUI/Entrance.h"
#include "EditorUI/MainFrame.h"
#include "Generic/utilities/DataReader.h"

#define DEFAULT_LAYOUT "notebook_layout0/1<panel>0|2<page0>0|3<page1>0|4<page2>0|*|layout2|name=dummy;caption=;state=2098174;dir=3;layer=0;row=0;pos=0;prop=100000;bestw=225;besth=225;minw=225;minh=225;maxw=-1;maxh=-1;floatx=-1;floaty=-1;floatw=-1;floath=-1|name=1;caption=;state=2098172;dir=5;layer=0;row=0;pos=0;prop=100000;bestw=250;besth=250;minw=-1;minh=-1;maxw=-1;maxh=-1;floatx=-1;floaty=-1;floatw=-1;floath=-1|name=2;caption=;state=2098172;dir=4;layer=1;row=0;pos=0;prop=100000;bestw=490;besth=338;minw=-1;minh=-1;maxw=-1;maxh=-1;floatx=-1;floaty=-1;floatw=-1;floath=-1|name=3;caption=;state=2098172;dir=3;layer=2;row=0;pos=0;prop=100000;bestw=225;besth=225;minw=-1;minh=-1;maxw=-1;maxh=-1;floatx=-1;floaty=-1;floatw=-1;floath=-1|name=4;caption=;state=2098172;dir=2;layer=3;row=0;pos=0;prop=100000;bestw=225;besth=225;minw=-1;minh=-1;maxw=-1;maxh=-1;floatx=-1;floaty=-1;floatw=-1;floath=-1|dock_size(5,0,0)=20|dock_size(4,1,0)=182|dock_size(3,2,0)=203|dock_size(2,3,0)=227|/"

char g_stdout[BUFSIZ];

void FlushStdout()
{
	fflush(stdout);
	OutputDebugStringA(g_stdout);
	memset(g_stdout, 0, strlen(g_stdout));
}

void ErrPrint(const char* s)
{
	OutputDebugStringA(s);
	OutputDebugStringA("\n");
#ifdef _DEBUG
	DebugBreak();
#endif
}

void Require(LuaState& lua, const char* requiredName)
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

	LuaState::SetErrorFunc(ErrPrint);
	LuaState::SetRequireFunc(Require);

	Engine::InitParam eip{};
	eip.hInst = wxGetInstance();
	assert(Engine::Initialize(eip));

	Engine::TerminalNotification n{};
	n.addEvent = vmWindow::AddEvent;
	n.flushStdout = n.flushStderr = FlushStdout;
	Engine::LuaRegister(g_vm->Lua(), n);

	LuaRegGlobalCollected(g_vm);

	g_vm->Run("BB.ff(nil, 1)");

	auto t = GetTickCount();
	g_vm->Run("for i = 1, 100000 do ff(1, 2) end");
	DebugLog(L"%d", GetTickCount() - t);

	g_vm->Run("function ff1(a, b) return a + b end");
	t = GetTickCount();
	for (size_t i = 0; i < 100000; i++)
	{
		g_vm->GetValue("ff1", LuaCall(1, 2));
		//lua_getglobal(l, "ff1");
		//lua_pushinteger(l, 1);
		//lua_pushinteger(l, 2);
		//lua_call(l, 2, 1);
		//lua_pop(l, 1);
	}
	DebugLog(L"%d", GetTickCount() - t);

	g_vm->SetValue("SYS", "CURSOR_IBEAM", wxCURSOR_IBEAM);
	g_vm->SetValue("SYS", "CURSOR_ARROW", wxCURSOR_ARROW);

	wchar_t s[256]{};
	GetCurrentDirectory(256, s);
	//FileData fd("../../Resources/lua-program/..Test.lua", true);
	//FileData fd("Resources/lua-program/utility.lua", true);
	FileData fd("Resources/lua-program/editor.lua", true);
	assert(fd.IsLoaded());
	g_vm->Run((char*)fd.GetData(), fd.GetSize());
	fd.Release();

	g_vm->Run("A = class() function A:f() end a = A() B = class(A) b = B() C = class(B) c = C()");

	t = GetTickCount();
	g_vm->Run("for i = 1, 100000 do A:f() end");
	DebugLog(L"%d", GetTickCount() - t);

	t = GetTickCount();
	g_vm->Run("for i = 1, 100000 do a:f() end");
	DebugLog(L"%d", GetTickCount() - t);

	t = GetTickCount();
	g_vm->Run("for i = 1, 100000 do b:f() end");
	DebugLog(L"%d", GetTickCount() - t);

	t = GetTickCount();
	g_vm->Run("for i = 1, 100000 do c:f() end");
	DebugLog(L"%d", GetTickCount() - t);

	Entrance en(nullptr, "");
	g_vm->SetValue("cEntrance", Lua_set_cobj(&en));
	g_vm->GetValue("LoadEntrance", LuaCall());

	assert(g_vm->GetTop() == 0);
	
	if (en.ShowModal() != wxID_OK)
		return false;

	MainFrame* mf = new MainFrame(nullptr, wxID_ANY, "");
	g_vm->SetValue("cMainFrame", Lua_set_cobj(mf));
	g_vm->GetValue("LoadMainFrame", LuaCall());
	mf->m_nb->LoadPerspective(DEFAULT_LAYOUT);

	mf->Show();

	return true;
}

void MyApp::CleanUp()
{
	g_vm->GetValue("AppCleanUp", LuaCall());
	delete g_vm;
	Engine::CleanUp();
	wxApp::CleanUp();
}