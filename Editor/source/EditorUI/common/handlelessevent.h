#pragma once
#include "wx/wxprec.h"

class HCPaintEvent;

wxDECLARE_EVENT(EVT_HC_PAINT, HCPaintEvent);

class HCPaintEvent : public wxEvent
{
public:
	HCPaintEvent(wxDC* paintDC = nullptr, int winid = 0)
		: wxEvent(winid, EVT_HC_PAINT), m_dc(paintDC) {}
	virtual ~HCPaintEvent() {}
	wxEvent *Clone() const wxOVERRIDE { return new HCPaintEvent(*this); }
	wxDC& GetDC() const { return *m_dc; }

protected:
	wxDC* m_dc;
};

//class HCSizeEvent : public wxEvent
//{
//public:
//	HCSizeEvent(int winid = 0, wxEventType commandType = NH_EVT_SIZE)
//		: wxEvent(winid, commandType) {}
//	wxEvent *Clone() const wxOVERRIDE { return new HCSizeEvent(*this); }
//};

//class NHMouseEvent : public wxMouseEvent
//{
//public:
//	NHMouseEvent(wxEventType mouseType = wxEVT_NULL) : wxMouseEvent(mouseType) {}
//	NHMouseEvent(const wxMouseEvent& event) : wxMouseEvent(event) {}
//};

//class HCSizeEvent : public wxEvent
//{
//public:
//	HCSizeEvent() : wxEvent(0, NH_EVT_SIZE)
//	{ }
//	HCSizeEvent(const wxSize& sz, int winid = 0)
//		: wxEvent(winid, NH_EVT_SIZE),
//		m_size(sz)
//	{ }
//	HCSizeEvent(const HCSizeEvent& event)
//		: wxEvent(event),
//		m_size(event.m_size), m_rect(event.m_rect)
//	{ }
//	HCSizeEvent(const wxRect& rect, int id = 0)
//		: m_size(rect.GetSize()), m_rect(rect)
//	{
//		//m_eventType = wxEVT_SIZING;
//		m_eventType = NH_EVT_SIZE;
//		m_id = id;
//	}
//
//	wxSize GetSize() const { return m_size; }
//	void SetSize(wxSize size) { m_size = size; }
//	wxRect GetRect() const { return m_rect; }
//	void SetRect(const wxRect& rect) { m_rect = rect; }
//
//	virtual wxEvent *Clone() const wxOVERRIDE { return new HCSizeEvent(*this); }
//
//public:
//	wxSize m_size;
//	wxRect m_rect;
//
//private:
//	//wxDECLARE_DYNAMIC_CLASS_NO_ASSIGN(HCSizeEvent);
//};