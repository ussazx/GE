#pragma once
#include "wx/clipbrd.h"
#include <string>
#include <codecvt>
#include <unordered_map>
#include <memory>
#include "common/uiutils.h"
#include "common/floatablenotebook.h"
#include "common/uimanager.h"
#include "../../../Generic/Terminal.h"

extern wxStockCursor g_cursor;

class vmWindow : public wxWindow
{
public:
	vmWindow(wxWindow* parent, const char* name, LuaIdx& self,
		wxWindowID id = wxID_ANY,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize)
		: wxWindow(parent, id, pos, size, wxNO_BORDER|wxWANTS_CHARS, name)
	{
		SetBackgroundStyle(wxBG_STYLE_PAINT);

		Bind(wxEVT_IDLE, &vmWindow::OnIdle, this);
		Bind(wxEVT_SIZE, &vmWindow::OnSize, this);
		Bind(wxEVT_PAINT, &vmWindow::OnPaint, this);

		Bind(wxEVT_ENTER_WINDOW, &vmWindow::OnMouseEvent, this);
		Bind(wxEVT_LEAVE_WINDOW, &vmWindow::OnMouseEvent, this);
		Bind(wxEVT_LEFT_DOWN, &vmWindow::OnMouseEvent, this);
		Bind(wxEVT_LEFT_UP, &vmWindow::OnMouseEvent, this);
		Bind(wxEVT_MIDDLE_DOWN, &vmWindow::OnMouseEvent, this);
		Bind(wxEVT_MIDDLE_UP, &vmWindow::OnMouseEvent, this);
		Bind(wxEVT_RIGHT_DOWN, &vmWindow::OnMouseEvent, this);
		Bind(wxEVT_RIGHT_UP, &vmWindow::OnMouseEvent, this);
		Bind(wxEVT_MOTION, &vmWindow::OnMouseEvent, this);
		Bind(wxEVT_LEFT_DCLICK, &vmWindow::OnMouseEvent, this);
		Bind(wxEVT_MIDDLE_DCLICK, &vmWindow::OnMouseEvent, this);
		Bind(wxEVT_RIGHT_DCLICK, &vmWindow::OnMouseEvent, this);
		Bind(wxEVT_MOUSEWHEEL, &vmWindow::OnMouseEvent, this);
		Bind(wxEVT_AUX1_DOWN, &vmWindow::OnMouseEvent, this);
		Bind(wxEVT_AUX1_UP, &vmWindow::OnMouseEvent, this);
		Bind(wxEVT_AUX1_DCLICK, &vmWindow::OnMouseEvent, this);
		Bind(wxEVT_AUX2_DOWN, &vmWindow::OnMouseEvent, this);
		Bind(wxEVT_AUX2_UP, &vmWindow::OnMouseEvent, this);
		Bind(wxEVT_MAGNIFY, &vmWindow::OnMouseEvent, this);

		Bind(wxEVT_MOUSE_CAPTURE_LOST, &vmWindow::OnCaptureLost, this);

		Bind(wxEVT_CHAR, &vmWindow::OnCharEvent, this);
		Bind(wxEVT_KEY_DOWN, &vmWindow::OnKeyDown, this);
		Bind(wxEVT_KEY_UP, &vmWindow::OnKeyUp, this);

		Bind(wxEVT_SET_CURSOR, &vmWindow::OnSetCursor, this);

		m_timer.Bind(wxEVT_TIMER, &vmWindow::OnTimer, this);

		Terminal::Lua().SetValue(LuaGetName(), this, self);
		Terminal::Lua().SetValue(self, Lua_set_cobj(this));
		int t{};
		Terminal::Lua().GetValue(LuaGetName(), this, "init", LuaObjCall(clock(), GetHWND(), GetRect().GetWidth(), GetRect().GetHeight()), &t);
		HandleTimer(t);
	}

	~vmWindow()
	{
		Terminal::Lua().SetValue(LuaGetName(), this, nullptr);
	}

	static int GetEventId(const wxEventType& e)
	{
		return m_eventId[e];
	}

