---object---
require 'class'

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
	self.event_table_obj = {}
end

function Object:delist()
	if (global_objects[self.id] == self) then
		global_objects[self.id] = nil
		global_recycled_id[self.id] = self.id
	end
end

function Object:dtor()
	self:delist()
end

function Object:bind_event(e, obj, func)
	self:unbind_event(e, obj, func)
	if (obj) then
		self.event_table_obj[e] = self.event_table_obj[e] or setmetatable({}, {__mode = 'k'})
		self.event_table_obj[e][obj] = self.event_table_obj[e][obj] or {}
		table.insert(self.event_table_obj[e][obj], func)
	else
		self.event_table[e] = self.event_table[e] or {}
		table.insert(self.event_table[e], func)
	end
end

function Object:unbind_event(e, obj, func)
	if (not e) then
		if (not obj) then
			for _, t in pairs(self.event_table) do
				for k, v in pairs(t) do
					if (v == func) then
						table.remove(t, k)
						break
					end
				end
			end
			return
		end
		for _, t in pairs(self.event_table_obj) do
			if (not func) then
				t[obj] = nil
			else
				for k, v in pairs(t) do
					if (v == func) then
						table.remove(t, k)
						break
					end
				end
			end
		end
		return
	end			
	if (not obj) then
		local t = self.event_table[e]
		if (t) then
			if (not func) then
				self.event_table[e] = nil
				return
			end
			for k, v in pairs(t) do
				if (v == func) then
					table.remove(t, k)
					break
				end
			end
		end
		return
	end
	local t = self.event_table_obj[e]
	if (t) then
		if (not obj) then
			self.event_table_obj[e] = nil
			return
		end
		t = self.event_table_obj[e][obj]
		if (t) then
			if (not func) then
				self.event_table_obj[e][obj] = nil
				return
			end
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
	e.obj = self
	local t = self.event_table[e]
	if (t) then
		for _, f in pairs(t) do
			f(e, ...)
		end
	end
	t = self.event_table_obj[e]
	if (t) then
		for obj, v in pairs(t) do
			if (global_objects[obj.id] == obj) then
				for _, f in pairs(v) do
					f(obj, e, ...)
				end
			else
				t[obj] = nil
			end
		end
	end
	e.obj = nil
end