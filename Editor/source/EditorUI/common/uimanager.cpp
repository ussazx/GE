#include "uimanager.h"
#include "wx/aui/dockart.h"

#include "wx/wxprec.h"
#ifndef WX_PRECOMP
#include "wx/statusbr.h"
#endif

#include <unordered_set>

UiManager::UiManager()
{
	SetExtraFlags(GetExtraFlags() | EntirelyLayoutResize | DockContiguousResize);
}

Cloneable<wxAuiManager>* UiManager::Clone()
{
	UiManager* mgr = new UiManager;
	SetUp(mgr);
	return mgr;
}
void UiManager::SetUp(Cloneable* target)
{
	UiManager* mgr = (UiManager*)target;
	mgr->SetFlags(GetFlags());
	mgr->SetExtraFlags(GetExtraFlags());
}

void UiManager::SetExtraFlags(unsigned int extraFlags)
{
	if (!(m_extraFlags & EntirelyLayoutResize))
	{
		for (int i = 0; i < m_docks.GetCount(); i++)
			SaveDockSize(&m_docks[i]);
	}
	m_extraFlags = extraFlags;
}

unsigned int UiManager::GetExtraFlags()
{
	return m_extraFlags;
}

void UiManager::SaveDockSize(const wxAuiDockInfo* dock)
{
	if (!m_frame)
		return;

	int frameSize = 0;

	switch (dock->dock_direction)
	{
	case wxAUI_DOCK_TOP:
	case wxAUI_DOCK_BOTTOM:
		frameSize = m_frame->GetClientSize().y;
		break;
	case wxAUI_DOCK_LEFT:
	case wxAUI_DOCK_RIGHT:
		frameSize = m_frame->GetClientSize().x;
		break;
	}

	if (frameSize > 0)
		m_dockSizes[dock] = { dock->size, frameSize };
}

void UiManager::DockResize(wxMouseEvent& e)
{
	wxAuiDockInfo* dock = m_actionPart ? m_actionPart->dock : nullptr;

	if (!(m_extraFlags & DockContiguousResize) ||
		m_actionPart->type != wxAuiDockUIPart::typeDockSizer)
	{
		wxAuiManager::DoEndResizeAction(e);
		if (dock)
			SaveDockSize(dock);
		return;
	}

	if (!dock)
		return;

	wxAuiDockInfo* dockNeighbor = nullptr;

	wxPoint newPos(e.m_x - m_actionOffset.x, e.m_y - m_actionOffset.y);
	int newSize = 0;
	int oldSize = m_actionPart->dock->size;
	
	int borderSize = m_art->GetMetric(wxAUI_DOCKART_PANE_BORDER_SIZE);
	int captionSize = m_art->GetMetric(wxAUI_DOCKART_CAPTION_SIZE);
	int sashSize = m_art->GetMetric(wxAUI_DOCKART_SASH_SIZE);

	if (dock->dock_direction == wxAUI_DOCK_TOP)
	{
		int dockY = m_frame->GetClientRect().GetHeight();
		for (int i = 0; i < m_docks.GetCount(); i++)
		{
			if (m_docks[i].rect.x >= dock->rect.GetLeft() &&
				m_docks[i].rect.x <= dock->rect.GetRight() &&
				m_docks[i].rect.y > dock->rect.y &&
				m_docks[i].rect.y <= dockY)
			{
				dockNeighbor = &m_docks[i];
				dockY = m_docks[i].rect.y;
			}
		}

		int upLimit = dock->rect.GetTop() + captionSize + 15;
		int downLimit = m_frame->GetClientRect().GetHeight();	
		if (dockNeighbor)
			downLimit = dockNeighbor->rect.GetBottom() - sashSize - captionSize - 15;
		
		if (newPos.y < upLimit)
			newPos.y = upLimit;
		else if (newPos.y > downLimit)
			newPos.y = downLimit;
		
		newSize = newPos.y - dock->rect.GetTop();
	}
	else if (dock->dock_direction == wxAUI_DOCK_BOTTOM)
	{
		int dockY = 0;
		for (int i = 0; i < m_docks.GetCount(); i++)
		{
			if (m_docks[i].rect.x >= dock->rect.GetLeft() && 
				m_docks[i].rect.x <= dock->rect.GetRight() &&
				m_docks[i].rect.y < dock->rect.y &&
				m_docks[i].rect.y >= dockY)
			{
				dockNeighbor = &m_docks[i];
				dockY = m_docks[i].rect.y;
			}
		}
		
		int upLimit = 0;
		int downLimit = dock->rect.GetBottom() - captionSize - 15;
		if (dockNeighbor)
			upLimit = dockNeighbor->rect.GetTop() + captionSize + 15;

		if (newPos.y < upLimit)
			newPos.y = upLimit;
		else if (newPos.y > downLimit)
			newPos.y = downLimit;

		newSize = dock->rect.GetBottom() - newPos.y - sashSize;
	}
	else if (dock->dock_direction == wxAUI_DOCK_LEFT)
	{
		int dockX = m_frame->GetClientRect().GetWidth();
		for (int i = 0; i < m_docks.GetCount(); i++)
		{
			if (m_docks[i].rect.y >= dock->rect.GetTop() &&
				m_docks[i].rect.y <= dock->rect.GetBottom() &&
				m_docks[i].rect.x > dock->rect.x &&
				m_docks[i].rect.x <= dockX)
			{
				dockNeighbor = &m_docks[i];
				dockX = m_docks[i].rect.x;
			}
		}

		int leftLimit = dock->rect.GetLeft() + borderSize * 2;
		int rightLimit = m_frame->GetClientRect().GetWidth();
		if (dockNeighbor)
			rightLimit = dockNeighbor->rect.GetRight() - sashSize - borderSize * 2;

		if (newPos.x < leftLimit)
			newPos.x = leftLimit;
		else if (newPos.x > rightLimit)
			newPos.x = rightLimit;

		newSize = newPos.x - dock->rect.GetLeft();
	}
	else if (dock->dock_direction == wxAUI_DOCK_RIGHT)
	{
		int dockX = 0;
		for (int i = 0; i < m_docks.GetCount(); i++)
		{
			if (m_docks[i].rect.y >= dock->rect.GetTop() &&
				m_docks[i].rect.y <= dock->rect.GetBottom() &&
				m_docks[i].rect.x < dock->rect.x &&
				m_docks[i].rect.x >= dockX)
			{
				dockNeighbor = &m_docks[i];
				dockX = m_docks[i].rect.x;
			}
		}

		int leftLimit = 0;
		int rightLimit = dock->rect.GetRight() - sashSize - borderSize * 2;
		if (dockNeighbor)
			leftLimit = dockNeighbor->rect.GetLeft() + borderSize * 2;

		if (newPos.x < leftLimit)
			newPos.x = leftLimit;
		else if (newPos.x > rightLimit)
			newPos.x = rightLimit;

		newSize = dock->rect.GetRight() - newPos.x - sashSize;
	}
	
	if (dockNeighbor && dockNeighbor->dock_direction == dock->dock_direction)
	{
		dockNeighbor->size -= newSize - oldSize;
		SaveDockSize(dockNeighbor);
	}

	dock->size = newSize;
	SaveDockSize(dock);

	Update();
}

