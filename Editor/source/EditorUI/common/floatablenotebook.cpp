#include "uiutils.h"
#include "FloatableNotebook.h"
#include "wx/aui/dockart.h"
#include "wx/dcgraph.h"

#define wxFRAME (wxRESIZE_BORDER | wxCAPTION | wxCLOSE_BOX)

wxDEFINE_EVENT(EVT_FLOATABLE_NOTEBOOK, FnbEvent);

FloatableNotebook::FloatableNotebook(wxWindow* parent,
	Cloneable<wxAuiManager>* mgr,
	wxWindowID id,
	const wxPoint& pos,
	const wxSize& size,
	long style) :
	wxAuiNotebook2(parent, mgr, id, pos, size, style)
{
	m_root = std::make_shared<Root>(Root(this));
	m_mgrCB = mgr;
	Init();
}

FloatableNotebook::FloatableNotebook(wxWindow* parent,
	std::shared_ptr<Root>& root,
	Cloneable<wxAuiManager>* mgr,
	wxWindowID id,
	const wxPoint& pos,
	const wxSize& size,
	long style) :
	wxAuiNotebook2(parent, mgr, id, pos, size, style)
{
	m_root = root;
	m_mgrCB = mgr;
	Init();
}

void FloatableNotebook::Init()
{
	m_frame = nullptr;
	m_locatingTab = nullptr;
	m_deletingTab = nullptr;
	m_lastHintNB = nullptr;
	m_bShowFullHint = false;

	m_root->NBs.insert(this);
	SetArtProvider(new wxAuiGenericTabArt);
	m_mgr.SetFlags(m_mgr.GetFlags() | wxAUI_MGR_LIVE_RESIZE);

	m_topLevelWnd = nullptr;
	BindTopLevelWnd(GetParent());

	SetWindowStyleFlag(GetWindowStyleFlag() & ~wxAUI_NB_CLOSE_ON_ACTIVE_TAB);
}

bool FloatableNotebook::Reparent(wxWindowBase* parent)
{
	bool b = __super::Reparent(parent);
	BindTopLevelWnd(GetParent());
	FnbEvent e(this, GetId());
	ProcessEvent(e);
	return b;
}

void FloatableNotebook::BindTopLevelWnd(wxWindow* parent)
{
	if (m_topLevelWnd)
	{
		//m_topLevelWnd->Unbind(wxEVT_ICONIZE, &FloatableNotebook::OnMinimized, this);
		//m_topLevelWnd->Unbind(wxEVT_SIZE, &FloatableNotebook::OnSize, this);
		m_topLevelWnd->Unbind(wxEVT_SHOW, &FloatableNotebook::OnShow, this);
		m_topLevelWnd = nullptr;
	}
	parent = wxGetTopLevelParent(parent);
	if (parent)
	{
		m_topLevelWnd = parent;
		//m_topLevelWnd->Bind(wxEVT_ICONIZE, &FloatableNotebook::OnMinimized, this);
		//m_topLevelWnd->Bind(wxEVT_SIZE, &FloatableNotebook::OnSize, this);
		m_topLevelWnd->Bind(wxEVT_SHOW, &FloatableNotebook::OnShow, this);
	}
}

void FloatableNotebook::OnPageAdded(wxWindow* page)
{
	if (GetPageCount() > 1)
		SetWindowStyleFlag(GetWindowStyleFlag() |
			wxAUI_NB_CLOSE_ON_ALL_TABS);
	else
		SetWindowStyleFlag(GetWindowStyleFlag() &
			~wxAUI_NB_CLOSE_ON_ALL_TABS);
}

FloatableNotebook::~FloatableNotebook()
{
	m_root->NBs.erase(this);
}

void FloatableNotebook::AddPage(wxWindow* page,
	const wxString& caption,
	bool select,
	const wxBitmap& bitmap)
{
	wxAuiNotebook2::AddPage(page, caption, select, bitmap);
	PageInfo& pi = m_root->pageInfos[page];
	pi.nb = this;
	if (!pi.nbTop)
		pi.nbTop = pi.nb;

	wxAuiTabCtrl* tab;
	int idx;
	if (FindTab(page, &tab, &idx))
		pi.info = tab->GetPage(idx);

	OnPageAdded(page);
}

