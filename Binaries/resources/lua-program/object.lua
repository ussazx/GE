---object---
require 'class'
require 'event'

local global_objects = setmetatable({}, {__mode = 'kv'})
local global_recycled_id = {}

local g_id = 0
local function new_object_id()
	local i, tid = next(global_recycled_id)
	if (tid) then
		global_recycled_id[i] = nil
	else
		tid = g_id
		g_id = g_id + 1
	end
	return tid
end

function get_object(id)
	return global_objects[id]
end

Object = class()

function Object:ctor()
	self.id = new_object_id()
	global_objects[self.id] = self
	self.event_table = {}
end

function Object:dtor()
	global_objects[self.id] = nil
	global_recycled_id[self.id] = self.id
end

function Object:bind_event(e, obj, func)
	self:unbind_event(e, obj, func)
	self.event_table[e] = self.event_table[e] or setmetatable({}, {__mode = 'k'})
	self.event_table[e][obj] = self.event_table[e][obj] or {}
	table.insert(self.event_table[e][obj], func)
end

function Object:unbind_event(e, obj, func)
	local t = self.event_table[e]
	if (t) then
		t = self.event_table[e][obj]
		if (t) then
			for k, v in pairs(t) do
				if (v == func) then
					table.remove(t, k)
					break
				end
			end
		end
	end
end

function Object:process_event(e, ...)
	local t = self.event_table[e]
	if (t) then
		for k, v in pairs(t) do
			for _, f in pairs(v) do
				f(k, e, ...)
			end
		end
	end
end