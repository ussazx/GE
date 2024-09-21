///////////////////////////////////////////////////////////////////////////////
// Name:        src/aui/auibook.cpp
// Purpose:     wxaui: wx advanced user interface - notebook
// Author:      Benjamin I. Williams
// Modified by: Jens Lody
// Created:     2006-06-28
// Copyright:   (C) Copyright 2006, Kirix Corporation, All Rights Reserved
// Licence:     wxWindows Library Licence, Version 3.1
///////////////////////////////////////////////////////////////////////////////

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#if wxUSE_AUI

#include "auibook2.h"

#ifndef WX_PRECOMP
    #include "wx/settings.h"
    #include "wx/dcclient.h"
    #include "wx/dcmemory.h"
    #include "wx/frame.h"
#endif

#include "wx/aui/tabmdi.h"

#ifdef __WXMAC__
#include "wx/osx/private.h"
#endif

#include "wx/arrimpl.cpp"

wxDEFINE_EVENT(wxEVT_AUINOTEBOOK_CANCEL_DRAG, wxAuiNotebookEvent);

// the utility function ShowWnd() is the same as show,
// except it handles wxAuiMDIChildFrame windows as well,
// as the Show() method on this class is "unplugged"
static void ShowWnd(wxWindow* wnd, bool show)
{
#if wxUSE_MDI
    if (wxDynamicCast(wnd, wxAuiMDIChildFrame))
    {
        wxAuiMDIChildFrame* cf = (wxAuiMDIChildFrame*)wnd;
        cf->wxWindow::Show(show);
    }
    else
#endif
    {
        wnd->Show(show);
    }
}

// wxTabFrame is an interesting case.  It's important that all child pages
// of the multi-notebook control are all actually children of that control
// (and not grandchildren).  wxTabFrame facilitates this.  There is one
// instance of wxTabFrame for each tab control inside the multi-notebook.
// It's important to know that wxTabFrame is not a real window, but it merely
// used to capture the dimensions/positioning of the internal tab control and
// it's managed page windows

class wxTabFrame : public wxWindow
{
public:

    wxTabFrame()
    {
        m_tabs = NULL;
        m_rect = wxRect(wxPoint(0,0), FromDIP(wxSize(200,200)));
        m_tabCtrlHeight = FromDIP(20);
    }

    ~wxTabFrame()
    {
        wxDELETE(m_tabs);
    }

    void SetTabCtrlHeight(int h)
    {
        m_tabCtrlHeight = h;
    }

protected:
    void DoSetSize(int x, int y,
                   int width, int height,
                   int WXUNUSED(sizeFlags = wxSIZE_AUTO)) wxOVERRIDE
    {
        m_rect = wxRect(x, y, width, height);
        DoSizing();
    }

    void DoGetClientSize(int* x, int* y) const wxOVERRIDE
    {
        *x = m_rect.width;
        *y = m_rect.height;
    }

public:
    bool Show( bool WXUNUSED(show = true) ) wxOVERRIDE { return false; }

    void DoSizing()
    {
        if (!m_tabs)
            return;

        if (m_tabs->IsFrozen() || m_tabs->GetParent()->IsFrozen())
            return;

        m_tab_rect = wxRect(m_rect.x, m_rect.y, m_rect.width, m_tabCtrlHeight);
        if (m_tabs->GetFlags() & wxAUI_NB_BOTTOM)
        {
            m_tab_rect = wxRect (m_rect.x, m_rect.y + m_rect.height - m_tabCtrlHeight, m_rect.width, m_tabCtrlHeight);
            m_tabs->SetSize     (m_rect.x, m_rect.y + m_rect.height - m_tabCtrlHeight, m_rect.width, m_tabCtrlHeight);
            m_tabs->SetRect     (wxRect(0, 0, m_rect.width, m_tabCtrlHeight));
        }
        else //TODO: if (GetFlags() & wxAUI_NB_TOP)
        {
            m_tab_rect = wxRect (m_rect.x, m_rect.y, m_rect.width, m_tabCtrlHeight);
            m_tabs->SetSize     (m_rect.x, m_rect.y, m_rect.width, m_tabCtrlHeight);
            m_tabs->SetRect     (wxRect(0, 0,        m_rect.width, m_tabCtrlHeight));
        }
        // TODO: else if (GetFlags() & wxAUI_NB_LEFT){}
        // TODO: else if (GetFlags() & wxAUI_NB_RIGHT){}

        m_tabs->Refresh();
        m_tabs->Update();

        wxAuiNotebookPageArray& pages = m_tabs->GetPages();
        size_t i, page_count = pages.GetCount();

        for (i = 0; i < page_count; ++i)
        {
            wxAuiNotebookPage& page = pages.Item(i);
            int border_space = m_tabs->GetArtProvider()->GetAdditionalBorderSpace(page.window);

            int height = m_rect.height - m_tabCtrlHeight - border_space;
            if ( height < 0 )
            {
                // avoid passing negative height to wxWindow::SetSize(), this
                // results in assert failures/GTK+ warnings
                height = 0;
            }
            int width = m_rect.width - 2 * border_space;
            if (width < 0)
                width = 0;

            if (m_tabs->GetFlags() & wxAUI_NB_BOTTOM)
            {
                page.window->SetSize(m_rect.x + border_space,
                                     m_rect.y + border_space,
                                     width,
                                     height);
            }
            else //TODO: if (GetFlags() & wxAUI_NB_TOP)
            {
                page.window->SetSize(m_rect.x + border_space,
                                     m_rect.y + m_tabCtrlHeight,
                                     width,
                                     height);
            }
            // TODO: else if (GetFlags() & wxAUI_NB_LEFT){}
            // TODO: else if (GetFlags() & wxAUI_NB_RIGHT){}
        }
    }

protected:
    void DoGetSize(int* x, int* y) const wxOVERRIDE
    {
        if (x)
            *x = m_rect.GetWidth();
        if (y)
            *y = m_rect.GetHeight();
    }

public:
    void Update() wxOVERRIDE
    {
        // does nothing
    }

    wxRect m_rect;
    wxRect m_tab_rect;
    wxAuiTabCtrl* m_tabs;
    int m_tabCtrlHeight;
};


const int wxAuiBaseTabCtrlId = 5380;


// -- wxAuiNotebook class implementation --

#define EVT_AUI_RANGE(id1, id2, event, func) \
    wx__DECLARE_EVT2(event, id1, id2, wxAuiNotebookEventHandler(func))

wxBEGIN_EVENT_TABLE(wxAuiNotebook2, wxControl)
    EVT_SIZE(wxAuiNotebook2::OnSize)
    EVT_CHILD_FOCUS(wxAuiNotebook2::OnChildFocusNotebook)
    EVT_AUI_RANGE(wxAuiBaseTabCtrlId, wxAuiBaseTabCtrlId+500,
                      wxEVT_AUINOTEBOOK_PAGE_CHANGING,
                      wxAuiNotebook2::OnTabClicked)
    EVT_AUI_RANGE(wxAuiBaseTabCtrlId, wxAuiBaseTabCtrlId+500,
                      wxEVT_AUINOTEBOOK_BEGIN_DRAG,
                      wxAuiNotebook2::OnTabBeginDrag)
    EVT_AUI_RANGE(wxAuiBaseTabCtrlId, wxAuiBaseTabCtrlId+500,
                      wxEVT_AUINOTEBOOK_END_DRAG,
                      wxAuiNotebook2::OnTabEndDrag)
    EVT_AUI_RANGE(wxAuiBaseTabCtrlId, wxAuiBaseTabCtrlId+500,
                      wxEVT_AUINOTEBOOK_CANCEL_DRAG,
                      wxAuiNotebook2::OnTabCancelDrag)
    EVT_AUI_RANGE(wxAuiBaseTabCtrlId, wxAuiBaseTabCtrlId+500,
                      wxEVT_AUINOTEBOOK_DRAG_MOTION,
                      wxAuiNotebook2::OnTabDragMotion)
    EVT_AUI_RANGE(wxAuiBaseTabCtrlId, wxAuiBaseTabCtrlId+500,
                      wxEVT_AUINOTEBOOK_BUTTON,
                      wxAuiNotebook2::OnTabButton)
    EVT_AUI_RANGE(wxAuiBaseTabCtrlId, wxAuiBaseTabCtrlId+500,
                      wxEVT_AUINOTEBOOK_TAB_MIDDLE_DOWN,
                      wxAuiNotebook2::OnTabMiddleDown)
    EVT_AUI_RANGE(wxAuiBaseTabCtrlId, wxAuiBaseTabCtrlId+500,
                      wxEVT_AUINOTEBOOK_TAB_MIDDLE_UP,
                      wxAuiNotebook2::OnTabMiddleUp)
    EVT_AUI_RANGE(wxAuiBaseTabCtrlId, wxAuiBaseTabCtrlId+500,
                      wxEVT_AUINOTEBOOK_TAB_RIGHT_DOWN,
                      wxAuiNotebook2::OnTabRightDown)
    EVT_AUI_RANGE(wxAuiBaseTabCtrlId, wxAuiBaseTabCtrlId+500,
                      wxEVT_AUINOTEBOOK_TAB_RIGHT_UP,
                      wxAuiNotebook2::OnTabRightUp)
    EVT_AUI_RANGE(wxAuiBaseTabCtrlId, wxAuiBaseTabCtrlId+500,
                      wxEVT_AUINOTEBOOK_BG_DCLICK,
                      wxAuiNotebook2::OnTabBgDClick)
    EVT_NAVIGATION_KEY(wxAuiNotebook2::OnNavigationKeyNotebook)
    EVT_SYS_COLOUR_CHANGED(wxAuiNotebook2::OnSysColourChanged)