void FloatableNotebook::AddPage(PageInfo& pi)
{
	wxAuiNotebook2::AddPage(pi.info.window, pi.info.caption, pi.info.active, pi.info.bitmap);
	pi.nb = this;
	if (!pi.nbTop)
		pi.nbTop = pi.nb;
	OnPageAdded(pi.info.window);
}

wxWindow* FloatableNotebook::AddFloatPage(wxWindow* page,
	const wxString& caption,
	const wxPoint& pos,
	const wxSize& size,
	bool bShow,
	const wxBitmap& bitmap)
{
	PageInfo& pi = m_root->pageInfos[page];
	if (pi.nbTop == nullptr)
		pi.nbTop = this;
	FloatPageFrame* pFrame = new FloatPageFrame(pi.nbTop->m_topLevelWnd, wxID_ANY, m_root->rootNB, caption, pos, size);
	pi.nbTop->Bind(wxEVT_SHOW, &FloatPageFrame::OnShow, pFrame);
	pi.nbTop->Bind(EVT_FLOATABLE_NOTEBOOK, &FloatPageFrame::OnNBTopReparent, pFrame);
	pFrame->m_nbTop = pi.nbTop;

	FloatableNotebook* nb = new FloatableNotebook(pFrame, m_root, m_mgrCB->Clone());
	nb->AddPage(page, caption, false, bitmap);
	nb->m_frame = pFrame;

	pFrame->m_mgr.AddPane(nb, wxAuiPaneInfo().CenterPane());
	pFrame->m_mgr.Update();
	if (bShow)
		pFrame->Show();
	return pFrame;
}

wxWindow* FloatableNotebook::AddFloatPage(PageInfo& pi, const wxPoint& pos, const wxSize& size)
{
	if (pi.nbTop == nullptr)
		pi.nbTop = this;
	FloatPageFrame* pFrame = new FloatPageFrame(pi.nbTop->m_topLevelWnd, wxID_ANY, m_root->rootNB, pi.info.caption, pos, size);
	pi.nbTop->Bind(wxEVT_SHOW, &FloatPageFrame::OnShow, pFrame);
	pi.nbTop->Bind(EVT_FLOATABLE_NOTEBOOK, &FloatPageFrame::OnNBTopReparent, pFrame);
	pFrame->m_nbTop = pi.nbTop;

	FloatableNotebook* nb = new FloatableNotebook(pFrame, m_root, m_mgrCB->Clone());
	nb->AddPage(pi);
	nb->m_frame = pFrame;

	pFrame->m_mgr.AddPane(nb, wxAuiPaneInfo().CenterPane());
	pFrame->m_mgr.Update();
	pFrame->Show();
	return pFrame;
}

bool FloatableNotebook::ShowPage(wxWindow* page)
{
	auto it = m_root->pageInfos.find(page);
	if (it == m_root->pageInfos.end())
		return false;

	wxAuiTabCtrl* tab;
	int idx;
	if (it->second.nb && it->second.nb->FindTab(page, &tab, &idx))
	{
		tab->SetActivePage(idx);
		tab->DoShowHide();
		tab->Refresh();
	}
	else
		AddFloatPage(it->second, wxPoint(wxSCREEN_CENTER_WH(500, 300)), wxSize(500, 300));

	if (it->second.nb->m_frame)
	{
		it->second.nb->m_frame->Show();
		it->second.nb->m_frame->SetFocus();
	}
	page->SetFocus();
	return true;
}

bool FloatableNotebook::IsPageShown(wxWindow* page)
{
	wxAuiTabCtrl* tab;
	int idx;
	auto it = m_root->pageInfos.find(page);
	return it != m_root->pageInfos.end() && 
		it->second.nb && 
		it->second.nb->FindTab(page, &tab, &idx) &&
		(!it->second.nb->m_frame || it->second.nb->m_frame->IsShown());
}

