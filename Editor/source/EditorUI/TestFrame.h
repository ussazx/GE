#pragma once

#include "wx/wxprec.h"
#include "wx/dcgraph.h"
#include "wx/msw/private/winstyle.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#include "common/floatablenotebook.h"
#include "common/uimanager.h"
#include "common/uiutils.h"

#define DEFAULT_LAYOUT "notebook_layout0/1<panel>0|2<page0>0|3<page1>0|4<page2>0|*|layout2|name=dummy;caption=;state=2098174;dir=3;layer=0;row=0;pos=0;prop=100000;bestw=225;besth=225;minw=225;minh=225;maxw=-1;maxh=-1;floatx=-1;floaty=-1;floatw=-1;floath=-1|name=1;caption=;state=2098172;dir=5;layer=0;row=0;pos=0;prop=100000;bestw=250;besth=250;minw=-1;minh=-1;maxw=-1;maxh=-1;floatx=-1;floaty=-1;floatw=-1;floath=-1|name=2;caption=;state=2098172;dir=4;layer=1;row=0;pos=0;prop=100000;bestw=490;besth=338;minw=-1;minh=-1;maxw=-1;maxh=-1;floatx=-1;floaty=-1;floatw=-1;floath=-1|name=3;caption=;state=2098172;dir=3;layer=2;row=0;pos=0;prop=100000;bestw=225;besth=225;minw=-1;minh=-1;maxw=-1;maxh=-1;floatx=-1;floaty=-1;floatw=-1;floath=-1|name=4;caption=;state=2098172;dir=2;layer=3;row=0;pos=0;prop=100000;bestw=225;besth=225;minw=-1;minh=-1;maxw=-1;maxh=-1;floatx=-1;floaty=-1;floatw=-1;floath=-1|dock_size(5,0,0)=20|dock_size(4,1,0)=182|dock_size(3,2,0)=203|dock_size(2,3,0)=227|/"

// -- application --

class EventBase
{

};

class EvtHandlerBase
{
public:
	virtual void Process(EventBase& e) = 0;
};

template<class ClassT, class EventT>
class EvtHandler : public EvtHandlerBase
{
public:
	void Process(EventBase& e) override
	{
		(m_class->*m_func)(static_cast<EventT&>(e));
	}
	void (ClassT::*m_func)(EventT& e);
	ClassT* m_class;
};

template<class ClassT, class EventT>
void Bind(unsigned int evtType, void(ClassT::*Func)(EventT&), ClassT* classObj)
{
	EvtHandler<ClassT, EventT>* evtHandler = new EvtHandler<ClassT, EventT>;
	evtHandler->m_func = Func;
	evtHandler->m_class = classObj;
}

class EventA : public EventBase
{
public:
	int a;
};

class A1
{
public:
	void Fun(EventA& a)
	{
		int aa = a.a;
		aa = 5;
	}
};

class WndlessEvtHandler : public wxEvtHandler
{
public:
	void OnPaint(wxPaintEvent& e)
	{
		e.Skip();
	}
	void OnEraseBackground(wxEraseEvent&) {}
};

struct Q
{
	static void Fun(int a) {}
};

