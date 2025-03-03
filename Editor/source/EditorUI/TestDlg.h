#pragma once

#include "common/uiutils.h"
#include "common/floatablenotebook.h"
#include "common/uimanager.h"
#include "common/button.h"
#include "common/selectionpanel.h"
#include "common/HandlelessCtrl.h"
#include "common/quicktextentry.h"
#include "wx/artprov.h"

Button* browseBtn;

class TestDlg : public wxFrame
{
public:
	TestDlg(wxWindow* parent, wxWindowID id, const wxString& title)
		: wxFrame(parent, id, title, wxPoint(wxSCREEN_CENTER_WH(1000, 800)), wxSize(1100, 800),
			wxCAPTION | wxCLOSE_BOX | wxRESIZE_BORDER |
			wxFRAME_NO_WINDOW_MENU)
	{
		SetBackgroundColour(wxColor(62, 62, 62));

		wxBoxSizer *sizer = new wxBoxSizer(wxVERTICAL);
		SetSizer(sizer);

		wxAuiNotebook* nb = new wxAuiNotebook(this, wxID_ANY);
		nb->SetArtProvider(new wxAuiGenericTabArt);
		nb->SetTabCtrlHeight(50);
		nb->SetWindowStyleFlag(nb->GetWindowStyleFlag() & 
			~(wxAUI_NB_CLOSE_BUTTON |
			wxAUI_NB_CLOSE_ON_ACTIVE_TAB |
			wxAUI_NB_CLOSE_ON_ALL_TABS |
			wxAUI_NB_TAB_MOVE |
			wxAUI_NB_TAB_SPLIT));
		sizer->Add(nb, 1, wxEXPAND);

		panelLoad = new wxPanel(this, wxID_ANY);
		panelLoad->SetBackgroundColour(GetBackgroundColour());
		nb->AddPage(panelLoad, _("Load Project"));

		panelLoad->Bind(wxEVT_LEFT_DOWN, &TestDlg::OnLeftDown, this);
		panelLoad->GetEventHandler()->Bind(wxEVT_LEFT_DOWN, &TestDlg::OnLeftDown, this);

		wxSizer* s = new wxBoxSizer(wxVERTICAL);
		panelLoad->SetSizer(s);

		m_textCtrl = new wxTextCtrl(panelLoad, wxID_ANY, "",
			wxDefaultPosition, wxSize(300, wxDefaultCoord), 0);
		m_textCtrl->Bind(wxEVT_TEXT, &TestDlg::OnTextChanged, this);
		m_textCtrl->SetMargins(wxPoint(5, -1));
		m_textCtrl->SetHint(_("Filter projects..."));
		s->Add(m_textCtrl, wxSizerFlags().Expand().Border(wxALL, 20));

		m_selTable = new SelectionPanel<wxString>(panelLoad);
		//m_selTable->SetBackgroundColour(GetBackgroundColour());
		s->Add(m_selTable, wxSizerFlags(1).Expand().Border(wxLEFT | wxRIGHT, 10));

		//m_selTable->Bind(wxEVT_ERASE_BACKGROUND, &TestDlg::OnSelectionTableEraseBG, this);
		m_selTable->hc.Bind(EVT_HC_PAINT, &TestDlg::OnSelectionTablePaint, this);

		//m_selTable->flags = SEL_IGNORE_LOST_FOCUS | SEL_IGNORE_CANCEL | SEL_RIGHT_ONLY_UP;
		m_selTable->flags = SEL_IGNORE_LOST_FOCUS | SEL_IGNORE_CANCEL | SEL_RIGHT_ONLY_UP;
		m_selTable->Bind(EVT_SELTAB_SELECT<wxString>, &TestDlg::OnSelTabSelect, this);
		m_selTable->Bind(EVT_SELTAB_SELECT<wxString>, &TestDlg::OnSelTabSelect0, this);
		m_selTable->Bind(EVT_SELTAB_COMPARE<wxString>, &TestDlg::OnSelTabCompare, this);
		m_selTable->Bind(EVT_SELTAB_FILTER<wxString>, &TestDlg::OnSelTabFilter, this);

		m_nameEntry = new QuickTextEntry(m_selTable, wxID_ANY);

		m_itemImage.LoadFile("scene.png");
		m_itemImage2.LoadFile("scene.png");
		for (int i = 0; i < 5; i++)
		{
			uint32_t n = m_selTable->GetCount();
			wxString s = wxString().Format("project%u", n);
			m_selTable->Add(s, new ImageItemRender(m_itemImage, s), "C:\\Users\\asus\\Desktop\\vs\\GE");
		}

		s->AddSpacer(50);

		wxGridSizer* gs = new wxGridSizer(2);
		s->Add(gs, 0, wxALIGN_RIGHT);

		browseBtn = new Button(panelLoad, wxID_ANY, _("Browse..."));
		browseBtn->SetSize(50, 10);
		gs->Add(browseBtn);
		

		m_openBtn = new Button(panelLoad, wxID_ANY, _("Load"));
		m_openBtn->SetSize(50, 10);
		gs->Add(m_openBtn, wxSizerFlags().Border(wxRIGHT | wxBOTTOM, 20));
		

		//return;

		panelNew = new wxPanel(this);
		nb->AddPage(panelNew, _("New Project"));
		panelNew->Bind(wxEVT_LEFT_DOWN, &TestDlg::OnLeftDown, this);

		//s = new wxBoxSizer(wxVERTICAL);
		//panelNew->SetSizer(s);

		//m_openBtn = new Button(panelNew, wxID_ANY, _("Load1"));
		//s->Add(m_openBtn, 1, wxEXPAND);

		//m_openBtn = new Button(panelNew, wxID_ANY, _("Load2"));
		//s->Add(m_openBtn, 1, wxEXPAND);
		
		m_hc.SetWindow(panelNew);
		m_hc.backgroundColor = wxColor(100, 100, 100);

		p1 = new HandlelessCtrl(&m_hc, -1, true, wxPoint(30, 50), wxSize(100, 300));
		p1->backgroundColor = wxColor(200, 200, 200, 200);

		p2 = new HandlelessCtrl(&m_hc, -1, true, wxPoint(50, 50), wxSize(100, 300));
		p2->backgroundColor = wxColor(255, 0, 0, 255);

		s = new wxBoxSizer(wxVERTICAL);
		//wxSizer* s1 = new wxGridSizer(1);
		//s->Add(s1, 0, wxEXPAND);
		//m_hc.SetSizer(s);

		auto si = p1->JoinSizer(s, wxSizerFlags(0).Border());
		//si->SetMinSize(100, 0);
		//si->SetInitSize(100, 0);

		si = p2->JoinSizer(s, wxSizerFlags(1).Border());
		si->SetMinSize(100, 0);

		m_hc.SetSizer(s);
		//m_hc.SetSizerAndFit(s);

		//s = new wxBoxSizer(wxHORIZONTAL);

		//wxBoxSizer* wrapSizer = new wxWrapSizer(wxHORIZONTAL);
		//s = new wxBoxSizer(wxVERTICAL);
		//s->Add(wrapSizer, 0, wxEXPAND);

		//p1->SetSizer(s);


		//HandlelessCtrl* p3 = new HandlelessCtrl(p1, -1, wxPoint(10, 30), wxSize(100, 300));
		//p3->backgroundColor = wxColor(0, 255, 255, 100);
		//p3->Bind(wxEVT_LEFT_DOWN, &TestDlg::OnHCMouseEvent, this);
		//p3->Bind(wxEVT_ENTER_WINDOW, &TestDlg::OnEnter, this);
		//p3->Bind(wxEVT_LEAVE_WINDOW, &TestDlg::OnLeave, this);
		//p3->JoinSizer(wrapSizer, wxSizerFlags(1).Border().Center());

		//HandlelessCtrl* p4 = new HandlelessCtrl(p1, -1, wxPoint(10, 30), wxSize(100, 300));
		//p4->backgroundColor = wxColor(0, 255, 255, 100);
		//p4->JoinSizer(s, wxSizerFlags(0).Border(wxALL, 10).Expand());

		//panelNew->Bind(wxEVT_PAINT, &TestDlg::OnPanelNewPaint, this);
		//panelNew->Bind(wxEVT_ERASE_BACKGROUND, &TestDlg::OnPanelNewEB, this);
		//s->Add(new ImageCtrl(panelNew, image, false , wxID_ANY, wxPoint(), wxSize(200, 200)), 1, wxEXPAND);

		Layout();

		m_openBtn->Enable(m_selTable->GetSelectedCount() > 0);
	}

private:
	void OnEnter(wxMouseEvent& e)
	{
		HandlelessCtrl* p = (HandlelessCtrl*)e.GetEventObject();
		p->backgroundColor = wxColor(255, 255, 255, 100);
		p->Refresh();
	}
	void OnLeave(wxMouseEvent& e)
	{
		HandlelessCtrl* p = (HandlelessCtrl*)e.GetEventObject();
		p->backgroundColor = wxColor(100, 100, 100, 100);
		p->Refresh();
	}
	void OnHCMouseEvent(wxMouseEvent& e)
	{
		HandlelessCtrl* p = (HandlelessCtrl*)e.GetEventObject();
		static bool b;
		p->backgroundColor = b ? wxColor(255, 255, 0, 100) : wxColor(0, 255, 255, 100);
		p->Refresh();
		b = !b;
		e.Skip();
	}
	void OnPanelNewEB(wxEraseEvent&)
	{

	}
	void OnPanelNewPaint(wxPaintEvent&)
	{

	}
	void OnSelTabSelect0(SelectionPanelEvent<wxString>& e)
	{
		e.Skip();
	}
	void OnSelTabSelect(SelectionPanelEvent<wxString>& e)
	{
		bool selected = m_selTable->GetSelectedCount() > 0;
		m_openBtn->Enable(selected);
		if (selected && e.byRight)
		{
			wxMenu menu;
			wxMenuItem* mi = new wxMenuItem(nullptr, ShowInExplorer, _("Open located directory"));
			mi->SetBitmap(wxArtProvider::GetIcon(wxART_GO_DIR_UP, wxART_MENU));
			menu.Append(mi);
			mi = new wxMenuItem(nullptr, Rename, _("Rename project"));
			mi->SetBitmap(wxArtProvider::GetIcon(wxART_FIND_AND_REPLACE, wxART_MENU));
			menu.Append(mi);
			mi = new wxMenuItem(nullptr, Delete, _("Delete project"));
			mi->SetBitmap(wxArtProvider::GetIcon(wxART_DELETE, wxART_MENU));
			menu.Append(mi);
			menu.Bind(wxEVT_MENU, &TestDlg::OnItemMenu, this);
			PopupMenu(&menu, ScreenToClient(wxGetMousePosition()));
		}
	}
	void OnSelTabCompare(SelectionPanelEvent<wxString>& e)
	{
		//e.resultLess = true;
	}
	void OnSelTabFilter(SelectionPanelEvent<wxString>& e)
	{
		e.passed = m_textCtrl->IsEmpty() || e.filterData->Find(m_textCtrl->GetValue()) != wxString::npos;
	}
	void OnTextChanged(wxCommandEvent& e)
	{
		m_selTable->Relayout();
	}
	void OnItemMenu(wxCommandEvent& e)
	{
		if (e.GetId() == ShowInExplorer)
			;// ShellExecute(NULL, NULL, _T("explorer"), "/select, " + selectedItem->data, NULL, SW_SHOW);
		else if (e.GetId() == Delete)
		{
			m_selTable->Remove(m_selTable->GetFirstSelectedId());
			if (m_selTable->GetDisplayedCount() == 0)
				m_selTable->Refresh();
		}
		else if (e.GetId() == Rename)
		{
			int id = m_selTable->GetFirstSelectedId();
			ImageItemRender* render = (ImageItemRender*)m_selTable->GetRender(id);
			wxRect rect = render->GetTextRect(5, 4);
			m_nameEntry->SetSize(rect.GetWidth(), -1);
			render->AddWidget(m_nameEntry, rect.GetPosition());
			m_nameEntry->ShowEntry(*m_selTable->GetData(id));
		}
	}
	void OnLeftDown(wxMouseEvent& e)
	{
		static bool b;
		wxImage& img = b ? m_itemImage : m_itemImage2;
		b = !b;
		wxString s = wxString().Format("project%d", m_selTable->GetCount());
		m_selTable->Add(s, new ImageItemRender(img, s));

		p1->Move(p1->GetRect().GetPosition().x + 5, p1->GetRect().GetPosition().y + 5);
	}
	void OnSelectionTablePaint(HCPaintEvent& e)
	{
		if (m_selTable->GetDisplayedCount() > 0)
			return e.Skip();

		static wxString text = _("No Existing Project(s) Found");

		e.GetDC().Clear();
		
		wxFont font = e.GetDC().GetFont();
		font.SetPointSize(20);
		e.GetDC().SetFont(font);
		e.GetDC().SetTextForeground(wxColor(255, 255, 255, 150));

		wxRect textRect;
		textRect.SetSize(e.GetDC().GetTextExtent(text));
		textRect = textRect.CenterIn(m_selTable->hc.GetRect());

		e.GetDC().DrawText(text, textRect.GetPosition());
	}
	
	enum ItemMenu
	{
		ShowInExplorer,
		Rename,
		Delete
	};

	HandlelessContainer m_hc;
	wxPanel* panelLoad;
	wxPanel* panelNew;
	wxImage m_itemImage;
	wxImage m_itemImage2;
	wxTextCtrl* m_textCtrl;
	QuickTextEntry* m_nameEntry;
	SelectionPanel<wxString>* m_selTable;
	Button* m_openBtn;

	HandlelessCtrl* p1;
	HandlelessCtrl* p2;
};