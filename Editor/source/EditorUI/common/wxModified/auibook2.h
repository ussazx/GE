#ifndef _WX_AUINOTEBOOK2_H_
#define _WX_AUINOTEBOOK2_H_

#include "wx/aui/auibook.h"

#if wxUSE_AUI

class wxAuiNotebook2;

// aui notebook event class

class wxAuiNotebookEvent2 : public wxAuiNotebookEvent
{
public:
	wxAuiNotebookEvent2(wxEventType commandType = wxEVT_NULL,
		int winId = 0)
		: wxAuiNotebookEvent(commandType, winId)
	{
		m_dragSource2 = NULL;
	}
	wxEvent *Clone() const wxOVERRIDE { return new wxAuiNotebookEvent2(*this); }

	void SetDragSource2(wxAuiNotebook2* s) { m_dragSource2 = s; }
	wxAuiNotebook2* GetDragSource() const { return m_dragSource2; }

private:
	wxAuiNotebook2* m_dragSource2;
};

class wxAuiNotebook2 : public wxNavigationEnabled<wxBookCtrlBase>
{
public:
	wxAuiNotebook2() : m_mgr(m_defaultMgr) { Init(); }

    wxAuiNotebook2(wxAuiManager* pmgr) : m_mgr(*pmgr) { Init(); }

	wxAuiNotebook2(wxWindow* parent,
		wxWindowID id = wxID_ANY,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize,
		long style = wxAUI_NB_DEFAULT_STYLE)
		: m_mgr(m_defaultMgr)
	{
		Init();
		Create(parent, id, pos, size, style);
	}

    wxAuiNotebook2(wxWindow* parent, wxAuiManager* pmgr,
                  wxWindowID id = wxID_ANY,
                  const wxPoint& pos = wxDefaultPosition,
                  const wxSize& size = wxDefaultSize,
                  long style = wxAUI_NB_DEFAULT_STYLE)
		: m_mgr(*pmgr)
    {
        Init();
        Create(parent, id, pos, size, style);
    }

    virtual ~wxAuiNotebook2();

	template<class T = wxAuiManager>
	T* GetSpecMgr()
	{
		if (wxDynamicCast(&m_mgr, T))
			return (T*)&m_mgr;
		return nullptr;
	}

    bool Create(wxWindow* parent,
                wxWindowID id = wxID_ANY,
                const wxPoint& pos = wxDefaultPosition,
                const wxSize& size = wxDefaultSize,
                long style = 0);

    void SetWindowStyleFlag(long style) wxOVERRIDE;
    void SetArtProvider(wxAuiTabArt* art);
    wxAuiTabArt* GetArtProvider() const;

    virtual void SetUniformBitmapSize(const wxSize& size);
    virtual void SetTabCtrlHeight(int height);

    bool AddPage(wxWindow* page,
                 const wxString& caption,
                 bool select = false,
                 const wxBitmap& bitmap = wxNullBitmap);

    bool InsertPage(size_t pageIdx,
                    wxWindow* page,
                    const wxString& caption,
                    bool select = false,
                    const wxBitmap& bitmap = wxNullBitmap);

    bool DeletePage(size_t page) wxOVERRIDE;
    bool RemovePage(size_t page) wxOVERRIDE;

    virtual size_t GetPageCount() const wxOVERRIDE;
    virtual wxWindow* GetPage(size_t pageIdx) const wxOVERRIDE;
    int GetPageIndex(wxWindow* pageWnd) const;

    bool SetPageText(size_t page, const wxString& text) wxOVERRIDE;
    wxString GetPageText(size_t pageIdx) const wxOVERRIDE;

    bool SetPageToolTip(size_t page, const wxString& text);
    wxString GetPageToolTip(size_t pageIdx) const;

    bool SetPageBitmap(size_t page, const wxBitmap& bitmap);
    wxBitmap GetPageBitmap(size_t pageIdx) const;

    int SetSelection(size_t newPage) wxOVERRIDE;
    int GetSelection() const wxOVERRIDE;

    virtual void Split(size_t page, int direction);

    const wxAuiManager& GetAuiManager() const { return m_mgr; }

    // Sets the normal font
    void SetNormalFont(const wxFont& font);

    // Sets the selected tab font
    void SetSelectedFont(const wxFont& font);

    // Sets the measuring font
    void SetMeasuringFont(const wxFont& font);

    // Sets the tab font
    virtual bool SetFont(const wxFont& font) wxOVERRIDE;

    // Gets the tab control height
    int GetTabCtrlHeight() const;

    // Gets the height of the notebook for a given page height
    int GetHeightForPageHeight(int pageHeight);