class WndLessFrame : public wxFrame
{
public:
	WndLessFrame(wxWindow* parent, wxWindowID id, const wxString& title)
		: wxFrame(parent, id, title, wxPoint(wxSCREEN_CENTER_WH(1000, 800)), wxSize(1000, 800), wxDEFAULT_FRAME_STYLE)
	{
		A1 a;
		EvtHandler<A1, EventA> eh;
		eh.m_class = &a;
		eh.m_func = &A1::Fun;

		EvtHandlerBase& b = eh;
		EventA ea;
		ea.a = 1;
		b.Process(ea);

		std::function<void(int)> f = &Q::Fun;

		wxMSWWinExStyleUpdater updateExStyle(GetHwnd());
		updateExStyle.TurnOn(WS_EX_LAYERED).Apply();

		wxBoxSizer *sizer = new wxBoxSizer(wxVERTICAL);
		SetSizer(sizer);

		wxAuiNotebook* nb = new wxAuiNotebook(this, m_ctrls.size());
		nb->SetArtProvider(new wxAuiGenericTabArt);
		//nb->Bind(wxEVT_PAINT, &WndLessFrame::OnPaint, this, m_ctrls.size(), m_ctrls.size());
		sizer->Add(nb, 1, wxEXPAND);
		m_ctrls.push_back(nb);

		//wxWindow* w = new wxWindow(this, -1, wxPoint(20, 20), wxSize(500, 500));

		//wxButton* b = new wxButton(w, 1, "", wxPoint(0, 0), wxSize(200, 20));
		//WndlessEvtHandler* e = new WndlessEvtHandler;
		//b->PushEventHandler(e);
		//e->Bind(wxEVT_PAINT, &WndlessEvtHandler::OnPaint, e);
		//b->Bind(wxEVT_BUTTON, &WndLessFrame::OnButton, this, 1);
		//m_ctrls.push_back(b);

		Bind(wxEVT_ERASE_BACKGROUND, &WndLessFrame::OnEraseBackground, this);

		Bind(wxEVT_PAINT, &WndLessFrame::OnPaint, this);
		Bind(wxEVT_LEFT_DOWN, &WndLessFrame::OnLeftDown, this);
		Bind(wxEVT_LEFT_UP, &WndLessFrame::OnLeftUp, this);
		Bind(wxEVT_MOTION, &WndLessFrame::OnMotion, this);
		Bind(wxEVT_IDLE, &WndLessFrame::OnIdle, this);
	}
	void OnEraseBackground(wxEraseEvent&)
	{

	}
	void OnButton(wxCommandEvent&)
	{
		int a = 5;
	}
	void OnLeftDown(wxMouseEvent&)
	{
		CaptureMouse();
	}
	void OnLeftUp(wxMouseEvent&)
	{
		if (GetCapture() == this)
			ReleaseMouse();
	}
	void OnMotion(wxMouseEvent&)
	{
		if (GetCapture() != this)
			return;
	}
	void OnPaint(wxPaintEvent& e)
	{
	}
	void OnIdle(wxIdleEvent& e)
	{
		//Refresh();
		wxClientDC pdc(this);

		wxBitmap bmp(GetSize());
		bmp.UseAlpha();
		wxMemoryDC mdc(bmp);

		wxGCDC gcdc;
		gcdc.SetGraphicsContext(wxGraphicsRenderer::GetDefaultRenderer()->CreateContext(mdc));

		gcdc.SetPen(*wxTRANSPARENT_PEN);
		gcdc.SetBrush(wxBrush(wxColour(80, 80, 80, 220)));
		gcdc.DrawRectangle(wxRect(wxPoint(0, 0), GetSize()));

		//for (int i = 0; i < m_ctrls.size(); i++)
		//{
		//	wxRect s = m_ctrls[i]->GetRect();
		//	gcdc.SetBrush(wxBrush(wxColour(0, 255, 255, 150)));
		//	gcdc.DrawRectangle(wxRect(m_ctrls[i]->GetRect().GetPosition(), m_ctrls[i]->GetRect().GetSize()));
		//}

		BLENDFUNCTION bf;
		bf.AlphaFormat = AC_SRC_ALPHA;
		bf.BlendFlags = 0;
		bf.BlendOp = AC_SRC_OVER;
		bf.SourceConstantAlpha = 255;

		POINT pt;
		POINT pts;
		SIZE  sz;
		pt.x = GetScreenPosition().x;
		pt.y = GetScreenPosition().y;

		sz.cx = GetRect().GetSize().GetWidth();
		sz.cy = GetRect().GetSize().GetHeight();
		pts.x = pts.y = 0;

		::UpdateLayeredWindow(GetHwnd(), pdc.GetHDC(), &pt, &sz, mdc.GetHDC(), &pts, 0, &bf, ULW_ALPHA);

		for (size_t i = 0; i < m_ctrls.size(); i++)
		{
			m_ctrls[i]->Refresh();
		}
	}
private:
	std::vector<wxWindow*> m_ctrls;
};

