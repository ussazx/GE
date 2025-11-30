#pragma once
#include "vmWindow.h"

class Entrance : public wxDialog, public vmFrame
{
public:
	Entrance(wxWindow* parent, const wxString& title) : vmFrame(this)
	{
		if (!wxDialog::Create(GetParentForModalDialog(parent, 0),
			wxID_ANY, title, wxPoint(wxSCREEN_CENTER_WH(1000, 800)), wxSize(1100, 800),
			wxCAPTION | wxCLOSE_BOX | wxMINIMIZE_BOX | wxRESIZE_BORDER |
			wxFRAME_NO_WINDOW_MENU))
			return;

		SetBackgroundStyle(wxBG_STYLE_PAINT);

		wxBoxSizer* s = new wxBoxSizer(wxVERTICAL);
		SetSizer(s);

		m_nb = new wxAuiNotebook2(this, wxID_ANY);
		m_nb->SetArtProvider(new wxAuiGenericTabArt);
		m_nb->SetTabCtrlHeight(50);
		m_nb->SetWindowStyleFlag(m_nb->GetWindowStyleFlag() &
			~(wxAUI_NB_CLOSE_BUTTON |
				wxAUI_NB_CLOSE_ON_ACTIVE_TAB |
				wxAUI_NB_CLOSE_ON_ALL_TABS |
				wxAUI_NB_TAB_MOVE |
				wxAUI_NB_TAB_SPLIT));
		s->Add(m_nb, 1, wxEXPAND);
	}

	void AddPageWnd(wxWindow* w, const wchar_t* title) override
	{
		m_nb->AddPage(w, title);
	}

	void Accept() override
	{
		AcceptAndClose();
	}

	void Reject() override
	{
		Close();
	}

private:
	wxAuiNotebook2* m_nb;
};