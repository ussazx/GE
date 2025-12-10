---event---
require 'object'

EVT = {}
local i = 0
local t = {}
function EVT.new(name)
	local e = {}
	if (name) then
		i = i + 1
		cTerminal:AddEvent(name, i)
		t[i] = e
	end
	return e
end

function EVT.trans(e)
	return t[e]
end

EVT.TIMER = EVT.new()
EVT.RENDER = EVT.new()
EVT.FOCUS_IN = EVT.new()
EVT.FOCUS_OUT = EVT.new()
EVT.ACTIVE = EVT.new()
EVT.INACTIVE = EVT.new()

EVT.CHAR = EVT.new()
EVT.KEY_DOWN = EVT.new()
EVT.KEY_UP = EVT.new()
EVT.ACC_KEY = EVT.new()
EVT.CAPTURE_LOST = EVT.new()

EVT.SHOW = EVT.new()
EVT.SIZE = EVT.new()
EVT.MOVE = EVT.new()

EVT.DELIST = EVT.new()
EVT.WIDGET_ADDED = EVT.new()
EVT.WIDGET_REMOVED = EVT.new()

EVT.OBJ_DELIST = EVT.new()

EVT.INNER_DRAG_ENTER = EVT.new()
EVT.INNER_DRAG_LEAVE = EVT.new()
EVT.INNER_DRAGGING = EVT.new()
EVT.INNER_DROP = EVT.new()
EVT.FILE_DROP = EVT.new()

EVT.UNDEFINED = EVT.new('EVT_UNDEFINED')
--EVT.ENTER_WINDOW = EVT.new('EVT_ENTER_WINDOW')
EVT.LEAVE_WINDOW = EVT.new('EVT_LEAVE_WINDOW')
EVT.MOVE_IN = EVT.new()
EVT.MOVE_OUT = EVT.new()
EVT.LEFT_DOWN = EVT.new('EVT_LEFT_DOWN')
EVT.LEFT_UP = EVT.new('EVT_LEFT_UP')
EVT.MIDDLE_DOWN = EVT.new('EVT_MIDDLE_DOWN')
EVT.MIDDLE_UP = EVT.new('EVT_MIDDLE_UP')
EVT.RIGHT_DOWN = EVT.new('EVT_RIGHT_DOWN')
EVT.RIGHT_UP = EVT.new('EVT_RIGHT_UP')
EVT.MOTION = EVT.new('EVT_MOTION')
EVT.LEFT_DCLICK = EVT.new('EVT_LEFT_DCLICK')
EVT.MIDDLE_DCLICK = EVT.new('EVT_MIDDLE_DCLICK')
EVT.RIGHT_DCLICK = EVT.new('EVT_RIGHT_DCLICK')
EVT.MOUSEWHEEL = EVT.new('EVT_MOUSEWHEEL')
EVT.AUX1_DOWN = EVT.new('EVT_AUX1_DOWN')
EVT.AUX1_UP = EVT.new('EVT_AUX1_UP')
EVT.AUX1_DCLICK = EVT.new('EVT_AUX1_DCLICK')
EVT.AUX2_DOWN = EVT.new('EVT_AUX2_DOWN')
EVT.AUX2_UP = EVT.new('EVT_AUX2_UP')
EVT.MAGNIFY = EVT.new('EVT_MAGNIFY')

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