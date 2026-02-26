#pragma once
#include "vmWindow.h"

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
	Lua_wrap_cpp_class(CMenuBar, Lua_ctor_void, Lua_mf(Clear), Lua_mf(Add));
};
Lua_global_add_cpp_class(CMenuBar)

class CNotebook
{
public:
	CNotebook(FloatableNotebook* nb) : m_nb(nb) {}
	void AddPage(const char* name, LString title, LuaIdx wnd)
	{
		m_nb->AddPage(new vmWindow(m_nb, name, wnd), title.c_str());
	}
	void SaveLayout(LuaReturn& ret)
	{
		ret.Push(m_nb->SavePerspective().c_str());
	}
	void LoadLayout(const char* layout)
	{
		m_nb->LoadPerspective(layout);
	}
	void ShowPage(LuacObj<vmWindow> wnd)
	{
		m_nb->ShowPage(wnd);
	}
	bool IsPageShown(LuacObj<vmWindow> wnd)
	{
		return m_nb->IsPageShown(wnd);
	}
	void* GetNB()
	{
		return m_nb;
	}
	Lua_wrap_cpp_class(CNotebook, Lua_abstract, Lua_mf(AddPage), Lua_mf(SaveLayout), Lua_mf(LoadLayout),
		Lua_mf(ShowPage), Lua_mf(IsPageShown), Lua_mf(GetNB));

	FloatableNotebook* m_nb;
};
Lua_global_add_cpp_class(CNotebook)

class MainFrame : public wxFrame, public vmFrame
{
public:
	MainFrame(wxWindow* parent, wxWindowID id, const wxString& title, const std::string name)
		: wxFrame(parent, id, title, wxPoint(wxSCREEN_CENTER_WH(1000, 800)), wxSize(1100, 800),
			wxDEFAULT_FRAME_STYLE), vmFrame(this)
	{
		SetBackgroundStyle(wxBG_STYLE_PAINT);
		m_self = this;
		wxBoxSizer* s = new wxBoxSizer(wxVERTICAL);
		SetSizer(s);

		m_name = name;
		m_nb = new FloatableNotebook(this, new UiManager);
		m_nb->m_allowTabDragOut = false;
		m_nb->m_desPageOnClose = true;
		m_nb->m_tabCloseBtnStyle = wxAUI_NB_CLOSE_ON_ACTIVE_TAB;
		s->Add(m_nb, 1, wxEXPAND);

		m_nb->SetWindowStyleFlag(m_nb->GetWindowStyleFlag() & ~wxAUI_NB_TAB_SPLIT);
		m_nb->Bind(wxEVT_AUINOTEBOOK_PAGE_CHANGED, &MainFrame::OnEvtPageChanged, this);

		Bind(wxEVT_CLOSE_WINDOW, &MainFrame::OnEvtClose, this);

		Terminal::Lua().SetValue("CNotebook", "NB_CLOSE_BUTTON", wxAUI_NB_CLOSE_BUTTON);
		Terminal::Lua().SetValue("CNotebook", "NB_CLOSE_ON_ALL_TABS", wxAUI_NB_CLOSE_ON_ALL_TABS);
		Terminal::Lua().SetValue("CNotebook", "NB_CLOSE_ON_ACTIVE_TAB", wxAUI_NB_CLOSE_ON_ACTIVE_TAB);
	}
	Lua_wrap_cpp_class_derived(vmFrame, MainFrame, Lua_abstract, Lua_mf(OnClose), Lua_mf(OnPageDestroy), Lua_mf(OnPageChanged),
		Lua_mf(SetMenuBar), Lua_mf(ResetMenuBar), Lua_mf(AddPageNotebook), Lua_mf(SetPageNotebookTitle), Lua_mf(SetNotebookStyleFlag),
		Lua_mf(SetSize), Lua_mf(GetSize), Lua_mf(Maximize), Lua_mf(IsMaximized), Lua_mf(LoadLayout), Lua_mf(SaveLayout))
	~MainFrame()
	{
		wxFrame::SetMenuBar({});
	}
	void SetNotebookStyleFlag(int style, bool enable)
	{
		if (enable)
			m_nb->SetWindowStyleFlag(m_nb->GetWindowStyleFlag() | style);
		else
			m_nb->SetWindowStyleFlag(m_nb->GetWindowStyleFlag() & ~style);
	}
	void SetPageNotebookTitle(LuacObj<CNotebook> nb, LString s)
	{
		m_nb->SetPageTitle(nb->m_nb, s.c_str());
	}
	void OnEvtPageChanged(wxAuiNotebookEvent& e)
	{
		Terminal::Lua().GetValue(m_name.c_str(), "OnPageChanged", LuaCall(m_nb->GetWindowFromIdx(e.GetSelection())));
	}
	static void OnPageChanged(void*) {}
	void SetMenuBar(LuacObj<CMenuBar> mb)
	{
		SetFrameMenuBar(mb);
	}
	void ResetMenuBar()
	{
		SetFrameMenuBar({});
	}
	void SetSize(int x, int y)
	{
		wxFrame::SetSize(x, y);
	}
	std::tuple<int, int> GetSize()
	{
		return { wxFrame::GetSize().x, wxFrame::GetSize().y };
	}
	void LoadLayout(const char* layout)
	{
		m_nb->LoadPerspective(layout);
	}
	void SaveLayout(LuaReturn& ret)
	{
		ret.Push(m_nb->SavePerspective().c_str());
	}

	void OnEvtClose(wxCloseEvent& e)
	{
		bool b{};
		Terminal::Lua().GetValue(m_name.c_str(), "OnClose", LuaCall(), &b);
		if (b)
			e.Skip();
		else
			e.Veto();
	}
	
	void OnEvtDestroy(wxWindowDestroyEvent& e)
	{
		Terminal::Lua().GetValue(m_name.c_str(), "OnPageDestroy", LuaCall(e.GetWindow()));
	}

	bool OnClose()
	{
		return true;
	}

	static void OnPageDestroy() {}

	void SetTitle(LString title) override
	{
		wxFrame::SetTitle(title.c_str());
	}

	void AddPageWnd(wxWindow* w, const wchar_t* title) override
	{
		w->Bind(wxEVT_DESTROY, &MainFrame::OnEvtDestroy, this);
		m_nb->AddPage(w, title);
	}
	LuacObjNew<CNotebook> AddPageNotebook(const char* name, LString title)
	{
		FloatableNotebook* nb = new FloatableNotebook(m_self, new UiManager);
		nb->SetName(name);
		AddPageWnd(nb, title.c_str());
		return new CNotebook(nb);
	}

	void SetFrameMenuBar(wxMenuBar* mb) override
	{
		wxFrame::SetMenuBar(mb);
	}

	void Accept() override
	{
		Close();
	}

	void Reject() override
	{
		Close();
	}

	std::string m_name;
	FloatableNotebook* m_nb;
};