	static void InitEventNameMap()
	{
		m_eventName.emplace("EVT_ENTER_WINDOW", wxEVT_ENTER_WINDOW);
		m_eventName.emplace("EVT_LEAVE_WINDOW", wxEVT_LEAVE_WINDOW);
		m_eventName.emplace("EVT_LEFT_DOWN", wxEVT_LEFT_DOWN);
		m_eventName.emplace("EVT_LEFT_UP", wxEVT_LEFT_UP);
		m_eventName.emplace("EVT_MIDDLE_DOWN", wxEVT_MIDDLE_DOWN);
		m_eventName.emplace("EVT_MIDDLE_UP", wxEVT_MIDDLE_UP);
		m_eventName.emplace("EVT_RIGHT_DOWN", wxEVT_RIGHT_DOWN);
		m_eventName.emplace("EVT_RIGHT_UP", wxEVT_RIGHT_UP);
		m_eventName.emplace("EVT_MOTION", wxEVT_MOTION);
		m_eventName.emplace("EVT_LEFT_DCLICK", wxEVT_LEFT_DCLICK);
		m_eventName.emplace("EVT_MIDDLE_DCLICK", wxEVT_MIDDLE_DCLICK);
		m_eventName.emplace("EVT_RIGHT_DCLICK", wxEVT_RIGHT_DCLICK);
		m_eventName.emplace("EVT_MOUSEWHEEL", wxEVT_MOUSEWHEEL);
		m_eventName.emplace("EVT_AUX1_DOWN", wxEVT_AUX1_DOWN);
		m_eventName.emplace("EVT_AUX1_UP", wxEVT_AUX1_UP);
		m_eventName.emplace("EVT_AUX1_DCLICK", wxEVT_AUX1_DCLICK);
		m_eventName.emplace("EVT_AUX2_DOWN", wxEVT_AUX2_DOWN);
		m_eventName.emplace("EVT_AUX2_UP", wxEVT_AUX2_UP);
		m_eventName.emplace("EVT_MAGNIFY", wxEVT_MAGNIFY);
	}

	static void AddEvent(const char* name, int id)
	{
		auto it = m_eventName.find(name);
		if (it != m_eventName.end())
			m_eventId[it->second] = id;
	}

	Lua_wrap_cpp_class(vmWindow, Lua_abstract, Lua_mf(Capture));
private:
	void Capture(bool b)
	{
		if (b)
		{
			if (GetCapture() != this)
				CaptureMouse();
		}
		else if (GetCapture() == this)
			ReleaseMouse();
	}

	void OnCaptureLost(wxMouseCaptureLostEvent&)
	{
		int t{};
		Terminal::Lua().GetValue(LuaGetName(), this, "on_capture_lost", LuaObjCall(clock()), &t);
		HandleTimer(t);
	}

	void OnSetCursor(wxSetCursorEvent& e)
	{
		//e.SetCursor(g_cursor);
	}

	void OnTimer(wxTimerEvent&)
	{
		int t{};
		Terminal::Lua().GetValue(LuaGetName(), this, "on_idle", LuaObjCall(clock(), true, IsShown()), &t);
		HandleTimer(t);
	}
	void OnIdle(wxIdleEvent& e)
	{
		int t{};
		auto i = GetTickCount();
		Terminal::Lua().GetValue(LuaGetName(), this, "on_idle", LuaObjCall(clock(), false, IsShown()), &t);
		//DebugLog(L"idle [ %u ]\n", GetTickCount() - i);
		Terminal::Lua().SetValue(LuaGetName(), this, "idle_cost", (uint32_t)(GetTickCount() - i));
		HandleTimer(t);
	}
	void OnPaint(wxPaintEvent&)
	{
		Terminal::Lua().GetValue(LuaGetName(), this, "render", LuaObjCall());
	}
	void OnSize(wxSizeEvent& e)
	{
		auto i = GetTickCount();
		Terminal::Lua().GetValue(LuaGetName(), this, "resize", LuaObjCall(e.GetSize().x, e.GetSize().y));
		//DebugLog(L"resize [ %u ]\n", GetTickCount() - i);
	}
	void OnCharEvent(wxKeyEvent& e)
	{
		if (e.GetRawKeyCode() != e.GetUnicodeKey())
			return;

		int t{};

		static wchar_t c[8];
		static std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;
		switch (e.GetUnicodeKey())
		{
		case VK_BACK:
		case VK_RETURN:
			break;
		case 1:   // Ctrl A
		case 2:   // Ctrl B
		case 3:	  // Ctrl C
		case 4:   // Ctrl D
		case 5:   // Ctrl E
		case 6:   // Ctrl F
		case 7:   // Ctrl G
		case 9:   // Ctrl I
		case 10:  // Ctrl J
		case 11:  // Ctrl K
		case 12:  // Ctrl L
		case 14:  // Ctrl N
		case 15:  // Ctrl O
		case 16:  // Ctrl P
		case 17:  // Ctrl Q
		case 18:  // Ctrl R
		case 19:  // Ctrl S
		case 20:  // Ctrl T
		case 21:  // Ctrl U
		case 22:  // Ctrl V
		case 23:  // Ctrl W
		case 24:  // Ctrl X
		case 25:  // Ctrl Y
		case 26:  // Ctrl Z
		case 27:  // Ctrl [
		case 28:  // Ctrl \ 
		case 29:  // Ctrl ]
			Terminal::Lua().GetValue(LuaGetName(), this, "on_acc_key", LuaObjCall(clock(), e.GetUnicodeKey()), &t);
			break;
		default:
			swprintf_s(c, L"%c", e.GetUnicodeKey());
			Terminal::Lua().GetValue(LuaGetName(), this, "on_char", LuaObjCall(clock(), conv.to_bytes(c).c_str()), &t);
			break;
		}
		HandleTimer(t);
	}

