---utility.lua---
require 'class'

function Print(...)
	print(...)
	if (CFlushStdout) then
		CFlushStdout()
	end
end

function _(s)
	return s
end

g_ui_scale = 1--CGetXDPI() / 120

function tcount(t)
	return #t
end

function Text(o)
	t = type(o)
	if (t == 'number') then
		return ''..o
	end
	if (t == 'boolean' or t == 'string') then
		return string.format('%q', o)
	end
	return nil
end

function SerializeToText(o)
	if (type(o) ~= 'table') then
		return Text(o)
	end
	local s = '{ '
	local comma = ''
	for k, v in pairs(o) do
		local ks = Text(k)
		if (ks) then
			local vs = SerializeToText(v)
			if (vs) then
				s = s..comma..'['..ks..']'..'='..vs
				comma = ', '
			end
		end
	end
	return s..' }'
end


function swap(a, b)
	return b, a
end

-----point-----
Point = class()

function Point:ctor(x, y)
	self.x = x or 0
	self.y = y or 0
end

function Point:set_pos(x, y)
	self.x = x
	self.y = y
end

function Point:move(x, y)
	self.x = self.x + x
	self.y = self.y + y
end

-----bound-----
Bound = class()

function Bound:ctor(o, left, right, top, bottom)
	if (o) then
		self.left = o.left or 0
		self.right = o.right or 0
		self.top = o.top or 0
		self.bottom = o.bottom or 0
	else
		self.left = left or 0
		self.right = right or 0
		self.top = top or 0
		self.bottom = bottom or 0
	end
end

-----rect-----
Rect = class()

function Rect:ctor(x, y, w, h)
	self:set(x, y, w, h)
end

function Rect:set(x, y, w, h)
	self.x = x or 0
	self.y = y or 0
	self.w = w or 0
	self.h = h or 0
end

function Rect:set_pos(x, y)
	self.x = x
	self.y = y
end

function Rect:move(x, y)
	self.x = self.x + x
	self.y = self.y + y
end

function Rect:intersect(rect)
	if (rect == nil) then
		return nil
	end

	x = math.max(self.x, rect.x)
	right = math.min(self.x + self.w, rect.x + rect.w)
	if (x >= right) then
		return nil
	end
	y = math.max(self.y, rect.y)
	h = math.min(self.y + self.h, rect.y + rect.h)
	if (y >= h) then
		return nil
	end
	return Rect(x, y, right - x, h - y)
end

function Rect:diff(rect)
	return self.x ~= rect.x or self.y ~= rect.y or self.w ~= rect.w or self.h ~= rect.h
end

Color = class()
function Color:ctor(r, g, b, a)
	self.r = r or 0
	self.g = g or 0
	self.b = b or 0
	self.a = a or 0
end

function Color:read(color)
	self.r = color.r or self.r
	self.g = color.r or self.g
	self.b = color.r or self.b
	self.a = color.r or self.a
end

function Color:diff(color)
	return self.r ~= rect.r or self.g ~= rect.g or self.b ~= rect.b or self.a ~= rect.a
end

function Color:set(r, g, b, a)
	self.r = r or 0
	self.g = g or 0
	self.b = b or 0
	self.a = a or 0
end

function Copy(src, dst)
	for k, v in pairs(src) do
		dst[k] = v
	end
end

function DiffAndCopy(dst, src)
	local diff = false
	for k, v in pairs(src) do
		if (dst[k] ~= v) then
			dst[k] = v
			diff = true
		end
	end
	return diff
end

-----SizeGroup-----
SizeGroup = class()

function SizeGroup:ctor(w, h, max_w, max_h)
	self.max_w = max_w or w
	self.max_h = max_h or h
	self.rtv = {}
	self.fb = {}
	
	self.w = w
	self.h = h
	self.rtv2 = {}
	self.fb2 = {}
end

function SizeGroup:resize(w, h)
	if (w <= 0 or h <= 0) then
		return
	end
	if (w > self.max_w or h > self.max_h) then
		self.max_w = math.max(w, self.max_w)
		self.max_h = math.max(h, self.max_h)
		for k, v in pairs(self.rtv) do
			v:Resize(self.max_w, self.max_h)
		end
		for k, v in pairs(self.fb) do
			v:Resize(self.max_w, self.max_h)
		end
	end
	if (w ~= self.w or h ~= self.h) then
		self.w = w
		self.h = h
		for k, v in pairs(self.rtv2) do
			v:Resize(w, h)
		end
		for k, v in pairs(self.fb2) do
			v:Resize(w, h)
		end
	end
end

function SizeGroup:add_rtv(rtv, larger_only)
	if (larger_only) then
		table.insert(self.rtv, rtv)
	else
		table.insert(self.rtv2, rtv)
	end
end

function SizeGroup:add_fb(fb, larger_only)
	if (larger_only) then
		table.insert(self.fb, fb)
	else
		table.insert(self.fb2, fb)
	end
end

--Recorder---
Recorder = class()
Recorder.saved = 0
Recorder.cursor = 0
Recorder.max = 5
function Recorder:Record(obj, func, param)
	self.cursor = self.cursor + 1
	if (self.cursor > self.max) then
		self.cursor = self.max
		table.remove(self, 1)
	end
	
	local o = {}
	o.obj = obj
	o.func = func
	o.param = param
	table.insert(self, self.cursor, o)
	
	for i = self.cursor + 1, self.saved do
		self[i] = nil
	end
	self.saved = self.cursor
end

function Recorder:Undo()
	if (self.cursor > 0) then
		local o = self[self.cursor]
		o.func(o.obj, false, o.param)
		self.cursor = self.cursor - 1
	end
end

function Recorder:Redo()
	if (self.cursor < self.saved) then
		self.cursor = self.cursor + 1
		local o = self[self.cursor]
		o.func(o.obj, true, o.param)
	end
end

--ObjectArray---
local objNotifier = Object()

function DelistObject(obj)
	objNotifier:process_event(EVT.DELIST, obj)
	obj:delist()
end

ObjectArray = class(Object)
function ObjectArray:ctor(mode)
	self.n = 0
	objNotifier:bind_event(EVT.DELIST, self, ObjectArray.on_obj_delist)
end

function ObjectArray:on_object_delist(e, obj)
	self:remove_obj(obj)
end

function ObjectArray:insert(obj, idx)
	self.n = self.n + 1
	table.insert(self, idx or self.n, obj)
	return idx or self.n
end

function ObjectArray:remove_idx(idx)
	local obj = self[idx]
	if (obj) then
		table.remove(self, idx)
		self.n = self.n - 1
	end
	return obj
end

function ObjectArray:remove_obj(obj)
	for v, i in self:pairs() do
		if (obj == v) then
			table.remove(self, i)
			self.n = self.n - 1
		end
	end
end

function ObjectArray:back()
	return self[self.n]
end

function ObjectArray:pop_back()
	return self:remove_idx(self.n)
end

function ObjectArray:pairs(filter_func, ...)
	local i = 0
	if (filter_func) then
		return function(...)
			i = i + 1
			while (self[i] and filter_func(self[i], ...) == false) do
				i = i + 1
			end
			return self[i], i
		end
	end
	return function()
		i = i + 1
		return self[i], i
	end
end