wxEND_EVENT_TABLE()

void wxAuiNotebook2::OnSysColourChanged(wxSysColourChangedEvent &event)
{
    event.Skip(true);
    wxAuiTabArt* art = m_tabs.GetArtProvider();
    art->UpdateColoursFromSystem();

    wxAuiPaneInfoArray& all_panes = m_mgr.GetAllPanes();
    size_t i, pane_count = all_panes.GetCount();
    for (i = 0; i < pane_count; ++i)
    {
        wxAuiPaneInfo& pane = all_panes.Item(i);
        if (pane.name == wxT("dummy"))
            continue;
        wxTabFrame* tab_frame = (wxTabFrame*)pane.window;
        wxAuiTabCtrl* tabctrl = tab_frame->m_tabs;
        tabctrl->GetArtProvider()->UpdateColoursFromSystem();
        tabctrl->Refresh();
    }
    Refresh();
}

void wxAuiNotebook2::Init()
{
    m_curPage = -1;
    m_tabIdCounter = wxAuiBaseTabCtrlId;
    m_dummyWnd = NULL;
    m_tabCtrlHeight = FromDIP(20);
    m_requestedBmpSize = wxDefaultSize;
    m_requestedTabCtrlHeight = -1;
}

bool wxAuiNotebook2::Create(wxWindow* parent,
                           wxWindowID id,
                           const wxPoint& pos,
                           const wxSize& size,
                           long style)
{
    if (!wxControl::Create(parent, id, pos, size, style))
        return false;

    InitNotebook(style);

    return true;
}

// InitNotebook() contains common initialization
// code called by all constructors
void wxAuiNotebook2::InitNotebook(long style)
{
    SetName(wxT("wxAuiNotebook"));
    m_curPage = -1;
    m_tabIdCounter = wxAuiBaseTabCtrlId;
    m_dummyWnd = NULL;
    m_flags = (unsigned int)style;
    m_tabCtrlHeight = FromDIP(20);

    m_normalFont = *wxNORMAL_FONT;
    m_selectedFont = *wxNORMAL_FONT;
    m_selectedFont.SetWeight(wxFONTWEIGHT_BOLD);

    SetArtProvider(new wxAuiDefaultTabArt);

    m_dummyWnd = new wxWindow(this, wxID_ANY, wxPoint(0,0), wxSize(0,0));
    m_dummyWnd->SetSize(FromDIP(wxSize(200, 200)));
    m_dummyWnd->Show(false);

    m_mgr.SetManagedWindow(this);
    m_mgr.SetFlags(wxAUI_MGR_DEFAULT);
    m_mgr.SetDockSizeConstraint(1.0, 1.0); // no dock size constraint

    m_mgr.AddPane(m_dummyWnd,
              wxAuiPaneInfo().Name(wxT("dummy")).Bottom().CaptionVisible(false).Show(false));

    m_mgr.Update();
}

wxAuiNotebook2::~wxAuiNotebook2()
{
    // Indicate we're deleting pages
    SendDestroyEvent();

    while ( GetPageCount() > 0 )
        DeletePage(0);

    m_mgr.UnInit();

	if (&m_mgr != &m_defaultMgr)
		delete &m_mgr;
}

void wxAuiNotebook2::SetArtProvider(wxAuiTabArt* art)
{
    m_tabs.SetArtProvider(art);

    // Update the height and do nothing else if it did something but otherwise
    // (i.e. if the new art provider uses the same height as the old one) we
    // need to manually set the art provider for all tabs ourselves.
    if ( !UpdateTabCtrlHeight() )
    {
        wxAuiPaneInfoArray& all_panes = m_mgr.GetAllPanes();
        const size_t pane_count = all_panes.GetCount();
        for (size_t i = 0; i < pane_count; ++i)
        {
            wxAuiPaneInfo& pane = all_panes.Item(i);
            if (pane.name == wxT("dummy"))
                continue;
            wxTabFrame* tab_frame = (wxTabFrame*)pane.window;
            wxAuiTabCtrl* tabctrl = tab_frame->m_tabs;
            tabctrl->SetArtProvider(art->Clone());
        }
    }
}

// SetTabCtrlHeight() is the highest-level override of the
// tab height.  A call to this function effectively enforces a
// specified tab ctrl height, overriding all other considerations,
// such as text or bitmap height.  It overrides any call to
// SetUniformBitmapSize().  Specifying a height of -1 reverts
// any previous call and returns to the default behaviour

void wxAuiNotebook2::SetTabCtrlHeight(int height)
{
    m_requestedTabCtrlHeight = height;

    // if window is already initialized, recalculate the tab height
    if (m_dummyWnd)
    {
        UpdateTabCtrlHeight();
    }
}


// SetUniformBitmapSize() ensures that all tabs will have
// the same height, even if some tabs don't have bitmaps
// Passing wxDefaultSize to this function will instruct
// the control to use dynamic tab height-- so when a tab
// with a large bitmap is added, the tab ctrl's height will
// automatically increase to accommodate the bitmap

void wxAuiNotebook2::SetUniformBitmapSize(const wxSize& size)
{
    m_requestedBmpSize = size;

    // if window is already initialized, recalculate the tab height
    if (m_dummyWnd)
    {
        UpdateTabCtrlHeight();
    }
}

// UpdateTabCtrlHeight() does the actual tab resizing. It's meant
// to be used internally
bool wxAuiNotebook2::UpdateTabCtrlHeight()
{
    // get the tab ctrl height we will use
    int height = CalculateTabCtrlHeight();

    // if the tab control height needs to change, update
    // all of our tab controls with the new height
    if (m_tabCtrlHeight == height)
        return false;

    wxAuiTabArt* art = m_tabs.GetArtProvider();

    m_tabCtrlHeight = height;

    wxAuiPaneInfoArray& all_panes = m_mgr.GetAllPanes();
    size_t i, pane_count = all_panes.GetCount();
    for (i = 0; i < pane_count; ++i)
    {
        wxAuiPaneInfo& pane = all_panes.Item(i);
        if (pane.name == wxT("dummy"))
            continue;
        wxTabFrame* tab_frame = (wxTabFrame*)pane.window;
        wxAuiTabCtrl* tabctrl = tab_frame->m_tabs;
        tab_frame->SetTabCtrlHeight(m_tabCtrlHeight);
        tabctrl->SetArtProvider(art->Clone());
        tab_frame->DoSizing();
    }

    return true;
}

void wxAuiNotebook2::UpdateHintWindowSize()
{
    wxSize size = CalculateNewSplitSize();

    // the placeholder hint window should be set to this size
    wxAuiPaneInfo& info = m_mgr.GetPane(wxT("dummy"));
    if (info.IsOk())
    {
        info.MinSize(size);
        info.BestSize(size);
        m_dummyWnd->SetSize(size);
    }
}


// calculates the size of the new split
wxSize wxAuiNotebook2::CalculateNewSplitSize()
{
    // count number of tab controls
    int tab_ctrl_count = 0;
    wxAuiPaneInfoArray& all_panes = m_mgr.GetAllPanes();
    size_t i, pane_count = all_panes.GetCount();
    for (i = 0; i < pane_count; ++i)
    {
        wxAuiPaneInfo& pane = all_panes.Item(i);
        if (pane.name == wxT("dummy"))
            continue;
        tab_ctrl_count++;
    }

    wxSize new_split_size;

    // if there is only one tab control, the first split
    // should happen around the middle
    if (tab_ctrl_count < 2)
    {
        new_split_size = GetClientSize();
        new_split_size.x /= 2;
        new_split_size.y /= 2;
    }
    else
    {
        // this is in place of a more complicated calculation
        // that needs to be implemented
        new_split_size = FromDIP(wxSize(180,180));
    }

    return new_split_size;
}

int wxAuiNotebook2::CalculateTabCtrlHeight()
{
    // if a fixed tab ctrl height is specified,
    // just return that instead of calculating a
    // tab height
    if (m_requestedTabCtrlHeight != -1)
        return m_requestedTabCtrlHeight;

    // find out new best tab height
    wxAuiTabArt* art = m_tabs.GetArtProvider();

    return art->GetBestTabCtrlSize(this,
                                   m_tabs.GetPages(),
                                   m_requestedBmpSize);
}


wxAuiTabArt* wxAuiNotebook2::GetArtProvider() const
{
    return m_tabs.GetArtProvider();
}