void UiManager::OnLeftUp(wxMouseEvent& e)
{
	if (m_action == actionResize)
	{
		m_frame->ReleaseMouse();

		if (!HasLiveResize())
			m_frame->Refresh();

		if (m_currentDragItem != -1 && HasLiveResize())
			m_actionPart = &(m_uiParts.Item(m_currentDragItem));

		DockResize(e);

		m_currentDragItem = -1;

		m_action = actionNone;
		m_lastMouseMove = wxPoint();
	}
	else
		e.Skip();
}

void UiManager::OnMotion(wxMouseEvent& e)
{
	wxPoint mouse_pos = e.GetPosition();
	if (m_lastMouseMove == mouse_pos)
		return;

	if (m_action == actionResize)
	{
		wxAuiDockUIPart* actionPart = m_actionPart;
		int currentDragItem = m_currentDragItem;

		if (m_currentDragItem != -1)
			m_actionPart = &(m_uiParts.Item(m_currentDragItem));
		else
			m_currentDragItem = m_uiParts.Index(*m_actionPart);

		if (m_actionPart && HasLiveResize())
		{
			m_frame->ReleaseMouse();
			DockResize(e);
			m_frame->CaptureMouse();

			m_frame->Update();
			m_frame->Refresh();

			m_lastMouseMove = mouse_pos;

			return;
		}

		m_actionPart = actionPart;
		m_currentDragItem = currentDragItem;
	}

	e.Skip();
}

void UiManager::OnSize(wxSizeEvent& e)
{
	if (m_extraFlags & EntirelyLayoutResize)
	{
		for (int i = 0; i < m_uiParts.Count(); i++)
		{
			wxAuiDockUIPart& d = m_uiParts[i];
			auto it = m_dockSizes.find(d.dock);
			if (d.type == wxAuiDockUIPart::typeDock && d.dock->dock_direction != wxAUI_DOCK_CENTER)
			{
				if (it != m_dockSizes.end())
				{
					if (d.dock->dock_direction == wxAUI_DOCK_LEFT ||
						d.dock->dock_direction == wxAUI_DOCK_RIGHT)
						d.dock->size = m_frame->GetClientSize().x * it->second.dockSize / it->second.frameSize;
					else if (d.dock->dock_direction == wxAUI_DOCK_TOP ||
						d.dock->dock_direction == wxAUI_DOCK_BOTTOM)
						d.dock->size = m_frame->GetClientSize().y * it->second.dockSize / it->second.frameSize;
				}
				else
					SaveDockSize(d.dock);
			}
		}
		Update();
	}
	else
		wxAuiManager::OnSize(e);

	m_frame->Refresh();
}

wxBEGIN_EVENT_TABLE(UiManager, wxAuiManager)

EVT_LEFT_UP(UiManager::OnLeftUp)
EVT_MOTION(UiManager::OnMotion)
EVT_SIZE(UiManager::OnSize)

wxEND_EVENT_TABLE()