	void OnKeyDown(wxKeyEvent& e)
	{
		int k = e.GetRawKeyCode();
		bool left{}, right{};
		switch (k)
		{
		case VK_CONTROL:
			left = GetKeyState(VK_LCONTROL) & 8000;
			right = GetKeyState(VK_RCONTROL) & 8000;
			break;
		case VK_SHIFT:
			left = GetKeyState(VK_LSHIFT) & 8000;
			right = GetKeyState(VK_RSHIFT) & 8000;
			break;
		case VK_MENU:
			left = GetKeyState(VK_LMENU) & 8000;
			right = GetKeyState(VK_RMENU) & 8000;
			break;
		default:
			break;
		}
		int t{};
		Terminal::Lua().GetValue(LuaGetName(), this, "on_key_down", LuaObjCall(clock(), k, left, right), &t);
		HandleTimer(t);
		e.Skip();
	}

	void OnKeyUp(wxKeyEvent& e)
	{
		int t{};
		Terminal::Lua().GetValue(LuaGetName(), this, "on_key_up", LuaObjCall(clock(), e.GetRawKeyCode()), &t);
		HandleTimer(t);
		e.Skip();
	}

	void OnMouseEvent(wxMouseEvent& e)
	{
		int t{}, c{};
		Terminal::Lua().GetValue(LuaGetName(), this, "on_mouse", LuaObjCall(clock(), GetEventId(e.GetEventType()), e.GetX(), e.GetY(), e.GetWheelRotation()), &t, &c);
		HandleTimer(t);
		g_cursor = (wxStockCursor)c;
		wxSetCursor(g_cursor);
	}
	void HandleTimer(int t)
	{
		if (t > 0)
			m_timer.Start(t);	
		else if (t < 0)
			m_timer.Stop();
	}

	wxTimer m_timer;

	static std::unordered_map<std::string, wxEventType> m_eventName;
	static std::unordered_map<wxEventType, int> m_eventId;
};
Lua_global_add_cpp_class(vmWindow)

class CMenu : public wxMenu
{
public:
	CMenu()
	{
		Bind(wxEVT_MENU, &CMenu::OnItem, this);
		Bind(wxEVT_MENU_OPEN, &CMenu::OnOpen, this);
	}
	void Clear()
	{
		wxMenuItemList& list = GetMenuItems();
		for (size_t i = 0; i < list.size(); i++)
			Destroy(list[i]);
	}
	void AddItem(int id, LString text)
	{
		Append(id, text.c_str());
	}
	wxMenuItem* AddCheckItem(int id, LString text)
	{
		return AppendCheckItem(id, text.c_str());
	}
	void AddSubMenu(LuaReturn& ret, LString text)
	{
		CMenu* sub = new CMenu;
		ret.Push(Lua_set_cobj(sub));
		AppendSubMenu(sub, text.c_str());
	}
	void Popup(LuaReturn& ret, LuacObj<vmWindow> window)
	{
		item = false;
		window->PopupMenu(this);
		if (item)
			ret.Push(id);
	}
	void OnItem(wxCommandEvent& e)
	{
		item = true;
		id = e.GetId();
		if (bindOnItem)
			Terminal::Lua().GetValue(LuaGetName(), &bindOnItem, LuaCall(id));
	}
	void OnOpen(wxMenuEvent&)
	{
		if (bindOnOpen)
			Terminal::Lua().GetValue(LuaGetName(), &bindOnOpen, LuaCall());
	}
	void BindOnItem(const LuaIdx& func)
	{
		if (func.GetValue(nullptr) == LUA_TFUNCTION)
		{
			bindOnItem = true;
			Terminal::Lua().SetValue(LuaGetName(), &bindOnItem, func);
		}
		else
			bindOnItem = false;
	}
	void BindOnOpen(LuaIdx func)
	{
		if (func.GetValue(nullptr) == LUA_TFUNCTION)
		{
			bindOnOpen = true;
			Terminal::Lua().SetValue(LuaGetName(), &bindOnOpen, func);
		}
		else
			bindOnOpen = false;
	}
	static void Check(wxMenuItem* item, bool check)
	{
		item->Check(check);
	}
	Lua_wrap_cpp_class(CMenu, Lua_ctor(), Lua_mf(AddItem), Lua_mf(AddCheckItem), Lua_mf(AddSubMenu), 
		Lua_mf(BindOnOpen), Lua_mf(Check), Lua_mf(Popup), Lua_mf(Clear));