class wxSizeReportCtrl : public wxControl
{
public:

	wxSizeReportCtrl(wxWindow* parent, wxWindowID id = wxID_ANY,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize,
		wxAuiManager* mgr = NULL)
		: wxControl(parent, id, pos, size, wxNO_BORDER)
	{
		m_mgr = mgr;
		SetBackgroundStyle(wxBG_STYLE_PAINT);
	}

private:

	void OnPaint(wxPaintEvent& WXUNUSED(evt))
	{
		wxPaintDC dc(this);
		wxSize size = GetClientSize();
		wxString s;
		int h, w, height;

		s.Printf("Size: %d x %d", size.x, size.y);

		dc.SetFont(*wxNORMAL_FONT);
		dc.GetTextExtent(s, &w, &height);
		height += FromDIP(3);
		dc.SetBrush(wxColor(100, 100, 100));
		dc.SetPen(wxColor(100, 100, 100));
		dc.DrawRectangle(0, 0, size.x, size.y);
		dc.SetPen(*wxLIGHT_GREY_PEN);
		dc.DrawLine(0, 0, size.x, size.y);
		dc.DrawLine(0, size.y, size.x, 0);
		dc.DrawText(s, (size.x - w) / 2, ((size.y - (height * 5)) / 2));

		if (m_mgr)
		{
			wxAuiPaneInfo pi = m_mgr->GetPane(this);

			s.Printf("Layer: %d", pi.dock_layer);
			dc.GetTextExtent(s, &w, &h);
			dc.DrawText(s, (size.x - w) / 2, ((size.y - (height * 5)) / 2) + (height * 1));

			s.Printf("Dock: %d Row: %d", pi.dock_direction, pi.dock_row);
			dc.GetTextExtent(s, &w, &h);
			dc.DrawText(s, (size.x - w) / 2, ((size.y - (height * 5)) / 2) + (height * 2));

			s.Printf("Position: %d", pi.dock_pos);
			dc.GetTextExtent(s, &w, &h);
			dc.DrawText(s, (size.x - w) / 2, ((size.y - (height * 5)) / 2) + (height * 3));

			s.Printf("Proportion: %d", pi.dock_proportion);
			dc.GetTextExtent(s, &w, &h);
			dc.DrawText(s, (size.x - w) / 2, ((size.y - (height * 5)) / 2) + (height * 4));
		}
	}

	void OnEraseBackground(wxEraseEvent& WXUNUSED(evt))
	{
		// intentionally empty
		int a = 5;
	}

	void OnSize(wxSizeEvent& WXUNUSED(evt))
	{
		Refresh();
	}
private:

	wxAuiManager* m_mgr;

	wxDECLARE_EVENT_TABLE();
};

class SceneWindow : public wxWindow
{
public:
	SceneWindow(wxWindow* parent, wxWindowID id = wxID_ANY,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize)
		: wxWindow(parent, id, pos, size, wxNO_BORDER)
	{
		SetBackgroundStyle(wxBG_STYLE_PAINT);
	}
private:
	void OnIdle(wxIdleEvent& evt)
	{
		//Render();
	}
	void OnPaint(wxPaintEvent& evt)
	{
		//Render();
	}
	void OnSize(wxSizeEvent& evt)
	{
		//InitDevice(GetHwnd());
	}

	wxDECLARE_EVENT_TABLE();
};

#define SIZE_PACK4(x) ((x) + ((x) % 4 > 0 ? 4 - (x) % 4 : 0))

