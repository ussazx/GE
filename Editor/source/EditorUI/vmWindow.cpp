#include "vmwindow.h"

wxStockCursor g_cursor = wxCURSOR_ARROW;
std::unordered_map<std::string, wxEventType> vmWindow::m_eventName;
std::unordered_map<wxEventType, int> vmWindow::m_eventId;