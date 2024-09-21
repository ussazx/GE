#pragma once

#include "common/uiutils.h"
#include "common/floatablenotebook.h"
#include "common/uimanager.h"
#include "common/button.h"
#include "common/selectionpanel.h"
#include "common/HandlelessCtrl.h"
#include "common/quicktextentry.h"
#include "wx/artprov.h"
#include "wx/srchctrl.h"

#include <memory>

#define TEXT_BG_COLOR wxColor(245, 245, 245)

#define DEFINE_EVENT(type, ...) \
	struct type \
	{ \
		static size_t ID() { return typeid(type).hash_code(); } \
		typedef std::function<bool(size_t, __VA_ARGS__)> Func; \
	};

class EvtFuncBase { public : virtual ~EvtFuncBase() {} };

template<class Func>
class EvtFunctor : public EvtFuncBase
{
public:
	EvtFunctor(const Func& f) : func(f) {}
	const Func func;
};

class EventHandler
{
public:
	virtual ~EventHandler() {}
	
	template<class Event>
	void Bind(const typename Event::Func& func)
	{
		typedef typename Event::Func Func;

		auto& f = m_functors[Event::ID()];
		f.argId = typeid(Func).hash_code();
		f.stack.emplace(f.stack.begin(), new EvtFunctor<Func>(func));
	}

	template<class Event>
	void Unbind(const typename Event::Func& func)
	{
		typedef typename Event::Func Func;

		auto it = m_functors.find(Event::ID());
		if (it != m_functors.end() && it->second.argId == typeid(Func).hash_code())
			for (auto it2 = it->second.stack.begin(); it2 != it->second.stack.end(); it2++)
				if (((EvtFunctor<Func>*)it2->get())->func.target_type() == func.target_type())
				{
					it->second.stack.erase(it2);
					break;
				}
	}

	template<class Event, class ...Args>
	void ProcessEvent(Args&&... args)
	{
		typedef typename Event::Func Func;

		auto it = m_functors.find(Event::ID());
		if (it != m_functors.end() && it->second.argId == typeid(Func).hash_code())
			for (auto i : it->second.stack)
				if (!((EvtFunctor<Func>*)i.get())->func(Event::ID(), std::forward<Args>(args)...))
					break;
	}

	template<class ...Args>
	void ProcessEvent(size_t eventId, Args&&... args)
	{
		typedef std::function<bool(size_t, Args...)> Func;

		auto it = m_functors.find(eventId);
		if (it != m_functors.end() && it->second.argId == typeid(Func).hash_code())
			for (auto i : it->second.stack)
				if (!((EvtFunctor<Func>*)i.get())->func(eventId, std::forward<Args>(args)...))
					break;
	}

protected:
	struct Functor
	{
		size_t argId;
		std::vector<std::shared_ptr<EvtFuncBase>> stack;
	};
	std::unordered_map<size_t, Functor> m_functors;
};

DEFINE_EVENT(EVT_UI, int)


class UIBtn : public EventHandler
{

};