void wxAuiNotebook2::SetWindowStyleFlag(long style)
{
    wxControl::SetWindowStyleFlag(style);

    m_flags = (unsigned int)style;

    // if the control is already initialized
    if (m_mgr.GetManagedWindow() == (wxWindow*)this)
    {
        // let all of the tab children know about the new style

        wxAuiPaneInfoArray& all_panes = m_mgr.GetAllPanes();
        size_t i, pane_count = all_panes.GetCount();
        for (i = 0; i < pane_count; ++i)
        {
            wxAuiPaneInfo& pane = all_panes.Item(i);
            if (pane.name == wxT("dummy"))
                continue;
            wxTabFrame* tabframe = (wxTabFrame*)pane.window;
            wxAuiTabCtrl* tabctrl = tabframe->m_tabs;
            tabctrl->SetFlags(m_flags);
            tabframe->DoSizing();
            tabctrl->Refresh();
            tabctrl->Update();
        }
    }
}


bool wxAuiNotebook2::AddPage(wxWindow* page,
                            const wxString& caption,
                            bool select,
                            const wxBitmap& bitmap)
{
    return InsertPage(GetPageCount(), page, caption, select, bitmap);
}

bool wxAuiNotebook2::InsertPage(size_t page_idx,
                               wxWindow* page,
                               const wxString& caption,
                               bool select,
                               const wxBitmap& bitmap)
{
    wxASSERT_MSG(page, wxT("page pointer must be non-NULL"));
    if (!page)
        return false;

    page->Reparent(this);

    wxAuiNotebookPage info;
    info.window = page;
    info.caption = caption;
    info.bitmap = bitmap;
    info.active = false;

    // if there are currently no tabs, the first added
    // tab must be active
    if (m_tabs.GetPageCount() == 0)
        info.active = true;

    m_tabs.InsertPage(page, info, page_idx);

    // if that was the first page added, even if
    // select is false, it must become the "current page"
    // (though no select events will be fired)
    if (!select && m_tabs.GetPageCount() == 1)
        select = true;
        //m_curPage = GetPageIndex(page);

    wxAuiTabCtrl* active_tabctrl = GetActiveTabCtrl();
    if (page_idx >= active_tabctrl->GetPageCount())
        active_tabctrl->AddPage(page, info);
    else
        active_tabctrl->InsertPage(page, info, page_idx);

    // Note that we don't need to call DoSizing() if the height has changed, as
    // it's already called from UpdateTabCtrlHeight() itself in this case.
    if ( !UpdateTabCtrlHeight() )
        DoSizing();

    active_tabctrl->DoShowHide();

    // adjust selected index
    if(m_curPage >= (int) page_idx)
        m_curPage++;

    if (select)
    {
        SetSelectionToWindow(page);
    }

    return true;
}


// DeletePage() removes a tab from the multi-notebook,
// and destroys the window as well
bool wxAuiNotebook2::DeletePage(size_t page_idx)
{
    if (page_idx >= m_tabs.GetPageCount())
        return false;

    wxWindow* wnd = m_tabs.GetWindowFromIdx(page_idx);

    // hide the window in advance, as this will
    // prevent flicker
    ShowWnd(wnd, false);

    if (!RemovePage(page_idx))
        return false;

#if wxUSE_MDI
    // actually destroy the window now
    if (wxDynamicCast(wnd, wxAuiMDIChildFrame))
    {
        // delete the child frame with pending delete, as is
        // customary with frame windows
        if (!wxPendingDelete.Member(wnd))
            wxPendingDelete.Append(wnd);
    }
    else
#endif
    {
        wnd->Destroy();
    }

    return true;
}



// RemovePage() removes a tab from the multi-notebook,
// but does not destroy the window
bool wxAuiNotebook2::RemovePage(size_t page_idx)
{
    // save active window pointer
    wxWindow* active_wnd = NULL;
    if (m_curPage >= 0)
        active_wnd = m_tabs.GetWindowFromIdx(m_curPage);

    // save pointer of window being deleted
    wxWindow* wnd = m_tabs.GetWindowFromIdx(page_idx);
    wxWindow* new_active = NULL;

    // make sure we found the page
    if (!wnd)
        return false;

    ShowWnd(wnd, false);

    // find out which onscreen tab ctrl owns this tab
    wxAuiTabCtrl* ctrl;
    int ctrl_idx;
    if (!FindTab(wnd, &ctrl, &ctrl_idx))
        return false;

    bool is_curpage = (m_curPage == (int)page_idx);
    bool is_active_in_split = ctrl->GetPage(ctrl_idx).active;


    // remove the tab from main catalog
    if (!m_tabs.RemovePage(wnd))
        return false;

    // remove the tab from the onscreen tab ctrl
    ctrl->RemovePage(wnd);

    if (is_active_in_split)
    {
        int ctrl_new_page_count = (int)ctrl->GetPageCount();

        if (ctrl_idx >= ctrl_new_page_count)
            ctrl_idx = ctrl_new_page_count-1;

        if (ctrl_idx >= 0 && ctrl_idx < (int)ctrl->GetPageCount())
        {
            // set new page as active in the tab split
            ctrl->SetActivePage(ctrl_idx);

            // if the page deleted was the current page for the
            // entire tab control, then record the window
            // pointer of the new active page for activation
            if (is_curpage)
            {
                new_active = ctrl->GetWindowFromIdx(ctrl_idx);
            }
        }
    }
    else
    {
        // we are not deleting the active page, so keep it the same
        new_active = active_wnd;
    }


    if (!new_active)
    {
        // we haven't yet found a new page to active,
        // so select the next page from the main tab
        // catalogue

        if (page_idx < m_tabs.GetPageCount())
        {
            new_active = m_tabs.GetPage(page_idx).window;
        }

        if (!new_active && m_tabs.GetPageCount() > 0)
        {
            new_active = m_tabs.GetPage(0).window;
        }
    }


    RemoveEmptyTabFrames();

    m_curPage = wxNOT_FOUND;

    // set new active pane unless we're being destroyed anyhow
    if (new_active && !m_isBeingDeleted)
        SetSelectionToWindow(new_active);

    return true;
}

// GetPageIndex() returns the index of the page, or -1 if the
// page could not be located in the notebook
int wxAuiNotebook2::GetPageIndex(wxWindow* page_wnd) const
{
    return m_tabs.GetIdxFromWindow(page_wnd);
}



// SetPageText() changes the tab caption of the specified page
bool wxAuiNotebook2::SetPageText(size_t page_idx, const wxString& text)
{
    if (page_idx >= m_tabs.GetPageCount())
        return false;

    // update our own tab catalog
    wxAuiNotebookPage& page_info = m_tabs.GetPage(page_idx);
    page_info.caption = text;

    // update what's on screen
    wxAuiTabCtrl* ctrl;
    int ctrl_idx;
    if (FindTab(page_info.window, &ctrl, &ctrl_idx))
    {
        wxAuiNotebookPage& info = ctrl->GetPage(ctrl_idx);
        info.caption = text;
        ctrl->Refresh();
        ctrl->Update();
    }

    return true;
}

// returns the page caption
wxString wxAuiNotebook2::GetPageText(size_t page_idx) const
{
    if (page_idx >= m_tabs.GetPageCount())
        return wxEmptyString;

    // update our own tab catalog
    const wxAuiNotebookPage& page_info = m_tabs.GetPage(page_idx);
    return page_info.caption;
}

bool wxAuiNotebook2::SetPageToolTip(size_t page_idx, const wxString& text)
{
    if (page_idx >= m_tabs.GetPageCount())
        return false;

    // update our own tab catalog
    wxAuiNotebookPage& page_info = m_tabs.GetPage(page_idx);
    page_info.tooltip = text;

    wxAuiTabCtrl* ctrl;
    int ctrl_idx;
    if (!FindTab(page_info.window, &ctrl, &ctrl_idx))
        return false;

    wxAuiNotebookPage& info = ctrl->GetPage(ctrl_idx);
    info.tooltip = text;

    // NB: we don't update the tooltip if it is already being displayed, it
    //     typically never happens, no need to code that
    return true;
}

wxString wxAuiNotebook2::GetPageToolTip(size_t page_idx) const
{
    if (page_idx >= m_tabs.GetPageCount())
        return wxString();

    const wxAuiNotebookPage& page_info = m_tabs.GetPage(page_idx);
    return page_info.tooltip;
}

bool wxAuiNotebook2::SetPageBitmap(size_t page_idx, const wxBitmap& bitmap)
{
    if (page_idx >= m_tabs.GetPageCount())
        return false;

    // update our own tab catalog
    wxAuiNotebookPage& page_info = m_tabs.GetPage(page_idx);
    page_info.bitmap = bitmap;

    // tab height might have changed
    UpdateTabCtrlHeight();

    // update what's on screen
    wxAuiTabCtrl* ctrl;
    int ctrl_idx;
    if (FindTab(page_info.window, &ctrl, &ctrl_idx))
    {
        wxAuiNotebookPage& info = ctrl->GetPage(ctrl_idx);
        info.bitmap = bitmap;
        ctrl->Refresh();
        ctrl->Update();
    }

    return true;
}

// returns the page bitmap
wxBitmap wxAuiNotebook2::GetPageBitmap(size_t page_idx) const
{
    if (page_idx >= m_tabs.GetPageCount())
        return wxBitmap();

    // update our own tab catalog
    const wxAuiNotebookPage& page_info = m_tabs.GetPage(page_idx);
    return page_info.bitmap;
}

