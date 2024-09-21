#pragma once
#include "wx/wxprec.h"
#include "wx/wrapsizer.h"
#include "handlelessctrl.h"
#include <unordered_set>
#include <map>

enum SelectionFlag
{
	SEL_LEFT_ONLY_UP = 1,
	SEL_MULTIPLE = 2,
	SEL_RIGHT_DOWN = 4,
	SEL_RIGHT_ONLY_UP = 8,
	SEL_IGNORE_LOST_FOCUS = 16,
	SEL_IGNORE_CANCEL = 32
};

class ItemRender
{
public:
	virtual ~ItemRender() {}
	ItemRender(const wxSize& s = wxDefaultSize) : size(s) {}
	void Init(HandlelessCtrl* ctrl) { m_ctrl = ctrl; }
	wxRect GetRect() { return m_ctrl->GetLocationRect(); }
	void Update() { m_ctrl->Layout(); }
	void Refresh() { m_ctrl->Refresh(); }

	void AddWidget(wxWindow* widget, const wxPoint& pt)
	{
		m_widgets[widget] = pt;
		widget->Move(GetRect().GetPosition() + pt);
	}
	void RemoveWidget(wxWindow* widget)
	{
		m_widgets.erase(widget);
	}
	void UpdateWidgets()
	{
		for (auto w : m_widgets)
			w.first->Move(GetRect().GetPosition() + w.second);
	}

	virtual wxSizer* GetSizer() { return nullptr; }
	virtual void OnSize(const wxSize& size) {}
	virtual void Render(wxDC& dc, const wxSize& size, bool selected, bool focused, bool hover) = 0;
	const wxSize size;

private:
	HandlelessCtrl* m_ctrl;
	std::unordered_map<wxWindow*, wxPoint> m_widgets;
};

class ImageItemRender : public ItemRender
{
public:
	virtual ~ImageItemRender() {}
	ImageItemRender(const wxImage& image = wxImage(), const wxString& text = wxString(),
		const wxSize& imageSize = wxSize(150, 150))
		: m_image(image), m_text(text), m_showText(true)
	{
		if (image.IsOk())
			m_imgBmp = image.Scale(imageSize.GetWidth(), imageSize.GetHeight());

		m_sizer = new wxBoxSizer(wxVERTICAL);
		m_imgSi = m_sizer->Add(imageSize.GetWidth(), imageSize.GetHeight(), wxSizerFlags().Border(wxALL & ~wxBOTTOM).Center());

		m_textSi = m_sizer->Add(0, 0);
		InitText(imageSize.GetWidth());
	}
	wxSizer* GetSizer() wxOVERRIDE
	{
		return m_sizer;
	}
	void OnSize(const wxSize& size) wxOVERRIDE
	{
		wxSize newImgSize = wxSize(m_imgSi->GetSize().GetWidth() - m_imgSi->GetBorder() * 2,
			m_imgSi->GetSize().GetHeight() - m_imgSi->GetBorder());
		if (m_image.IsOk() && m_imgBmp.GetSize() != newImgSize
			&& newImgSize.GetWidth() > 0 && newImgSize.GetWidth() > 0)
			m_imgBmp = m_image.Scale(newImgSize.GetWidth(), newImgSize.GetHeight());
	}
	wxRect GetTextRect(int hpad = 0, int vpad = 0)
	{
		wxPoint pt(hpad, m_textSi->GetRect().GetY() + vpad);
		wxSize size(GetRect().GetWidth() - hpad * 2, m_textSi->GetRect().GetHeight());
		return wxRect(pt, size);
	}
	void Render(wxDC& dc, const wxSize& size, bool selected, bool focused, bool hover) wxOVERRIDE
	{
		dc.SetPen(*wxTRANSPARENT_PEN);
		if (selected)
		{
			wxColor color = focused ? wxColor(255, 180, 0, 200) : wxColor(200, 200, 200, 100);
			dc.SetBrush(color);
			dc.DrawRectangle(size);
		}
		else if (hover)
		{
			dc.SetBrush(wxBrush(wxColor(100, 100, 100, 100)));
			dc.DrawRectangle(size);
		}

		if (m_imgBmp.IsOk())
			dc.DrawBitmap(m_imgBmp, m_imgSi->GetRect().GetPosition());

		if (m_showText)
		{
			dc.SetTextForeground(*wxWHITE);
			dc.DrawText(m_text, m_textSi->GetRect().GetPosition());
		}
	}
	void ShowText(bool show)
	{
		if (show)
			InitText(m_imgSi->GetRect().GetWidth());
		else
			m_textSi->SetInitSize(0, 0);
		Update();
	}
	void SetText(const wxString& text)
	{
		m_text = text;
		if (m_showText)
			ShowText(true);
	}

protected:
	void InitText(int maxWidth)
	{
		wxGCDC gcdc;
		wxSize textSize = gcdc.GetTextExtent(m_text);
		int textWidth = textSize.GetWidth() < maxWidth ? textSize.GetWidth() : maxWidth;
		m_textSi->SetMinSize(textWidth, textSize.GetHeight());
		m_textSi->SetFlag(wxBOTTOM | wxCENTER);
		m_textSi->SetBorder(15);
	}