class EntranceDlg : public wxFrame
{
public:
	EntranceDlg(wxWindow* parent, wxWindowID id, const wxString& title)
		: wxFrame(parent, id, title, wxPoint(wxSCREEN_CENTER_WH(1000, 800)), wxSize(1100, 800),
			wxCAPTION | wxCLOSE_BOX | wxRESIZE_BORDER |
			wxFRAME_NO_WINDOW_MENU)
	{
		//RsrcReader::ReadPath("generic/", [](const char* fileName, const char*, bool) { wxMessageOutputDebug().Printf(fileName); });

		SetBackgroundColour(wxColor(65, 65, 65));

		auto f = [this](size_t id, int)
		{ 
			if (id == EVT_UI::ID())
				return false;
			return true;
		};

		UIBtn btn;
		btn.Bind<EVT_UI>(f);

		btn.ProcessEvent(EVT_UI::ID());

		btn.ProcessEvent<EVT_UI>(1);

		btn.Unbind<EVT_UI>(f);

		wxBoxSizer* s = new wxBoxSizer(wxVERTICAL);
		SetSizer(s);

		wxAuiNotebook* nb = new wxAuiNotebook(this, wxID_ANY);
		nb->SetArtProvider(new wxAuiGenericTabArt);
		nb->SetTabCtrlHeight(50);
		nb->SetWindowStyleFlag(nb->GetWindowStyleFlag() & 
			~(wxAUI_NB_CLOSE_BUTTON |
			wxAUI_NB_CLOSE_ON_ACTIVE_TAB |
			wxAUI_NB_CLOSE_ON_ALL_TABS |
			wxAUI_NB_TAB_MOVE |
			wxAUI_NB_TAB_SPLIT));
		s->Add(nb, 1, wxEXPAND);

		nb->AddPage(CreatePageLoad(), _("Load Project"));
		nb->AddPage(CreatePageNew(), _("New Project"));

		Layout();
	}

private:
	void OnSelTabSelect(SelectionPanelEvent<wxString>& e)
	{
		bool selected = m_savedProjTable->GetSelectedCount() > 0;
		m_openBtn->Enable(selected);
		if (selected && e.byRight)
		{
			wxMenu menu;
			wxMenuItem* mi = new wxMenuItem(nullptr, ShowInExplorer, _("Open located directory"));
			mi->SetBitmap(wxArtProvider::GetIcon(wxART_GO_DIR_UP, wxART_MENU));
			menu.Append(mi);
			//mi = new wxMenuItem(nullptr, Rename, _("Rename project"));
			//mi->SetBitmap(wxArtProvider::GetIcon(wxART_FIND_AND_REPLACE, wxART_MENU));
			//menu.Append(mi);
			mi = new wxMenuItem(nullptr, Delete, _("Delete project"));
			mi->SetBitmap(wxArtProvider::GetIcon(wxART_DELETE, wxART_MENU));
			menu.Append(mi);
			menu.Bind(wxEVT_MENU, &EntranceDlg::OnItemMenu, this);
			PopupMenu(&menu, ScreenToClient(wxGetMousePosition()));
		}
	}
	void OnSelTabCompare(SelectionPanelEvent<wxString>& e)
	{
		//e.resultLess = true;
	}
	void OnSelTabFilter(SelectionPanelEvent<wxString>& e)
	{
		e.passed = m_savedSearch->IsEmpty() || e.filterData->Find(m_savedSearch->GetValue()) != wxString::npos;
	}
	void OnTextChanged(wxCommandEvent& e)
	{
		m_savedProjTable->Relayout();
	}
	void OnItemMenu(wxCommandEvent& e)
	{
		if (e.GetId() == ShowInExplorer)
			;// ShellExecute(NULL, NULL, _T("explorer"), "/select, " + selectedItem->data, NULL, SW_SHOW);
		else if (e.GetId() == Delete)
		{
			m_savedProjTable->Remove(m_savedProjTable->GetFirstSelectedId());
			if (m_savedProjTable->GetDisplayedCount() == 0)
				m_savedProjTable->Refresh();
		}
		//else if (e.GetId() == Rename)
		//{
		//	int id = m_savedProjTable->GetFirstSelectedId();
		//	ImageItemRender* render = (ImageItemRender*)m_savedProjTable->GetRender(id);
		//	wxRect rect = render->GetTextRect(5, 4);
		//	m_nameEntry->SetSize(rect.GetWidth(), -1);
		//	render->AddWidget(m_nameEntry, rect.GetPosition());
		//	m_nameEntry->ShowEntry(*m_savedProjTable->GetData(id));
		//}
	}
	void OnSelectionTablePaint(HCPaintEvent& e)
	{
		if (m_savedProjTable->GetDisplayedCount() > 0)
			return e.Skip();

		static wxString text = _("No Existing Project(s) Found");

		e.GetDC().Clear();
		
		wxFont font = e.GetDC().GetFont();
		font.SetPointSize(20);
		e.GetDC().SetFont(font);
		e.GetDC().SetTextForeground(wxColor(255, 255, 255, 150));

		wxRect textRect;
		textRect.SetSize(e.GetDC().GetTextExtent(text));
		textRect = textRect.CenterIn(m_savedProjTable->hc.GetRect());

		e.GetDC().DrawText(text, textRect.GetPosition());
	}
	wxWindow* CreatePageLoad()
	{
		wxPanel* panelLoad = new wxPanel(this, wxID_ANY);
		panelLoad->SetBackgroundColour(GetBackgroundColour());

		wxSizer* s = new wxBoxSizer(wxVERTICAL);
		panelLoad->SetSizer(s);

		m_savedSearch = new wxTextCtrl(panelLoad, wxID_ANY, "",
			wxDefaultPosition, wxSize(30, -1));
		m_savedSearch->Bind(wxEVT_TEXT, &EntranceDlg::OnTextChanged, this);
		m_savedSearch->SetMargins(wxPoint(5, -1));
		m_savedSearch->SetHint(_("Filter projects..."));
		m_savedSearch->SetBackgroundColour(TEXT_BG_COLOR);
		s->Add(m_savedSearch, wxSizerFlags().Expand().Border(wxALL, 20));

		m_savedProjTable = new SelectionPanel<wxString>(panelLoad);
		m_savedProjTable->hc.Bind(EVT_HC_PAINT, &EntranceDlg::OnSelectionTablePaint, this);
		m_savedProjTable->flags = SEL_IGNORE_LOST_FOCUS | SEL_IGNORE_CANCEL | SEL_RIGHT_ONLY_UP;
		m_savedProjTable->Bind(EVT_SELTAB_SELECT<wxString>, &EntranceDlg::OnSelTabSelect, this);
		m_savedProjTable->Bind(EVT_SELTAB_COMPARE<wxString>, &EntranceDlg::OnSelTabCompare, this);
		m_savedProjTable->Bind(EVT_SELTAB_FILTER<wxString>, &EntranceDlg::OnSelTabFilter, this);

		s->Add(m_savedProjTable, wxSizerFlags(1).Expand().Border(wxLEFT | wxRIGHT, 10));

		//m_nameEntry = new QuickTextEntry(m_savedProjTable, wxID_ANY);

		//m_itemImage.LoadFile("scene.png");
		//m_itemImage2.LoadFile("pb.png");
		//for (int i = 0; i < 5; i++)
		//{
		//	wxString s = wxString().Format("project%d", m_savedProjTable->GetCount());
		//	m_savedProjTable->Add(s, new ImageItemRender(m_itemImage, s), "C:\\Users\\asus\\Desktop\\vs\\GE");
		//}

		s->AddSpacer(50);

		wxGridSizer* gs = new wxGridSizer(2);
		s->Add(gs, 0, wxALIGN_RIGHT);

		Button* browseBtn = new Button(panelLoad, wxID_ANY, _("Browse..."));
		gs->Add(browseBtn, wxSizerFlags().Border(wxRIGHT | wxBOTTOM, 20).Right().Bottom());

		m_openBtn = new Button(panelLoad, wxID_ANY, _("Load"));
		gs->Add(m_openBtn, wxSizerFlags().Border(wxRIGHT | wxBOTTOM, 20).Right().Bottom());

		m_openBtn->Enable(m_savedProjTable->GetSelectedCount() > 0);

		return panelLoad;
	}
	wxWindow* CreatePageNew()
	{
		wxPanel* panelNew = new wxPanel(this);
		panelNew->SetBackgroundColour(GetBackgroundColour());

		wxSizer* s = new wxBoxSizer(wxVERTICAL);
		panelNew->SetSizer(s);

		s->AddSpacer(130);

		wxPanel* panelImage = new wxPanel(panelNew, -1, wxPoint(), wxSize(), wxBORDER);
		s->Add(panelImage, 0, wxCENTER);

		wxSizer* s0 = new wxBoxSizer(wxVERTICAL);

		wxGridSizer* gs = new wxGridSizer(1);
		s0->Add(gs);
		wxStaticBitmap* image = new wxStaticBitmap(panelImage, wxID_ANY, wxArtProvider::GetIcon(wxART_NORMAL_FILE, wxART_MENU, wxSize(128, 128)));
		gs->Add(image, wxSizerFlags().Border(wxLEFT|wxRIGHT, 35));
		
		wxStaticText* text = new wxStaticText(panelImage, wxID_ANY, _("Empty Project"));
		text->SetForegroundColour(wxColor(255, 255, 255, 150));
		wxFont font = GetFont();
		font.SetPointSize(10);
		text->SetFont(font);
		s0->Add(text, wxSizerFlags().Border(wxBOTTOM, 10).Center());

		panelImage->SetSizerAndFit(s0);

		s->AddSpacer(80);

		wxSizer* s1 = new wxBoxSizer(wxHORIZONTAL);
		s->Add(s1, 0, wxCENTER);

		wxSizer* s11 = new wxBoxSizer(wxVERTICAL);
		s1->Add(s11, 300);

		wxTextCtrl* pathEntry = new wxTextCtrl(panelNew, wxID_ANY, "");
		pathEntry->SetBackgroundColour(TEXT_BG_COLOR);
		s11->Add(pathEntry, 0, wxEXPAND);

		wxStaticText* text1 = new wxStaticText(panelNew, wxID_ANY, _("Directory"));
		text1->SetForegroundColour(AlphaBlend(*wxWHITE, panelNew->GetBackgroundColour(), 200));
		s11->Add(text1, 0, wxCENTER);

		gs = new wxGridSizer(1);
		s1->Add(gs);
		wxStaticBitmap* dirIcon = new wxStaticBitmap(panelNew, wxID_ANY, wxArtProvider::GetIcon(wxART_NEW_DIR, wxART_MENU, wxSize(24, 24)));
		dirIcon->Bind(wxEVT_ENTER_WINDOW, &EntranceDlg::OnDirIcon, this);
		dirIcon->Bind(wxEVT_LEAVE_WINDOW, &EntranceDlg::OnDirIcon, this);
		dirIcon->Bind(wxEVT_LEFT_UP, &EntranceDlg::OnDirIcon, this);
		gs->Add(dirIcon, wxSizerFlags().Border(wxLEFT|wxTOP, 2));

		s1->AddSpacer(15);

		wxSizer* s12 = new wxBoxSizer(wxVERTICAL);
		s1->Add(s12, 150);

		wxTextCtrl* nameEntry = new wxTextCtrl(panelNew, wxID_ANY, "", wxDefaultPosition);
		nameEntry->SetBackgroundColour(TEXT_BG_COLOR);
		s12->Add(nameEntry, 0, wxEXPAND);

		wxStaticText* text2 = new wxStaticText(panelNew, wxID_ANY, _("Name"));
		text2->SetForegroundColour(AlphaBlend(*wxWHITE, panelNew->GetBackgroundColour(), 200));
		s12->Add(text2, 0, wxCENTER);

		gs = new wxGridSizer(1);
		s1->Add(gs);
		Button* createBtn = new Button(panelNew, wxID_ANY, _("Create"));
		gs->Add(createBtn, wxSizerFlags().Border(wxLEFT, 40));

		//m_newProjTable = new SelectionPanel<unsigned int>(panelNew);
		//m_newProjTable->hc.backgroundColor = wxColor(75, 75, 75);
		//m_newProjTable->flags = SEL_IGNORE_LOST_FOCUS | SEL_IGNORE_CANCEL | SEL_RIGHT_ONLY_UP;
		//m_newProjTable->Bind(EVT_SELTAB_SELECT<unsigned int>, &EntranceDlg::OnSelTabSelect, this);
		//s->Add(m_newProjTable, wxSizerFlags(1).Expand().Border(wxLEFT | wxRIGHT, 40));

		//wxBitmap bmp = wxArtProvider::GetIcon(wxART_NORMAL_FILE, wxART_MENU, wxSize(128, 128));
		//size_t i = m_newProjTable->Add(0, new ImageItemRender(bmp.ConvertToImage(), "New Project", wxSize(100, 100)), "", true, wxSizerFlags().Border(wxALL, 10).Center());
		//m_newProjTable->Select(i, true, true);

		//s->AddSpacer(230);

		return panelNew;
	}

	void OnDirIcon(wxMouseEvent& event)
	{
		wxStaticBitmap* dirIcon = (wxStaticBitmap*)event.GetEventObject();
		if (event.GetEventType() == wxEVT_ENTER_WINDOW)
		{
			dirIcon->SetBackgroundColour(AlphaBlend(wxColor(255, 255, 255, 100), dirIcon->GetParent()->GetBackgroundColour()));
			dirIcon->Refresh();
		}
		else if (event.GetEventType() == wxEVT_LEAVE_WINDOW)
		{
			dirIcon->SetBackgroundColour(dirIcon->GetParent()->GetBackgroundColour());
			dirIcon->Refresh();
		}
		else if (event.GetEventType() == wxEVT_LEFT_UP) {}
	}
	
	enum ItemMenu
	{
		ShowInExplorer,
		Rename,
		Delete
	};

	wxTextCtrl* m_savedSearch;
	//QuickTextEntry* m_nameEntry;
	SelectionPanel<wxString>* m_savedProjTable;
	SelectionPanel<unsigned int>* m_newProjTable;
	Button* m_openBtn;
};