    // Shows the window menu
    bool ShowWindowMenu();

    // we do have multiple pages
    virtual bool HasMultiplePages() const wxOVERRIDE { return true; }

    // we don't want focus for ourselves
    // virtual bool AcceptsFocus() const { return false; }

    //wxBookCtrlBase functions

    virtual void SetPageSize (const wxSize &size) wxOVERRIDE;
    virtual int  HitTest (const wxPoint &pt, long *flags=NULL) const wxOVERRIDE;

    virtual int GetPageImage(size_t n) const wxOVERRIDE;
    virtual bool SetPageImage(size_t n, int imageId) wxOVERRIDE;

    virtual int ChangeSelection(size_t n) wxOVERRIDE;

    virtual bool AddPage(wxWindow *page, const wxString &text, bool select,
                         int imageId) wxOVERRIDE;
    virtual bool DeleteAllPages() wxOVERRIDE;
    virtual bool InsertPage(size_t index, wxWindow *page, const wxString &text,
                            bool select, int imageId) wxOVERRIDE;

    virtual wxSize DoGetBestSize() const wxOVERRIDE;

    wxAuiTabCtrl* GetTabCtrlFromPoint(const wxPoint& pt);
    wxAuiTabCtrl* GetActiveTabCtrl();
    bool FindTab(wxWindow* page, wxAuiTabCtrl** ctrl, int* idx);

protected:
    // Common part of all ctors.
    void Init();

    // choose the default border for this window
    virtual wxBorder GetDefaultBorder() const wxOVERRIDE { return wxBORDER_NONE; }

    // Redo sizing after thawing
    virtual void DoThaw() wxOVERRIDE;

    // these can be overridden

    // update the height, return true if it was done or false if the new height
    // calculated by CalculateTabCtrlHeight() is the same as the old one
    virtual bool UpdateTabCtrlHeight();

    virtual int CalculateTabCtrlHeight();
    virtual wxSize CalculateNewSplitSize();

    // remove the page and return a pointer to it
    virtual wxWindow *DoRemovePage(size_t WXUNUSED(page)) wxOVERRIDE { return NULL; }

    //A general selection function
    virtual int DoModifySelection(size_t n, bool events);

protected:

    void DoSizing();
    void InitNotebook(long style);
    wxWindow* GetTabFrameFromTabCtrl(wxWindow* tabCtrl);
    void RemoveEmptyTabFrames();
    void UpdateHintWindowSize();

	void OnTabFocused(wxFocusEvent& e);

protected:

    void OnChildFocusNotebook(wxChildFocusEvent& evt);
    void OnRender(wxAuiManagerEvent& evt);
    void OnSize(wxSizeEvent& evt);
    void OnTabClicked(wxAuiNotebookEvent& evt);
    void OnTabBeginDrag(wxAuiNotebookEvent& evt);
    void OnTabDragMotion(wxAuiNotebookEvent& evt);
    void OnTabEndDrag(wxAuiNotebookEvent& evt);
    void OnTabCancelDrag(wxAuiNotebookEvent& evt);
    void OnTabButton(wxAuiNotebookEvent& evt);
    void OnTabMiddleDown(wxAuiNotebookEvent& evt);
    void OnTabMiddleUp(wxAuiNotebookEvent& evt);
    void OnTabRightDown(wxAuiNotebookEvent& evt);
    void OnTabRightUp(wxAuiNotebookEvent& evt);
    void OnTabBgDClick(wxAuiNotebookEvent& evt);
    void OnNavigationKeyNotebook(wxNavigationKeyEvent& event);
    void OnSysColourChanged(wxSysColourChangedEvent& event);

    // set selection to the given window (which must be non-NULL and be one of
    // our pages, otherwise an assert is raised)
    void SetSelectionToWindow(wxWindow *win);
    void SetSelectionToPage(const wxAuiNotebookPage& page)
    {
        SetSelectionToWindow(page.window);
    }

protected:

	wxAuiManager& m_mgr;
	wxAuiManager m_defaultMgr;
    wxAuiTabContainer m_tabs;
    int m_curPage;
    int m_tabIdCounter;
    wxWindow* m_dummyWnd;

    wxSize m_requestedBmpSize;
    int m_requestedTabCtrlHeight;
    wxFont m_selectedFont;
    wxFont m_normalFont;
    int m_tabCtrlHeight;

    int m_lastDragX;
    unsigned int m_flags;

    wxDECLARE_EVENT_TABLE();
};

#endif  // wxUSE_AUI
#endif  // _WX_AUINOTEBOOK_H_