	wxString m_text;
	wxImage m_image;
	wxBitmap m_imgBmp;
	wxSizer* m_sizer;
	wxSizerItem* m_imgSi;
	wxSizerItem* m_textSi;
	bool m_showText;
};

class ItemLayout
{
public:
	virtual ~ItemLayout() {}
	virtual void Init(const wxSize& size) {}
	virtual wxSizer* GetSizer() { return nullptr; }
	virtual bool OnSize(const wxSize& size) { return false; }
	virtual wxSizerItem* Insert(size_t pos, wxWindow* item, const wxSizerFlags& flags) = 0;
	virtual wxSizerItem* Insert(size_t pos, HandlelessCtrl* item, const wxSizerFlags& flags) = 0;
	virtual void Remove(wxWindow* item) {};
	virtual void Remove(HandlelessCtrl* item) {};
	virtual wxWindow* GetWnd(size_t pos) = 0;
	virtual HandlelessCtrl* GetHdlCtrl(size_t pos) = 0;
	virtual size_t GetCount() = 0;
	virtual void Clear() = 0;
};

//class WrapLayout : public ItemLayout
//{
//public:
//	WrapLayout()
//	{
//		m_rootSizer = new wxBoxSizer(wxVERTICAL);
//		m_wrapSizer = new wxWrapSizer(wxHORIZONTAL);
//		m_rootSizer->Add(m_wrapSizer, 0, wxEXPAND);
//	}
//	wxSizer* GetSizer() wxOVERRIDE
//	{
//		return m_rootSizer;
//	}
//	wxSizerItem* Insert(size_t pos, wxWindow* item, const wxSizerFlags& sizerFlags) wxOVERRIDE
//	{
//		return m_wrapSizer->Insert(pos, item, sizerFlags);
//	}
//	wxSizerItem* Insert(size_t pos, HandlelessCtrl* item, const wxSizerFlags& sizerFlags) wxOVERRIDE
//	{
//		return item->JoinSizer(pos, m_wrapSizer, sizerFlags);
//	}
//	wxWindow* GetWnd(size_t pos) wxOVERRIDE
//	{
//		return m_wrapSizer->GetChildren()[pos]->GetWindow();
//	}
//	HandlelessCtrl* GetHdlCtrl(size_t pos) wxOVERRIDE
//	{
//		return &((HandlelessCtrl::SizerChecker*)m_wrapSizer->GetChildren()[pos]->GetUserData())->owner;
//	}
//	size_t GetCount() wxOVERRIDE
//	{
//		return m_wrapSizer->GetChildren().GetCount();
//	}
//	void Clear() wxOVERRIDE
//	{
//		m_wrapSizer->Clear();
//	}
//
//protected:
//	wxBoxSizer* m_rootSizer;
//	wxWrapSizer* m_wrapSizer;
//};