bool FloatableNotebook::ClosePage(wxWindow* page, bool remove)
{
	auto it = m_root->pageInfos.find(page);
	if (it == m_root->pageInfos.end())
		return false;

	if (it->second.nb)
	{
		if (!remove && it->second.nb->GetPageCount() == 1 && it->second.nb->m_frame)
		{
			it->second.nb->m_frame->Close();
			return true;
		}

		wxAuiTabCtrl* tab;
		int idx;
		if (it->second.nb->FindTab(page, &tab, &idx))
		{
			it->second.info = tab->GetPage(idx);
			it->second.nb->RemovePage(tab->GetIdxFromWindow(page));
			it->second.nb = nullptr;
			page->Reparent(m_root->rootNB);
			it->second.nb->UpdateHintWindowSize();

			if (it->second.nb->GetPageCount() == 0 && it->second.nb->m_frame)
				m_frame->Destroy();
		}
	}

	if (remove)
	{
		page->Reparent(nullptr);
		m_root->pageInfos.erase(it);
	}

	return true;
}

void FloatableNotebook::OnPageClose(wxAuiNotebookEvent& evt)
{
	if (GetPageCount() == 1 && m_frame)
		m_frame->Close();
	else
	{
		int selection = evt.GetSelection();
		wxWindow* close_wnd = m_tabs.GetWindowFromIdx(selection);
		PageInfo& pi = m_root->pageInfos[close_wnd];

		wxAuiTabCtrl* tab;
		int idx;
		if (FindTab(close_wnd, &tab, &idx))
			pi.info = tab->GetPage(idx);

		wxShowEvent e(close_wnd->GetId(), false);
		ProcessEvent(e);
		
		RemovePage(selection);
		pi.nb = nullptr;
		close_wnd->Reparent(m_root->rootNB);
		UpdateHintWindowSize();

		if (GetPageCount() < 2)
			SetWindowStyleFlag(GetWindowStyleFlag() & 
				~wxAUI_NB_CLOSE_ON_ALL_TABS);
	}

	evt.Veto();
}

void FloatableNotebook::HideHint()
{
	m_bShowFullHint = false;
	if (GetPageCount() == 0)
		Refresh();
	m_mgr.HideHint();
}

void FloatableNotebook::OnCaptureLost(wxMouseCaptureLostEvent& WXUNUSED(evt))
{
	if (m_deletingTab)
	{
		wxPendingDelete.Append(m_deletingTab);
		m_deletingTab = nullptr;
	}

	if (m_locatingTab)
	{
		m_locatingTab = nullptr;
		if (m_lastHintNB)
		{
			m_lastHintNB->HideHint();
			m_lastHintNB = nullptr;
		}
	}

	if (m_frame)
		m_frame->SetWindowStyle(m_frame->GetWindowStyle() & ~wxSTAY_ON_TOP | wxFRAME);
	wxSetCursor(wxCursor(wxCURSOR_ARROW));
}

