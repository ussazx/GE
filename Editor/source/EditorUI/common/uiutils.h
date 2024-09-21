#pragma once
#include "wx/wxprec.h"
#include "wx/dcgraph.h"

inline void DebugLog(const wchar_t* szFmt, ...)
{
	wchar_t sz[4096]{};
	va_list argp;
	va_start(argp, szFmt);
	vswprintf_s(sz, 4096, szFmt, argp);
	va_end(argp);

	OutputDebugString(sz);
	OutputDebugString(L"\n");
}

#define wxSCREEN_CENTER_W(width)\
	((wxSystemSettings::GetMetric(wxSYS_SCREEN_X) - width) / 2)

#define wxSCREEN_CENTER_H(height)\
	((wxSystemSettings::GetMetric(wxSYS_SCREEN_Y) - height) / 2)

#define wxSCREEN_CENTER_WH(width, height)\
	((wxSystemSettings::GetMetric(wxSYS_SCREEN_X) - width) / 2),\
	((wxSystemSettings::GetMetric(wxSYS_SCREEN_Y) - height) / 2)

static wxColor AlphaBlend(const wxColor& A, const wxColor& B, unsigned char alphaA)
{
	unsigned char t = 255 - alphaA;
	return wxColor((A.Red() * alphaA + B.Red() * t) / 255,
		(A.Green() * alphaA + B.Green() * t) / 255,
		(A.Blue() * alphaA + B.Blue() * t) / 255);
}

static wxColor AlphaBlend(const wxColor& A, const wxColor& B)
{
	unsigned char t = 255 - A.Alpha();
	return wxColor((A.Red() * A.Alpha() + B.Red() * t) / 255,
		(A.Green() * A.Alpha() + B.Green() * t) / 255,
		(A.Blue() * A.Alpha() + B.Blue() * t) / 255);
}

class WXDLLIMPEXP_CORE ScrolledWindow : public wxScrolled<wxWindow>
{
public:
	ScrolledWindow() : wxScrolled<wxWindow>() {}
	ScrolledWindow(wxWindow *parent,
		wxWindowID winid = wxID_ANY,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize,
		long style = wxScrolledWindowStyle,
		const wxString& name = wxASCII_STR(wxPanelNameStr))
		: wxScrolled<wxWindow>(parent, winid, pos, size, style, name) {}

	wxDECLARE_NO_COPY_CLASS(ScrolledWindow);
};

class GCDCWndBlit : public wxGCDC
{
public:
	GCDCWndBlit(wxWindow* dstWnd, const wxSize& MdcSize = wxDefaultSize, wxGraphicsRenderer* render = wxGraphicsRenderer::GetDefaultRenderer())
	{
		m_wnd = dstWnd;
		destSize = dstWnd->GetClientSize();
		
		wxSize mdcSize;
		mdcSize.SetWidth(MdcSize.GetWidth() > 0 ? MdcSize.GetWidth() : destSize.GetWidth());
		mdcSize.SetHeight(MdcSize.GetHeight() > 0 ? MdcSize.GetHeight() : destSize.GetHeight());

		m_memBitmap = wxBitmap(mdcSize);
		m_mdc.SelectObject(m_memBitmap);
		SetGraphicsContext(render->CreateContext(m_mdc));

		//m_wnd->PrepareDC(m_mdc);

		rop = wxCOPY;
		useMask = false;
		srcPtMask = wxDefaultPosition;
		m_presented = false;
	}
	void ClearBg()
	{
		ClearBg(m_wnd->GetBackgroundColour());
	}
	void ClearBg(const wxColor& clearColor)
	{
		SetBackground(wxBrush(clearColor));
		Clear();
	}
	void Present(bool setPresented = true)
	{
		wxPaintDC pdc(m_wnd);
		pdc.Blit(destPoint, destSize, &m_mdc, srcPoint, rop, useMask, srcPtMask);
		m_presented = setPresented;
	}
	void Present(wxDC& dc, bool setPresented = true)
	{
		dc.Blit(destPoint, destSize, &m_mdc, srcPoint, rop, useMask, srcPtMask);
		m_presented = setPresented;
	}
	~GCDCWndBlit()
	{
		if (!m_presented)
		{
			wxPaintDC pdc(m_wnd);
			//m_wnd->PrepareDC(pdc);
			pdc.Blit(destPoint, destSize, &m_mdc, srcPoint, rop, useMask, srcPtMask);
		}
	}

	wxPoint destPoint;
	wxSize destSize;
	wxPoint srcPoint;
	wxRasterOperationMode rop;
	bool useMask;
	wxPoint srcPtMask;

protected:
	wxWindow* m_wnd;
	wxBitmap m_memBitmap;
	wxMemoryDC m_mdc;
	bool m_presented;
};

template<class T>
class Cloneable : public T
{
public:
	virtual ~Cloneable() {}
	virtual Cloneable* Clone()
	{
		return new Cloneable;
	}
	virtual void SetUp(Cloneable* target) {}
};