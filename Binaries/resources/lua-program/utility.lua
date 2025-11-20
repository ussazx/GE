---utility.lua---
require 'object'

function Print(...)
	print(...)
	cTerminal:FlushStdout()
end

function _(s)
	return s
end

g_ui_scale = 1--CGetXDPI() / 120

function tcount(t)
	return #t
end

ser_array = setmetatable({}, {__mode = 'k'})

local function Text(o)
	t = type(o)
	if (t == 'number') then
		return ''..o
	end
	if (t == 'boolean' or t == 'string') then
		return string.format('%q', o)
	end
	return nil
end

function SerializeToTableText(o)
	if (type(o) ~= 'table') then
		return Text(o)
	end
	if (o[LString]) then
		return Text(o:utf8())
	end
	local s = '{ '
	local comma = ''
	for k, v in pairs(o) do
		local ks = Text(k)
		if (ks) then
			local vs = SerializeToTableText(v)
			if (vs) then
				if (ser_array[o]) then
					s = s..comma..vs
				else
					s = s..comma..'['..ks..']'..'='..vs
				end
				comma = ', '
			end
		end
	end
	return s..' }'
end

function SerializeToText(o)
	if (type(o) ~= 'table') then
		return Text(o)
	end
	if (o[LString]) then
		return Text(o:utf8())
	end
	local s = ''
	local newLine = ''
	for k, v in pairs(o) do
		local ks = Text(k)
		if (ks) then
			local vs = SerializeToTableText(v)
			if (vs) then
				if (ser_array[o]) then
					s = s..newLine..vs
				else
					s = s..newLine..'['..ks..']'..'='..vs
				end
				newLine = '\n'
			end
		end
	end
	return s
end

function LoadFile(f, path, env, run)
	local input = f or CNewFileInput(false)
	if (input:Open(path, false)) then
		local c = CLoadInput(input)
		if (f) then
			f:Close()
		end
		if (type(c) == 'function') then
			if (env) then
				CSetLoadedEnv(f, env)
			end
			if (run) then
				return true, c()
			end
			return true, c
		end
		return false, c
	else
		return false, path .. ' not found!'
	end
end

function LoadInput(input, env)
	local c = CLoadInput(input)
	CSetLoadedEnv(c, env)
	return c
end

function WriteTableToFile(t, a, path, f)
	f = f or CNewFileOutput()
	f:Open(path, false)
	ser_array[t] = a
	f:WriteUtf8('return' .. SerializeToTableText(t))
	f:Close()
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
	self:set(x or 0, y or 0, w or 0, h or 0)
end

function Rect:set(x, y, w, h)
	self.x = x or self.x
	self.y = y or self.y
	self.w = w or self.w
	self.h = h or self.h
end

function Rect:copy(rect)
	self.x = rect.x or self.x
	self.y = rect.y or self.y
	self.w = rect.w or self.w
	self.h = rect.h or self.h
end

function Rect:set_pos(x, y)
	self.x = x
	self.y = y
end

function Rect:move(x, y)
	self.x = self.x + x
	self.y = self.y + y
end

function Rect:intersect(rectIn, rectOut)
	x = math.max(self.x, rectIn.x)
	right = math.min(self.x + self.w, rectIn.x + rectIn.w)
	if (x >= right) then
		return false
	end
	y = math.max(self.y, rectIn.y)
	h = math.min(self.y + self.h, rectIn.y + rectIn.h)
	if (y >= h) then
		return false
	end
	rectOut:set(x, y, right - x, h - y)
	return true
end

function Rect:diff(rect)
	return self.x ~= rect.x or self.y ~= rect.y or self.w ~= rect.w or self.h ~= rect.h
end

-----Color-----
Color = class()
function Color:ctor(r, g, b, a)
	self:set(r or 0, g or 0, b or 0, a or 0)
end

function Color:copy(color)
	self.r = color.r or self.r
	self.g = color.g or self.g
	self.b = color.b or self.b
	self.a = color.a or self.a
end

function Color:diff(color)
	return self.r ~= rect.r or self.g ~= rect.g or self.b ~= rect.b or self.a ~= rect.a
end

function Color:set(r, g, b, a)
	self.r = r or self.r
	self.g = g or self.g
	self.b = b or self.b
	self.a = a or self.a
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

function WeakTable()
	return setmetatable({}, {__mode = 'kv'})
end

--SortedMap---
local function SetSorted(m, k, v)
	if (v) then
		table.insert(m._keys, k)
		rawset(m, '_sorted', false)
	end
	rawset(m, k, v)
end

local function UnsortedPairs(m)
	return pairs(m)
end

local function SortedPairs(m)
	if (not m._sorted) then
		table.sort(m._keys)
		rawset(m, '_sorted', true)
	end
	rawset(m, '_k', nil)
	return m._pairs
end
	
function SortedMap(unsort, mode)
	local keys = setmetatable({}, {__mode = 'v'})
	if (unsort) then
		return setmetatable({}, {__index = {pairs = UnsortedPairs}, __mode = mode})
	end
	local m = setmetatable({_keys = keys, pairs = SortedPairs}, {__mode = mode, __newindex = SetSorted})
	rawset(m, '_pairs', function()
		local i, k = next(m._keys, m._k)
		local v = m[k]
		while (i and not v) do
			i, k = next(m._keys, i)
			v = m[k]
		end
		rawset(m, '_k', i)
		return k, m[k]
	end)
	return m
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
	objNotifier:bind_event(EVT.DELIST, self, ObjectArray.on_object_delist)
	
	local o = self
	self.newPairs = function()
		local i = o.i
		i = i + 1
		if (o[i]) then
			o.i = i
			return i, o[i]
		end
	end
	self.filterredPairs = function()
		local i = o.i
		i = i + 1
		local filter_func = o.filter_func
		local filter_param = o.filter_param
		while (o[i] and not filter_func(o[i], filter_param)) do
			i = i + 1
		end
		if (o[i]) then
			o.i = i
			return i, o[i]
		end
	end
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
	for i, v in self:pairs() do
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

function ObjectArray:pairs(filter_func, filter_param)
	self.i = 0
	if (filter_func) then
		self.filter_func = filter_func
		self.filter_param = filter_param
		return self.filterredPairs
	end
	return self.newPairs
end