// GetSelection() returns the index of the currently active page
int wxAuiNotebook2::GetSelection() const
{
    return m_curPage;
}

// SetSelection() sets the currently active page
int wxAuiNotebook2::SetSelection(size_t new_page)
{
    return DoModifySelection(new_page, true);
}

void wxAuiNotebook2::SetSelectionToWindow(wxWindow *win)
{
    const int idx = m_tabs.GetIdxFromWindow(win);
    wxCHECK_RET( idx != wxNOT_FOUND, wxT("invalid notebook page") );


    // since a tab was clicked, let the parent know that we received
    // the focus, even if we will assign that focus immediately
    // to the child tab in the SetSelection call below
    // (the child focus event will also let wxAuiManager, if any,
    // know that the notebook control has been activated)

    wxWindow* parent = GetParent();
    if (parent)
    {
        wxChildFocusEvent eventFocus(this);
        parent->GetEventHandler()->ProcessEvent(eventFocus);
    }


    SetSelection(idx);
}

// GetPageCount() returns the total number of
// pages managed by the multi-notebook
size_t wxAuiNotebook2::GetPageCount() const
{
    return m_tabs.GetPageCount();
}

// GetPage() returns the wxWindow pointer of the
// specified page
wxWindow* wxAuiNotebook2::GetPage(size_t page_idx) const
{
    wxASSERT(page_idx < m_tabs.GetPageCount());

    return m_tabs.GetWindowFromIdx(page_idx);
}

// DoSizing() performs all sizing operations in each tab control
void wxAuiNotebook2::DoSizing()
{
    wxAuiPaneInfoArray& all_panes = m_mgr.GetAllPanes();
    size_t i, pane_count = all_panes.GetCount();
    for (i = 0; i < pane_count; ++i)
    {
        if (all_panes.Item(i).name == wxT("dummy"))
            continue;

        wxTabFrame* tabframe = (wxTabFrame*)all_panes.Item(i).window;
        tabframe->DoSizing();
    }
}

void wxAuiNotebook2::OnTabFocused(wxFocusEvent& e)
{
	if (e.GetWindow())
		e.GetWindow()->SetFocus();
}

// GetActiveTabCtrl() returns the active tab control.  It is
// called to determine which control gets new windows being added
wxAuiTabCtrl* wxAuiNotebook2::GetActiveTabCtrl()
{
    if (m_curPage >= 0 && m_curPage < (int)m_tabs.GetPageCount())
    {
        wxAuiTabCtrl* ctrl;
        int idx;

        // find the tab ctrl with the current page
        if (FindTab(m_tabs.GetPage(m_curPage).window,
                    &ctrl, &idx))
        {
            return ctrl;
        }
    }

    // no current page, just find the first tab ctrl
    wxAuiPaneInfoArray& all_panes = m_mgr.GetAllPanes();
    size_t i, pane_count = all_panes.GetCount();
    for (i = 0; i < pane_count; ++i)
    {
        if (all_panes.Item(i).name == wxT("dummy"))
            continue;

        wxTabFrame* tabframe = (wxTabFrame*)all_panes.Item(i).window;
        return tabframe->m_tabs;
    }

    // If there is no tabframe at all, create one
    wxTabFrame* tabframe = new wxTabFrame;
    tabframe->SetTabCtrlHeight(m_tabCtrlHeight);
    tabframe->m_tabs = new wxAuiTabCtrl(this,
                                        m_tabIdCounter++,
                                        wxDefaultPosition,
                                        wxDefaultSize,
                                        wxNO_BORDER);
	tabframe->m_tabs->Bind(wxEVT_SET_FOCUS, &wxAuiNotebook2::OnTabFocused, this);
    tabframe->m_tabs->SetFlags(m_flags);
    tabframe->m_tabs->SetArtProvider(m_tabs.GetArtProvider()->Clone());
    m_mgr.AddPane(tabframe,
                  wxAuiPaneInfo().Center().CaptionVisible(false));

    m_mgr.Update();

    return tabframe->m_tabs;
}

// FindTab() finds the tab control that currently contains the window as well
// as the index of the window in the tab control.  It returns true if the
// window was found, otherwise false.
bool wxAuiNotebook2::FindTab(wxWindow* page, wxAuiTabCtrl** ctrl, int* idx)
{
    wxAuiPaneInfoArray& all_panes = m_mgr.GetAllPanes();
    size_t i, pane_count = all_panes.GetCount();
    for (i = 0; i < pane_count; ++i)
    {
        if (all_panes.Item(i).name == wxT("dummy"))
            continue;

        wxTabFrame* tabframe = (wxTabFrame*)all_panes.Item(i).window;

        int page_idx = tabframe->m_tabs->GetIdxFromWindow(page);
        if (page_idx != -1)
        {
            *ctrl = tabframe->m_tabs;
            *idx = page_idx;
            return true;
        }
    }

    return false;
}

void wxAuiNotebook2::Split(size_t page, int direction)
{
    wxSize cli_size = GetClientSize();

    // get the page's window pointer
    wxWindow* wnd = GetPage(page);
    if (!wnd)
        return;

    // notebooks with 1 or less pages can't be split
    if (GetPageCount() < 2)
        return;

    // find out which tab control the page currently belongs to
    wxAuiTabCtrl *src_tabs, *dest_tabs;
    int src_idx = -1;
    src_tabs = NULL;
    if (!FindTab(wnd, &src_tabs, &src_idx))
        return;
    if (!src_tabs || src_idx == -1)
        return;

    // choose a split size
    wxSize split_size;
    if (GetPageCount() > 2)
    {
        split_size = CalculateNewSplitSize();
    }
    else
    {
        // because there are two panes, always split them
        // equally
        split_size = GetClientSize();
        split_size.x /= 2;
        split_size.y /= 2;
    }


    // create a new tab frame
    wxTabFrame* new_tabs = new wxTabFrame;
    new_tabs->m_rect = wxRect(wxPoint(0,0), split_size);
    new_tabs->SetTabCtrlHeight(m_tabCtrlHeight);
    new_tabs->m_tabs = new wxAuiTabCtrl(this,
                                        m_tabIdCounter++,
                                        wxDefaultPosition,
                                        wxDefaultSize,
                                        wxNO_BORDER|wxWANTS_CHARS);
    new_tabs->m_tabs->SetArtProvider(m_tabs.GetArtProvider()->Clone());
    new_tabs->m_tabs->SetFlags(m_flags);
    dest_tabs = new_tabs->m_tabs;

    // create a pane info structure with the information
    // about where the pane should be added
    wxAuiPaneInfo paneInfo = wxAuiPaneInfo().Bottom().CaptionVisible(false);
    wxPoint mouse_pt;

    if (direction == wxLEFT)
    {
        paneInfo.Left();
        mouse_pt = wxPoint(0, cli_size.y/2);
    }
    else if (direction == wxRIGHT)
    {
        paneInfo.Right();
        mouse_pt = wxPoint(cli_size.x, cli_size.y/2);
    }
    else if (direction == wxTOP)
    {
        paneInfo.Top();
        mouse_pt = wxPoint(cli_size.x/2, 0);
    }
    else if (direction == wxBOTTOM)
    {
        paneInfo.Bottom();
        mouse_pt = wxPoint(cli_size.x/2, cli_size.y);
    }

    m_mgr.AddPane(new_tabs, paneInfo, mouse_pt);
    m_mgr.Update();

    // remove the page from the source tabs
    wxAuiNotebookPage page_info = src_tabs->GetPage(src_idx);
    page_info.active = false;
    src_tabs->RemovePage(page_info.window);
    if (src_tabs->GetPageCount() > 0)
    {
        src_tabs->SetActivePage((size_t)0);
        src_tabs->DoShowHide();
        src_tabs->Refresh();
    }


    // add the page to the destination tabs
    dest_tabs->InsertPage(page_info.window, page_info, 0);

    if (src_tabs->GetPageCount() == 0)
    {
        RemoveEmptyTabFrames();
    }

    DoSizing();
    dest_tabs->DoShowHide();
    dest_tabs->Refresh();

    // force the set selection function reset the selection
    m_curPage = -1;

    // set the active page to the one we just split off
    SetSelectionToPage(page_info);

    UpdateHintWindowSize();
}


void wxAuiNotebook2::OnSize(wxSizeEvent& evt)
{
    UpdateHintWindowSize();

    evt.Skip();
}

void wxAuiNotebook2::OnTabClicked(wxAuiNotebookEvent& evt)
{
    wxAuiTabCtrl* ctrl = (wxAuiTabCtrl*)evt.GetEventObject();
    wxASSERT(ctrl != NULL);

    wxWindow* wnd = ctrl->GetWindowFromIdx(evt.GetSelection());
    wxASSERT(wnd != NULL);

    SetSelectionToWindow(wnd);
}