class GridLayout : public ItemLayout
{
public:
	GridLayout()
	{
		m_rootSizer = new wxBoxSizer(wxVERTICAL);
		m_gridSizer = new wxGridSizer(1);
		m_gridSI = m_rootSizer->Add(m_gridSizer, 0, 0);
	}
	void Init(const wxSize& size) wxOVERRIDE
	{
		m_size = size;
	}
	wxSizer* GetSizer() wxOVERRIDE
	{
		return m_rootSizer;
	}
	bool OnSize(const wxSize& size) wxOVERRIDE
	{
		m_size = size;
		SetGrids(m_gridSizer->GetItemCount());
		return true;
	}
	wxSizerItem* Insert(size_t pos, wxWindow* item, const wxSizerFlags& sizerFlags) wxOVERRIDE
	{
		int w = item->GetSize().GetWidth();
		if (sizerFlags.GetFlags() & wxLEFT)
			w += sizerFlags.GetBorderInPixels();
		if (sizerFlags.GetFlags() & wxRIGHT)
			w += sizerFlags.GetBorderInPixels();

		auto it = m_gridWidths.find(w);
		if (it != m_gridWidths.end())
			it->second++;
		else
			m_gridWidths[w] = 1;

		SetGrids(m_gridSizer->GetItemCount() + 1);
		return m_gridSizer->Insert(pos, item, sizerFlags);
	}
	wxSizerItem* Insert(size_t pos, HandlelessCtrl* item, const wxSizerFlags& sizerFlags) wxOVERRIDE
	{
		int w = item->GetRect().GetWidth();
		if (sizerFlags.GetFlags() & wxLEFT)
			w += sizerFlags.GetBorderInPixels();
		if (sizerFlags.GetFlags() & wxRIGHT)
			w += sizerFlags.GetBorderInPixels();

		auto it = m_gridWidths.find(w);
		if (it != m_gridWidths.end())
			it->second++;
		else
			m_gridWidths[w] = 1;

		SetGrids(m_gridSizer->GetItemCount() + 1);
		return item->JoinSizer(pos, m_gridSizer, sizerFlags);
	}
	void Remove(wxWindow* item) wxOVERRIDE
	{
		wxSizerItem* si = m_gridSizer->GetItem(item);
		if (si)
		{
			auto it = m_gridWidths.find(si->GetSize().GetWidth());
			if (it != m_gridWidths.end() && --it->second <= 0)
				m_gridWidths.erase(it);
		}
	}
	void Remove(HandlelessCtrl* item) wxOVERRIDE
	{
		wxSizerItem* si = item->sizerChecker->sizerItem;
		if (si)
		{
			auto it = m_gridWidths.find(si->GetSize().GetWidth());
			if (it != m_gridWidths.end() && --it->second <= 0)
				m_gridWidths.erase(it);
		}
	}
	wxWindow* GetWnd(size_t pos) wxOVERRIDE
	{
		return m_gridSizer->GetChildren()[pos]->GetWindow();
	}
	HandlelessCtrl* GetHdlCtrl(size_t pos) wxOVERRIDE
	{
		return &((HandlelessCtrl::SizerChecker*)m_gridSizer->GetChildren()[pos]->GetUserData())->owner;
	}
	size_t GetCount() wxOVERRIDE
	{
		return m_gridSizer->GetChildren().GetCount();
	}
	void Clear() wxOVERRIDE
	{
		m_gridSizer->Clear();
		m_gridWidths.clear();
	}

protected:
	void SetGrids(int itemSize)
	{
		if (m_gridWidths.size() == 0)
			return;

		int cols = m_size.GetWidth() / (--m_gridWidths.end())->first;
		if (cols == 0)
			cols = 1;
		else if (cols >= itemSize)
			cols = itemSize;

		int rows = itemSize / cols + (itemSize % cols > 0 ? 1 : 0);

		m_gridSizer->SetCols(cols);
		m_gridSizer->SetRows(rows);

		m_gridSI->SetFlag(rows > 1 ? wxEXPAND : 0);
	}

	wxSize m_size;
	wxSizer* m_rootSizer;
	wxGridSizer* m_gridSizer;
	wxSizerItem* m_gridSI;
	std::map<int, int> m_gridWidths;
};

class SelectionCtrl : public HandlelessCtrl
{
public:
	~SelectionCtrl() { delete m_render; }
	SelectionCtrl(HandlelessCtrl* parent, ItemRender* render, bool show)
		: HandlelessCtrl(parent, -1, show), m_render(render)
	{
		Bind(wxEVT_SIZE, &SelectionCtrl::OnSize, this);
		Bind(EVT_HC_PAINT, &SelectionCtrl::OnPaint, this);
		Bind(wxEVT_MOVE, &SelectionCtrl::OnMove, this);
		Bind(wxEVT_ENTER_WINDOW, &SelectionCtrl::OnMouseEnter, this);
		Bind(wxEVT_LEAVE_WINDOW, &SelectionCtrl::OnMouseLeave, this);

		m_render->Init(this);

		wxSizer* sizer = m_render->GetSizer();
		if (sizer)
			SetSizerAndFit(sizer);
		else
			SetSize(m_render->size);
	}
	void SetSelected(bool flag, bool focus)
	{
		if (flag != m_selected || focus != m_focus)
		{
			m_selected = flag;
			m_focus = focus;
			Refresh();
		}
	}
	ItemRender* GetRender()
	{
		return m_render;
	}

protected:
	void OnMove(wxMoveEvent&)
	{
		m_render->UpdateWidgets();
	}
	void OnSize(wxSizeEvent& e)
	{
		m_render->OnSize(GetRect().GetSize());
	}
	void OnPaint(HCPaintEvent& e)
	{
		e.GetDC().SetBackground(wxBrush(backgroundColor));
		m_render->Render(e.GetDC(), GetRect().GetSize(), m_selected, m_focus, m_showHover);
	}
	void OnMouseEnter(wxMouseEvent&)
	{
		m_showHover = true;
		Refresh();
	}
	void OnMouseLeave(wxMouseEvent&)
	{
		m_showHover = false;
		Refresh();
	}

protected:
	ItemRender* m_render;
	bool m_showHover;
	bool m_selected;
	bool m_focus;
};