	wxMenu sub;
	bool item{};
	int id{};
	bool bindOnItem{};
	bool bindOnOpen{};
};
Lua_global_add_cpp_class(CMenu)

class CMenuBar : public wxMenuBar
{
public:
	void Clear()
	{
		for (size_t i = 0; i < GetMenuCount(); i++)
			Remove(i);
	}
	void Add(LuaReturn& ret, LString text, LuaIdx func)
	{
		CMenu* sub = new CMenu;
		sub->BindOnItem(func);
		ret.Push(Lua_set_cobj(sub));
		Append(sub, text.c_str());
	}
	Lua_wrap_cpp_class(CMenuBar, Lua_ctor(), Lua_mf(Clear), Lua_mf(Add));
};
Lua_global_add_cpp_class(CMenuBar)

class CNotebook
{
public:
	static void AddPage(LuacObj<FloatableNotebook> fnb, const char* name, LString title, LuaIdx wnd)
	{
		fnb->AddPage(new vmWindow(fnb, name, wnd), title.c_str());
	}
	static void SaveLayout(LuaReturn& ret, LuacObj<FloatableNotebook> fnb)
	{
		ret.Push(fnb->SavePerspective().c_str());
	}
	static void LoadLayout(LuacObj<FloatableNotebook> fnb, const char* layout)
	{
		fnb->LoadPerspective(layout);
	}
	static void ShowPage(LuacObj<FloatableNotebook> fnb, LuacObj<wxWindow> wnd)
	{
		fnb->ShowPage(wnd);
	}
	static bool IsPageShown(LuacObj<FloatableNotebook> fnb, LuacObj<wxWindow> wnd)
	{
		return fnb->IsPageShown(wnd);
	}
	Lua_wrap_cpp_class(CNotebook, Lua_abstract, Lua_mf(AddPage), Lua_mf(SaveLayout), Lua_mf(LoadLayout),
		Lua_mf(ShowPage), Lua_mf(IsPageShown));
};

class vmFrame
{
public:
	vmFrame(wxWindow* self) : m_self(self) {}

	~vmFrame() {}
	
	void AddPageWindow(const char* name, LString title, LuaIdx wnd)
	{
		AddPageWnd(new vmWindow(m_self, name, wnd), title.c_str());
	}

	void AddPageNotebook(LuaReturn& ret, const char* name, LString title)
	{
		FloatableNotebook* nb = new FloatableNotebook(m_self, new UiManager);
		nb->SetName(name);
		AddPageWnd(nb, title.c_str());
		ret.Push(LuaSub(0, nb), LuaSub(1, typeid(decltype(nb)).hash_code()), (LuaCustomSet)CNotebook::LuaSetClassTable);
	}

	void SetMenuBar(LuacObj<CMenuBar> mb)
	{
		SetFrameMenuBar(mb);
	}

	void ResetMenuBar()
	{
		SetFrameMenuBar({});
	}

	virtual void AddPageWnd(wxWindow*, const wchar_t* title) = 0;
	virtual void SetFrameMenuBar(wxMenuBar*) {}

	virtual void Accept() = 0;
	virtual void Reject() = 0;

	Lua_wrap_cpp_class(vmFrame, Lua_abstract, Lua_mf(AddPageWindow), Lua_mf(AddPageNotebook),
		Lua_mf(SetMenuBar), Lua_mf(ResetMenuBar),
		Lua_mf(Accept), Lua_mf(Reject));
private:
	wxWindow* m_self{};
};
