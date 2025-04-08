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

	void AddPageWnd(vmWindow* w, const char* title) override
	{
		m_nb->AddPage(w, _(title));
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