//class SelectionItem : public wxWindow
//{
//public:
//	~SelectionItem() { delete m_render; }
//	SelectionItem(wxWindow* parent, ItemRender* render)
//		: wxWindow(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize),
//		m_render(render)
//	{
//		SetBackgroundColour(GetParent()->GetBackgroundColour());
//		wxSizer* sizer = m_render->GetSizer();
//		if (sizer)
//			SetSizerAndFit(sizer);
//		else
//			SetSize(m_render->size);
//
//		Bind(wxEVT_SIZE, &SelectionItem::OnSize, this);
//		Bind(wxEVT_PAINT, &SelectionItem::OnPaint, this);
//		Bind(wxEVT_ERASE_BACKGROUND, &SelectionItem::OnEraseBackground, this);
//		Bind(wxEVT_ENTER_WINDOW, &SelectionItem::OnMouseEnter, this);
//		Bind(wxEVT_LEAVE_WINDOW, &SelectionItem::OnMouseLeave, this);
//	}
//	void SetSelected(bool flag, bool focus)
//	{
//		if (m_selected != flag || focus != m_focus)
//		{
//			m_selected = flag;
//			m_focus = focus;
//			Refresh();
//		}
//	}
//
//protected:
//	void OnSize(wxSizeEvent&)
//	{
//		m_render->OnSize(GetRect().GetSize());
//	}
//	void OnPaint(wxPaintEvent&)
//	{
//		GCDCWndBlit gcdc(this);
//		gcdc.SetBackground(wxBrush(GetBackgroundColour()));
//		m_render->Render(gcdc, GetRect().GetSize(), m_selected, m_focus, m_showHover);
//	}
//	void OnMouseEnter(wxMouseEvent&)
//	{
//		m_showHover = true;
//		Refresh();
//	}
//	void OnMouseLeave(wxMouseEvent&)
//	{
//		m_showHover = false;
//		Refresh();
//	}
//	void OnEraseBackground(wxEraseEvent&) {}
//
//	ItemRender* m_render;
//	bool m_showHover;
//	bool m_selected;
//	bool m_focus;
//};

template<class T>
class SelectionPanelEvent : public wxEvent
{
public:
	SelectionPanelEvent(int winid = 0, wxEventType commandType = wxEVT_NULL)
		: wxEvent(winid, commandType), leftDClickId(0), byRight(false), filterData(nullptr), passed(true),
		compareData1(nullptr), compareData2(nullptr), resultLess(true) {}
	wxEvent *Clone() const wxOVERRIDE { return new SelectionPanelEvent(*this); }
	size_t leftDClickId;
	bool byRight;
	T* filterData;
	bool passed;
	T* compareData1;
	T* compareData2;
	bool resultLess;
};

template<class T>
wxDEFINE_EVENT(EVT_SELTAB_SELECT, SelectionPanelEvent<T>);
template<class T>
wxDEFINE_EVENT(EVT_SELTAB_COMPARE, SelectionPanelEvent<T>);
template<class T>
wxDEFINE_EVENT(EVT_SELTAB_FILTER, SelectionPanelEvent<T>);