void wxAuiNotebook2::OnTabBgDClick(wxAuiNotebookEvent& evt)
{
    // select the tab ctrl which received the db click
    int selection;
    wxWindow* wnd;
    wxAuiTabCtrl* ctrl = (wxAuiTabCtrl*)evt.GetEventObject();
    if (   (ctrl != NULL)
        && ((selection = ctrl->GetActivePage()) != wxNOT_FOUND)
        && ((wnd = ctrl->GetWindowFromIdx(selection)) != NULL))
    {
        SetSelectionToWindow(wnd);
    }

    // notify owner that the tabbar background has been double-clicked
    wxAuiNotebookEvent e(wxEVT_AUINOTEBOOK_BG_DCLICK, m_windowId);
    e.SetEventObject(this);
    GetEventHandler()->ProcessEvent(e);
}

void wxAuiNotebook2::OnTabBeginDrag(wxAuiNotebookEvent&)
{
    m_lastDragX = 0;
}

void wxAuiNotebook2::OnTabDragMotion(wxAuiNotebookEvent& evt)
{
    wxPoint screen_pt = ::wxGetMousePosition();
    wxPoint client_pt = ScreenToClient(screen_pt);
    wxPoint zero(0,0);

    wxAuiTabCtrl* src_tabs = (wxAuiTabCtrl*)evt.GetEventObject();
    wxAuiTabCtrl* dest_tabs = GetTabCtrlFromPoint(client_pt);

    if (dest_tabs == src_tabs)
    {
        if (src_tabs)
        {
            src_tabs->SetCursor(wxCursor(wxCURSOR_ARROW));
        }

        // always hide the hint for inner-tabctrl drag
        m_mgr.HideHint();

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

        }

        return;
    }


    // if external drag is allowed, check if the tab is being dragged
    // over a different wxAuiNotebook control
    if (m_flags & wxAUI_NB_TAB_EXTERNAL_MOVE)
    {
        wxWindow* tab_ctrl = ::wxFindWindowAtPoint(screen_pt);

        // if we aren't over any window, stop here
        if (!tab_ctrl)
            return;

        // make sure we are not over the hint window
        if (!wxDynamicCast(tab_ctrl, wxFrame))
        {
            while (tab_ctrl)
            {
                if (wxDynamicCast(tab_ctrl, wxAuiTabCtrl))
                    break;
                tab_ctrl = tab_ctrl->GetParent();
            }

            if (tab_ctrl)
            {
                wxAuiNotebook2* nb = (wxAuiNotebook2*)tab_ctrl->GetParent();

                if (nb != this)
                {
                    wxRect hint_rect = tab_ctrl->GetClientRect();
                    tab_ctrl->ClientToScreen(&hint_rect.x, &hint_rect.y);
                    m_mgr.ShowHint(hint_rect);
                    return;
                }
            }
        }
        else
        {
            if (!dest_tabs)
            {
                // we are either over a hint window, or not over a tab
                // window, and there is no where to drag to, so exit
                return;
            }
        }
    }


    // if there are less than two panes, split can't happen, so leave
    if (m_tabs.GetPageCount() < 2)
        return;

    // if tab moving is not allowed, leave
    if (!(m_flags & wxAUI_NB_TAB_SPLIT))
        return;


    if (src_tabs)
    {
        src_tabs->SetCursor(wxCursor(wxCURSOR_SIZING));
    }


    if (dest_tabs)
    {
        wxRect hint_rect = dest_tabs->GetRect();
        ClientToScreen(&hint_rect.x, &hint_rect.y);
        m_mgr.ShowHint(hint_rect);
    }
    else
    {
        m_mgr.DrawHintRect(m_dummyWnd, client_pt, zero);
    }
}



void wxAuiNotebook2::OnTabEndDrag(wxAuiNotebookEvent& evt)
{
    m_mgr.HideHint();


    wxAuiTabCtrl* src_tabs = (wxAuiTabCtrl*)evt.GetEventObject();
    wxCHECK_RET( src_tabs, wxT("no source object?") );

    src_tabs->SetCursor(wxCursor(wxCURSOR_ARROW));

    // get the mouse position, which will be used to determine the drop point
    wxPoint mouse_screen_pt = ::wxGetMousePosition();
    wxPoint mouse_client_pt = ScreenToClient(mouse_screen_pt);

    // Update our selection (it may be updated again below but the code below
    // can also return without doing anything else and this ensures that the
    // selection is updated even then).
    m_curPage = src_tabs->GetActivePage();

    // check for an external move
    if (m_flags & wxAUI_NB_TAB_EXTERNAL_MOVE)
    {
        wxWindow* tab_ctrl = ::wxFindWindowAtPoint(mouse_screen_pt);

        while (tab_ctrl)
        {
            if (wxDynamicCast(tab_ctrl, wxAuiTabCtrl))
                break;
            tab_ctrl = tab_ctrl->GetParent();
        }

        if (tab_ctrl)
        {
            wxAuiNotebook2* nb = (wxAuiNotebook2*)tab_ctrl->GetParent();

            if (nb != this)
            {
                // find out from the destination control
                // if it's ok to drop this tab here
                wxAuiNotebookEvent2 e(wxEVT_AUINOTEBOOK_ALLOW_DND, m_windowId);
                e.SetSelection(evt.GetSelection());
                e.SetOldSelection(evt.GetSelection());
                e.SetEventObject(this);
                e.SetDragSource2(this);
                e.Veto(); // dropping must be explicitly approved by control owner

                nb->GetEventHandler()->ProcessEvent(e);

                if (!e.IsAllowed())
                {
                    // no answer or negative answer
                    m_mgr.HideHint();
                    return;
                }

                // drop was allowed
                int src_idx = evt.GetSelection();
                wxWindow* src_page = src_tabs->GetWindowFromIdx(src_idx);

                // Check that it's not an impossible parent relationship
                wxWindow* p = nb;
                while (p && !p->IsTopLevel())
                {
                    if (p == src_page)
                    {
                        return;
                    }
                    p = p->GetParent();
                }

                // get main index of the page
                int main_idx = m_tabs.GetIdxFromWindow(src_page);
                wxCHECK_RET( main_idx != wxNOT_FOUND, wxT("no source page?") );


                // make a copy of the page info
                wxAuiNotebookPage page_info = m_tabs.GetPage(main_idx);

                // remove the page from the source notebook
                RemovePage(main_idx);

                // reparent the page
                src_page->Reparent(nb);


                // found out the insert idx
                wxAuiTabCtrl* dest_tabs = (wxAuiTabCtrl*)tab_ctrl;
                wxPoint pt = dest_tabs->ScreenToClient(mouse_screen_pt);

                wxWindow* target = NULL;
                int insert_idx = -1;
                dest_tabs->TabHitTest(pt.x, pt.y, &target);
                if (target)
                {
                    insert_idx = dest_tabs->GetIdxFromWindow(target);
                }


                // add the page to the new notebook
                if (insert_idx == -1)
                    insert_idx = dest_tabs->GetPageCount();
                dest_tabs->InsertPage(page_info.window, page_info, insert_idx);
                nb->m_tabs.InsertPage(page_info.window, page_info, insert_idx);

                nb->DoSizing();
                dest_tabs->SetActivePage(insert_idx);
                dest_tabs->DoShowHide();
                dest_tabs->Refresh();

                // set the selection in the destination tab control
                nb->DoModifySelection(insert_idx, false);

                // notify owner that the tab has been dragged
                wxAuiNotebookEvent e2(wxEVT_AUINOTEBOOK_DRAG_DONE, m_windowId);
                e2.SetSelection(evt.GetSelection());
                e2.SetOldSelection(evt.GetSelection());
                e2.SetEventObject(this);
                GetEventHandler()->ProcessEvent(e2);

                return;
            }
        }
    }




    // only perform a tab split if it's allowed
    wxAuiTabCtrl* dest_tabs = NULL;

    if ((m_flags & wxAUI_NB_TAB_SPLIT) && m_tabs.GetPageCount() >= 2)
    {
        // If the pointer is in an existing tab frame, do a tab insert
        wxWindow* hit_wnd = ::wxFindWindowAtPoint(mouse_screen_pt);
        wxTabFrame* tab_frame = (wxTabFrame*)GetTabFrameFromTabCtrl(hit_wnd);
        int insert_idx = -1;
        if (tab_frame)
        {
            dest_tabs = tab_frame->m_tabs;

            if (dest_tabs == src_tabs)
            {
                m_curPage = evt.GetSelection();
                return;
            }

            wxPoint pt = dest_tabs->ScreenToClient(mouse_screen_pt);
            wxWindow* target = NULL;
            dest_tabs->TabHitTest(pt.x, pt.y, &target);
            if (target)
            {
                insert_idx = dest_tabs->GetIdxFromWindow(target);
            }
        }
        else
        {
            wxPoint zero(0,0);
            wxRect rect = m_mgr.CalculateHintRect(m_dummyWnd,
                                                  mouse_client_pt,
                                                  zero);
            if (rect.IsEmpty())
            {
                // there is no suitable drop location here, exit out
                return;
            }

            // If there is no tabframe at all, create one
            wxTabFrame* new_tabs = new wxTabFrame;
            new_tabs->m_rect = wxRect(wxPoint(0,0), CalculateNewSplitSize());
            new_tabs->SetTabCtrlHeight(m_tabCtrlHeight);
            new_tabs->m_tabs = new wxAuiTabCtrl(this,
                                                m_tabIdCounter++,
                                                wxDefaultPosition,
                                                wxDefaultSize,
                                                wxNO_BORDER|wxWANTS_CHARS);
            new_tabs->m_tabs->SetArtProvider(m_tabs.GetArtProvider()->Clone());
            new_tabs->m_tabs->SetFlags(m_flags);

            m_mgr.AddPane(new_tabs,
                          wxAuiPaneInfo().Bottom().CaptionVisible(false),
                          mouse_client_pt);
            m_mgr.Update();
            dest_tabs = new_tabs->m_tabs;
        }



        // remove the page from the source tabs
        wxAuiNotebookPage page_info = src_tabs->GetPage(evt.GetSelection());
        page_info.active = false;
        src_tabs->RemovePage(page_info.window);
        if (src_tabs->GetPageCount() > 0)
        {
            src_tabs->SetActivePage((size_t)0);
            src_tabs->DoShowHide();
            src_tabs->Refresh();
        }



        // add the page to the destination tabs
        if (insert_idx == -1)
            insert_idx = dest_tabs->GetPageCount();
        dest_tabs->InsertPage(page_info.window, page_info, insert_idx);

        if (src_tabs->GetPageCount() == 0)
        {
            RemoveEmptyTabFrames();
        }

        DoSizing();
        dest_tabs->DoShowHide();
        dest_tabs->Refresh();

        // force the set selection function reset the selection
        m_curPage = -1;

        // set the active page to the one we just split off
        SetSelectionToPage(page_info);

        UpdateHintWindowSize();
    }

    // notify owner that the tab has been dragged
    wxAuiNotebookEvent e(wxEVT_AUINOTEBOOK_DRAG_DONE, m_windowId);
    e.SetSelection(evt.GetSelection());
    e.SetOldSelection(evt.GetSelection());
    e.SetEventObject(this);
    GetEventHandler()->ProcessEvent(e);
}



