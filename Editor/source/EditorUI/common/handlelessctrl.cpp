#include "handlelessctrl.h"
#include "uiutils.h"

HandlelessApp HandlelessApp::m_app;

void HandlelessApp::SetFocus(HandlelessCtrl* ctrl)
{
	if (m_app.m_focusedCtrl)
	{
		//wxFocusEvent e(wxEVT_KILL_FOCUS, m_app.m_focusedCtrl->GetId());
		//m_app.m_focusedCtrl->eventHandler.ProcessEvent(e);
	}
	m_app.m_focusedCtrl = ctrl;
	//wxFocusEvent e(wxEVT_SET_FOCUS, m_app.m_focusedCtrl->GetId());
	//m_app.m_focusedCtrl->eventHandler.ProcessEvent(e);
}
HandlelessCtrl* HandlelessApp::GetFocus()
{
	return m_app.m_focusedCtrl;
}
void HandlelessApp::Add(HandlelessCtrl* ctrl)
{
	m_app.m_objects.insert(ctrl);
}
void HandlelessApp::Remove(HandlelessCtrl* ctrl)
{
	if (m_app.m_focusedCtrl == ctrl)
		m_app.m_focusedCtrl = nullptr;
	m_app.m_objects.erase(ctrl);
}
void HandlelessApp::AddObject(wxObject* object)
{
	m_app.m_objects.insert(object);
}
bool HandlelessApp::FindObject(wxObject* object)
{
	return m_app.m_objects.find(object) != m_app.m_objects.end();
}
void HandlelessApp::RemoveObject(wxObject* object)
{
	m_app.m_objects.erase(object);
}

//----------HandlelessCtrl----------

HandlelessCtrl::HandlelessCtrl(HandlelessCtrl* parent, int id, bool show, const wxPoint& pos,
	const wxSize& size) : m_parent(parent), m_internalSizer(nullptr), sizerChecker(nullptr), skipMouseEvt(false)
{
	m_container = m_parent ? m_parent->m_container : nullptr;

	backgroundColor = *wxWHITE;

	HandlelessApp::Add(this);

	m_location = pos;
	if (m_parent)
	{
		m_location += m_parent->m_location;
		m_parent->m_ctrls.push_back(this);
	}
	
	m_rect.SetPosition(pos);
	m_rect.SetSize(size);

	Bind(EVT_HC_PAINT, &HandlelessCtrl::OnPaint, this);

	m_show = show;
	if (m_show)
		Refresh();
}

void HandlelessCtrl::Show(bool show)
{
	if (m_show != show)
		Refresh();
	m_show = show;
}

int HandlelessCtrl::GetId() 
{ 
	return m_id; 
}

HandlelessCtrl::~HandlelessCtrl()
{
	HandlelessApp::Remove(this);
	if (m_parent)
		m_parent->RemoveChild(this);

	if (sizerChecker)
	{
		sizerChecker->sizer->GetChildren().DeleteObject(sizerChecker->sizerItem);
		delete sizerChecker->sizerItem;
		delete sizerChecker;
	}

	for (size_t i = 0; i < m_ctrls.size(); i++)
	{
		m_ctrls[i]->m_parent = nullptr;
		delete m_ctrls[i];
	}
	if (m_internalSizer)
		delete m_internalSizer;

	Show(false);
}
bool HandlelessCtrl::RemoveChild(HandlelessCtrl* ctrl)
{
	for (auto it = m_ctrls.begin(); it != m_ctrls.end(); it++)
		if (*it == ctrl)
		{
			(*it)->m_parent = nullptr;
			m_ctrls.erase(it);
			return true;
		}
	return false;
}

wxSizerItem* HandlelessCtrl::JoinSizer(wxSizer* sizer, int proportion, int flag, int border)
{
	return JoinSizer(sizer->GetItemCount(), sizer, proportion, flag, border);
}

wxSizerItem* HandlelessCtrl::JoinSizer(wxSizer* sizer, const wxSizerFlags& flags)
{
	return JoinSizer(sizer->GetItemCount(), sizer, flags.GetProportion(), flags.GetFlags(), flags.GetBorderInPixels());
}