class TestFrame : public wxFrame
{
public:
	TestFrame(wxWindow* parent,
		wxWindowID id,
		const wxString& title,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize,
		long style = wxDEFAULT_FRAME_STYLE)
		: wxFrame(parent, id, title, pos, size, style)
	{
		//XGI_VULKAN_ENV_PARAM vkParam{};
		//vkParam.hInst = wxGetInstance();
		//vkParam.vulkanLayerFlags = XGI_VkLayer_Validate;
		//vkParam.vulkanExtensionFlags = XGI_VkExtension_DebugReport;
		//bool b = xgiInitializeVulkan(vkParam);

		//char* cc = &{1, 2, 3};

		//xgiInstance* xi = xgiGetInstanceVulkan();


		//xgiFrameBufferDescTemp<1, 1> fbd;
		//fbd.subpassArray[0].rtvs[0].format = XGI_Format_R8G8B8A8_UNORM;
		//fbd.subpassArray[0].rtvs[0].format = XGI_Format_R8G8B8A8_UNORM;
		//fbd.subpassArray[0].rtvs[0].resolved = false;
		//fbd.subpassArray[0].rtvs[0].store = false;
		//fbd.swapchain.lastSubpassRtvIdx = 

		//fbd.width = fbd.height = 500;

		SetMinSize(FromDIP(wxSize(400, 300)));

		//Test2(); return;

		m_mgr.SetManagedWindow(this);

		m_pNoteBook = new FloatableNotebook(this, new UiManager);

		//m_pPage0 = new wxSizeReportCtrl(this, wxID_ANY,
		//	wxDefaultPosition, wxDefaultSize);
		//m_pPage0->SetName(_("page0"));
		//m_pNoteBook->AddPage(m_pPage0, "page0");

		m_pSubNoteBook = new FloatableNotebook(m_pNoteBook, new UiManager);
		m_pPage0 = m_pSubNoteBook;
		auto p0 = new wxSizeReportCtrl(m_pNoteBook, wxID_ANY,
			wxDefaultPosition, wxDefaultSize);
		p0->SetName("p0");
		m_pSubNoteBook->AddPage(p0, "p0");
		auto p1 = new wxSizeReportCtrl(m_pNoteBook, wxID_ANY,
			wxDefaultPosition, wxDefaultSize);
		p1->SetName("p1");
		m_pSubNoteBook->AddPage(p1, "p1");
		auto p2 = new wxSizeReportCtrl(m_pNoteBook, wxID_ANY,
			wxDefaultPosition, wxDefaultSize);
		p1->SetName("p2");
		m_pSubNoteBook->AddPage(p2, "p2");
		//wxAuiManager* am = new wxAuiManager;
		//am->SetManagedWindow(nb);
		//am->AddPane(nb, wxAuiPaneInfo().Caption("Notebook").CenterPane());
		//am->Update();
		unsigned int extraFlags = m_pSubNoteBook->GetSpecMgr<UiManager>()->GetExtraFlags();
		extraFlags ^= UiManager::EntirelyLayoutResize;
		m_pSubNoteBook->GetSpecMgr<UiManager>()->SetExtraFlags(extraFlags);
		m_pPage0->SetName(_("page0"));
		m_pNoteBook->AddPage(m_pPage0, "page0");

		//m_pPage1 = new wxSizeReportCtrl(this, wxID_ANY,
		//	wxDefaultPosition, wxDefaultSize);
		//m_pPage1->SetName(_("page1"));
		//AddPage(p1, "page1");
		//m_pNoteBook->AddFloatPage(m_pPage1, "page1", wxDefaultPosition, wxSize(500, 300));

		//m_pPage1->SetForegroundColour(wxColor(0, 0, 0));

		int linesize = (155 * 24 + 31) / 8;
		int a = SIZE_PACK4(155 * 24 / 8);

		m_pPage2 = new wxSizeReportCtrl(this, wxID_ANY,
			wxDefaultPosition, wxDefaultSize);
		m_pPage2->SetName(_("page2"));
		m_pNoteBook->AddPage(m_pPage2, "page2");

		m_pPage3 = new SceneWindow(this);
		m_pPage3->SetName("Page3");
		m_pNoteBook->AddPage(m_pPage3, "page3");

		//m_pNoteBook->LoadPerspective(DEFAULT_LAYOUT);

		m_mgr.AddPane(m_pNoteBook, wxAuiPaneInfo().Caption("Notebook").CenterPane());

		m_mgr.Update();

		wxMenuBar* mb = new wxMenuBar;

		wxMenu* m = new wxMenu;
		m->Append(ID_Save, _("Save"));
		m->Append(ID_Load, _("Load"));
		m->Append(ID_LoadDefault, _("Load Default"));

		wxMenu* m1 = new wxMenu;
		m1->AppendCheckItem(ID_ShowP0, _("Show Page0"));
		m1->AppendCheckItem(ID_ShowP1, _("Show Page1"));
		m1->AppendCheckItem(ID_ShowP2, _("Show Page2"));
		m1->AppendCheckItem(ID_ShowP3, _("Show Page3"));

		wxMenu* m2 = new wxMenu;
		m2->AppendCheckItem(DockLiveResize, _("Dock Live Resize"));
		m2->AppendCheckItem(DockContiguousResize, _("Dock Contiguous Resize"));
		m2->AppendCheckItem(EntirelyLayoutResize, _("Entirely Layout Resize"));
		m2->AppendCheckItem(EntirelyLayoutResize + 1, _("Hide"));

		mb->Append(m, _("Perspective"));
		mb->Append(m1, _("Show Page"));
		mb->Append(m2, _("Layout"));

		SetMenuBar(mb);
	}

private:
	enum
	{
		ID_Save = wxID_HIGHEST + 1,
		ID_Load,
		ID_LoadDefault,
		ID_ShowP0,
		ID_ShowP1,
		ID_ShowP2,
		ID_ShowP3,
		DockLiveResize,
		DockContiguousResize,
		EntirelyLayoutResize
	};

