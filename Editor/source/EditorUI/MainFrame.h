#pragma once
#include "vmWindow.h"

class MainFrame : public wxFrame, public vmFrame
{
public:
	MainFrame(wxWindow* parent, wxWindowID id, const wxString& title)
		: wxFrame(parent, id, title, wxPoint(wxSCREEN_CENTER_WH(1000, 800)), wxSize(1100, 800),
			wxDEFAULT_FRAME_STYLE), vmFrame(this)
	{
		SetBackgroundStyle(wxBG_STYLE_PAINT);

		wxBoxSizer* s = new wxBoxSizer(wxVERTICAL);
		SetSizer(s);

		m_nb = new FloatableNotebook(this, new UiManager);
		s->Add(m_nb, 1, wxEXPAND);
	}
	~MainFrame()
	{
		wxFrame::SetMenuBar({});
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

	FloatableNotebook* m_nb;
};