wxSizerItem* HandlelessCtrl::JoinSizer(int pos, wxSizer* sizer, int proportion, int flag, int border)
{
	delete sizerChecker;
	sizerChecker = new SizerChecker(*this);
	sizerChecker->sizer = sizer;
	sizerChecker->sizerItem = sizer->Insert(pos, m_rect.width, m_rect.height, proportion, flag, border, sizerChecker);
	return sizerChecker->sizerItem;
}

wxSizerItem* HandlelessCtrl::JoinSizer(int pos, wxSizer* sizer, const wxSizerFlags& flags)
{
	return JoinSizer(pos, sizer, flags.GetProportion(), flags.GetFlags(), flags.GetBorderInPixels());
}

void HandlelessCtrl::SetSizer(wxSizer* sizer, bool deleteOld)
{
	if (m_internalSizer && deleteOld)
		delete m_internalSizer;
	m_internalSizer = sizer;
}

void HandlelessCtrl::SetSizerAndFit(wxSizer* sizer, bool deleteOld)
{
	SetSizer(sizer, deleteOld);
	SetSize(sizer->GetMinSize());
}

const wxPoint& HandlelessCtrl::GetLocation() const
{
	return m_location;
}

const wxRect& HandlelessCtrl::GetRect() const
{
	return m_rect;
}

wxRect HandlelessCtrl::GetLocationRect() const
{
	return wxRect(m_location, m_rect.GetSize());
}

void HandlelessCtrl::Move(int x, int y, bool refresh)
{
	wxRect oldRect = GetLocationRect();
	if (DoMove(x, y))
	{
		UpdateLocations();
		wxRect newRect;
		if (refresh)
			Invalidate(oldRect.Union(GetLocationRect()));
	}
}

void HandlelessCtrl::Move(const wxPoint& pt, bool refresh)
{
	Move(pt.x, pt.y, refresh);
}

void HandlelessCtrl::SetSize(int width, int height)
{
	bool refresh = DoSetSize(width, height);
	DoLayout(!refresh);
	if (refresh)
		Refresh();
}

void HandlelessCtrl::SetSize(const wxSize& size)
{
	SetSize(size.GetWidth(), size.GetHeight());
}

void HandlelessCtrl::Idle(wxIdleEvent& e)
{
	wxIdleEvent e2 = e;
	ProcessEvent(e2);
	for (int i = 0; i < m_ctrls.size(); i++)
		m_ctrls[i]->Idle(e);
}

void HandlelessCtrl::Layout()
{
	DoLayout(true);
}

HandlelessCtrl* HandlelessCtrl::LocateCtrl(int x, int y, std::function<bool(HandlelessCtrl*)> fSkip)
{
	if (!m_show || !wxRect(0, 0, m_rect.width, m_rect.height).Contains(x, y) || fSkip(this))
		return nullptr;

	for (int i = m_ctrls.size() - 1; i >= 0; i--)
	{
		HandlelessCtrl* ctrl = m_ctrls[i]->LocateCtrl(x - m_ctrls[i]->m_rect.x, y - m_ctrls[i]->m_rect.y);
		if (ctrl) return ctrl;
	}

	return this;
}

HandlelessCtrl* HandlelessCtrl::LocateCtrl(const wxPoint& pt, std::function<bool(HandlelessCtrl*)> fSkip)
{
	return LocateCtrl(pt.x, pt.y, fSkip);
}

void HandlelessCtrl::Refresh()
{
	Invalidate(wxRect(m_location, m_rect.GetSize()));
}

void HandlelessCtrl::DoLayout(bool redraw)
{
	if (m_internalSizer)
		m_internalSizer->SetDimension(0, 0, m_rect.GetWidth(), m_rect.GetHeight());

	for (int i = 0; i < m_ctrls.size(); i++)
	{
		HandlelessCtrl* ctrl = m_ctrls[i];

		wxRect oldRect = ctrl->GetLocationRect();

		bool moved = false, resized = false;
		if (ctrl->sizerChecker)
		{
			moved = ctrl->DoMove(ctrl->sizerChecker->sizerItem->GetRect().GetPosition());
			resized = ctrl->DoSetSize(ctrl->sizerChecker->sizerItem->GetRect().GetSize());
		}
		bool rectChanged = moved || resized;
		
		ctrl->UpdateLocations(false);
		
		ctrl->DoLayout(!rectChanged && redraw);

		if (rectChanged && redraw)
		{
			wxRect newRect;
			ctrl->Invalidate(oldRect.Union(ctrl->GetLocationRect()));
		}
	}
}