void wxAuiNotebook2::OnTabCancelDrag(wxAuiNotebookEvent& command_evt)
{
    wxAuiNotebookEvent& evt = (wxAuiNotebookEvent&)command_evt;

    m_mgr.HideHint();

    wxAuiTabCtrl* src_tabs = (wxAuiTabCtrl*)evt.GetEventObject();
    wxCHECK_RET( src_tabs, wxT("no source object?") );

    src_tabs->SetCursor(wxCursor(wxCURSOR_ARROW));
}

wxAuiTabCtrl* wxAuiNotebook2::GetTabCtrlFromPoint(const wxPoint& pt)
{
    // if we've just removed the last tab from the source
    // tab set, the remove the tab control completely
    wxAuiPaneInfoArray& all_panes = m_mgr.GetAllPanes();
    size_t i, pane_count = all_panes.GetCount();
    for (i = 0; i < pane_count; ++i)
    {
        if (all_panes.Item(i).name == wxT("dummy"))
            continue;

        wxTabFrame* tabframe = (wxTabFrame*)all_panes.Item(i).window;
        if (tabframe->m_tab_rect.Contains(pt))
            return tabframe->m_tabs;
    }

    return NULL;
}

wxWindow* wxAuiNotebook2::GetTabFrameFromTabCtrl(wxWindow* tab_ctrl)
{
    // if we've just removed the last tab from the source
    // tab set, the remove the tab control completely
    wxAuiPaneInfoArray& all_panes = m_mgr.GetAllPanes();
    size_t i, pane_count = all_panes.GetCount();
    for (i = 0; i < pane_count; ++i)
    {
        if (all_panes.Item(i).name == wxT("dummy"))
            continue;

        wxTabFrame* tabframe = (wxTabFrame*)all_panes.Item(i).window;
        if (tabframe->m_tabs == tab_ctrl)
        {
            return tabframe;
        }
    }

    return NULL;
}

void wxAuiNotebook2::RemoveEmptyTabFrames()
{
    // if we've just removed the last tab from the source
    // tab set, the remove the tab control completely
    wxAuiPaneInfoArray all_panes = m_mgr.GetAllPanes();
    size_t i, pane_count = all_panes.GetCount();
    for (i = 0; i < pane_count; ++i)
    {
        if (all_panes.Item(i).name == wxT("dummy"))
            continue;

        wxTabFrame* tab_frame = (wxTabFrame*)all_panes.Item(i).window;
        if (tab_frame->m_tabs->GetPageCount() == 0)
        {
            m_mgr.DetachPane(tab_frame);

            // use pending delete because sometimes during
            // window closing, refreshs are pending
            if (!wxPendingDelete.Member(tab_frame->m_tabs))
                wxPendingDelete.Append(tab_frame->m_tabs);

            tab_frame->m_tabs = NULL;

            delete tab_frame;
        }
    }


    // check to see if there is still a center pane;
    // if there isn't, make a frame the center pane
    wxAuiPaneInfoArray panes = m_mgr.GetAllPanes();
    pane_count = panes.GetCount();
    wxWindow* first_good = NULL;
    bool center_found = false;
    for (i = 0; i < pane_count; ++i)
    {
        if (panes.Item(i).name == wxT("dummy"))
            continue;
        if (panes.Item(i).dock_direction == wxAUI_DOCK_CENTRE)
            center_found = true;
        if (!first_good)
            first_good = panes.Item(i).window;
    }

    if (!center_found && first_good)
    {
        m_mgr.GetPane(first_good).Centre();
    }

    if (!m_isBeingDeleted)
        m_mgr.Update();
}

void wxAuiNotebook2::OnChildFocusNotebook(wxChildFocusEvent& evt)
{
    evt.Skip();

    // if we're dragging a tab, don't change the current selection.
    // This code prevents a bug that used to happen when the hint window
    // was hidden.  In the bug, the focus would return to the notebook
    // child, which would then enter this handler and call
    // SetSelection, which is not desired turn tab dragging.

    wxAuiPaneInfoArray& all_panes = m_mgr.GetAllPanes();
    size_t i, pane_count = all_panes.GetCount();
    for (i = 0; i < pane_count; ++i)
    {
        wxAuiPaneInfo& pane = all_panes.Item(i);
        if (pane.name == wxT("dummy"))
            continue;
        wxTabFrame* tabframe = (wxTabFrame*)pane.window;
        if (tabframe->m_tabs->IsDragging())
            return;
    }


    // find the page containing the focused child
    wxWindow* win = evt.GetWindow();
    while ( win )
    {
        // pages have the notebook as the parent, so stop when we reach one
        // (and also stop in the impossible case of no parent at all)
        wxWindow* const parent = win->GetParent();
        if ( !parent || parent == this )
            break;

        win = parent;
    }

    // change the tab selection to this page
    int idx = m_tabs.GetIdxFromWindow(win);
    if (idx != -1 && idx != m_curPage)
    {
        SetSelection(idx);
    }
}

void wxAuiNotebook2::OnNavigationKeyNotebook(wxNavigationKeyEvent& event)
{
    if ( event.IsWindowChange() ) {
        // change pages
        // FIXME: the problem with this is that if we have a split notebook,
        // we selection may go all over the place.
        AdvanceSelection(event.GetDirection());
    }
    else {
        // we get this event in 3 cases
        //
        // a) one of our pages might have generated it because the user TABbed
        // out from it in which case we should propagate the event upwards and
        // our parent will take care of setting the focus to prev/next sibling
        //
        // or
        //
        // b) the parent panel wants to give the focus to us so that we
        // forward it to our selected page. We can't deal with this in
        // OnSetFocus() because we don't know which direction the focus came
        // from in this case and so can't choose between setting the focus to
        // first or last panel child
        //
        // or
        //
        // c) we ourselves (see MSWTranslateMessage) generated the event
        //
        wxWindow * const parent = GetParent();

        // the wxObject* casts are required to avoid MinGW GCC 2.95.3 ICE
        const bool isFromParent = event.GetEventObject() == (wxObject*) parent;
        const bool isFromSelf = event.GetEventObject() == (wxObject*) this;

        if ( isFromParent || isFromSelf )
        {
            // no, it doesn't come from child, case (b) or (c): forward to a
            // page but only if direction is backwards (TAB) or from ourselves,
            if ( GetSelection() != wxNOT_FOUND &&
                    (!event.GetDirection() || isFromSelf) )
            {
                // so that the page knows that the event comes from it's parent
                // and is being propagated downwards
                event.SetEventObject(this);

                wxWindow *page = GetPage(GetSelection());
                if ( !page->GetEventHandler()->ProcessEvent(event) )
                {
                    page->SetFocus();
                }
                //else: page manages focus inside it itself
            }
            else // otherwise set the focus to the notebook itself
            {
                SetFocus();
            }
        }
        else
        {
            // it comes from our child, case (a), pass to the parent, but only
            // if the direction is forwards. Otherwise set the focus to the
            // notebook itself. The notebook is always the 'first' control of a
            // page.
            if ( !event.GetDirection() )
            {
                SetFocus();
            }
            else if ( parent )
            {
                event.SetCurrentFocus(this);
                parent->GetEventHandler()->ProcessEvent(event);
            }
        }
    }
}

