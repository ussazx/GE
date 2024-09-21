#pragma once
#include "wx/button.h"

class Button : public wxButton
{
public:
	Button(wxWindow *parent,
		wxWindowID id,
		const wxString& label = wxEmptyString,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize,
		long style = 0,
		const wxValidator& validator = wxDefaultValidator,
		const wxString& name = wxASCII_STR(wxButtonNameStr))
		: wxButton(parent, id, label, pos, size, style, validator, name)
	{
		SetBackgroundColour(wxColor(230, 230, 230));
		Bind(wxEVT_SET_FOCUS, &Button::OnFocus, this);
	}
protected:
	void OnFocus(wxFocusEvent&) {}
};