void HandlelessCtrl::SetFocus()
{
	HandlelessApp::SetFocus(this);
}

HandlelessCtrl* HandlelessCtrl::GetFocus()
{
	return HandlelessApp::GetFocus();
}

HandlelessCtrl* HandlelessCtrl::GetParent()
{
	return m_parent;
}

void HandlelessCtrl::Invalidate(const wxRect& rect)
{
	if (m_container)
		m_container->InvalidateRect(rect);
}

void HandlelessCtrl::OnPaint(HCPaintEvent& e)
{
	e.GetDC().SetBackground(wxBrush(backgroundColor));
	e.GetDC().Clear();
}

bool HandlelessCtrl::DoMove(int x, int y)
{
	if (m_rect.x != x || m_rect.y != y)
	{
		m_rect.x = x;
		m_rect.y = y;
		return true;
	}
	return false;
}

bool HandlelessCtrl::DoMove(const wxPoint& point)
{
	return DoMove(point.x, point.y);
}

bool HandlelessCtrl::DoSetSize(int width, int height)
{
	if (m_rect.width != width || m_rect.height != height)
	{
		m_rect.width = width;
		m_rect.height = height;
		wxSizeEvent e(m_rect.GetSize(), m_id);
		ProcessEvent(e);
		return true;
	}
	return false;
}

bool HandlelessCtrl::DoSetSize(const wxSize& size)
{
	return DoSetSize(size.GetWidth(), size.GetHeight());
}

void HandlelessCtrl::UpdateLocations(bool updateChildren)
{
	wxPoint prevLoc = m_location;
	m_location = m_rect.GetPosition();
	if (m_parent)
		m_location += m_parent->m_location;

	if (prevLoc != m_location)
	{
		wxMoveEvent e(m_location, m_id);
		ProcessEvent(e);
	}

	if (updateChildren)
		for (int i = 0; i < m_ctrls.size(); i++)
			m_ctrls[i]->UpdateLocations();
}

void HandlelessCtrl::Paint(wxDC& dc, const wxPoint& dcLocation, wxRect clipRect)
{
	if (!m_show)
		return;
	
	wxPoint paintPos = m_location - dcLocation;
	clipRect.Intersect(wxRect(paintPos, m_rect.GetSize()));
	if (clipRect.GetWidth() > 0 && clipRect.GetHeight() > 0)
	{
		dc.SetDeviceOrigin(0, 0);
		dc.DestroyClippingRegion();
		dc.SetClippingRegion(clipRect);
		dc.SetDeviceOrigin(paintPos.x, paintPos.y);
		dc.SetBackground(wxBrush(backgroundColor));
		
		HCPaintEvent e(&dc, m_id);
		ProcessEvent(e);
		
		for (int i = 0; i < m_ctrls.size(); i++)
			m_ctrls[i]->Paint(dc, dcLocation, clipRect);
	}
}

//----------HandlelessContainer----------

HandlelessContainer::HandlelessContainer(wxWindow* window)
	: m_wnd(nullptr), m_lastEnteredCtrl(nullptr), m_sizeEvt(nullptr), m_scrollEvt(nullptr)
{
	m_location = wxPoint();
	m_container = this;
	if (window)
		SetWindow(window);
}

HandlelessContainer::~HandlelessContainer()
{
	UnInit();
}