void wxAuiNotebook2::OnTabButton(wxAuiNotebookEvent& evt)
{
    wxAuiTabCtrl* tabs = (wxAuiTabCtrl*)evt.GetEventObject();

    int button_id = evt.GetInt();

    if (button_id == wxAUI_BUTTON_CLOSE)
    {
        int selection = evt.GetSelection();

        if (selection == -1)
        {
            // if the close button is to the right, use the active
            // page selection to determine which page to close
            selection = tabs->GetActivePage();
        }

        if (selection != -1)
        {
            wxWindow* close_wnd = tabs->GetWindowFromIdx(selection);

            // ask owner if it's ok to close the tab
            wxAuiNotebookEvent e(wxEVT_AUINOTEBOOK_PAGE_CLOSE, m_windowId);
            e.SetSelection(m_tabs.GetIdxFromWindow(close_wnd));
            const int idx = m_tabs.GetIdxFromWindow(close_wnd);
            e.SetSelection(idx);
            e.SetOldSelection(evt.GetSelection());
            e.SetEventObject(this);
            GetEventHandler()->ProcessEvent(e);
            if (!e.IsAllowed())
                return;


#if wxUSE_MDI
            if (wxDynamicCast(close_wnd, wxAuiMDIChildFrame))
            {
                close_wnd->Close();
            }
            else
#endif
            {
                int main_idx = m_tabs.GetIdxFromWindow(close_wnd);
                wxCHECK_RET( main_idx != wxNOT_FOUND, wxT("no page to delete?") );

                DeletePage(main_idx);
            }

            // notify owner that the tab has been closed
            wxAuiNotebookEvent e2(wxEVT_AUINOTEBOOK_PAGE_CLOSED, m_windowId);
            e2.SetSelection(idx);
            e2.SetEventObject(this);
            GetEventHandler()->ProcessEvent(e2);
        }
    }
}


void wxAuiNotebook2::OnTabMiddleDown(wxAuiNotebookEvent& evt)
{
    // patch event through to owner
    wxAuiTabCtrl* tabs = (wxAuiTabCtrl*)evt.GetEventObject();
    wxWindow* wnd = tabs->GetWindowFromIdx(evt.GetSelection());

    wxAuiNotebookEvent e(wxEVT_AUINOTEBOOK_TAB_MIDDLE_DOWN, m_windowId);
    e.SetSelection(m_tabs.GetIdxFromWindow(wnd));
    e.SetEventObject(this);
    GetEventHandler()->ProcessEvent(e);
}

void wxAuiNotebook2::OnTabMiddleUp(wxAuiNotebookEvent& evt)
{
    // if the wxAUI_NB_MIDDLE_CLICK_CLOSE is specified, middle
    // click should act like a tab close action.  However, first
    // give the owner an opportunity to handle the middle up event
    // for custom action

    wxAuiTabCtrl* tabs = (wxAuiTabCtrl*)evt.GetEventObject();
    wxWindow* wnd = tabs->GetWindowFromIdx(evt.GetSelection());

    wxAuiNotebookEvent e(wxEVT_AUINOTEBOOK_TAB_MIDDLE_UP, m_windowId);
    e.SetSelection(m_tabs.GetIdxFromWindow(wnd));
    e.SetEventObject(this);
    if (GetEventHandler()->ProcessEvent(e))
        return;
    if (!e.IsAllowed())
        return;

    // check if we are supposed to close on middle-up
    if ((m_flags & wxAUI_NB_MIDDLE_CLICK_CLOSE) == 0)
        return;

    // simulate the user pressing the close button on the tab
    evt.SetInt(wxAUI_BUTTON_CLOSE);
    OnTabButton(evt);
}

void wxAuiNotebook2::OnTabRightDown(wxAuiNotebookEvent& evt)
{
    // patch event through to owner
    wxAuiTabCtrl* tabs = (wxAuiTabCtrl*)evt.GetEventObject();
    wxWindow* wnd = tabs->GetWindowFromIdx(evt.GetSelection());

    wxAuiNotebookEvent e(wxEVT_AUINOTEBOOK_TAB_RIGHT_DOWN, m_windowId);
    e.SetSelection(m_tabs.GetIdxFromWindow(wnd));
    e.SetEventObject(this);
    GetEventHandler()->ProcessEvent(e);
}

void wxAuiNotebook2::OnTabRightUp(wxAuiNotebookEvent& evt)
{
    // patch event through to owner
    wxAuiTabCtrl* tabs = (wxAuiTabCtrl*)evt.GetEventObject();
    wxWindow* wnd = tabs->GetWindowFromIdx(evt.GetSelection());

    wxAuiNotebookEvent e(wxEVT_AUINOTEBOOK_TAB_RIGHT_UP, m_windowId);
    e.SetSelection(m_tabs.GetIdxFromWindow(wnd));
    e.SetEventObject(this);
    GetEventHandler()->ProcessEvent(e);
}

// Sets the normal font
void wxAuiNotebook2::SetNormalFont(const wxFont& font)
{
    m_normalFont = font;
    GetArtProvider()->SetNormalFont(font);
}

// Sets the selected tab font
void wxAuiNotebook2::SetSelectedFont(const wxFont& font)
{
    m_selectedFont = font;
    GetArtProvider()->SetSelectedFont(font);
}

// Sets the measuring font
void wxAuiNotebook2::SetMeasuringFont(const wxFont& font)
{
    GetArtProvider()->SetMeasuringFont(font);
}

// Sets the tab font
bool wxAuiNotebook2::SetFont(const wxFont& font)
{
    wxControl::SetFont(font);

    wxFont normalFont(font);
    wxFont selectedFont(normalFont);
    selectedFont.SetWeight(wxFONTWEIGHT_BOLD);

    SetNormalFont(normalFont);
    SetSelectedFont(selectedFont);
    SetMeasuringFont(selectedFont);

    return true;
}

// Gets the tab control height
int wxAuiNotebook2::GetTabCtrlHeight() const
{
    return m_tabCtrlHeight;
}

// Gets the height of the notebook for a given page height
int wxAuiNotebook2::GetHeightForPageHeight(int pageHeight)
{
    UpdateTabCtrlHeight();

    int tabCtrlHeight = GetTabCtrlHeight();
    int decorHeight = 2;
    return tabCtrlHeight + pageHeight + decorHeight;
}

// Shows the window menu
bool wxAuiNotebook2::ShowWindowMenu()
{
    wxAuiTabCtrl* tabCtrl = GetActiveTabCtrl();

    int idx = tabCtrl->GetArtProvider()->ShowDropDown(tabCtrl, tabCtrl->GetPages(), tabCtrl->GetActivePage());

    if (idx != -1)
    {
        wxAuiNotebookEvent e(wxEVT_AUINOTEBOOK_PAGE_CHANGING, tabCtrl->GetId());
        e.SetSelection(idx);
        e.SetOldSelection(tabCtrl->GetActivePage());
        e.SetEventObject(tabCtrl);
        GetEventHandler()->ProcessEvent(e);

        return true;
    }
    else
        return false;
}

void wxAuiNotebook2::DoThaw()
{
    DoSizing();

    wxBookCtrlBase::DoThaw();
}

void wxAuiNotebook2::SetPageSize (const wxSize& WXUNUSED(size))
{
    wxFAIL_MSG("Not implemented for wxAuiNotebook");
}

int wxAuiNotebook2::HitTest (const wxPoint &pt, long *flags) const
{
    wxWindow *w = NULL;
    long position = wxBK_HITTEST_NOWHERE;
    const wxAuiPaneInfoArray& all_panes = const_cast<wxAuiManager&>(m_mgr).GetAllPanes();
    const size_t pane_count = all_panes.GetCount();
    for (size_t i = 0; i < pane_count; ++i)
    {
        if (all_panes.Item(i).name == wxT("dummy"))
            continue;

        wxTabFrame* tabframe = (wxTabFrame*) all_panes.Item(i).window;
        if (tabframe->m_tab_rect.Contains(pt))
        {
            wxPoint tabpos = tabframe->m_tabs->ScreenToClient(ClientToScreen(pt));
            if (tabframe->m_tabs->TabHitTest(tabpos.x, tabpos.y, &w))
                position = wxBK_HITTEST_ONITEM;
            break;
        }
        else if (tabframe->m_rect.Contains(pt))
        {
            w = tabframe->m_tabs->GetWindowFromIdx(tabframe->m_tabs->GetActivePage());
            if (w)
                position = wxBK_HITTEST_ONPAGE;
            break;
        }
    }

    if (flags)
        *flags = position;
    return w ? GetPageIndex(w) : wxNOT_FOUND;
}

int wxAuiNotebook2::GetPageImage(size_t WXUNUSED(n)) const
{
    wxFAIL_MSG("Not implemented for wxAuiNotebook");
    return -1;
}

bool wxAuiNotebook2::SetPageImage(size_t n, int imageId)
{
    return SetPageBitmap(n, GetImageList()->GetBitmap(imageId));
}

int wxAuiNotebook2::ChangeSelection(size_t n)
{
    return DoModifySelection(n, false);
}

