#pragma once
#include "wx/aui/framemanager.h"
#include "uiutils.h"
#include <unordered_map>

class UiManager : public Cloneable<wxAuiManager>
{
public:
	virtual ~UiManager() {};
	
	void SetExtraFlags(unsigned int extraFlags);
	unsigned int GetExtraFlags();
	
	Cloneable<wxAuiManager>* Clone() override;
	void SetUp(Cloneable* target) override;
	
	enum ExtraOption
	{
		EntirelyLayoutResize = 1,
		DockContiguousResize = 1 << 1
	};
protected:
	void DockResize(wxMouseEvent& e);
	void SaveDockSize(const wxAuiDockInfo* dock);
	
	void OnLeftUp(wxMouseEvent& e);
	void OnMotion(wxMouseEvent& e);
	void OnSize(wxSizeEvent& e);

	struct Size
	{
		int dockSize;
		int frameSize;
	};
	std::unordered_map<const wxAuiDockInfo*, Size> m_dockSizes;

	unsigned int m_extraFlags = 0;
	
	wxDECLARE_EVENT_TABLE();
};