void HandlelessContainer::SetWindow(wxWindow* window)
{
	UnInit();
	m_firstPaint = true;
	m_wnd = window;
	m_rect = m_wnd->GetClientRect();
	m_wnd->PushEventHandler(&m_evtHandler);
	
	m_evtHandler.Bind(wxEVT_SIZE, &HandlelessContainer::OnSize, this);
	m_evtHandler.Bind(wxEVT_PAINT, &HandlelessContainer::OnPaint, this);
	m_evtHandler.Bind(wxEVT_ERASE_BACKGROUND, &HandlelessContainer::OnEraseBackground, this);
	m_evtHandler.Bind(wxEVT_IDLE, &HandlelessContainer::OnIdle, this);
	
	m_evtHandler.Bind(wxEVT_SCROLLWIN_TOP, &HandlelessContainer::OnScrollChanged, this);
	m_evtHandler.Bind(wxEVT_SCROLLWIN_BOTTOM, &HandlelessContainer::OnScrollChanged, this);
	m_evtHandler.Bind(wxEVT_SCROLLWIN_LINEUP, &HandlelessContainer::OnScrollChanged, this);
	m_evtHandler.Bind(wxEVT_SCROLLWIN_LINEDOWN, &HandlelessContainer::OnScrollChanged, this);
	m_evtHandler.Bind(wxEVT_SCROLLWIN_PAGEUP, &HandlelessContainer::OnScrollChanged, this);
	m_evtHandler.Bind(wxEVT_SCROLLWIN_PAGEDOWN, &HandlelessContainer::OnScrollChanged, this);
	m_evtHandler.Bind(wxEVT_SCROLLWIN_THUMBTRACK, &HandlelessContainer::OnScrollChanged, this);
	m_evtHandler.Bind(wxEVT_SCROLLWIN_THUMBRELEASE, &HandlelessContainer::OnScrollChanged, this);

	m_evtHandler.Bind(wxEVT_ENTER_WINDOW, &HandlelessContainer::OnMouseEvent, this);
	m_evtHandler.Bind(wxEVT_LEAVE_WINDOW, &HandlelessContainer::OnMouseEvent, this);
	m_evtHandler.Bind(wxEVT_LEFT_DOWN, &HandlelessContainer::OnMouseEvent, this);
	m_evtHandler.Bind(wxEVT_LEFT_UP, &HandlelessContainer::OnMouseEvent, this);
	m_evtHandler.Bind(wxEVT_MIDDLE_DOWN, &HandlelessContainer::OnMouseEvent, this);
	m_evtHandler.Bind(wxEVT_MIDDLE_UP, &HandlelessContainer::OnMouseEvent, this);
	m_evtHandler.Bind(wxEVT_RIGHT_DOWN, &HandlelessContainer::OnMouseEvent, this);
	m_evtHandler.Bind(wxEVT_RIGHT_UP, &HandlelessContainer::OnMouseEvent, this);
	m_evtHandler.Bind(wxEVT_MOTION, &HandlelessContainer::OnMouseEvent, this);
	m_evtHandler.Bind(wxEVT_LEFT_DCLICK, &HandlelessContainer::OnMouseEvent, this);
	m_evtHandler.Bind(wxEVT_MIDDLE_DCLICK, &HandlelessContainer::OnMouseEvent, this);
	m_evtHandler.Bind(wxEVT_RIGHT_DCLICK, &HandlelessContainer::OnMouseEvent, this);
	m_evtHandler.Bind(wxEVT_MOUSEWHEEL, &HandlelessContainer::OnMouseEvent, this);
	m_evtHandler.Bind(wxEVT_AUX1_DOWN, &HandlelessContainer::OnMouseEvent, this);
	m_evtHandler.Bind(wxEVT_AUX1_UP, &HandlelessContainer::OnMouseEvent, this);
	m_evtHandler.Bind(wxEVT_AUX1_DCLICK, &HandlelessContainer::OnMouseEvent, this);
	m_evtHandler.Bind(wxEVT_AUX2_DOWN, &HandlelessContainer::OnMouseEvent, this);
	m_evtHandler.Bind(wxEVT_AUX2_UP, &HandlelessContainer::OnMouseEvent, this);
	m_evtHandler.Bind(wxEVT_AUX2_DOWN, &HandlelessContainer::OnMouseEvent, this);
	m_evtHandler.Bind(wxEVT_MAGNIFY, &HandlelessContainer::OnMouseEvent, this);
}

void HandlelessContainer::UnInit()
{
	if (m_wnd)
	{
		m_wnd->RemoveEventHandler(&m_evtHandler);
		m_wnd = nullptr;
	}
}

void HandlelessContainer::InvalidateRect(const wxRect& rect)
{
	if (m_wnd && m_wnd->IsShown())
		m_wnd->Refresh(false, &rect);
}

bool HandlelessContainer::DoSetSize(int width, int height)
{
	bool ret = HandlelessCtrl::DoSetSize(width, height);
	if (ret && m_wnd->GetClientSize() != m_rect.GetSize())
		m_wnd->SetClientSize(width, height);
	return ret;
}

bool HandlelessContainer::DoMove(int x, int y)
{
	bool ret = HandlelessCtrl::DoMove(x, y);
	if (ret && m_wnd)
		m_wnd->Move(x, y);
	return ret;
}