void FloatableNotebook::OnLeftUp(wxMouseEvent& evt)
{
	if (m_frame)
		m_frame->SetWindowStyle(m_frame->GetWindowStyle() & ~wxSTAY_ON_TOP | wxFRAME);
	wxSetCursor(wxCursor(wxCURSOR_ARROW));

	if (GetCapture() == this)
		ReleaseMouse();

	if (m_deletingTab)
	{
		wxPendingDelete.Append(m_deletingTab);
		m_deletingTab = nullptr;
	}

	if (!m_locatingTab)
		return;

	if (m_lastHintNB)
	{
		m_lastHintNB->HideHint();
		m_lastHintNB = nullptr;
	}

	wxWindow* page = m_locatingTab->GetPage(0).window;
	int idx = m_locatingTab->GetIdxFromWindow(page);

	wxPoint pt = wxGetMousePosition();
	m_frame->Move(pt.x - 20, pt.y + 1);

	FloatableNotebook* nb = nullptr;

	wxWindow* pWnd = wxFindWindowAtPoint(pt);
	for (; pWnd != nullptr; pWnd = pWnd->GetParent())
	{
		wxAuiTabCtrl* pTabCtrl = nullptr;
		if (wxDynamicCast(pWnd, wxAuiTabCtrl))
		{
			pTabCtrl = (wxAuiTabCtrl*)pWnd;
			nb = (FloatableNotebook*)pWnd->GetParent();
		}
		else if (wxDynamicCast(pWnd, FloatableNotebook))
			nb = (FloatableNotebook*)pWnd;

		if (nb && nb->m_root == m_root)
		{
			pt = nb->ScreenToClient(pt);
			
			if (!pTabCtrl)
				pTabCtrl = nb->GetTabCtrlFromPoint(pt);
			if (pTabCtrl)
			{
				m_root->pageInfos[page].nb = nb;
				wxAuiNotebookEvent e(wxEVT_AUINOTEBOOK_END_DRAG, m_locatingTab->GetId());
				e.SetSelection(idx);
				e.SetOldSelection(e.GetSelection());
				e.SetEventObject(m_locatingTab);
				m_locatingTab->GetEventHandler()->ProcessEvent(e);
				
				nb->OnPageAdded(page);
			}
			else
			{
				wxRect rc = nb->m_mgr.CalculateHintRect(nb->m_dummyWnd, pt, wxPoint(0, 0));
				if (nb->GetPageCount() > 0 && rc.IsEmpty())
					break;

				wxAuiNotebookPage info = m_locatingTab->GetPage(idx);
				RemovePage(m_tabs.GetIdxFromWindow(info.window));

				wxAuiTabCtrl* destTab = nb->GetActiveTabCtrl();
				int active = nb->GetSelection();

				PageInfo& pi = m_root->pageInfos[info.window];
				pi.info = info;

				nb->Freeze();
				
				nb->AddPage(pi);
				if (!rc.IsEmpty())
				{
					wxAuiNotebookEvent e(wxEVT_AUINOTEBOOK_END_DRAG, destTab->GetId());
					e.SetSelection(nb->GetActiveTabCtrl()->GetIdxFromWindow(info.window));
					e.SetOldSelection(e.GetSelection());
					e.SetEventObject(nb->GetActiveTabCtrl());
					nb->GetActiveTabCtrl()->GetEventHandler()->ProcessEvent(e);
					nb->SetSelection(active);
				}

				nb->Thaw();
			}
			break;
		}
	}

	m_locatingTab = nullptr;

	if (GetPageCount() == 0)
	{
		m_frame->Destroy();

		if (nb && nb->m_frame)
			nb->m_frame->SetFocus();
	}
}

void FloatableNotebook::OnMotion(wxMouseEvent& evt)
{
	if (GetCapture() != this)
		return;

	wxPoint pt = wxGetMousePosition();
	if (!m_locatingTab)
	{
		m_frame->Move(pt.x - 20, pt.y - 10);
		return;
	}
	
	m_frame->Move(pt.x - 20, pt.y + 1);

	FloatableNotebook* nb = nullptr;
	
	wxWindow* pWnd = ::wxFindWindowAtPoint(pt);
	
	for (; pWnd != nullptr; pWnd = pWnd->GetParent())
	{
		if (!pWnd->GetClientRect().Contains(pWnd->ScreenToClient(pt)))
			break;

		wxAuiTabCtrl* pTabCtrl = nullptr;
		if (wxDynamicCast(pWnd, wxAuiTabCtrl))
		{
			pTabCtrl = (wxAuiTabCtrl*)pWnd;
			nb = (FloatableNotebook*)pWnd->GetParent();
		}
		else if (wxDynamicCast(pWnd, FloatableNotebook))
			nb = (FloatableNotebook*)pWnd;

		if (nb && nb->m_root == m_root)
		{
			pt = nb->ScreenToClient(pt);
			if (!pTabCtrl)
				pTabCtrl = nb->GetTabCtrlFromPoint(pt);
			
			if (pTabCtrl)
			{
				wxRect hint_rect = pTabCtrl->GetClientRect();
				pTabCtrl->ClientToScreen(&hint_rect.x, &hint_rect.y);
				nb->m_mgr.ShowHint(hint_rect);
			}
			else if (nb->GetPageCount() == 0)
			{
				if (nb == m_lastHintNB)
					return;

				wxRect hint_rect = nb->GetClientRect();
				nb->ClientToScreen(&hint_rect.x, &hint_rect.y);
				nb->m_mgr.ShowHint(hint_rect);

				//wxColour color = wxSystemSettings::GetColour(wxSYS_COLOUR_ACTIVECAPTION);

				//wxGCDC gcdc;
				//gcdc.SetGraphicsContext(wxGraphicsRenderer::GetDefaultRenderer()->CreateContext(nb));
				//gcdc.SetPen(*wxTRANSPARENT_PEN);
				//gcdc.SetBrush(wxBrush(wxColour(color.Red(), color.Green(), color.Blue(), 155)));

				//int w1 = nb->FromDIP(10);
				//int w2 = w1 * 2;
				//gcdc.DrawRectangle(0, 0, w1, hint_rect.height);
				//gcdc.DrawRectangle(w1, 0, hint_rect.width - w2, w1);
				//gcdc.DrawRectangle(hint_rect.width - w1, 0, w1, hint_rect.height);
				//gcdc.DrawRectangle(w1, hint_rect.height - w1, hint_rect.width - w2, w1);
			}
			else
				nb->m_mgr.DrawHintRect(nb->m_dummyWnd, pt, wxPoint(0, 0));
			break;
		}
	}

	if (m_lastHintNB && m_lastHintNB != nb)
		m_lastHintNB->HideHint();
	m_lastHintNB = nb;
}