template<class T, class LayoutT = GridLayout>
class SelectionPanel : public wxScrolled<wxWindow>
{
public:
	SelectionPanel() : m_layout(m_specLayout) { Init(); }
	SelectionPanel(wxWindow *parent,
		wxWindowID winid = wxID_ANY,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize,
		long style = wxScrolledWindowStyle,
		const wxString& name = wxASCII_STR(wxPanelNameStr))
		: wxScrolled<wxWindow>(parent, winid, pos, size, style, name),
		m_layout(m_specLayout)
	{
		Init();
	}
	void Init()
	{
		hc.backgroundColor = GetParent()->GetBackgroundColour();
		hc.SetWindow(this);

		m_layout.Init(GetSize());
		SetSizer(m_layout.GetSizer());

		flags = 0;
		m_order = 0;
		
		hc.Bind(wxEVT_LEFT_DOWN, &SelectionPanel::OnTableMouseEvent, this);
		hc.Bind(wxEVT_RIGHT_DOWN, &SelectionPanel::OnTableMouseEvent, this);
		
		Bind(wxEVT_SET_FOCUS, &SelectionPanel::OnTableSetFocus, this);
		Bind(wxEVT_KILL_FOCUS, &SelectionPanel::OnTableKillFocus, this);
		Bind(wxEVT_SIZE, &SelectionPanel::OnSize, this);

		SetScrollbars(0, 10, 0, 0);
	}
	size_t GetCount()
	{
		return m_data.size();
	}
	size_t GetDisplayedCount()
	{
		return m_layout.GetCount();
	}
	size_t Add(const T& data, ItemRender* renderer, const wxString& toolTip = wxEmptyString, bool updateLayout = true, const wxSizerFlags& sizerFlags = wxSizerFlags().Border().Center())
	{
		SelectionCtrl* item = new SelectionCtrl(&hc, renderer, false);
		item->toolTip = toolTip;
		item->backgroundColor = GetParent()->GetBackgroundColour();
		item->Bind(wxEVT_LEFT_DOWN, &SelectionPanel::OnItemMouseEvent, this);
		item->Bind(wxEVT_LEFT_UP, &SelectionPanel::OnItemMouseEvent, this);
		item->Bind(wxEVT_LEFT_DCLICK, &SelectionPanel::OnItemMouseEvent, this);
		item->Bind(wxEVT_RIGHT_DOWN, &SelectionPanel::OnItemMouseEvent, this);
		item->Bind(wxEVT_RIGHT_UP, &SelectionPanel::OnItemMouseEvent, this);
		m_data[item] = { data, sizerFlags, m_order++ };

		AddToLayout(item, sizerFlags, updateLayout);

		return (size_t)item;
	}
	void Select(size_t id, bool select, bool notify)
	{
		SelectionCtrl* item = (SelectionCtrl*)id;
		if (m_data.find(item) == m_data.end())
			return;
		if (select)
		{
			SetFocus();
			if (!(flags & SEL_MULTIPLE))
				ClearSelected(item);
			m_selected.insert(item);
		}
		else
			m_selected.erase(item);

		item->SetSelected(select, true);

		if (notify)
			NotifySelection();
	}
	bool Remove(size_t id, bool updateLayout = true)
	{
		auto it = m_data.find((SelectionCtrl*)id);
		if (it != m_data.end())
		{
			m_selected.erase(it->first);
			m_layout.Remove(it->first);
			delete it->first;
			m_data.erase(it);

			if (updateLayout)
				GetParent()->Layout();

			NotifySelection();
			return true;
		}
		return false;
	}
	size_t GetFirstSelectedId()
	{
		return m_selected.size() > 0 ? (size_t)*m_selected.begin() : 0;
	}
	T* GetSelected(size_t id)
	{
		auto it = m_selected.find((SelectionCtrl*)id);
		if (it == m_selected.end())
			return nullptr;

		id = ++it != m_selected.end() ? (size_t)*it : 0;
		return &m_data[*it].data;
	}
	size_t GetSelectedCount()
	{
		return m_selected.size();
	}
	T* GetData(size_t Id)
	{
		auto it = m_data.find((SelectionCtrl*)Id);
		return it != m_data.end() ? &it->second.data : nullptr;
	}
	void Relayout()
	{
		m_layout.Clear();
		for (auto it = m_data.begin(); it != m_data.end(); it++)
			AddToLayout(it->first, m_data[it->first].sizerFlags, false);

		Freeze();
		GetParent()->Layout();
		Thaw();
	}
	void NotifySelection(bool byRight = false, size_t leftDClickId = 0)
	{
		SelectionPanelEvent<T> e(m_windowId, EVT_SELTAB_SELECT<T>);
		e.byRight = byRight;
		e.leftDClickId = leftDClickId;
		ProcessEvent(e);
	}
	ItemRender* GetRender(size_t id)
	{
		auto it = m_data.find((SelectionCtrl*)id);
		return it != m_data.end() ? it->first->GetRender() : nullptr;
	}
	unsigned int flags;
	HandlelessContainer hc;

protected:
	void AddToLayout(SelectionCtrl* item, const wxSizerFlags& sizerFlags, bool updateLayout)
	{
		if (!FilterDisplay(item))
		{
			Select((size_t)item, false, true);
			item->Show(false);
			return;
		}
		if (m_layout.GetCount() == 0 || !LessCompare(item, (SelectionCtrl*)m_layout.GetHdlCtrl(m_layout.GetCount() - 1)))
			m_layout.Insert(m_layout.GetCount(), item, sizerFlags);
		else
		{
			size_t i = 0;
			for (; i < m_layout.GetCount() - 1 && !LessCompare(item, (SelectionCtrl*)m_layout.GetHdlCtrl(i)); i++) {}
			m_layout.Insert(i, item, sizerFlags);
		}
		if (updateLayout)
		{
			Freeze();
			GetParent()->Layout();
			Thaw();
		}
		item->Show(true);
	}
	void OnSize(wxSizeEvent& e)
	{
		wxSize size = GetSize();
		if (IsScrollbarShown(wxVERTICAL))
			size.SetWidth(GetSize().GetWidth() - 21);
		m_layout.OnSize(size);
		e.Skip();
	}
	void OnTableMouseEvent(wxMouseEvent& e)
	{
		e.Skip();
		if (!(flags & SEL_IGNORE_CANCEL))
		{
			ClearSelected();
			NotifySelection();
		}
	}
	void OnTableSetFocus(wxFocusEvent& e)
	{
		e.Skip();
		for (auto it = m_selected.begin(); it != m_selected.end(); it++)
			(*it)->SetSelected(true, true);
	}
	void OnTableKillFocus(wxFocusEvent& e)
	{
		e.Skip();
		if (flags & SEL_IGNORE_LOST_FOCUS)
			return;

		for (auto it = m_selected.begin(); it != m_selected.end(); it++)
			(*it)->SetSelected(true, false);
	}
	void OnItemMouseEvent(wxMouseEvent& e)
	{
		SetFocus();

		if ((e.LeftIsDown() && flags & SEL_LEFT_ONLY_UP)
			|| (e.RightIsDown() && (!(flags & SEL_RIGHT_DOWN) || flags & SEL_RIGHT_ONLY_UP))
			|| (e.LeftUp() && !(flags & SEL_LEFT_ONLY_UP))
			|| (e.RightUp() && !(flags & SEL_RIGHT_ONLY_UP)))
			return;

		SelectionCtrl* item = (SelectionCtrl*)e.GetEventObject();

		if (flags & SEL_MULTIPLE && e.ControlDown())
		{
			auto ret = m_selected.insert(item);
			item->SetSelected(ret.second, FindFocus() == this);
			if (!ret.second)
				m_selected.erase(ret.first);
		}
		else
		{
			ClearSelected(item);
			m_selected.insert(item);
			item->SetSelected(true, FindFocus() == this);
		}

		NotifySelection(e.RightIsDown() || e.RightUp(), e.LeftDClick() ? (size_t)item : 0);
	}
	void ClearSelected(SelectionCtrl* exception = nullptr)
	{
		auto it = m_selected.begin();
		while (it != m_selected.end())
		{
			if (*it == exception)
				it++;
			else
			{
				(*it)->SetSelected(false, false);
				it = m_selected.erase(it);
			}
		}
	}
	bool LessCompare(SelectionCtrl* item1, SelectionCtrl* item2)
	{
		SelectionPanelEvent<T> e(m_windowId, EVT_SELTAB_COMPARE<T>);
		e.compareData1 = &m_data[item1].data;
		e.compareData2 = &m_data[item2].data;
		e.resultLess = m_data[item1].order < m_data[item2].order;
		ProcessEvent(e);
		
		return e.resultLess;
	}
	bool FilterDisplay(SelectionCtrl* item)
	{
		SelectionPanelEvent<T> e(m_windowId, EVT_SELTAB_FILTER<T>);
		e.filterData = &m_data[item].data;
		ProcessEvent(e);
		return e.passed;
	}

	struct Data
	{
		T data;
		wxSizerFlags sizerFlags;
		size_t order;
	};
	std::unordered_set<SelectionCtrl*> m_selected;
	std::unordered_map<SelectionCtrl*, Data> m_data;
	ItemLayout& m_layout;
	LayoutT m_specLayout;
	size_t m_order;
};