bool wxAuiNotebook2::AddPage(wxWindow *page, const wxString &text, bool select,
                            int imageId)
{
    if(HasImageList())
    {
        return AddPage(page, text, select, GetImageList()->GetBitmap(imageId));
    }
    else
    {
        return AddPage(page, text, select, wxNullBitmap);
    }
}

bool wxAuiNotebook2::DeleteAllPages()
{
    size_t count = GetPageCount();
    for(size_t i = 0; i < count; i++)
    {
        DeletePage(0);
    }
    return true;
}

bool wxAuiNotebook2::InsertPage(size_t index, wxWindow *page,
                               const wxString &text, bool select,
                               int imageId)
{
    if(HasImageList())
    {
        return InsertPage(index, page, text, select,
                          GetImageList()->GetBitmap(imageId));
    }
    else
    {
        return InsertPage(index, page, text, select, wxNullBitmap);
    }
}

namespace
{

// Helper class to calculate the best size of a wxAuiNotebook
class wxAuiLayoutObject
{
public:
    enum
    {
        DockDir_Center,
        DockDir_Left,
        DockDir_Right,
        DockDir_Vertical,   // Merge elements from here vertically
        DockDir_Top,
        DockDir_Bottom,
        DockDir_None
    };

    wxAuiLayoutObject(const wxSize &size, const wxAuiPaneInfo &pInfo)
        : m_size(size)
    {
        m_pInfo = &pInfo;
        /*
            To speed up the sorting of the panes, the direction is mapped to a
            useful increasing value. This avoids complicated comparison of the
            enum values during the sort. The size calculation is done from the
            inner to the outermost direction. Therefore CENTER < LEFT/RIGHT <
            TOP/BOTTOM (It doesn't matter it LEFT or RIGHT is done first, as
            both extend the best size horizontally; the same applies for
            TOP/BOTTOM in vertical direction)
         */
        switch ( pInfo.dock_direction )
        {
            case wxAUI_DOCK_CENTER: m_dir = DockDir_Center; break;
            case wxAUI_DOCK_LEFT:   m_dir = DockDir_Left; break;
            case wxAUI_DOCK_RIGHT:  m_dir = DockDir_Right; break;
            case wxAUI_DOCK_TOP:    m_dir = DockDir_Top; break;
            case wxAUI_DOCK_BOTTOM: m_dir = DockDir_Bottom; break;
            default:                m_dir = DockDir_None;
        }
    }
    void MergeLayout(const wxAuiLayoutObject &lo2)
    {
        if ( this == &lo2 )
            return;

        bool mergeHorizontal;
        if ( m_pInfo->dock_layer != lo2.m_pInfo->dock_layer || m_dir != lo2.m_dir )
            mergeHorizontal = lo2.m_dir < DockDir_Vertical;
        else if ( m_pInfo->dock_row != lo2.m_pInfo->dock_row )
            mergeHorizontal = true;
        else
            mergeHorizontal = lo2.m_dir >= DockDir_Vertical;

        if ( mergeHorizontal )
        {
            m_size.x += lo2.m_size.x;
            if ( lo2.m_size.y > m_size.y )
                m_size.y = lo2.m_size.y;
        }
        else
        {
            if ( lo2.m_size.x > m_size.x )
                m_size.x = lo2.m_size.x;
            m_size.y += lo2.m_size.y;
        }
    }

    wxSize m_size;
    const wxAuiPaneInfo *m_pInfo;
    unsigned char m_dir;

    /*
        As the caulculation is done from the inner to the outermost pane, the
        panes are sorted in the following order: layer, direction, row,
        position.
     */
    bool operator<(const wxAuiLayoutObject& lo2) const
    {
        int diff = m_pInfo->dock_layer - lo2.m_pInfo->dock_layer;
        if ( diff )
            return diff < 0;
        diff = m_dir - lo2.m_dir;
        if ( diff )
            return diff < 0;
        diff = m_pInfo->dock_row - lo2.m_pInfo->dock_row;
        if ( diff )
            return diff < 0;
        return m_pInfo->dock_pos < lo2.m_pInfo->dock_pos;
    }
};

} // anonymous namespace

wxSize wxAuiNotebook2::DoGetBestSize() const
{
    /*
        The best size of the wxAuiNotebook is a combination of all panes inside
        the object. To be able to efficiently  calculate the dimensions (i.e.
        without iterating over the panes multiple times) the panes need to be
        processed in a specific order. Therefore we need to collect them in the
        following variable which is sorted later on.
     */
    wxVector<wxAuiLayoutObject> layouts;
    const wxAuiPaneInfoArray& all_panes =
        const_cast<wxAuiManager&>(m_mgr).GetAllPanes();
    const size_t pane_count = all_panes.GetCount();
    const int tabHeight = GetTabCtrlHeight();
    for ( size_t n = 0; n < pane_count; ++n )
    {
        const wxAuiPaneInfo &pInfo = all_panes[n];
        if ( pInfo.name == wxT("dummy") || pInfo.IsFloating() )
            continue;

        const wxTabFrame* tabframe = (wxTabFrame*) all_panes.Item(n).window;
        const wxAuiNotebookPageArray &pages = tabframe->m_tabs->GetPages();

        wxSize bestPageSize;
        for ( size_t pIdx = 0; pIdx < pages.GetCount(); pIdx++ )
            bestPageSize.IncTo(pages[pIdx].window->GetBestSize());

        bestPageSize.y += tabHeight;
        // Store the current pane with its largest window dimensions
        layouts.push_back(wxAuiLayoutObject(bestPageSize, pInfo));
    }

    if ( layouts.empty() )
        return wxSize(0, 0);

    wxVectorSort(layouts);

    /*
        The sizes of the panes are merged here. As the center pane is always at
        position 0 all sizes are merged there. As panes can be stacked using
        the dock_pos property, different positions are merged at the first
        (i.e. dock_pos = 0) element before being merged with the center pane.
     */
    size_t pos = 0;
    for ( size_t n = 1; n < layouts.size(); n++ )
    {
        if ( layouts[n].m_pInfo->dock_layer == layouts[pos].m_pInfo->dock_layer &&
             layouts[n].m_dir == layouts[pos].m_dir &&
             layouts[n].m_pInfo->dock_row == layouts[pos].m_pInfo->dock_row )
        {
            layouts[pos].MergeLayout(layouts[n]);
        }
        else
        {
            layouts[0].MergeLayout(layouts[pos]);
            pos = n;
        }
    }
    layouts[0].MergeLayout(layouts[pos]);

    return layouts[0].m_size;
}

int wxAuiNotebook2::DoModifySelection(size_t n, bool events)
{
    wxWindow* wnd = m_tabs.GetWindowFromIdx(n);
    if (!wnd)
        return m_curPage;

    // don't change the page unless necessary;
    // however, clicking again on a tab should give it the focus.
    if ((int)n == m_curPage)
    {
        wxAuiTabCtrl* ctrl;
        int ctrl_idx;
        if (FindTab(wnd, &ctrl, &ctrl_idx))
        {
            if (FindFocus() != ctrl)
                ctrl->SetFocus();
        }
        return m_curPage;
    }

    bool vetoed = false;

    wxAuiNotebookEvent evt(wxEVT_AUINOTEBOOK_PAGE_CHANGING, m_windowId);

    if(events)
    {
        evt.SetSelection(n);
        evt.SetOldSelection(m_curPage);
        evt.SetEventObject(this);
        GetEventHandler()->ProcessEvent(evt);
        vetoed = !evt.IsAllowed();
    }

    if (!vetoed)
    {
        int old_curpage = m_curPage;
        m_curPage = n;

        wxAuiTabCtrl* ctrl;
        int ctrl_idx;
        if (FindTab(wnd, &ctrl, &ctrl_idx))
        {
            m_tabs.SetActivePage(wnd);

            ctrl->SetActivePage(ctrl_idx);
            DoSizing();
            ctrl->DoShowHide();

            ctrl->MakeTabVisible(ctrl_idx, ctrl);

            // set fonts
            wxAuiPaneInfoArray& all_panes = m_mgr.GetAllPanes();
            size_t i, pane_count = all_panes.GetCount();
            for (i = 0; i < pane_count; ++i)
            {
                wxAuiPaneInfo& pane = all_panes.Item(i);
                if (pane.name == wxT("dummy"))
                    continue;
                wxAuiTabCtrl* tabctrl = ((wxTabFrame*)pane.window)->m_tabs;
                if (tabctrl != ctrl)
                    tabctrl->SetSelectedFont(m_normalFont);
                else
                    tabctrl->SetSelectedFont(m_selectedFont);
                tabctrl->Refresh();
            }

            // Set the focus to the page if we're not currently focused on the tab.
            // This is Firefox-like behaviour.
            if (wnd->IsShownOnScreen() && FindFocus() != ctrl)
                wnd->SetFocus();

            // program allows the page change
            if(events)
            {
                evt.SetEventType(wxEVT_AUINOTEBOOK_PAGE_CHANGED);
                (void)GetEventHandler()->ProcessEvent(evt);
            }

            return old_curpage;
        }
    }

    return m_curPage;
}

#endif // wxUSE_AUI
