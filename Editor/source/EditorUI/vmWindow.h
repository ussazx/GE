#pragma once
#include "wx/clipbrd.h"
#include <string>
#include <codecvt>
#include <unordered_map>
#include <memory>
#include "common/uiutils.h"
#include "../../../Generic/Terminal.h"

extern wxStockCursor g_cursor;

class vmWindow : public wxWindow
{
public:
	vmWindow(wxWindow* parent, const char* name,
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

		Terminal::Lua().SetValue(GetName(), Lua_set_cobj(this));
		int t{};
		Terminal::Lua().GetValue(GetName(), "init", LuaObjCall(clock(), GetHWND(), GetRect().GetWidth(), GetRect().GetHeight()), &t);
		HandleTimer(t);
	}

	~vmWindow()
	{
		Terminal::Lua().SetValue(GetName(), nullptr);
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
		Terminal::Lua().GetValue(GetName(), "on_capture_lost", LuaObjCall(clock()), &t);
		HandleTimer(t);
	}

	void OnSetCursor(wxSetCursorEvent& e)
	{
		//e.SetCursor(g_cursor);
	}

	void OnTimer(wxTimerEvent&)
	{
		int t{};
		Terminal::Lua().GetValue(GetName(), "on_idle", LuaObjCall(clock(), true, IsShown()), &t);
		HandleTimer(t);
	}
	void OnIdle(wxIdleEvent& e)
	{
		int t{};
		auto i = GetTickCount();
		Terminal::Lua().GetValue(GetName(), "on_idle", LuaObjCall(clock(), false, IsShown()), &t);
		//DebugLog(L"idle [ %u ]\n", GetTickCount() - i);
		Terminal::Lua().SetValue(GetName(), "idle_cost", (uint32_t)(GetTickCount() - i));
		HandleTimer(t);
	}
	void OnPaint(wxPaintEvent&)
	{
		Terminal::Lua().GetValue(GetName(), "render", LuaObjCall());
	}
	void OnSize(wxSizeEvent& e)
	{
		auto i = GetTickCount();
		Terminal::Lua().GetValue(GetName(), "resize", LuaObjCall(e.GetSize().x, e.GetSize().y));
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
			Terminal::Lua().GetValue(GetName(), "on_acc_key", LuaObjCall(clock(), e.GetUnicodeKey()), &t);
			break;
		default:
			swprintf_s(c, L"%c", e.GetUnicodeKey());
			Terminal::Lua().GetValue(GetName(), "on_char", LuaObjCall(clock(), conv.to_bytes(c).c_str()), &t);
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
		Terminal::Lua().GetValue(GetName(), "on_key_down", LuaObjCall(clock(), k, left, right), &t);
		HandleTimer(t);
		e.Skip();
	}

	void OnKeyUp(wxKeyEvent& e)
	{
		int t{};
		Terminal::Lua().GetValue(GetName(), "on_key_up", LuaObjCall(clock(), e.GetRawKeyCode()), &t);
		HandleTimer(t);
		e.Skip();
	}

	void OnMouseEvent(wxMouseEvent& e)
	{
		int t{}, c{};
		Terminal::Lua().GetValue(GetName(), "on_mouse", LuaObjCall(clock(), GetEventId(e.GetEventType()), e.GetX(), e.GetY(), e.GetWheelRotation()), &t, &c);
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

class vmFrame
{
public:
	vmFrame(wxWindow* self) : m_self(self) {}

	~vmFrame() {}
	
	void AddPageWindow(const char* name, const char* title, LuaIdx wnd)
	{
		Terminal::Lua().SetValue(name, wnd);
		AddPageWnd(new vmWindow(m_self, name), _(title));
	}

	virtual void AddPageWnd(vmWindow*, const char* title) = 0;

	virtual void Accept() = 0;
	virtual void Reject() = 0;

	Lua_wrap_cpp_class(vmFrame, Lua_abstract, Lua_mf(AddPageWindow), Lua_mf(Accept), Lua_mf(Reject));
private:
	wxWindow* m_self{};
};