	void Test2()
	{
		wxWindow* p1 = new wxSizeReportCtrl(this, wxID_ANY, wxPoint(0, 0), wxSize(500, 100));
		wxWindow* p2 = new wxSizeReportCtrl(this, wxID_ANY, wxPoint(0, 100), wxSize(400, 200));
		p2->SetBackgroundColour(wxColor(100, 100, 100));

		wxSizer* s = new wxBoxSizer(wxHORIZONTAL);
		SetSizer(s);

		wxSizer* s1 = new wxBoxSizer(wxVERTICAL);
		s1->Add(p1, 0, wxEXPAND);

		wxSizer* s2 = new wxBoxSizer(wxVERTICAL);
		s2->Add(p2, 0, wxEXPAND);

		s->Add(s1, 0, wxEXPAND);
		s->Add(s2, 0, wxEXPAND);
		//s->Add(p2, 1, wxEXPAND)->SetProportion(10000);
	}

	void Test()
	{
		m_mgr.SetManagedWindow(this);

		//m_myMgr.GetArtProvider()->SetColor(wxAUI_DOCKART_SASH_COLOUR, wxColor(255, 0, 0));

		wxWindow* p1 = new wxSizeReportCtrl(this, wxID_ANY, wxPoint(0, 0), wxSize(200, 200));
		m_mgr.AddPane(p1, wxAuiPaneInfo().Name("test1").Caption("test1").Top().MinSize(100, 100));

		wxWindow* p2 = new wxSizeReportCtrl(this, wxID_ANY, wxPoint(0, 0), wxSize(200, 200));
		m_mgr.AddPane(p2, wxAuiPaneInfo().Name("test2").Caption("test2").Left().Layer(1).MinSize(100, 100));

		wxWindow* p21 = new wxSizeReportCtrl(this, wxID_ANY, wxPoint(0, 0), wxSize(200, 200));
		m_mgr.AddPane(p21, wxAuiPaneInfo().Name("test21").Caption("test21").Left().Layer(0));

		wxWindow* p22 = new wxSizeReportCtrl(this, wxID_ANY, wxPoint(0, 0), wxSize(200, 200));
		m_mgr.AddPane(p22, wxAuiPaneInfo().Name("test22").Caption("test22").Left().Layer(0));

		wxWindow* p3 = new wxSizeReportCtrl(this, wxID_ANY, wxPoint(0, 0), wxSize(200, 200));
		m_mgr.AddPane(p3, wxAuiPaneInfo().Name("test3").Caption("test3").Center());

		wxWindow* p4 = new wxSizeReportCtrl(this, wxID_ANY, wxPoint(0, 0), wxSize(200, 200));
		m_mgr.AddPane(p4, wxAuiPaneInfo().Name("test4").Caption("test4").Bottom());

		wxWindow* p5 = new wxSizeReportCtrl(this, wxID_ANY, wxPoint(0, 0), wxSize(200, 200));
		m_mgr.AddPane(p5, wxAuiPaneInfo().Name("test5").Caption("test5").Bottom());

		m_mgr.Update();
	}