void FloatableNotebook::OnTabDragMotion(wxAuiNotebookEvent& evt)
{
	if (evt.GetSelection() < 0)
		return;

	wxPoint screen_pt = ::wxGetMousePosition();
	wxPoint client_pt = ScreenToClient(screen_pt);

	wxAuiTabCtrl* src_tabs = (wxAuiTabCtrl*)evt.GetEventObject();
	wxAuiTabCtrl* dest_tabs = GetTabCtrlFromPoint(client_pt);

	wxWindow* page = src_tabs->GetWindowFromIdx(evt.GetSelection());

	if (dest_tabs == src_tabs)
	{
		if (src_tabs)
		{
			src_tabs->SetCursor(wxCursor(wxCURSOR_ARROW));
		}

		// if tab moving is not allowed, leave
		if (!(m_flags & wxAUI_NB_TAB_MOVE))
		{
			return;
		}

		wxPoint pt = dest_tabs->ScreenToClient(screen_pt);
		wxWindow* dest_location_tab;

		// this is an inner-tab drag/reposition
		if (dest_tabs->TabHitTest(pt.x, pt.y, &dest_location_tab))
		{
			int src_idx = evt.GetSelection();
			int dest_idx = dest_tabs->GetIdxFromWindow(dest_location_tab);

			// prevent jumpy drag
			if ((src_idx == dest_idx) || dest_idx == -1 ||
				(src_idx > dest_idx && m_lastDragX <= pt.x) ||
				(src_idx < dest_idx && m_lastDragX >= pt.x))
			{
				m_lastDragX = pt.x;
				return;
			}


			wxWindow* src_tab = dest_tabs->GetWindowFromIdx(src_idx);
			dest_tabs->MovePage(src_tab, dest_idx);
			m_tabs.MovePage(m_tabs.GetPage(src_idx).window, dest_idx);
			dest_tabs->SetActivePage((size_t)dest_idx);
			dest_tabs->DoShowHide();
			dest_tabs->Refresh();
			m_lastDragX = pt.x;
			return;
		}
	}

	if (GetCapture())
		GetCapture()->ReleaseMouse();

	if (GetPageCount() < 2 && m_frame)
	{
		m_locatingTab = src_tabs;

		m_frame->SetWindowStyle(m_frame->GetWindowStyle() & ~wxFRAME | wxSTAY_ON_TOP);
		m_frame->Move(screen_pt.x - 20, screen_pt.y + 1);

		SetFocus();
		CaptureMouse();

		wxSetCursor(wxCursor(wxCURSOR_SIZING));
	}
	else if (GetPageCount() > 1)
	{
		PageInfo& pi = m_root->pageInfos[page];
		pi.info = src_tabs->GetPage(evt.GetSelection());
		AddFloatPage(pi, wxPoint(screen_pt.x - 20, screen_pt.y - 10), page->GetSize());
		
		bool isShown = page->IsShown();
		
		//Hide();

		RemovePage(m_tabs.GetIdxFromWindow(page));
		UpdateHintWindowSize();

		if (GetPageCount() < 2)
			SetWindowStyleFlag(GetWindowStyleFlag() & 
				~wxAUI_NB_CLOSE_ON_ALL_TABS);

		//Show();
		
		page->Show(isShown);
			
		if (src_tabs->GetPageCount() == 0 && wxPendingDelete.Member(src_tabs))
		{
			src_tabs->Hide();
			wxPendingDelete.remove(src_tabs);
			m_deletingTab = src_tabs;
		}

		pi.nb->SetFocus();
		pi.nb->CaptureMouse();
	}
}