void HandlelessContainer::ProcessMouseEvent(HandlelessCtrl* ctrl, wxMouseEvent& e, const wxEventType& evtType)
{
	if (evtType == wxEVT_ENTER_WINDOW)
	{
		m_wnd->UnsetToolTip();
		m_wnd->SetToolTip(m_lastEnteredCtrl->toolTip);
	}
	wxMouseEvent e2 = e;
	e2.SetEventObject(ctrl);
	e2.SetEventType(evtType);
	ctrl->ProcessEvent(e2);
}

void HandlelessContainer::OnMouseEvent(wxMouseEvent& e)
{
	HandlelessCtrl* lastEntered = HandlelessApp::FindObject(m_lastEnteredCtrl) ? m_lastEnteredCtrl : nullptr;
	
	if (e.GetEventType() == wxEVT_LEAVE_WINDOW)
	{
		if (lastEntered)
		{
			ProcessMouseEvent(lastEntered, e, e.GetEventType());
			lastEntered = nullptr;
		}
		return e.Skip();
	}

	m_lastEnteredCtrl = LocateCtrl(m_wnd->ScreenToClient(wxGetMousePosition()), [](HandlelessCtrl* ctrl) { return ctrl->skipMouseEvt; });
	if (!m_lastEnteredCtrl)
		return e.Skip();
	
	if (m_lastEnteredCtrl != lastEntered)
	{
		if (lastEntered)
			ProcessMouseEvent(lastEntered, e, wxEVT_LEAVE_WINDOW);
		
		if (e.GetEventType() != wxEVT_ENTER_WINDOW)
			ProcessMouseEvent(m_lastEnteredCtrl, e, wxEVT_ENTER_WINDOW);
	}

	ProcessMouseEvent(m_lastEnteredCtrl, e, e.GetEventType());

	e.Skip();
}

void HandlelessContainer::OnSize(wxSizeEvent& e)
{
	if (&e == m_sizeEvt)
		return e.Skip();

	m_sizeEvt = &e;
	m_wnd->GetEventHandler()->ProcessEvent(e);
	m_sizeEvt = nullptr;

	HandleScrollChanged(false);
	
	SetSize(m_wnd->GetClientRect().GetSize());
}

void HandlelessContainer::OnIdle(wxIdleEvent& e)
{
	Idle(e);
	e.Skip();
}

void HandlelessContainer::OnScrollChanged(wxScrollWinEvent& e)
{
	if (&e == m_scrollEvt)
		return e.Skip();

	m_scrollEvt = &e;
	m_wnd->GetEventHandler()->ProcessEvent(e);
	m_scrollEvt = nullptr;

	HandleScrollChanged(true);
}

void HandlelessContainer::HandleScrollChanged(bool updateLocation)
{
	if (m_wnd->GetScrollHelper())
	{
		wxPoint scrollPos;
		m_wnd->GetScrollHelper()->GetScrollPixelsPerUnit(&scrollPos.x, &scrollPos.y);
		scrollPos.x *= m_wnd->GetScrollPos(wxHORIZONTAL);
		scrollPos.y *= m_wnd->GetScrollPos(wxVERTICAL);

		if (updateLocation && m_scrollPos != scrollPos)
			for (int i = 0; i < m_ctrls.size(); i++)
				m_ctrls[i]->Move(m_ctrls[i]->GetRect().GetPosition() + m_scrollPos - scrollPos, false);

		m_scrollPos = scrollPos;
	}
}

void HandlelessContainer::OnEraseBackground(wxEraseEvent& e)
{
	if (m_firstPaint)
	{
		wxGCDC gcdc;
		gcdc.SetGraphicsContext(wxGraphicsRenderer::GetDefaultRenderer()->CreateContext(m_wnd));
		Paint(gcdc, wxPoint(), wxRect(wxPoint(), e.GetDC()->GetSize()));
		m_firstPaint = false;
	}
}

void HandlelessContainer::OnPaint(wxPaintEvent& e)
{
	if (m_scrollEvt)
		HandleScrollChanged(true);

	wxRect rect;
	wxRegion& region = m_wnd->GetUpdateRegion();
	region.GetBox(rect.x, rect.y, rect.width, rect.height);

	GCDCWndBlit dc(m_wnd, rect.GetSize());
	dc.destPoint = rect.GetPosition();
	rect.SetPosition(wxPoint());
	Paint(dc, dc.destPoint, rect);

	e.Skip();
}