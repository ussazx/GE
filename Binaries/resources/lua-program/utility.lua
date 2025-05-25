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
	self.g = color.r or self.g
	self.b = color.r or self.b
	self.a = color.r or self.a
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

function Normalize2D(vx, vy)
	local d = vx * vx + vy * vy
	if (d == 0) then
		d = 1
	elseif (d > 1) then
		d = math.sqrt(d)
	end
	return vx / d, vy / d
end

function GetLineNormal2D(x0, y0, x1, y1, clockwise)
	if (x0 == x1) then
		if (y0 == y1) then
			return 0, 0
		end
		local nx = y1 - y0
		if (clockwise) then
			return -nx / math.abs(nx), 0
		end
		return nx / math.abs(nx), 0
	end
	if (y0 == y1) then
		local ny = x0 - x1
		if (clockwise) then
			return 0, -ny / math.abs(ny)
		end
		return 0, ny / math.abs(ny)
	end
	local nx = y1 - y0
	local ny = x0 - x1
	if (clockwise) then
		nx = -nx
		ny = -ny
	end
	return Normalize2D(nx, ny)
end

local function _ExtrudeLines2D(pt, k, vol, clockwise, n0x, n0y, nnx, nny)
	local p0
	k, p0 = next(pt, k)
	local x0, y0 = p0[1], p0[2]
	local n1x, n1y = nnx, nny
	local _, p1 = next(pt, k)
	if (p1) then
		n1x, n1y = GetLineNormal2D(x0, y0, p1[1], p1[2], clockwise)
	end
	local nx, ny = n0x, n0y
	if (nx == 0 and ny == 0) then
		nx, ny = n1x, n1y
	elseif (n1x ~= 0 or n1y ~= 0) then
		nx, ny = Normalize2D((nx + n1x) * 0.5, (ny + n1y) * 0.5)
	end
	if (p1) then
		return {x0 + nx * vol, y0 + ny * vol}, _ExtrudeLines2D(pt, k, vol, clockwise, n1x, n1y, nnx, nny)
	end
	return {x0 + nx * vol, y0 + ny * vol}
end
	

-----ExtrudeLines-----
function ExtrudeLines2D(pt, vol, clockwise, closed)
	local n = #pt
	if (n < 2) then
		return
	end
	local nx = 0
	local ny = 0
	if (closed and n > 2) then
		nx, ny = GetLineNormal2D(pt[n][1], pt[n][2], pt[1][1], pt[1][2], clockwise)
		_ExtrudeLines2D(pt, nil, vol, clockwise, nx, ny, nx, ny)
	end
	return _ExtrudeLines2D(pt, nil, vol, clockwise, nx, ny, nx, ny)
end

-----DrawLines-----
function DrawLines(thickness, closed, ...)
	local n0 = {...}
	local n = #n0
	local pn0 = n0[n]
	local n1 = {ExtrudeLines2D(n0, thickness, true, closed)}
	local pn1 = n1[n]
	for i = n, 1, -1 do
		table.insert(n0, n1[i])
	end
	if (closed) then
		local n2 = n * 2
		pn1.combined = {n + 1, n2 + 1}
		table.insert(n0, pn1)
		pn0.combined = {n, n2 + 2}
		table.insert(n0, pn0)
	end
	return n0
end

-----BakePolyNormals-----
function BakePolyNormals2D(v)
	local num = #v
	if (num < 3) then
		return
	end
	local combined = {}
	local p0, p1 = v[num], v[1]
	local nx, ny = GetLineNormal2D(p0[1], p0[2], p1[1], p1[2], false)
	local n0x, n0y = nx, ny
	for i = 1, num do
		local n1x, n1y = nx, ny
		
		p0 = v[i]
		if (i < num) then
			p1 = v[i + 1]
			n1x, n1y = GetLineNormal2D(p0[1], p0[2], p1[1], p1[2], false)
		else
			n1x, n1y = nx, ny
		end
		if (combined[p0]) then
			p0.normal[1] = p0.normal[1] + n0x + n1x
			p0.normal[2] = p0.normal[2] + n0y + n1y
		elseif (p0.combined) then
			p0.normal = {n0x + n1x, n0y + n1y}
			combined[p0] = p0
		else
			p0.normal = {Normalize2D((n0x + n1x) * 0.5, (n0y + n1y) * 0.5)}
		end
		n0x, n0y = n1x, n1y
	end
	for _, v in pairs(combined) do
		v.normal[1], v.normal[2] = Normalize2D(v.normal[1] * 0.5, v.normal[2] * 0.5)
	end
end

-----PolyAntiAlias-----
function PolyAntiAlias(vertices, width, gap)
	width = math.max(1, width or 1)
	gap = gap or 0
	local outer = width + gap
	local v = vertices
	
	local num = #v
	
	--remove vertices with 0, 0 normal
	
	local t = {}
	local i = 1
	while (i) do
		local n0 = {}
		local j = i
		i = nil
		local n = 0
		local w = false
		while (j <= num) do
			local vp = v[j]
			if (n > 0 and vp.combined) then
				if (vp.combined[2] == j) then
					break
				end
				i = j + 1
				j = vp.combined[2] + 1
			else
				j = j + 1
			end
			n = n + 1
			local vn = vp.normal
			table.insert(n0, {vp[1] + vn[1] * outer, vp[2] + vn[2] * outer, normal = {vn[1], vn[2]}})
		end
		if (n > 0) then
			for k = n, 1, -1 do
				local vp = n0[k]
				local vn = vp.normal
				table.insert(n0, {vp[1] - vn[1] * width, vp[2] - vn[2] * width, normal = {-vn[1], -vn[2]}})
			end
			local pn0 = n0[n]
			local pn1 = n0[n + 1]
			local n2 = n * 2
			pn1.combined = {n + 1, n2 + 1}
			table.insert(n0, pn1)
			pn0.combined = {n, n2 + 2}
			table.insert(n0, pn0)
			
			table.insert(t, n0)
		end
	end
	return t
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
	objNotifier:bind_event(EVT.DELIST, self, ObjectArray.on_object_delist)
	
	self.newPairs = load('i = i + 1 return o[i], i', '', 't', self)
	
	self.o = self
	local filterredPairs = [[
		i = i + 1
		while (o[i] and filter_func(o[i], filter_param) == false) do
			i = i + 1
		end
		return o[i], i
	]]
	self.filterredPairs = load(filterredPairs, '', 't', self)
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

function ObjectArray:pairs(filter_func, filter_param)
	self.i = 0
	if (filter_func) then
		self.filter_func = filter_func
		self.filter_param = filter_param
		return self.filterredPairs
	end
	return self.newPairs
end