void FloatableNotebook::OnPageChanged(wxAuiNotebookEvent& evt)
{
	if (m_frame)
		m_frame->SetLabel(GetPageText(GetActiveTabCtrl()->GetActivePage()));
}

void FloatableNotebook::RemoveAllPages()
{
	Freeze();
	while (GetPageCount() > 0)
	{
		PageInfo& pi = m_root->pageInfos[GetPage(0)];
		pi.nb = nullptr;
		GetPage(0)->Reparent(m_root->rootNB);
		RemovePage(0);
	}
	Thaw();
}

wxString FloatableNotebook::SavePerspective()
{
	if (this != m_root->rootNB)
		return m_root->rootNB->SavePerspective();

	wxString result;
	result = wxT("notebook_layout0/");
	for (auto it = m_root->NBs.begin(); it != m_root->NBs.end(); it++)
	{
		FloatPageFrame* p = (*it)->m_frame;
		if (p)
		{
			result += "float:";
			result += wxString().Format("%s;%d;%d;%d;%d;%d|",
				p->GetLabel(),
				p->IsShown(),
				p->GetPosition().x,
				p->GetPosition().y,
				p->GetSize().GetWidth(),
				p->GetSize().GetHeight());
		}
		result += (*it)->Save();
		result += "/";
	}

	return result;
}

wxString FloatableNotebook::Save()
{
	wxString result;
	bool first = true;
	for (int i = 0; i < m_mgr.GetAllPanes().Count(); i++)
	{
		wxAuiPaneInfo& pi = m_mgr.GetAllPanes()[i];
		if (pi.name == wxT("dummy"))
			continue;

		wxAuiTabCtrl* tab = GetTabCtrlFromPoint(pi.rect.GetPosition());
		if (!tab)
			continue;

		if (!first)
			result += "|";
		first = false;

		pi.Name(wxString().Format("%d", i));
		result += pi.name;
		result += "<";

		int n = 0;
		for (int j = 0; j < tab->GetPageCount(); j++)
		{
			if (j > 0)
				result += "*";
			wxAuiNotebookPage& np = tab->GetPage(j);
			result += np.window->GetName();
			if (np.active)
				n = j;
		}

		result += wxString().Format(">%d", n);
	}
	result += "|*|";
	return result + m_mgr.SavePerspective();
}