	void OnMenuLayout(wxCommandEvent& e)
	{
		unsigned int flags = m_pNoteBook->GetAuiManager().GetFlags();
		unsigned int extraFlags = m_pNoteBook->GetSpecMgr<UiManager>()->GetExtraFlags();

		switch (e.GetId())
		{
		case DockLiveResize:
			flags ^= wxAUI_MGR_LIVE_RESIZE;
			break;
		case DockContiguousResize:
			extraFlags ^= UiManager::DockContiguousResize;
			break;
		case EntirelyLayoutResize:
			extraFlags ^= UiManager::EntirelyLayoutResize;
			break;
		case EntirelyLayoutResize + 1:
			Hide();
			break;
		}

		m_pNoteBook->GetSpecMgr()->SetFlags(flags);
		m_pNoteBook->GetSpecMgr<UiManager>()->SetExtraFlags(extraFlags);
	}

	void OnMenuPerspective(wxCommandEvent& e)
	{
		switch (e.GetId())
		{
		case ID_Save:
			s = m_pNoteBook->SavePerspective();
			s1 = m_pSubNoteBook->SavePerspective();
			break;
		case ID_Load:
			m_pNoteBook->LoadPerspective(s);
			m_pSubNoteBook->LoadPerspective(s1);
			break;
		case ID_LoadDefault:
			m_pNoteBook->LoadPerspective(DEFAULT_LAYOUT);
			break;
		}
	}

	void OnMenuShowPage(wxCommandEvent& e)
	{
		switch (e.GetId())
		{
		case ID_ShowP0:
			m_pNoteBook->ShowPage(m_pPage0);
			break;
		case ID_ShowP1:
			m_pNoteBook->ShowPage(m_pPage1);
			break;
		case ID_ShowP2:
			m_pNoteBook->ShowPage(m_pPage2);
			break;
		case ID_ShowP3:
			m_pNoteBook->ShowPage(m_pPage3);
			break;
		}
	}

	void OnUpdateUI(wxUpdateUIEvent& e)
	{
		switch (e.GetId())
		{
		case ID_ShowP0:
			e.Check(m_pNoteBook->IsPageShown(m_pPage0));
			break;
		case ID_ShowP1:
			e.Check(m_pNoteBook->IsPageShown(m_pPage1));
			break;
		case ID_ShowP2:
			e.Check(m_pNoteBook->IsPageShown(m_pPage2));
			break;
		case ID_ShowP3:
			e.Check(m_pNoteBook->IsPageShown(m_pPage3));
			break;
		case DockLiveResize:
			e.Check(m_pNoteBook->GetAuiManager().GetFlags() & wxAUI_MGR_LIVE_RESIZE);
			break;
		case DockContiguousResize:
			e.Check(m_pNoteBook->GetSpecMgr<UiManager>()->GetExtraFlags() & UiManager::DockContiguousResize);
			break;
		case EntirelyLayoutResize:
			e.Check(m_pNoteBook->GetSpecMgr<UiManager>()->GetExtraFlags() & UiManager::EntirelyLayoutResize);
			break;
		}
	}

	wxAuiManager m_mgr;
	wxString s;
	wxString s1;
	wxArrayString m_perspectives;
	wxMenu* m_perspectives_menu;
	long m_notebook_style;
	long m_notebook_theme;

	SceneWindow* m_pScene;
	FloatableNotebook* m_pNoteBook;
	FloatableNotebook* m_pSubNoteBook;
	long m_nbFlags;

	wxWindow* m_pPage0;
	wxWindow* m_pPage1;
	wxWindow* m_pPage2;
	wxWindow* m_pPage3;

	wxDECLARE_EVENT_TABLE();
};