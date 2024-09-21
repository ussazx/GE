#pragma once
#include "wx/wxprec.h"
#include "wx/richtooltip.h"
#include "wx/tipwin.h"

class QuickTextEntry : public wxTextCtrl
{
public:
	QuickTextEntry(wxWindow *parent, wxWindowID id,
		const wxString& value = wxEmptyString,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize,
		long style = wxTE_CENTER,
		const wxValidator& validator = wxDefaultValidator,
		const wxString& name = wxASCII_STR(wxTextCtrlNameStr))
		: wxTextCtrl(parent, id, value, pos, size, style, validator, name)
	{
		Hide();
		Bind(wxEVT_TEXT, &QuickTextEntry::OnText, this);
		Bind(wxEVT_CHAR, &QuickTextEntry::OnChar, this);
		Bind(wxEVT_KILL_FOCUS, &QuickTextEntry::OnKillFocus, this);
	}
	void ShowEntry(const wxString& value = wxEmptyString, bool selectAll = true)
	{
		SetValue(value);
		if (selectAll)
			SelectAll();
		Show();
		SetFocus();
	}
	void SetInvalidChars(const wxString& chars, const wxString& tip);
		
private:
	void OnKillFocus(wxFocusEvent& e)
	{
		e.Skip();
	}
	void OnText(wxCommandEvent& e)
	{

	}
	void OnChar(wxKeyEvent& e)
	{
		wxTipWindow* m_tipWindow = new wxTipWindow
		(
			this,
			"This is just some text to be shown in the tip "
			"window, broken into multiple lines, each less "
			"than 60 logical pixels wide.",
			FromDIP(200),
			&m_tipWindow
		);
		m_tipWindow->Move(100, 100);
	}
	wxString m_invalidChars;
};