bool FloatableNotebook::LoadPerspective(const wxString& s)
{
	if (this != m_root->rootNB)
		return m_root->rootNB->LoadPerspective(s);

	wxString part = s.BeforeFirst(wxT('/'));
	wxString input = s.AfterFirst(wxT('/'));
	part.Trim(true);
	part.Trim(false);
	if (part != wxT("notebook_layout0"))
		return false;

	for (auto it = m_root->NBs.begin(); it != m_root->NBs.end(); it++)
	{
		(*it)->RemoveAllPages();
		if ((*it)->m_frame)
			(*it)->m_frame->Destroy();
	}
	m_root->NBs.clear();
	m_root->NBs.insert(this);

	m_root->pageLookup.clear();
	for (auto it = m_root->pageInfos.begin(); it != m_root->pageInfos.end(); it++)
		m_root->pageLookup[it->second.info.window->GetName()] = it->second.info.window;

	while (true)
	{
		part = input.BeforeFirst(wxT('/'));
		input = input.AfterFirst(wxT('/'));
		part.Trim(true);

		if (part.empty())
			break;

		FloatableNotebook* nb = this;

		wxString caption;
		long show, x, y, w, h;
		if (part.StartsWith("float:", &part))
		{
			caption = part.BeforeFirst(wxT(';'));
			part = part.AfterFirst(wxT(';'));
			part.BeforeFirst(wxT(';')).ToLong(&show);
			part = part.AfterFirst(wxT(';'));
			part.BeforeFirst(wxT(';')).ToLong(&x);
			part = part.AfterFirst(wxT(';'));
			part.BeforeFirst(wxT(';')).ToLong(&y);
			part = part.AfterFirst(wxT(';'));
			part.BeforeFirst(wxT(';')).ToLong(&w);
			part = part.AfterFirst(wxT(';'));
			part.BeforeFirst(wxT('|')).ToLong(&h);
			part = part.AfterFirst(wxT('|'));

			FloatPageFrame* pFrame = new FloatPageFrame(m_topLevelWnd, 
				wxID_ANY, m_root->rootNB, caption, wxPoint(x, y), wxSize(w, h));
			nb = new FloatableNotebook(pFrame, m_root, m_mgrCB->Clone());
			nb->m_frame = pFrame;
			pFrame->m_mgr.AddPane(nb, wxAuiPaneInfo().CenterPane());
			pFrame->m_mgr.Update();
			pFrame->Show();
		}

		nb->Freeze();
		
		if (!nb->Load(part) && nb->m_frame)
			nb->m_frame->Destroy();
		else if (nb->m_frame)
			nb->m_frame->Show(show);
		
		nb->Thaw();
	}

	return true;
}

bool FloatableNotebook::Load(const wxString& s)
{
	wxString input = s;

	std::vector<int> actives;

	int pageAdded = 0;
	int frameAdded = 1;
	while (true)
	{
		wxString part = input.BeforeFirst(wxT('|'));
		input = input.AfterFirst(wxT('|'));
		part.Trim(true);

		if (part.empty() || part == wxT("*"))
			break;

		wxString paneName = part.BeforeFirst(wxT('<'));
		if (paneName.empty())
			continue;
		wxString pageNames = part.AfterFirst(wxT('<'));
		if (pageNames.empty())
			continue;

		bool isAdded = false;
		while (!pageNames.empty())
		{
			wxString name = pageNames.BeforeFirst(wxT('*'));

			auto it = m_root->pageLookup.find(name);
			if (it == m_root->pageLookup.end())
			{
				name = pageNames.BeforeFirst(wxT('>'));
				it = m_root->pageLookup.find(name);
			}

			if (it != m_root->pageLookup.end())
			{
				AddPage(m_root->pageInfos[it->second]);
				if (!isAdded)
				{
					Split(pageAdded, wxRIGHT);
					m_mgr.GetAllPanes()[frameAdded++].Name(paneName);
					isAdded = true;

					unsigned long n = 0;
					pageNames.AfterFirst(wxT('>')).ToULong(&n);
					actives.push_back(n);
				}
				pageAdded++;
			}
			pageNames = pageNames.AfterFirst(wxT('*'));
		}
	}
	m_mgr.LoadPerspective(input);

	wxWindow* first_good = nullptr;
	bool center_found = false;
	for (int i = 0, j = 0; i < m_mgr.GetAllPanes().Count(); i++)
	{
		wxAuiPaneInfo& p = m_mgr.GetAllPanes()[i];
		if (p.name == wxT("dummy"))
			continue;

		if (p.dock_direction == wxAUI_DOCK_CENTRE)
			center_found = true;
		if (!first_good)
			first_good = p.window;

		wxAuiTabCtrl* tab = GetTabCtrlFromPoint(p.rect.GetPosition());
		if (tab)
		{
			tab->SetActivePage(actives[j]);
			tab->DoShowHide();
			tab->Refresh();
		}
		j++;
	}

	if (!center_found && first_good)
		m_mgr.GetPane(first_good).Centre();

	m_mgr.Update();
	
	return true;
}

