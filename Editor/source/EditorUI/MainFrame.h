#pragma once
#include "vmWindow.h"

class MainFrame : public vmFrame, public wxFrame
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
		s->Add(m_nb, 1, wxEXPAND);

		Bind(wxEVT_CLOSE_WINDOW, &MainFrame::OnEvtClose, this);
	}
	Lua_wrap_cpp_class_derived(vmFrame, MainFrame, Lua_abstract, Lua_mf(OnClose))
	~MainFrame()
	{
		wxFrame::SetMenuBar({});
	}

	void OnEvtClose(wxCloseEvent& e)
	{
		bool b{};
		Terminal::Lua().GetValue(m_name.c_str(), "OnClose", LuaObjCall(), &b);
		if (b)
			e.Skip();
		else
			e.Veto();
	}

	bool OnClose()
	{
		return true;
	}

	void SetTitle(LString title) override
	{
		wxFrame::SetTitle(title.c_str());
	}

	void AddPageWnd(wxWindow* w, const wchar_t* title) override
	{
		m_nb->AddPage(w, title);
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