#pragma once
#include "wx/wxprec.h"
#include "handlelessevent.h"
#include <unordered_map>
#include <unordered_set>
#include "uiutils.h"

class HandlelessCtrl;

class HandlelessApp
{
public:
	static void SetFocus(HandlelessCtrl* ctrl);
	
	static HandlelessCtrl* GetFocus();
	
	static void Add(HandlelessCtrl* ctrl);
	
	static void Remove(HandlelessCtrl* ctrl);
	
	static void AddObject(wxObject* sizer);
	
	static bool FindObject(wxObject* sizer);
	
	static void RemoveObject(wxObject* sizer);

protected:
	HandlelessApp() : m_focusedCtrl(nullptr) {}

	static HandlelessApp m_app;
	HandlelessCtrl* m_focusedCtrl;
	std::unordered_set<wxObject*> m_objects;
};

class HandlelessContainer;

class HandlelessCtrl : public wxEvtHandler
{
public:
	HandlelessCtrl(HandlelessCtrl* parent = nullptr, int id = -1, bool show = true, const wxPoint& pos = wxPoint(),
		const wxSize& size = wxSize());
	
	int GetId();
	
	virtual ~HandlelessCtrl();

	void Show(bool show);
	
	bool RemoveChild(HandlelessCtrl* ctrl);
	
	wxSizerItem* JoinSizer(wxSizer* sizer, int proportion = 0, int flag = 0, int border = 0);
	
	wxSizerItem* JoinSizer(wxSizer* sizer, const wxSizerFlags& flags);

	wxSizerItem* JoinSizer(int pos, wxSizer* sizer, int proportion = 0, int flag = 0, int border = 0);

	wxSizerItem* JoinSizer(int pos, wxSizer* sizer, const wxSizerFlags& flags);
	
	void SetSizer(wxSizer* sizer, bool deleteOld = true);

	void SetSizerAndFit(wxSizer* sizer, bool deleteOld = true);

	const wxPoint& GetLocation() const;
	
	const wxRect& GetRect() const;

	wxRect GetLocationRect() const;
	 
	void Move(int x, int y, bool refresh = true);
	
	void Move(const wxPoint& pt, bool refresh = true);
	
	void SetSize(int width, int height);
	
	void SetSize(const wxSize& size);

	void Layout();
	
	void SetFocus();

	void Refresh();

	void Idle(wxIdleEvent& e);

	HandlelessCtrl* LocateCtrl(const wxPoint& pt, std::function<bool(HandlelessCtrl*)> fSkip = [](HandlelessCtrl*) { return false; });
	HandlelessCtrl* LocateCtrl(int x, int y, std::function<bool(HandlelessCtrl*)> fSkip = [](HandlelessCtrl*) { return false; });
	
	HandlelessCtrl* GetFocus();

	HandlelessCtrl* GetParent();

	class SizerChecker : public wxObject
	{
	public:
		SizerChecker(HandlelessCtrl& ctrl) : owner(ctrl) {}
		~SizerChecker() { owner.sizerChecker = nullptr; }
		HandlelessCtrl& owner;
		wxSizer* sizer;
		wxSizerItem* sizerItem;
	};
	
	wxColor backgroundColor;
	wxString toolTip;
	bool skipMouseEvt;
	SizerChecker* sizerChecker;

protected:
	void Invalidate(const wxRect& rect);
	
	void OnPaint(HCPaintEvent& e);
	
	virtual bool DoMove(int x, int y);

	bool DoMove(const wxPoint& point);
	
	virtual bool DoSetSize(int width, int height);

	bool DoSetSize(const wxSize& size);

	void DoLayout(bool redraw);
	
	void UpdateLocations(bool updateChildren = true);
	
	void Paint(wxDC& dc, const wxPoint& dcLocation, wxRect clipRect);

	HandlelessCtrl* m_parent;
	HandlelessContainer* m_container;
	int m_id;
	wxRect m_rect;
	wxPoint m_location;
	wxSizer* m_internalSizer;
	std::vector<HandlelessCtrl*> m_ctrls;
	bool m_show;
};

class HandlelessContainer : public HandlelessCtrl
{
public:
	HandlelessContainer(wxWindow* window = nullptr);
	
	virtual ~HandlelessContainer();
	
	void SetWindow(wxWindow* window);
	
	void UnInit();
	
	void InvalidateRect(const wxRect& rect);

protected:
	bool DoSetSize(int width, int height) wxOVERRIDE;

	bool DoMove(int x, int y) wxOVERRIDE;

	void OnMouseEvent(wxMouseEvent& e);

	void ProcessMouseEvent(HandlelessCtrl* ctrl, wxMouseEvent& e, const wxEventType& evtType);

	void OnSize(wxSizeEvent& e);

	void OnEraseBackground(wxEraseEvent& e);
	
	void OnPaint(wxPaintEvent& e);

	void OnIdle(wxIdleEvent& e);

	void OnScrollChanged(wxScrollWinEvent& e);

	void HandleScrollChanged(bool updateLocation);
	
	wxWindow* m_wnd;
	bool m_firstPaint;
	HandlelessCtrl* m_lastEnteredCtrl;
	wxEvtHandler m_evtHandler;
	wxSizeEvent* m_sizeEvt;
	wxScrollWinEvent* m_scrollEvt;
	wxPoint m_scrollPos;
};