void FloatableNotebook::OnAllowNotebookDnD(wxAuiNotebookEvent& evt)
{
	evt.Allow();
}

void FloatableNotebook::OnIdle(wxIdleEvent&)
{
	if (m_root->rootNB != this)
		m_root->rootNB->m_mgrCB->SetUp(m_mgrCB);
}

void FloatableNotebook::OnTabRightUp(wxAuiNotebookEvent& evt)
{

}

void FloatableNotebook::OnMinimized(wxIconizeEvent& e)
{
	if (m_topLevelWnd && e.GetId() == m_topLevelWnd->GetId())
	{
		wxShowEvent e2(GetId(), false);
		ProcessEvent(e2);
	}
	e.Skip();
}

void FloatableNotebook::OnSize(wxSizeEvent& e)
{
	if (m_topLevelWnd && e.GetId() == m_topLevelWnd->GetId())
	{
		wxShowEvent e2(GetId(), true);
		ProcessEvent(e2);
	}
	e.Skip();
}

void FloatableNotebook::OnShow(wxShowEvent& e)
{
	if (m_topLevelWnd && e.GetId() == m_topLevelWnd->GetId())
	{
		wxShowEvent e2(GetId(), e.IsShown());
		ProcessEvent(e2);
	}
	e.Skip();
}

wxBEGIN_EVENT_TABLE(FloatableNotebook, wxAuiNotebook2)

EVT_AUINOTEBOOK_ALLOW_DND(wxID_ANY, FloatableNotebook::OnAllowNotebookDnD)

EVT_AUINOTEBOOK_PAGE_CLOSE(wxID_ANY, FloatableNotebook::OnPageClose)

EVT_AUINOTEBOOK_PAGE_CHANGED(wxID_ANY, FloatableNotebook::OnPageChanged)

EVT_AUINOTEBOOK_DRAG_MOTION(wxID_ANY, FloatableNotebook::OnTabDragMotion)

EVT_MOUSE_CAPTURE_LOST(FloatableNotebook::OnCaptureLost)

EVT_MOTION(FloatableNotebook::OnMotion)

EVT_PAINT(FloatableNotebook::OnPaint)

EVT_LEFT_UP(FloatableNotebook::OnLeftUp)

EVT_SET_FOCUS(FloatableNotebook::OnFocus)

EVT_IDLE(FloatableNotebook::OnIdle)

EVT_AUINOTEBOOK_TAB_RIGHT_UP(wxID_ANY, FloatableNotebook::OnTabRightUp)

wxEND_EVENT_TABLE()

FloatPageFrame::FloatPageFrame(wxWindow *parent, wxWindowID id, wxWindow* focusWhenHide,
	const wxString& title, const wxPoint& pos, const wxSize& size) :
	wxFrame(parent, id, title, pos, size,
		wxRESIZE_BORDER | wxCAPTION | wxCLOSE_BOX | wxNO_BORDER |
		wxFRAME_NO_WINDOW_MENU | wxFRAME_NO_TASKBAR |
		wxFRAME_FLOAT_ON_PARENT | wxCLIP_CHILDREN)
{
	m_isClosed = true;
	m_mgr.SetManagedWindow(this);
	m_focus = focusWhenHide;
	m_nbTop = {};
}

bool FloatPageFrame::Show(bool show)
{
	if (show)
		m_isClosed = false;
	return __super::Show(show);
}

void FloatPageFrame::OnClose(wxCloseEvent& e)
{
	Hide();
	m_focus->SetFocus();
	m_isClosed = true;
}

void FloatPageFrame::OnNBTopReparent(FnbEvent& e)
{
	wxWindow* top = e.nb->m_topLevelWnd;
	if (top != GetParent())
		Reparent(top);
}

void FloatPageFrame::OnShow(wxShowEvent& e)
{
	if (e.GetId() == m_nbTop->GetId())
		if (e.IsShown() && !m_isClosed)
			__super::Show();
		else if (!e.IsShown())
			Hide();

	e.Skip();
}

wxBEGIN_EVENT_TABLE(FloatPageFrame, wxFrame)

EVT_CLOSE(FloatPageFrame::OnClose)

wxEND_EVENT_TABLE()