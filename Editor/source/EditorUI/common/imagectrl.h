#pragma once
#include "wx/statbmp.h"

class ImageLabel
{
public:
	ImageLabel(wxWindow *parent, const wxImage& image = wxImage());
};

class ImageCtrl : public wxStaticBitmap
{
public:
	ImageCtrl(wxWindow *parent,
		const wxImage& image = wxImage(),
		wxWindowID id = wxID_ANY,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize,
		long style = 0,
		const wxString& name = wxASCII_STR(wxStaticBitmapNameStr))
		: wxStaticBitmap(parent, id, wxBitmap(), pos, size, style, name),
		m_image(image), m_size(size)
	{
		SetImage(image, size);
		Bind(wxEVT_SIZE, &ImageCtrl::OnSize, this);
		Bind(wxEVT_ERASE_BACKGROUND, &ImageCtrl::OnEraseBackground, this);
	}
	
	void SetImage(const wxImage& image, const wxSize& size = wxDefaultSize)
	{
		m_image = image;
		if (m_image.IsOk() && m_size.GetWidth() > 0 && m_size.GetHeight() > 0)
			if (size != m_image.GetSize())
				SetBitmap(wxBitmap(m_image.Scale(m_size.GetWidth(), m_size.GetHeight())));
			else
				SetBitmap(wxBitmap(m_image));
		else
			SetBitmap(wxBitmap());
	}

protected:
	void OnEraseBackground(wxEraseEvent&)
	{

	}
	void OnSize(wxSizeEvent& e)
	{
		if (m_image.IsOk() && GetRect().GetWidth() > 0 && GetRect().GetHeight() > 0
			&& GetRect().GetSize() != m_size)
		{
			m_size = GetRect().GetSize();
			SetBitmap(wxBitmap(m_image.Scale(m_size.GetWidth(), m_size.GetHeight())));
		}
		e.Skip();
	}
	wxSize m_size;
	wxImage m_image;
};