
#ifndef _FloatableNotebook_H_
#define _FloatableNotebook_H_

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include "wxmodified/auibook2.h"
#include "uiutils.h"
#include "wx/frame.h"
#include <unordered_map>
#include <unordered_set>

class FnbEvent;
class FloatPageFrame;
class FloatableNotebook : public wxAuiNotebook2
{
public:
	FloatableNotebook(wxWindow* parent,
		Cloneable<wxAuiManager>* mgr = new Cloneable<wxAuiManager>,
		wxWindowID id = wxID_ANY,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize,
		long style = wxAUI_NB_DEFAULT_STYLE | wxAUI_NB_TAB_EXTERNAL_MOVE | wxNO_BORDER);

	virtual ~FloatableNotebook();

	bool Reparent(wxWindowBase* parent) override;

	void AddPage(wxWindow* page,
		const wxString& caption,
		bool select = false,
		const wxBitmap& bitmap = wxNullBitmap);

	wxWindow* AddFloatPage(wxWindow* page,
		const wxString& caption,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize,
		bool bShow = false,
		const wxBitmap& bitmap = wxNullBitmap);

	bool ShowPage(wxWindow* page);
	bool IsPageShown(wxWindow* page);

	bool ClosePage(wxWindow* page, bool remove = false);

	wxString SavePerspective();
	bool LoadPerspective(const wxString& s);

	wxWindow* m_topLevelWnd;
protected:
	struct PageInfo
	{
		wxAuiNotebookPage info;
		FloatableNotebook* nb;
		FloatableNotebook* nbTop;
	};
	struct Root
	{
		Root(FloatableNotebook* rootNB) :
			rootNB(rootNB) {}
		FloatableNotebook* rootNB;
		std::unordered_map<wxWindow*, PageInfo> pageInfos;
		std::unordered_set<FloatableNotebook*> NBs;
		std::unordered_map<wxString, wxWindow*> pageLookup;
	};

	FloatableNotebook(wxWindow* parent,
		Root* root,
		Cloneable<wxAuiManager>* mgr,
		wxWindowID id = wxID_ANY,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize,
		long style = wxAUI_NB_DEFAULT_STYLE | wxAUI_NB_TAB_EXTERNAL_MOVE | wxNO_BORDER);

	void Init();

	void AddPage(PageInfo& pi);
	wxWindow* AddFloatPage(PageInfo& pi,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize);

	void HideHint();
	
	void OnTabDragMotion(wxAuiNotebookEvent& evt);
	void OnPageClose(wxAuiNotebookEvent& evt);
	void OnPageChanged(wxAuiNotebookEvent& evt);
	void OnAllowNotebookDnD(wxAuiNotebookEvent& evt);
	void OnTabRightUp(wxAuiNotebookEvent& evt);

	void OnMotion(wxMouseEvent& evt);
	void OnLeftUp(wxMouseEvent& evt);
	void OnCaptureLost(wxMouseCaptureLostEvent& evt);
	void OnIdle(wxIdleEvent&);
	void OnMinimized(wxIconizeEvent& e);
	void OnSize(wxSizeEvent& e);
	void OnShow(wxShowEvent& e);

	wxString Save();
	bool Load(const wxString& s);
	void RemoveAllPages();

	void BindTopLevelWnd(wxWindow* parent);

	void OnPageAdded(wxWindow* page);
	void OnPageRemoved();
	
	wxAuiTabCtrl* m_locatingTab;
	wxAuiTabCtrl* m_deletingTab;
	FloatableNotebook* m_lastHintNB;
	Root* m_root;
	FloatPageFrame* m_frame;
	bool m_bShowFullHint;

	Cloneable<wxAuiManager>* m_mgrCB;

	wxDECLARE_EVENT_TABLE();
};

class FloatPageFrame : public wxFrame
{
public:
	FloatPageFrame(wxWindow *parent,
		wxWindowID id,
		wxWindow* focusWhenHide,
		const wxString& title,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize);

	bool Show(bool show = true) override;

	void OnNBTopReparent(FnbEvent& e);
	void OnShow(wxShowEvent& e);
	void OnParentDestory(wxWindowDestroyEvent& e);

	wxAuiManager m_mgr;
	bool m_isClosed;
	FloatableNotebook* m_nbTop;

protected:
	void OnClose(wxCloseEvent& e);
	wxWindow* m_focus;
	
	wxDECLARE_EVENT_TABLE();
};

wxDECLARE_EVENT(EVT_FLOATABLE_NOTEBOOK, FnbEvent);
class FnbEvent : public wxEvent
{
public:
	FnbEvent(FloatableNotebook* fnb, int winid)
		: wxEvent(winid, EVT_FLOATABLE_NOTEBOOK), nb(fnb) {}
	virtual ~FnbEvent() {}
	wxEvent *Clone() const wxOVERRIDE { return new FnbEvent(*this); }

	FloatableNotebook* nb;
};


#endif  // _FloatableNotebook_H_
