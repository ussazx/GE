---event---
require 'object'
local i = 0
function new_event(name)
	i = i + 1
	local e = i
	if (name) then
		cTerminal:AddEvent(name, e)
	end
	return e
end

EVT = {}
EVT.TIMER = new_event()
EVT.RENDER = new_event()
EVT.FOCUS_IN = new_event()
EVT.FOCUS_OUT = new_event()
EVT.ACTIVE = new_event()
EVT.INACTIVE = new_event()

EVT.CHAR = new_event()
EVT.KEY_DOWN = new_event()
EVT.KEY_UP = new_event()
EVT.ACC_KEY = new_event()
EVT.CAPTURE_LOST = new_event()

EVT.SHOW = new_event()
EVT.SIZE = new_event()
EVT.MOVE = new_event()

EVT.DELIST = new_event()
EVT.WIDGET_ADDED = new_event()
EVT.WIDGET_REMOVED = new_event()

EVT.OBJ_DELIST = new_event()

EVT.DRAG_HOLDING = new_event()
EVT.DROP = new_event()

EVT.UNDEFINED = new_event('EVT_UNDEFINED')
--EVT.ENTER_WINDOW = new_event('EVT_ENTER_WINDOW')
--EVT.LEAVE_WINDOW = new_event('EVT_LEAVE_WINDOW')
EVT.MOVE_IN = new_event()
EVT.MOVE_OUT = new_event()
EVT.LEFT_DOWN = new_event('EVT_LEFT_DOWN')
EVT.LEFT_UP = new_event('EVT_LEFT_UP')
EVT.MIDDLE_DOWN = new_event('EVT_MIDDLE_DOWN')
EVT.MIDDLE_UP = new_event('EVT_MIDDLE_UP')
EVT.RIGHT_DOWN = new_event('EVT_RIGHT_DOWN')
EVT.RIGHT_UP = new_event('EVT_RIGHT_UP')
EVT.MOTION = new_event('EVT_MOTION')
EVT.LEFT_DCLICK = new_event('EVT_LEFT_DCLICK')
EVT.MIDDLE_DCLICK = new_event('EVT_MIDDLE_DCLICK')
EVT.RIGHT_DCLICK = new_event('EVT_RIGHT_DCLICK')
EVT.MOUSEWHEEL = new_event('EVT_MOUSEWHEEL')
EVT.AUX1_DOWN = new_event('EVT_AUX1_DOWN')
EVT.AUX1_UP = new_event('EVT_AUX1_UP')
EVT.AUX1_DCLICK = new_event('EVT_AUX1_DCLICK')
EVT.AUX2_DOWN = new_event('EVT_AUX2_DOWN')
EVT.AUX2_UP = new_event('EVT_AUX2_UP')
EVT.MAGNIFY = new_event('EVT_MAGNIFY')

local mouseAll = {
	EVT.MOVE_IN, 
	EVT.MOVE_OUT,
	EVT.LEFT_DOWN,
	EVT.LEFT_UP,
	EVT.MIDDLE_DOWN,
	EVT.MIDDLE_UP,
	EVT.RIGHT_DOWN,
	EVT.RIGHT_UP,
	EVT.MOTION,
	EVT.LEFT_DCLICK, 
	EVT.MIDDLE_DCLICK,
	EVT.RIGHT_DCLICK,
	EVT.MOUSEWHEEL,
	EVT.AUX1_DOWN,
	EVT.AUX1_UP,
	EVT.AUX1_DCLICK, 
	EVT.AUX2_DOWN,
	EVT.AUX2_UP,
	EVT.MAGNIFY}
	
function EVT.BindMouseAll(a, b, func, ...)
	local o = {}
	for _, e in pairs ({...}) do
		o[e] = true
	end
	for _, e in pairs(mouseAll) do
		if (not o[e]) then
			a:bind_event(e, b, func)
		end
	end
end