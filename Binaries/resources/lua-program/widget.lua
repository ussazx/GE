---widget---
require 'object'
require 'utility'
require 'global'

-----Widget2D-----
Widget2D = class(Object)

function Widget2D:ctor(parent)
	if (parent) then
		parent:AddChild(self)
	end
	self.children = ObjectArray()
	
	self.rect = Rect()
	self.location = Point()
	self.moved = true
	self.sized = true
	self.show = true
end

function Widget2D:dtor()
	if (self.window) then
		self.window:OnWidgetDtor(self)
	end
end

function Widget2D:EnableActive(flag)
	local o = self.active
	if (flag) then
		self.active = self
	else
		self.active = nil
	end
	if (self.window and o ~= self.active) then
		self.window.update = true
	end
end

function Widget2D:SetActive(flag)
	if (self.window) then
		self.window:SetActive(self, flag)
	end
end

function Widget2D:AddChild(widget, ...)
	if (widget.parent) then
		widget.parent:RemoveChild(widget)
	end
	widget.active = widget.active or self.active
	widget.parent = self
	widget.window = self.window
	widget.w_idx = self.children:insert(widget)
	if (self.window) then
		self.window.update = true
	end
	return self:OnAddChild(widget, ...)
end

function Widget2D:OnAddChild(w, x, y)
	w:SetPos(x, y)
end

function Widget2D:RemoveChild(widget)
	if (widget.parent == self) then
		if (widget.active ~= widget) then
			widget.active = nil
		end
		widget.parent = nil
		widget.window = nil
		self.children:remove_idx(widget.w_idx)
		if (self.window) then
			self.window:OnWidgetShow(self, show)
		end
		if (self.OnRemoveChild) then
			self:OnRemoveChild(widget)
		end
	end
end

function Widget2D:Show(show)
	show = show or false
	if (self.show ~= show) then
		if (self.window) then
			self.window:OnWidgetShow(self, show)
		elseif(self.window) then
			self.window.update = true
		end
		if (self.inLayout) then
			self.inLayout:SetUpdate(true)
		end
		self.show = show
		self:process_event(EVT.SHOW)
	end
end

function Widget2D:Update(...)
	self.location.x = self.rect.x
	self.location.y = self.rect.y
	if (self.parent) then
		self.active = self.active or self.parent.active
		self.window = self.parent.window
		self.location:move(self.parent.location.x, self.parent.location.y)
		self.moved = self.moved or self.parent.moved
	end

	self.moving = false
	self.sizing = false
	if (self.show) then
		self.updating = true
		if (self:UpdateChildren(self:DoUpdate(...))) then
			self.moved = self.moving
			self.sized = self.sizing
		end
		self.updating = false
	end
end

function Widget2D:DoUpdate(...)
	return true, ...
end

function Widget2D:UpdateChildren(b, ...)
	if (b) then
		for w in self.children:pairs() do
			w:Update(...)
		end
	end
	return b
end

local function FilterShown(c, allowHided)
	return allowHided or c.show
end

function Widget2D:ChildrenPairs()
	return self.children:pairs(FilterShown, allowHided)
end

function Widget2D:SetPos(x, y)
	if (self.inLayout and self.inLayout.update == false) then
		return
	end
	self:DoSetPos(x, y)
	if (self.window) then
		self.window.update = true
	end
end

function Widget2D:DoSetPos(x, y)
	x = x or self.rect.x
	y = y or self.rect.y
	if (self.rect.x ~= x or self.rect.y ~= y) then
		self.rect.x = x
		self.rect.y = y
		self.moved = not self.updating
		self.moving = self.updating
		if (self.OnMoved) then
			self:OnMoved()
		end
	end
end

function Widget2D:Move(x, y)
	self:SetPos(self.rect.x + x, self.rect.y + y)
end

function Widget2D:SetSize(w, h)
	w = w or self.rect.w
	h = h or self.rect.h
	if (self.rect.w == w and self.rect.h == h) then
		return
	end
	
	local w0, h0 = self.rect.w, self.rect.h
	
	self.rect.w = w
	self.rect.h = h
	self.sized = not self.updating
	self.sizing = self.updating
	
	self:process_event(EVT.SIZE, w0, h0)
	
	if (self.inLayout) then
		self.inLayout:SetUpdate(true)
	end
	
	if (self.OnSized) then
		self:OnSized()
	end
end

--Layout--
Layout = class(Widget2D)

function Layout:ctor()
	self.props = setmetatable({}, {__mode = 'k'})
	self.update = true
	self.sized = false
	self.expands = {}
end

function Layout:SetSize(w, h)
	local ww = w or self.rect.w
	local hh = h or self.rect.h
	if (ww ~= self.rect.w or hh ~= self.rect.h or self.update) then
		local w0, h0 = self.rect.w, self.rect.h
		if (self.update) then
			ww = w or 0
			hh = h or 0
		end
		self:Layout(ww, hh)
		self.update = false
		self.sized = false
		self.upNotified = false
		self:process_event(EVT.SIZE, w0, h0)
	end
end

function Layout:GetProp(w)
	return self.props[w]
end

function Layout:OnAddChild(w, ...)
	w.inLayout = self
	self:SetUpdate(true)
	local o = {}
	self.props[w] = o
	self.InitProp(o, ...)
	return o
end

function Layout:SetUpdate(upNotify, updateWindow)
	self.update = true
	if (upNotify and self.inLayout and not self.upNotified) then
		self.inLayout:SetUpdate(true)
		self.upNotified = true
	end
	if (updateWindow and self.window) then
		self.window.update = true
	end
end

function Layout:OnRemoveChild(w)
	self.props[w] = nil
	self:SetUpdate(true)
end

function Layout:DoUpdate(...)
	if (self.parent.sized or self.update) then
		self:SetSize(self.parent.rect.w, self.parent.rect.h)
	end
	return true, ...
end

local function InitVBoxLayoutProp(o, ratio, gapTop, gapBottom, expand, gapLeft, gapRight)
	o.ratio = ratio
	o.expand = expand
	o.left = gapLeft or 0
	o.alignL = gapLeft
	o.right = gapRight or 0
	o.alignR = gapRight
	o.top = gapTop or 0
	o.alignT = gapTop
	o.bottom = gapBottom or 0
	o.alignB = gapBottom
end

local function InitHBoxLayoutProp(o, ratio, gapLeft, gapRight, expand, gapTop, gapBottom)
	InitVBoxLayoutProp(o, ratio, gapTop, gapBottom, expand, gapLeft, gapRight)
end

local function InitGridLayoutProp(o, gapLeft, gapRight, gapTop, gapBottom)
	o.gapLeft = gapLeft or 0
	o.gapRight = gapRight or 0
	o.gapTop = gapTop or 0
	o.gapBottom = gapBottom or 0
end

-----VBoxLayout-----
VBoxLayout = class(Layout)
VBoxLayout.InitProp = InitVBoxLayoutProp

function VBoxLayout:Layout(w, h)
	local ww = w
	local y = 0
	local scale = 0
	local expands = self.expands
	local exnum = 0
	for v, k in self:ChildrenPairs() do
		local prop = self.props[v]
	
		if (prop.expand) then
			if (prop.ratio) then
				scale = scale + prop.ratio
			else
				exnum = exnum + 1
				expands[exnum] = k
			end
		else
			v:SetSize()
			ww = math.max(ww, prop.left + v.rect.w + prop.right)
			if (prop.ratio) then
				scale = scale + prop.ratio
			else
				y = y + v.rect.h
			end
		end
		y = y + prop.top + prop.bottom
	end
	for i = 1, exnum do
		local v = self.children[expands[i]]
		local prop = self.props[v]
		v:SetSize(math.max(0, math.floor(ww - prop.left - prop.right)), nil)
		y = y + v.rect.h
	end
	self.flex = math.max(0, h - y)
	self.scale = math.max(0, scale)
	
	h = 0
	for v, k in self:ChildrenPairs() do
		local prop = self.props[v]
		
		local rh = 0
		if (self.flex > 0 and self.scale > 0) then
			rh = math.floor((prop.ratio or 0) / self.scale * self.flex)
		end
		if (prop.ratio) then
			local rw
			if (prop.expand) then
				rw = math.max(0, math.floor(ww - prop.left - prop.right))
			end
			if (prop.alignT and prop.alignB) then
				v:SetSize(rw, rh)
			else
				v:SetSize(rw, nil)
			end
		end
		
		local x = 0
		if (prop.alignL) then
			x = prop.left
		elseif (prop.alignR) then
			x = ww - v.rect.w - prop.right
		else
			x = (ww - v.rect.w) // 2
		end
		
		local d = math.max(0, rh - v.rect.h)
		if (prop.alignT) then
			h = h + prop.top
		elseif (prop.alignB) then
			h = h + d
			d = 0
		else
			local dd = d // 2
			h = h + dd
			d = d - dd
		end
		v:DoSetPos(x, h)
		h = h + v.rect.h + d + prop.bottom
	end
	self.rect.w = ww
	self.rect.h = h
end

-----HBoxLayout-----
HBoxLayout = class(Layout)
HBoxLayout.InitProp = InitHBoxLayoutProp

function HBoxLayout:Layout(w, h)
	local hh = h
	local x = 0
	local scale = 0
	local expands = self.expands
	local exnum = 0
	for v, k in self:ChildrenPairs() do
		local prop = self.props[v]
		
		if (prop.expand) then
			if (prop.ratio) then
				scale = scale + prop.ratio
			else
				exnum = exnum + 1
				expands[exnum] = k
			end
		else
			v:SetSize()
			hh = math.max(hh, prop.top + v.rect.h + prop.bottom)
			if (prop.ratio) then
				scale = scale + prop.ratio
			else
				x = x + v.rect.w
			end
		end
		x = x + prop.left + prop.right
	end
	for i = 1, exnum do
		local v = self.children[expands[i]]
		local prop = self.props[v]
		v:SetSize(nil, math.max(0, hh - prop.top - prop.bottom))
		x = x + v.rect.w
	end
	self.flex = math.max(0, w - x)
	self.scale = math.max(0, scale)
	
	w = 0
	for v in self:ChildrenPairs() do
		local prop = self.props[v]
		
		local rw = 0
		if (self.flex > 0 and self.scale > 0) then
			rw = math.floor((prop.ratio or 0) / self.scale * self.flex)
		end
		if (prop.ratio) then
			local rh
			if (prop.expand) then
				rh = math.max(0, hh - math.floor(prop.top - prop.bottom))
			end
			if (prop.alignL and prop.alignR) then
				v:SetSize(rw, rh)
			else
				v:SetSize(nil, rh)
			end
		end
		
		local y = 0
		if (prop.alignT) then
			y = prop.top
		elseif (prop.alignB) then
			y = hh - v.rect.h - prop.bottom
		else
			y = (hh - v.rect.h) // 2
		end
		
		local d = math.max(0, rw - v.rect.w)
		if (prop.alignL) then
			w = w + prop.left
		elseif (prop.alignR) then
			w = w + d
			d = 0
		else
			local dd = d // 2
			w = w + dd
			d = d - dd
		end
		v:DoSetPos(w, y)
		w = w + v.rect.w + d + prop.right
	end
	self.rect.w = w
	self.rect.h = hh
end

-----SizerLayout-----
local function SizerLayout_GetVLen(w)
	return w.rect.h
end

local function SizerLayout_GetHLen(w)
	return w.rect.w
end

local function SizerLayout_SetVLen(w, n)
	w:SetSize(nil, n)
end

local function SizerLayout_SetHLen(w, n)
	w:SetSize(n, nil)
end

local function SizerLayout_OnSizerCaptureLost(layout)
	self.mp = false
	g_actWindow.cursor = SYS.CURSOR_ARROW
end

local function SizerLayout_OnSizerMouse(layout, e, x, y)
	local GetLen = layout.GetLen
	local SetLen = layout.SetLen
	local p
	if (layout.vertical) then
		p = y
	else
		p = x
	end
	if (e == EVT.MOVE_IN) then
		g_actWindow.cursor = layout.cursor
	elseif (e == EVT.MOVE_OUT) then
		g_actWindow.cursor = SYS.CURSOR_ARROW
	elseif (e == EVT.LEFT_DOWN) then
		layout.c0 = nil
		layout.c1 = nil
		local sizer
		for c in layout:ChildrenPairs() do
			if (c == EVT.obj) then
				if (not layout.c0) then
					return
				end
				sizer = c
			elseif (sizer) then
				layout.c1 = c
				break
			else
				layout.c0 = c
			end
		end
		if (layout.c0 and layout.c1) then
			if (layout.vertical) then
				layout.mp = y
			else
				layout.mp = x
			end
			g_actWindow:CaptureMouse(EVT.obj)
		end
	elseif (e == EVT.MOTION) then
		if (layout.mp) then
			local c0 = layout.c0
			local c1 = layout.c1
			local dp = p - layout.mp
			if (dp == 0 or (dp < 0 and GetLen(c0) == 0) or (dp > 0 and GetLen(c1) == 0) or 
				(GetLen(c0) == 0 and GetLen(c1) == 0)) then
				return
			end
			if (dp > 0) then
				dp = math.min(dp, GetLen(c1))
			else
				dp = math.max(dp, -GetLen(c0))
			end
			
			local prop0 = layout.props[c0]
			local prop1 = layout.props[c1]
			if (prop0.ratio) then
				if (prop1.ratio) then
					if (layout.flex > 0 and layout.scale > 0) then
						prop0.ratio = (GetLen(c0) + dp) / layout.flex * layout.scale
						prop1.ratio = (GetLen(c1) - dp) / layout.flex * layout.scale
					end
				else
					if (prop0.ratio < layout.scale and layout.flex > 0) then
						prop0.ratio = math.max(0, prop0.ratio - layout.scale * (1 - (layout.flex + dp) / layout.flex))
					end
					SetLen(c1, GetLen(c1) - dp)
				end
			else
				SetLen(c0, GetLen(c0) + dp)
				if (prop1.ratio) then
					if (prop1.ratio < layout.scale and layout.flex > 0) then
						prop1.ratio = math.max(0, prop1.ratio - layout.scale * (1 - (layout.flex - dp) / layout.flex))
					end
				else
					SetLen(c1, GetLen(c1) - dp)
				end
			end
			if (layout.vertical) then
				layout.mp = y
			else
				layout.mp = x
			end
			layout:SetUpdate(false, true)
		end
	elseif (e == EVT.LEFT_UP) then
		if (g_actWindow.captured == EVT.obj) then
			layout.mp = false
			g_actWindow:ReleaseCaptured()
		end
	end
end

local function SizerLayout_AddChild(layout, w, ...)
	local sizer
	local align
	if (layout.vertical) then
		sizer = UiButton(0, layout.sizerWidth)
	else
		sizer = UiButton(layout.sizerWidth, 0)
	end
	sizer:SetDefaultColor(0, 0, 0, 0)
	sizer:bind_event(EVT.MOVE_IN, layout, SizerLayout_OnSizerMouse)
	sizer:bind_event(EVT.MOVE_OUT, layout, SizerLayout_OnSizerMouse)
	sizer:bind_event(EVT.LEFT_DOWN, layout, SizerLayout_OnSizerMouse)
	sizer:bind_event(EVT.MOTION, layout, SizerLayout_OnSizerMouse)
	sizer:bind_event(EVT.LEFT_UP, layout, SizerLayout_OnSizerMouse)
	sizer:bind_event(EVT.CAPTURE_LOST, layout, SizerLayout_OnSizerCaptureLost)
	local show = false
	for c in layout:ChildrenPairs() do
		show = true
		break
	end
	sizer:Show(show)
	Widget2D.AddChild(layout, sizer, nil, 0, 0, true)
	layout.sizers[w] = sizer
	
	w:bind_event(EVT.SHOW, layout, SizerLayout_OnWidgetShow)
	Widget2D.AddChild(layout, w, ...)
end

local function SizerLayout_Ctor(layout)
	layout.sizers = setmetatable({}, {__mode = 'k'})
end

local function SizerLayout_OnWidgetShow(layout)
	if (EVT.obj.show) then
		layout.sizers[w]:Show(true)
	else
		layout.sizers[w]:Show(false)
	end
end

local function SizerLayout_OnRemoveChild(layout, w)
	local sizer = layout.sizers[w]
	if (sizer) then
		layout:OnRemoveChild(sizer)
	end
end

VSizerLayout = class(VBoxLayout)
VSizerLayout.vertical = true
VSizerLayout.ctor = SizerLayout_Ctor
VSizerLayout.OnRemoveChild = SizerLayout_OnRemoveChild
VSizerLayout.AddChild = SizerLayout_AddChild
VSizerLayout.GetLen = SizerLayout_GetVLen
VSizerLayout.SetLen = SizerLayout_SetVLen
VSizerLayout.cursor = SYS.CURSOR_SIZENS
VSizerLayout.sizerWidth = 7

HSizerLayout = class(HBoxLayout)
HSizerLayout.vertical = false
HSizerLayout.ctor = SizerLayout_Ctor
VSizerLayout.OnRemoveChild = SizerLayout_OnRemoveChild
HSizerLayout.AddChild = SizerLayout_AddChild
HSizerLayout.GetLen = SizerLayout_GetHLen
HSizerLayout.SetLen = SizerLayout_SetHLen
HSizerLayout.cursor = SYS.CURSOR_SIZEWE
HSizerLayout.sizerWidth = 7

-----GridLayout-----
GridLayout = class(Layout)
GridLayout.InitProp = InitGridLayoutProp

function GridLayout:ctor(col)
	if (col and col < 1) then
		col = 1
	end
	self.col = col
end

function GridLayout:Layout(w, h)
	local gw = 0
	local gh = 0
	for v in self.children:pairs() do
		local prop = self.props[v]
		v:SetSize()
		gw = math.max(gw, prop.gapLeft + v.rect.w + prop.gapRight)
		gh = math.max(gh, prop.gapTop + v.rect.h + prop.gapBottom)
	end
	if (gw == 0 or gh == 0) then
		return
	end
	local col = self.col
	if (col == nil) then
		col = math.max(1, (w or 0) // gw)
		gw = math.max(gw, (w or 0) // col)
	end
	local x = 0
	local y = 0
	local i = 1
	for v in self.children:pairs() do
		local prop = self.props[v]
		v:DoSetPos(x + prop.gapLeft - prop.gapRight + (gw - v.rect.w) // 2, y + prop.gapTop - prop.gapBottom + (gh - v.rect.h) // 2)
		if (i == col) then
			x = 0
			y = y + gh
			i = 1
		else
			x = x + gw
			i = i + 1
		end
	end
	self.rect.w = gw * col
	self.rect.h = y + gh * (1 - 1 // i)
end

-----UiWidget-----
UiWidget = class(Widget2D)
UiWidget.pipeline = g_plUi
UiWidget.font = uiFont
UiWidget.cached = false
UiWidget.cpuClip = true
UiWidget.gpuClip = false
UiWidget.bakeCount = 4
UiWidget.writeId = true
UiWidget.drawClipRect = false
UiWidget.acceptFocus = true
UiWidget.show = true
UiWidget.drawSelf = true

function UiWidget:ctor(w, h)
	self.rect:set(0, 0, w, h)
	self.cr = Rect()
	self.crNew = Rect()
	self.crColor = Color()
	self.color = Color(255, 255, 255, 255)
	self.renderDisables = {[g_rp0[2]] = true}
	
	self.renderer = MeshRenderer(self, 1|2|4, self.FillVB, self.cached)
	self.renderer:SetMaterial(g_mtlUi, {0, 1}, {self.id, 1})
	
	self.rcRenderer = MeshRenderer(self, 1|2|4, self.FillClipRectVB, false)
	self.rcRenderer:SetMaterial(g_mtlUi, {0, 1}, {self.id, 1})
end

function UiWidget:GetDrawcall(spId, mergeType, order)
	return g_dcLists[spId]
end

function UiWidget:Refresh()
	self.renderer.update = true
	if (self.window) then
		self.window.update = true
	end
end

function UiWidget:OnMoved()
	self:Refresh()
end

function UiWidget:OnSized()
	self:Refresh()
end

local function OlUpdateChildren(ol, b, ...)
	local rect = ol.w.rect
	local location = ol.w.location
	ol.w.rect = ol.rect
	ol.w.location = ol.location
	local b = Widget2D.UpdateChildren(ol, b, ...)
	ol.w.rect = rect
	ol.w.location = location
	return b
end

function UiWidget:ShowOutline(flag, color)
	local olLayout = self.olLayout
	if (not flag) then
		if (olLayout) then
			self.children = self.oc
			self:Refresh()
		end
		return
	end
	if (not olLayout) then	
		olLayout = VBoxLayout()
		self.olLayout = olLayout
		
		olLayout.lineTop = UiWidget(0, 1)
		olLayout.lineTop.writeId = false
		olLayout:AddChild(olLayout.lineTop, nil, 0, 0, true)
		
		local midLayout = HBoxLayout()
		olLayout:AddChild(midLayout, 1, 0, 0, true)
		
		olLayout.lineLeft = UiWidget(1, 0)
		olLayout.lineLeft.writeId = false
		midLayout:AddChild(olLayout.lineLeft, nil, 0, 0, true)
		
		local midWidget = UiWidget()
		midWidget.drawSelf = false
		midWidget.children = self.children
		midWidget.w = self
		midWidget.UpdateChildren = OlUpdateChildren
		midLayout:AddChild(midWidget, 1, 0, 0, true)
		
		olLayout.lineRight = UiWidget(1, 0)
		olLayout.lineRight.writeId = false
		midLayout:AddChild(olLayout.lineRight, nil, 0, 0, true)
		
		olLayout.lineBottom = UiWidget(0, 1)
		olLayout.lineBottom.writeId = false
		olLayout:AddChild(olLayout.lineBottom, nil, 0, 0, true)
		
		local w = Widget2D()
		w:AddChild(olLayout)
		olLayout.parent = self
		self.oc = self.children
		self.olc = w.children
	end
	if (color) then
		self.olLayout.lineTop.color:copy(color)
		self.olLayout.lineLeft.color:copy(color)
		self.olLayout.lineRight.color:copy(color)
		self.olLayout.lineBottom.color:copy(color)
	end
	self.children = self.olc
	olLayout:SetUpdate()
end

function UiWidget:FillVB(vbPos, wpPos, vbUVW, wpUVW, vbColor, wpColor, ib, iwp, ibStart)
	return self:FillRectVB(self.color, vbPos, wpPos, vbUVW, wpUVW, vbColor, wpColor, ib, iwp, ibStart)
end

function UiWidget:FillRectVB(color, vbPos, wpPos, vbUVW, wpUVW, vbColor, wpColor, ib, iwp, ibStart)
	if (self.cpuClip) then
		CAddRectFloat3(vbPos, wpPos, self.cr.x, self.cr.y, self.cr.w, self.cr.h, Z_2D)
	else
		CAddRectFloat3(vbPos, wpPos, self.location.x, self.location.y, self.rect.w, self.rect.h, Z_2D)
	end
	CAddFloat3(vbUVW, wpUVW, 4, self.font.pixels, 0, 0)
	CAddUByte4(vbColor, wpColor, 4, color.r, color.g, color.b, color.a)
	return 4, CAddConvexPolyIndex(ib, iwp, 1, ibStart, 4)
end

function UiWidget:FillClipRectVB(vbPos, wpPos, vbUVW, wpUVW, vbColor, wpColor, ib, iwp, ibStart)
	return self:FillRectVB(self.crColor, vbPos, wpPos, vbUVW, wpUVW, vbColor, wpColor, ib, iwp, ibStart)
end

function UiWidget:DoUpdate(crCpu, crGpu)
	local crCpuNew
	local crGpuNew
	if (self.cpuClip or self.gpuClip) then
		self.crNew:set(self.location.x, self.location.y, self.rect.w, self.rect.h)
		if (self.cpuClip) then
			if (crCpu and not self.crNew:intersect(crCpu, self.crNew)) then
				return false
			end
			crCpuNew = self.crNew
		end
		if (self.gpuClip) then
			if (crCpuNew and (crCpu == crGpu or crGpu == nil)) then
				crGpuNew = crCpuNew
			else
				local cr = crGpu or crCpu
				if (cr and not self.crNew:intersect(cr, self.crNew)) then
					return false
				end
				crGpuNew = self.crNew
			end
		end
	end
	crCpuNew = crCpuNew or crCpu
	crGpuNew = crGpuNew or crGpu

	if (not DrawcallList.cr or (crGpuNew and DrawcallList.cr:diff(crGpuNew))) then
		DrawcallList.cr = crGpuNew
	end
	
	local renderer = self.renderer
	local changed = renderer.doCache and (renderer.update or self.moved or self.sized or
	(crCpuNew and self.cr:diff(crCpuNew)))
	
	if (self.cpuClip) then
		self.cr:copy(crCpuNew)
	end
	
	local d
	if (not self.writeId) then
		d = self.renderDisables
	end
	
	if (self.drawClipRect) then
		self.rcRenderer:Render(self, d)
	end
	renderer.update = changed
	if (self.drawSelf) then
		renderer:Render(self, d)
	end
	
	return true, crCpuNew, crGpuNew
end

-----UiButtonBase-----
UiButtonBase = class(UiWidget)

function UiButtonBase:ctor()
	self:bind_event(EVT.MOVE_IN, self, UiButtonBase.OnMouse)
	self:bind_event(EVT.MOVE_OUT, self, UiButtonBase.OnMouse)
	self:bind_event(EVT.LEFT_DOWN, self, UiButtonBase.OnMouse)
	self:bind_event(EVT.LEFT_UP, self, UiButtonBase.OnMouse)
end

function UiButtonBase:OnMouse(e)
	if (e == EVT.LEFT_DOWN) then
		self.down = true
	elseif (e == EVT.LEFT_UP) then
		self.down = false
	elseif (e == EVT.MOVE_IN) then
		self.hoverring = true
	elseif (e == EVT.MOVE_OUT) then
		self.down = false
		self.hoverring = false
	end
	
	if (self.down) then
		self:OnPressing()
		
	elseif (self.hoverring) then
		self:OnHoverring()
	else
		self:OnDefault()
	end
	
	self:Refresh()
end

function UiButtonBase:OnDefault()
end

function UiButtonBase:OnHoverring()
end

function UiButtonBase:OnPressing()
end

-----UiButton-----
UiButton = class(UiButtonBase)
UiButton.colorDefault = Color(50, 50, 50, 255)
UiButton.colorHoverring = Color(100, 100, 100, 255)
UiButton.colorPressing = Color(150, 150, 150, 255)

function UiButton:ctor(w, h, s, font)
	self.color0 = Color()
	self.color0:copy(UiButton.colorDefault)
	self.color1 = Color()
	self.color1:copy(UiButton.colorHoverring)
	self.color2 = Color()
	self.color2:copy(UiButton.colorPressing)
	self.color = self.color0
	self.layout = HBoxLayout()
	self:AddChild(self.layout)
	self.text = UiText(s, font)
	self.text.writeId = false
	self.layout:AddChild(self.text, 1)
	self:SetSize(w or self.text.rect.w, h or self.text.rect.h)
end

function UiButton:OnDefault()
	self.color = self.color0
end

function UiButton:OnHoverring()
	self.color = self.color1
end

function UiButton:OnPressing()
	self.color = self.color2
end

function UiButton:SetDefaultColor(r, g, b, a)
	self.color0:set(r, g, b, a)
end

function UiButton:SetHoverringColor(r, g, b, a)
	self.color1:set(r, g, b, a)
end

function UiButton:SetPressingColor(r, g, b, a)
	self.color2:set(r, g, b, a)
end

function UiButton:ResetAllColors()
	self.color0:copy(UiButton.colorDefault)
	self.color1:copy(UiButton.colorHoverring)
	self.color2:copy(UiButton.colorPressing)
end

function UiButton:SetAllColors(r, g, b, a)
	self.color0:set(r, g, b, a)
	self.color1:set(r, g, b, a)
	self.color2:set(r, g, b, a)
end

-----Text-----
UiText = class(UiWidget)
UiText.cached = true

function UiText:ctor(s, font)
	self.text = LString('')
	self:SetText(s, font)
end

function UiText:SetText(s, font)
	s = s or self.text
	font = font or uiFont
	self:Show(s ~= '')
	if (self.text ~= s or self.font ~= font) then
		self.text:set(s)
		self.font = font
		self:SetSize(CMeasureText(s, -1, -1, font), font.fontSize)
		self:Refresh()
	end
end

function UiText:FillVB(vbPos, wpPos, vbUVW, wpUVW, vbColor, wpColor, ib, iwp, ibStart)
	local n
	if (self.cpuClip) then
		n = CAddTextClip(vbPos, wpPos, vbUVW, wpUVW, self.font,
		self.location.x - self.cr.x, self.location.y - self.cr.y + self.font.fontSize + self.font.descender,
			self.cr.x, self.cr.y, self.cr.w, self.cr.h, Z_2D, self.text)
	else
		n = CAddText(vbPos, wpPos, vbUVW, wpUVW, self.font, self.location.x, self.location.y + self.rect.h + self.font.descender, Z_2D, self.text)
	end
	
	CAddUByte4(vbColor, wpColor, 4 * n, self.color.r, self.color.g, self.color.b, self.color.a)
	return 4 * n, CAddConvexPolyIndex(ib, iwp, n, ibStart, 4)
end

-----TextInput-----
UiTextInput = class(UiWidget)
UiTextInput.cached = true
UiTextInput.cpuClip = true
UiTextInput.drawClipRect = true

local function TextInputAssign(a, b)
	a.text = LString(b.text)
	a.font = b.font
	a.textWidth = b.textWidth
	a.insertIdx = b.insertIdx
	a.textOffset = b.textOffset
	a.selectedIdx = b.selectedIdx
	a.selected_x = b.selected_x
	local caret_x = b.caret_x
	if (b.caret) then
		caret_x = b.caret.rect.x
	end
	if (a.caret) then
		a.caret.rect.x = caret_x
	else
		a.caret_x = caret_x
	end
end

function UiTextInput:ctor(w, h, font)
	self.rect:set(0, 0, w, h)
	
	self.crColor:set(80, 80, 80, 255)
	self.selectedColor = Color(0, 130, 255, 100)
	
	self.caret = UiWidget(1, self.rect.h)
	self.caret.writeId = false
	self.caret:Show(false)
	self:AddChild(self.caret)
	
	self.selected_x = 0
	self.selectedIdx = -1
	
	self.textWidth = 0
	self.textOffset = 0
	self.insertIdx = 0
	
	self.text = LString('')
	self:SetText('', font or uiFont)
	
	self.timer = Timer()
	self.timer:bind_event(EVT.TIMER, self, UiTextInput.OnTimer)
	
	self:bind_event(EVT.FOCUS_IN, self, UiTextInput.OnFocus)
	self:bind_event(EVT.FOCUS_OUT, self, UiTextInput.OnFocus)
	
	self:bind_event(EVT.LEFT_DOWN, self, UiTextInput.OnMouseDown)
	self:bind_event(EVT.RIGHT_DOWN, self, UiTextInput.OnMouseDown)
	self:bind_event(EVT.LEFT_DCLICK, self, UiTextInput.OnMouseDown)
	
	self:bind_event(EVT.LEFT_UP, self, UiTextInput.OnMouseUp)
	self:bind_event(EVT.RIGHT_UP, self, UiTextInput.OnMouseUp)
	
	self:bind_event(EVT.MOTION, self, UiTextInput.OnMouseMotion)
	self:bind_event(EVT.CAPTURE_LOST, self, UiTextInput.OnCaptureLost)
	
	self:bind_event(EVT.CHAR, self, UiTextInput.OnChar)
	self:bind_event(EVT.KEY_DOWN, self, UiTextInput.OnKeyDown)
	self:bind_event(EVT.ACC_KEY, self, UiTextInput.OnAccKey)
	
	self:bind_event(EVT.MOVE_IN, self, UiTextInput.OnMoveInOut)
	self:bind_event(EVT.MOVE_OUT, self, UiTextInput.OnMoveInOut)
	
	self.record = {}
	TextInputAssign(self.record, self)
end

function UiTextInput:Record()
	local o = {redo = {}, undo = self.record}
	TextInputAssign(o.redo, self)
	g_recorder:Record(self, self.HandleRecord, o)
	
	self.record = {}
	TextInputAssign(self.record, self)
end

function UiTextInput:OnMoveInOut(e)
	if (e == EVT.MOVE_IN) then
		g_actWindow.cursor = SYS.CURSOR_IBEAM
	elseif (e == EVT.MOVE_OUT) then
		g_actWindow.cursor = SYS.CURSOR_ARROW
	end
end

function UiTextInput:ClearSelected()
	if (self.selectedIdx >= 0) then
		self.selectedIdx = -1
		self:Refresh()
	end
end

function UiTextInput:ResetCaret()
	if (self.hasFocus) then
		self.caret:Show(true)
		self.timer:Start(500, true)
	end
end

function UiTextInput:HideCaret()
	self.caret:Show(false)
	self.timer:Stop()	
end

function UiTextInput:GetSelectedRange()
	if (self.selectedIdx < 0 or self.selectedIdx == self.insertIdx) then
		return self.insertIdx, 0
	end
	TextInputAssign(self.record, self)
	if (self.selectedIdx < self.insertIdx) then
		return self.selectedIdx, self.insertIdx - self.selectedIdx
	end
	return self.insertIdx, self.selectedIdx - self.insertIdx
end

function UiTextInput:OnTimer()
	self.caret:Show(self.caret.show == false)
end

function UiTextInput:OnFocus(e)
	if (e == EVT.FOCUS_IN) then
		self.hasFocus = true
		self:ResetCaret()
	else
		self:HideCaret()
		self:ClearSelected()
		self.hasFocus = false
	end
end

function UiTextInput:OnSized()
	local endSpace = self.rect.w - self.textOffset + self.textWidth
	if (endSpace > 0) then
		if (self.textOffset + endSpace > 0) then
			endSpace = -self.textOffset
			self.textOffset = 0
		else
			self.textOffset = self.textOffset + endSpace
		end
		self.selected_x = self.selected_x + endSpace
		self.caret:Move(endSpace, 0)
	end
	self.caret:SetPos(nil, self.rect.h - self.caret.rect.h)
	self:RestrictCaretPos(self.caret.rect.x, true)
end

function UiTextInput:SelectAll()
	if (self.text:length() == 0) then
		return
	end
	self.selectedIdx = 0
	self.selected_x = self.textOffset
	local x
	x, self.insertIdx = CMeasureText(self.text, -1, -1, self.font)
	self.caret:SetPos(x + self.textOffset, 0)
	self:HideCaret()
	self:Refresh()
end

function UiTextInput:OnMouseDown(e, x)
	if (e == EVT.LEFT_DOWN) then
		x, self.insertIdx = CMeasureText(self.text, -1, x - self.location.x - self.textOffset, self.font)
		self:RestrictCaretPos(x + self.textOffset)
	
		self.selectedIdx = self.insertIdx
		self.selected_x = self.caret.rect.x
		g_actWindow:CaptureMouse(self)
	elseif(e == EVT.LEFT_DCLICK) then
		self:SelectAll()
	end
end

function UiTextInput:OnMouseUp(e, x)
	if (g_actWindow.captured == self) then
		g_actWindow:ReleaseCaptured()
	end
end

function UiTextInput:OnCaptureLost()
	self:ClearSelected()
end

function UiTextInput:OnMouseMotion(e, x)
	if (g_actWindow.captured == self) then
		x = x - self.location.x - self.textOffset
		if (x < 0) then
			x = 0
		end
		x, self.insertIdx = CMeasureText(self.text, -1, x, self.font)
		self:RestrictCaretPos(x + self.textOffset)
	end
end

function UiTextInput:HandleRecord(redo, data)
	if (redo) then
		TextInputAssign(self, data.redo)
	else
		TextInputAssign(self, data.undo)
		TextInputAssign(self.record, data.undo)
	end
	self:RestrictCaretPos(self.caret.rect.x)
end

function UiTextInput:OnChar(e, c)
	local idx, count = self:GetSelectedRange()
	if (count > 0) then
		self:RemoveText(idx, count, true)
	end
	
	local w = CMeasureText(c, -1, -1, self.font)
	self.insertIdx = self.insertIdx + self.text:insert(self.insertIdx, c)
	self.textWidth = self.textWidth + w
	self:RestrictCaretPos(self.caret.rect.x + w)
	self:ClearSelected()
	
	self:Record()
end

function UiTextInput:RemoveText(idx, count, recorded)
	local s = self.text:substr(idx, count)
	if (s:length() == 0) then
		return
	end
	self.text:erase(idx, count)
	local d = CMeasureText(s, -1, -1, self.font)
	self.textWidth = self.textWidth - d
	if (self.textOffset + d > 0) then
		if (self.insertIdx == idx) then
			d = 0
		end
		self.caret:Move(-self.textOffset - d, 0)
		self.textOffset = 0
	else
		self.textOffset = self.textOffset + d
		if (self.insertIdx == idx) then
			self.caret:Move(d, 0)
		end
	end
	self.insertIdx = idx
	self:RestrictCaretPos(self.caret.rect.x)
	self:ClearSelected()
	
	if (recorded == nil) then
		self:Record()
	end
	return s
end

function UiTextInput:OnKeyDown(e, k)
	local x
	local shiftDown = g_actWindow.keyDowns[SYS.VK_SHIFT] or false
	if (k == SYS.VK_SHIFT) then
		if (self.selectedIdx < 0) then
			self.selectedIdx = self.insertIdx
			self.selected_x = self.caret.rect.x
		end
		
	elseif (k == SYS.VK_BACK or k == SYS.VK_DELETE) then
		local idx, count = self:GetSelectedRange()
		if (count == 0) then
			if (k == SYS.VK_BACK) then
				if (self.insertIdx == 0) then
				return end
				idx = self.insertIdx - 1
			else
				idx = self.insertIdx
			end
			count = 1
		end
		self:RemoveText(idx, count)
	
	elseif (k == SYS.VK_LEFT) then
		if (self.selectedIdx >= 0 and self.selectedIdx ~= self.insertIdx and shiftDown == false) then
			self.insertIdx = math.min(self.selectedIdx, self.insertIdx)
			self:RestrictCaretPos(math.min(self.selected_x, self.caret.rect.x))
			self:ClearSelected()
			return
		end
		if (self.insertIdx == 0) then
		return end
		self.insertIdx = self.insertIdx - 1
		x, self.insertIdx = CMeasureText(self.text, self.insertIdx, -1, self.font)
		self:RestrictCaretPos(x + self.textOffset)
		if (shiftDown == false) then
			self:ClearSelected()
		end
	
	elseif (k == SYS.VK_RIGHT) then
		if (self.selectedIdx >= 0 and self.selectedIdx ~= self.insertIdx and shiftDown == false) then
			self.insertIdx = math.max(self.selectedIdx, self.insertIdx)
			self:RestrictCaretPos(math.max(self.selected_x, self.caret.rect.x))
			self:ClearSelected()
			return
		end
		self.insertIdx = self.insertIdx + 1
		x, self.insertIdx = CMeasureText(self.text, self.insertIdx, -1, self.font)
		self:RestrictCaretPos(x + self.textOffset)
		if (shiftDown == false) then
			self:ClearSelected()
		end
	
	elseif (k == SYS.VK_HOME) then
		self.caret:SetPos(0, 0)
		self.insertIdx = 0
		self.selected_x = self.selected_x - self.textOffset
		self.textOffset = 0
		self:Refresh()
		self:ResetCaret()
		if (shiftDown == false) then
			self:ClearSelected()
		end
	
	elseif (k == SYS.VK_END) then
		self.textOffset = 0
		x, self.insertIdx = CMeasureText(self.text, -1, -1, self.font)
		self:RestrictCaretPos(x)
		if (shiftDown == false) then
			self:ClearSelected()
		end
	end
end

function UiTextInput:OnAccKey(e, k)
	if (k == SYS.VK_CTRL_A) then
		self:SelectAll()
	
	elseif (k == SYS.VK_CTRL_C or k == SYS.VK_CTRL_X) then
		local idx, count = self:GetSelectedRange()
		if (count == 0) then
		return end
		local s
		if (k == SYS.VK_CTRL_X) then
			s = self:RemoveText(idx, count)
		else
			s = self.text:substr(idx, count)
		end
		if (s) then
			cTerminal.SetClipboardText(s)
		end
	
	elseif (k == SYS.VK_CTRL_V) then
		local s = cTerminal.GetClipboardText()
		if (s) then
			self:OnChar(0, s)
		end
	end
end

function UiTextInput:RestrictCaretPos(x, remainCaret)
	local cr = x + self.caret.rect.w - 1
	if (x < 0) then
		self.textOffset = self.textOffset - x
		self.selected_x = self.selected_x - x
		if (self.textOffset > 0) then
			self.textOffset = 0
		end
		x = 0
	elseif (cr >= self.rect.w) then
		self.textOffset = self.textOffset - cr + self.rect.w - 1
		self.selected_x = self.selected_x - cr + self.rect.w - 1
		x = self.rect.w - 1
	end
	self.caret:SetPos(x)
	self:Refresh()
	if (not remainCaret) then
		self:ResetCaret()
	end
end

function UiTextInput:SetText(s, font)
	s = s or ''
	font = font or uiFont
	if (self.text ~= s or self.font ~= font) then
		self.text:set(s)
		self.font = font
		self.insertIdx = 0
		self.caret:SetSize(1, font.fontSize)
		self.caret:SetPos(0, self.rect.h - self.caret.rect.h)
		if (s ~= '') then
			local w
			w, self.insertIdx = CMeasureText(s, -1, -1, self.font)
			self:RestrictCaretPos(w)
		end
	end
end

function UiTextInput:FillVB(vbPos, wpPos, vbUVW, wpUVW, vbColor, wpColor, ib, iwp, ibStart)
	local rect = self.cr or self.rect
	local n0 = 0
	if (self.selectedIdx >= 0 and self.selectedIdx ~= self.insertIdx) then
		local w = self.selected_x - self.caret.rect.x
		local x = self.caret.rect.x
		if (w < 0) then
			x = self.selected_x
			if (x < 0) then
				x = 0
			end
			w = math.min(self.caret.rect.x - x + 1, rect.w - x)
		elseif (self.selected_x >= rect.w) then
			w = rect.w - self.caret.rect.x
		end
		CAddRectFloat3(vbPos, wpPos, math.max(self.location.x + x, rect.x), rect.y, w, rect.h, Z_2D)
		CAddFloat3(vbUVW, wpUVW, 4, self.font.pixels, 0, 0)
		CAddUByte4(vbColor, wpColor, 4, self.selectedColor.r, self.selectedColor.g, self.selectedColor.b, self.selectedColor.a)
		n0 = 1
		wpPos = APPEND
		wpUVW = APPEND
		wpColor = APPEND
	end
	local n = CAddTextClip(vbPos, wpPos, vbUVW, wpUVW, self.font, self.textOffset, self.rect.h + self.font.descender, 
	rect.x, rect.y, rect.w, rect.h, Z_2D, self.text)
	CAddUByte4(vbColor, wpColor, 4 * n, self.color.r, self.color.g, self.color.b, self.color.a)
	n = n + n0
	return 4 * n, CAddConvexPolyIndex(ib, iwp, n, ibStart, 4)
end

UiSlideBar = class(UiWidget)
UiSlideBar.EVT_SLIDE = {}

function UiSlideBar:ctor(vertical, length, width)
	self.color:set(100, 100, 100, 100)
	self.scale = 1
	self.slScale = 1
	self.step = 1
	self.pos = 0
	self.vertical = vertical
	if (vertical) then
		self.slider = UiButton(math.max(1, width), 0)
		self:SetSize(width, length)
	else
		self.slider = UiButton(0, math.max(1, width))
		self:SetSize(length, width)
	end
	self.slider:SetDefaultColor(150, 150, 150, 255)
	self.slider:SetHoverringColor(200, 200, 200, 255)
	self.slider:SetPressingColor(230, 230, 230, 255)
	
	self:AddChild(self.slider)
	self.slider:bind_event(EVT.LEFT_DOWN, self, UiSlideBar.OnSliderMouseButton)
	self.slider:bind_event(EVT.LEFT_UP, self, UiSlideBar.OnSliderMouseButton)
	self.slider:bind_event(EVT.MOTION, self, UiSlideBar.OnSliding)
end

function UiSlideBar:SetScale(scale, sliderScale)
	self.scale = math.max(1, scale)
	self.slScale = sliderScale
	self:Rescale()
end

function UiSlideBar:Rescale()
	local slRect = self.slider.rect
	if (self.vertical) then
		self.length = math.max(1, self.rect.h)
		self.slider:SetSize(slRect.w, math.max(1, self.rect.h * self.slScale // self.scale))
		--self.step = math.max(1, self.length // self.scale)
		self:SetScalePos(self.pos)
	else
		self.length = math.max(1, self.rect.w)
		self.slider:SetSize(math.max(1, self.rect.w * self.slScale // self.scale), slRect.h)
		--self.step = math.max(1, self.length // self.scale)
		self:SetScalePos(self.pos)
	end
end	

function UiSlideBar:OnSliderMouseButton(e, x, y)
	if (e == EVT.LEFT_DOWN) then
		if (self.vertical) then
			self.slStart = y
		else
			self.slStart = x
		end
		g_actWindow:CaptureMouse(self.slider)
	elseif (e == EVT.LEFT_UP) then
		if (g_actWindow.captured == self.slider) then
			g_actWindow:ReleaseCaptured()
		end
	end
end

function UiSlideBar:OnSliding(e, x, y)
	if (g_actWindow.captured ~= self.slider) then
		return
	end
	if (self.vertical) then
		if (self:SetScalePos(self.pos + math.modf((y - self.slStart) * self.scale / self.length))) then
			self.slStart = y
		end
	else
		if (self:SetScalePos(self.pos + math.modf((x - self.slStart) * self.scale / self.length))) then
			self.slStart = x
		end
	end
end

function UiSlideBar:SetScalePos(n)
	n = math.max(0, n)
	local d = 0
	if (self.vertical) then
		d = n * self.length // self.scale
		while (n > 0 and d + self.slider.rect.h > self.length) do
			n = n - 1
			d = n * self.length // self.scale
		end
		self.slider:SetPos(self.slider.rect.x, d)
	else
		d = n * self.length // self.scale
		while (n > 0 and d + self.slider.rect.w > self.length) do
			n = n - 1
			d = n * self.length // self.scale
		end
		self.slider:SetPos(d, self.slider.rect.y)
	end
	if (self.pos ~= n) then
		self.pos = n
		self:process_event(UiSlideBar.EVT_SLIDE, self.scale * d // self.length, self.scale)
		return true
	end
	return false
end

function UiSlideBar:MoveScalePos(n)
	self:SetScalePos(self.pos + n)
end

function UiSlideBar:OnSized()
	self:Rescale()
end

-----UiScrollPanel-----
UiScrollPanel = class(UiWidget)
UiScrollPanel.barWidth = 12

function UiScrollPanel:ctor(widget, w, h)
	self:SetSize(w, h)
	self:bind_event(EVT.MOUSEWHEEL, self, UiScrollPanel.OnMouseWheel)
	self:bind_event(EVT.SIZE, self, UiScrollPanel.OnWidgetSize)
	
	self.color:set(40, 40, 40, 255)
	
	local vLayout = VBoxLayout()
	self:AddChild(vLayout)
	
	local hLayout = HBoxLayout()
	vLayout:AddChild(hLayout, 1, 0, 0, true)
	
	self.pane = UiWidget()
	self.pane.drawSelf = false
	self.pane.gpuClip = true
	hLayout:AddChild(self.pane, 1, 0, 0, true)
	
	self.vScrollBar = UiSlideBar(true, 0, UiScrollPanel.barWidth)
	self.vScrollBar:bind_event(UiSlideBar.EVT_SLIDE, self, UiScrollPanel.OnVScroll)
	self.vScrollBar:Show(false)
	hLayout:AddChild(self.vScrollBar, nil, 0, 0, true)
	
	self.hScrollBar = UiSlideBar(false, 0, UiScrollPanel.barWidth)
	self.hScrollBar:bind_event(UiSlideBar.EVT_SLIDE, self, UiScrollPanel.OnHScroll)
	self.hScrollBar:Show(false)
	vLayout:AddChild(self.hScrollBar, nil, 0, 0, true, nil, UiScrollPanel.barWidth)
	
	if (widget) then
		self:SetWidget(widget)
	end
end

function UiScrollPanel:SetWidget(widget)
	if (self.widget) then
		self.widget:unbind_event(EVT.SIZE, self, UiScrollPanel.OnWidgetSize)
		self.pane:RemoveChild(widget)
	end
	self.widget = widget
	self.widget:bind_event(EVT.SIZE, self, UiScrollPanel.OnWidgetSize)
	self.pane:AddChild(widget)
end

function UiScrollPanel:OnWidgetSize()
	self.vScrollBar:Show(self.widget.rect.h > self.rect.h)
	self.hScrollBar:Show(self.widget.rect.w > self.rect.w)
	self.vScrollBar:SetScale(self.widget.rect.h, self.pane.rect.h)
	self.hScrollBar:SetScale(self.widget.rect.w, self.pane.rect.w)
end

function UiScrollPanel:OnVScroll(e, pos)
	self.widget:SetPos(self.widget.rect.x, -pos)
end

function UiScrollPanel:OnHScroll(e, pos)
	self.widget:SetPos(-pos, self.widget.rect.y)
end

function UiScrollPanel:OnMouseWheel(e, x, y, n)
	if (n ~= 0) then
		self.vScrollBar:MoveScalePos(-n)
	end
end

-----UiPolyIcon-----
UiPolyIcon = class(UiButtonBase)
UiPolyIcon.cpuClip = false
UiPolyIcon.cached = false
UiPolyIcon.drawClipRect = true
UiPolyIcon.funcSetColors = {}
UiPolyIcon.CAddUByte4 = CAddUByte4

function UiPolyIcon:ctor(iconPoly, stretch, w, h)
	self.poly = iconPoly
	self.colors0 = {}
	self.colors1 = {}
	self.colors2 = {}
	self.colors = self.colors0
	self.scale = stretch
	if (stretch) then
		self.mat3d = CMatrix3D()
		self:SetSize(w, h)
	else
		self:SetSize(math.ceil(iconPoly.w), math.ceil(iconPoly.h))
	end
	self.crColor:set(0, 0, 0, 0)
	
	local SetColors = UiPolyIcon.funcSetColors[iconPoly]
	if (SetColors) then
		self.SetColors = SetColors
		return
	end
	
	SetColors = 'local c, o, aa '
	for k, v in pairs(self.poly.colors) do
		SetColors = SetColors .. 'o = oldColors[' .. k .. '] '
		SetColors = SetColors .. 'c = newColors[' .. k .. '] or o '
		SetColors = SetColors .. 'CAddUByte4(vbColor, wpColor + o.wp, o.nvc, c.r, c.g, c.b, c.a) '
		for k, _ in pairs(v.aa) do
			SetColors = SetColors .. 'aa = o.aa[' .. k .. '] '
			SetColors = SetColors .. 'CAddUByte4(vbColor, wpColor + o.wp + aa[1], aa[2], c.r, c.g, c.b, 0) '
		end
	end
	self.SetColors = load(SetColors, '', 't', UiPolyIcon)
	UiPolyIcon.funcSetColors[iconPoly] = self.SetColors
end

function UiPolyIcon:OnDefault()
	self.colors = self.colors0
end

function UiPolyIcon:OnHoverring()
	self.colors = self.colors1
end

function UiPolyIcon:OnPressing()
	self.colors = self.colors2
end

function UiPolyIcon:SetDefaultColor(idx, r, g, b, a)
	if (self.colors0[idx]) then
		self.colors0[idx]:set(r, g, b, a)
	else
		self.colors0[idx] = Color(r, g, b, a)
	end
end

function UiPolyIcon:SetHoverringColor(idx, r, g, b, a)
	if (self.colors1[idx]) then
		self.colors1[idx]:set(r, g, b, a)
	else
		self.colors1[idx] = Color(r, g, b, a)
	end
end

function UiPolyIcon:SetPressingColor(idx, r, g, b, a)
	if (self.colors2[idx]) then
		self.colors2[idx]:set(r, g, b, a)
	else
		self.colors2[idx] = Color(r, g, b, a)
	end
end

function UiPolyIcon:FillVB(vbPos, wpPos, vbUVW, wpUVW, vbColor, wpColor, ib, iwp, ibStart)
	local n = self.poly.vtx_count
	if (self.scale) then
		if (self.sized) then
			self.mat3d:SetVecX(self.rect.w / self.poly.w, 0, 0, 0)
			self.mat3d:SetVecY(0, self.rect.h / self.poly.h, 0, 0)
		end
		if (self.moved) then
			self.mat3d:SetVecW(self.location.x, self.location.y, 0, 0) 			
		end
		CTransformFloat3(g_innerPolyVB, self.poly.vb_offset, n, self.mat3d, vbPos, wpPos)
	else
		CMoveFloat3(g_innerPolyVB, self.poly.vb_offset, n, self.location.x, self.location.y, 0, vbPos, wpPos)
	end
	CAddFloat3(vbUVW, wpUVW, n, self.font.pixels, 0, 0)
	
	UiPolyIcon.oldColors = self.poly.colors
	UiPolyIcon.newColors = self.colors
	UiPolyIcon.vbColor = vbColor
	UiPolyIcon.wpColor = wpColor
	self.SetColors()
	CCopyIndexBuffer(g_innerPolyIB, self.poly.ib_offset, self.poly.idx_count, ibStart, ib, iwp)
	return n, self.poly.idx_count
end

-----Selector-----
Selector = class(Object)
Selector.EVT_CHANGED = {}
Selector.EVT_KEY = {}

function Selector:ctor()
	self.selected = {}
end
 
function Selector:Add(w, h, data)
	local item = UiButton(w, h)
	item:SetDefaultColor(0, 0, 0, 0)
	item.data = data
	item:bind_event(EVT.FOCUS_IN, self, Selector.OnFocus)
	item:bind_event(EVT.FOCUS_OUT, self, Selector.OnFocus)
	item:bind_event(EVT.MOVE_IN, self, Selector.OnMouse)
	item:bind_event(EVT.MOVE_OUT, self, Selector.OnMouse)
	item:bind_event(EVT.LEFT_DOWN, self, Selector.OnMouse)
	item:bind_event(EVT.RIGHT_UP, self, Selector.OnMouse)
	item:bind_event(EVT.KEY_DOWN, self, Selector.OnKeyDown)
	return item
end

function Selector:Remove(item)
	self:Select(item, false, true)
end

function Selector:OnFocus(e)
	if (e == EVT.FOCUS_IN) then
		self.focused = EVT.obj
		for item in pairs(self.selected) do
			item:SetAllColors(0, 130, 255, 100)
			item:Refresh()
		end
	else
		self.focused = nil
		for item in pairs(self.selected) do
			item:ResetAllColors()
			item:SetDefaultColor(100, 100, 100, 100)
			item:Refresh()
		end
	end
end

function Selector:OnMouse(e, x, y, n)
	local item = EVT.obj
	if (e == EVT.LEFT_DOWN) then
		if (g_actWindow.keyDowns[SYS.VK_CONTROL] and self.SEL_MULTIPLE) then
			if (self.selected[item]) then
				self:Select(item, false, true)
			else
				self:Select(item, true, true)
			end
		else
			self:ClearSelection(item)
			self:Select(item, true, true)
		end
	elseif (e == EVT.RIGHT_UP) then
		if (not self.selected[item]) then
			self:ClearSelection()
		end
		self:Select(item, true, true)
	end
end

function Selector:OnKeyDown(t, k, left, right)
	self:process_event(Selector.EVT_KEY, EVT.KEY_DOWN, k, left, right)
end

function Selector:Select(item, flag, notify)
	local changed
	if (flag) then
		if (self.focused == item) then
			item:SetAllColors(0, 130, 255, 100)
		else
			item:ResetAllColors()
			item:SetDefaultColor(100, 100, 100, 100)
		end
		changed = self.selected[item] ~= item.data
		self.selected[item] = item.data
	else
		item:ResetAllColors()
		item:SetDefaultColor(0, 0, 0, 0)
		changed = self.selected[item] ~= nil
		self.selected[item] = nil
	end
	item:Refresh()
	if (notify and changed) then
		self:process_event(Selector.EVT_CHANGED, e)
	end
	return changed
end

function Selector:GetSelection(item)
	local data
	item, data = next(self.selected, item)
	if (data) then
		return data, self:GetSelection(item)
	end
end

function Selector:ClearSelection(ignore, notify)
	local changed
	for item in pairs(self.selected) do
		if (item ~= ignore) then
			changed = self:Select(item, false, false) or changed
		end
	end
	if (notify and changed) then
		self:process_event(Selector.EVT_CHANGED, e)
	end
end

-----UiTreeList-----
UiTreeList = class(VBoxLayout)
UiTreeList.EVT_MOUSE_NODE = {}
UiTreeList.SEL_MULTIPLE = true

function UiTreeList:ctor()
	self.selector = Selector()

	self:bind_event(EVT.SIZE, self, UiTreeList.OnListSized)
	
	self.nodes = {}
end

function UiTreeList:AddNode(nodeId, icon, text)
	local node = VBoxLayout()
	local superior = self.nodes[nodeId]
	if (superior) then
		superior.list:AddChild(node)
		if (superior.fold) then
			superior.iconFold:Show(true)
			superior.spacer:Show(false)
		end
		node.superior = superior
		node.indent = superior.indent + 17
	else
		self:AddChild(node, nil, 0, 0, false, 0)
		node.indent = 0
	end
	node.fold = true
	node.treeList = self
	self.nodes[node.id] = node
	
	node.item = UiWidget()
	node.item.node = node
	node.item.drawSelf = false
	node.item.cpuClip = false
	node.item.color:set(0, 0, 0, 0)
	node:AddChild(node.item, nil, 0, 0, false, 0)
	
	node.item.highlight = self.selector:Add(0, 0, node.id)
	node.item:AddChild(node.item.highlight)
	
	node.item.box = VBoxLayout()
	node.item:AddChild(node.item.box)
	node.title = HBoxLayout()
	node.item.box:AddChild(node.title, nil, 1, 2, false, node.indent)
	
	node.iconFold = UiPolyIcon(g_iconTriangleR, false)
	node.iconFold.node = node
	node.iconFold:SetDefaultColor(1, 150, 150, 150, 255)
	node.iconFold:SetHoverringColor(1, 200, 200, 200, 255)
	node.iconFold:SetPressingColor(1, 230, 230, 230, 255)
	node.iconFold:Show(false)
	node.iconFold.Expand = UiTreeList.Expand
	node.iconFold:bind_event(EVT.LEFT_UP, node.iconFold, node.iconFold.Expand)
	node.title:AddChild(node.iconFold, nil, 0, 2, false)
	
	node.iconExpand = UiPolyIcon(g_iconTriangleDR, false)
	node.iconExpand.node = node
	node.iconExpand:SetDefaultColor(1, 150, 150, 150, 255)
	node.iconExpand:SetHoverringColor(1, 200, 200, 200, 255)
	node.iconExpand:SetPressingColor(1, 230, 230, 230, 255)
	node.iconExpand:Show(false)
	node.iconExpand.Fold = UiTreeList.Fold
	node.iconExpand:bind_event(EVT.LEFT_UP, node.iconExpand, node.iconExpand.Fold)
	node.title:AddChild(node.iconExpand, nil, 0, 2, false)
	
	node.spacer = HBoxLayout()
	node.spacer:Show(true)
	node.title:AddChild(node.spacer, nil, 14, 0, false)
	
	node.icon = UiPolyIcon(icon, true, 20, 14)
	node.icon.writeId = false
	node.title:AddChild(node.icon, nil, 7, 0, false, nil, 4)
	
	node.text = UiText(text)
	node.text.writeId = false
	node.title:AddChild(node.text, nil, 7, 0, false)
	
	node.item.box:SetSize()
	node.item:SetSize(node.indent + node.title.rect.w, node.item.box.rect.h)
	
	node.list = VBoxLayout()
	node.list:Show(false)
	node:AddChild(node.list, nil, 0, 0, false, 0)
	
	return node.id
end

function UiTreeList:RemoveNode(id)
	local node = self.nodes[id]
	if (node) then
		if (node.superior) then
			local superior = node.superior
			superior.list:RemoveChild(node)
			if (#superior.list.children == 0) then
				superior.iconFold:Show(false)
				superior.iconExpand:Show(false)
				superior.spacer:Show(true)
			end
		else
			self:RemoveChild(node)
		end
		self.selector:Remove(node.item.highlight)
		self.nodes[id] = nil
	end
end

function UiTreeList:OnListSized(e, w, h, list)
	list = list or self
	for node in list.children:pairs() do
		node.item.highlight:SetSize(self.rect.w, node.item.box.rect.h)
		UiTreeList.OnListSized(self, e, w, h, node.list)
	end
end

function UiTreeList.Expand(icon)
	icon.node.iconFold:Show(false)
	icon.node.iconExpand:Show(true)
	icon.node.list:Show(true)
end

function UiTreeList.Fold(icon)
	icon.node.iconExpand:Show(false)
	icon.node.iconFold:Show(true)
	icon.node.list:Show(false)
end

-----UiCombo-----
UiCombo = class(UiButton)
UiCombo.width = 150
UiCombo.height = 28
UiCombo.maxBoxWidth = 500
UiCombo.maxBoxHeight = 2000

function UiCombo:ctor(w, h)
	w = w or UiCombo.width
	h = h or UiCombo.height
	self:SetSize(w, h)
	
	self:EnableActive(true)
	self:bind_event(EVT.INACTIVE, self, UiCombo.OnInactive)

	self.selector = Selector()
	self:bind_event(EVT.LEFT_UP, self, UiCombo.OnLeftUp)
	self.selector:bind_event(Selector.EVT_CHANGED, self, UiCombo.OnSelected)
	
	self.layout:AddChild(self.text, 1, 5, 5, false)
	local iconDown = UiPolyIcon(g_iconTriangleD)
	iconDown:SetDefaultColor(200, 200, 200, 255)
	iconDown.writeId = false
	self.layout:AddChild(iconDown, nil, nil, 5, false)

	self.box = UiScrollPanel()
	self.box.active = self
	self.box:ShowOutline(true, Color(150, 150, 150, 255))
	
	self.list = VBoxLayout()
	self.list:bind_event(EVT.SIZE, self, UiCombo.OnListSized)
	self.box:SetWidget(self.list)
	
	if (UiCombo.maxBoxWidth < w) then
		self.maxBoxWidth = w
	end
end

function UiCombo:SetDefault(index)
	local item = self.list.children[index]
	if (item) then
		self.text:SetText(item.text.text)
		self.selector:Select(item.highlight, true, true)
	end
end

function UiCombo:OnLeftUp()
	if (not self.showBox and #self.list.children > 0 and self.window) then
		self.window:AddChild(self.box)
		self.list:SetSize()
		self.box:SetSize(math.min(self.list.rect.w, self.maxBoxWidth) + 2, math.min(self.list.rect.h, self.maxBoxHeight) + 2)
		self.box:SetPos(self.location.x, self.location.y + self.rect.h)
		self:SetActive(true)
		self.showBox = true
	else
		self:OnInactive()
	end
end

function UiCombo:OnInactive()
	if (self.showBox and self.window) then
		self.window:RemoveChild(self.box)
		self.showBox = false
	end
end

function UiCombo:OnSelected()
	self:SetDefault(self.selector:GetSelection())
	self:OnInactive()
end

function UiCombo:AddItem(text)
	local item = UiWidget()
	--if (index) then
		--index = math.min(math.max(1, index), #self.list)
	--end
	self.list:AddChild(item, nil, 0, 0, false, 0)
	
	item.drawSelf = false
	item.cpuClip = false
	item.color:set(0, 0, 0, 0)
	
	local n = #self.list.children
	item.highlight = self.selector:Add(0, 0, n)
	item:AddChild(item.highlight)
	
	item.layout = HBoxLayout()
	item:AddChild(item.layout)
	
	item.text = UiText(text)
	item.text.writeId = false
	item.layout:AddChild(item.text, nil, 5, 5, false, 1, 2)
	
	item.layout:SetSize()
	item:SetSize(math.max(item.layout.rect.w, self.rect.w), item.layout.rect.h)
	
	return n
end

function UiCombo:OnListSized(e, w, h)
	local list = self.list
	for item in list.children:pairs() do
		item.highlight:SetSize(list.rect.w, item.layout.rect.h)
	end
end

-----UiLine-----
UiLine = class(UiWidget)

function UiLine:FillVB(vbPos, wpPos, vbUVW, wpUVW, vbColor, wpColor, ib, iwp, ibStart)
	CAddFloat3(vbPos, wpPos, 1, self.location.x, self.location.y, 0)
	CAddFloat3(vbPos, APPEND, 1, self.location.x + 100, self.location.y + 100, 0)
	CAddFloat3(vbPos, APPEND, 1, self.location.x + 200, self.location.y - 50, 0)
	
	local n = CAddLine2D(vbPos, wpPos, 3, false, 10, false, false, vbPos, wpPos, ib, iwp, ibStart)
	
	n = n + CAddLine2D(vbPos, wpPos, 6, true, 10, true, false, vbPos, wpPos, ib, APPEND, ibStart)
	
	CAddFloat3(vbUVW, wpUVW, 12, self.font.pixels, 0, 0)
	CAddUByte4(vbColor, wpColor, 6, self.color.r, self.color.g, self.color.b, self.color.a)
	CAddUByte4(vbColor, APPEND, 6, self.color.r, self.color.g, self.color.b, 0)
	
	return 12, n
	
	-- CAddRectFloat3(vbPos, wpPos, self.location.x, self.location.y, self.rect.w, self.rect.h, Z_2D)
	-- CAddFloat3(vbUVW, wpUVW, 8, self.font.pixels, 0, 0)
	-- CAddUByte4(vbColor, wpColor, 8, self.color.r, self.color.g, self.color.b, self.color.a)
	-- return 8, CAddLine2D(vbPos, wpPos, 4, true, 2, 1, vbPos, wpPos, ib, iwp, ibStart)
end

-----SceneObject-----
SceneObject = class(Object)

function SceneObject:ctor()
	self.mRoot = CMatrix3D()
end

-----Camera-----
Camera = class(SceneObject)

function Camera:ctor()
	self.mProj = CMatrix3D()
end

-----SceneWidget-----
SceneWidget = class(Widget2D)

function SceneWidget:ctor()
	self.vpNew = Rect()
	self.crNew = Rect()
	self.dcLists = {}
end

function SceneWidget:DoUpdate(crCpu, crGpu)
	local crGpuNew = self.crNew
	crGpuNew:set(self.location.x, self.location.y, self.rect.w, self.rect.h)
	local cr = crGpu or crCpu
	if (cr and not self.crNew:intersect(cr, self.crNew)) then
		return false
	end
	if (not DrawcallList.cr or DrawcallList.cr:diff(crGpuNew)) then
		DrawcallList.cr = crGpuNew
	end
	local vpNew = self.vpNew
	vpNew:set(self.location.x, self.location.y, self.rect.w, self.rect.h)
	if (not DrawcallList.vp or DrawcallList.vp:diff(vpNew)) then
		DrawcallList.vp = vpNew
	end

	self:Render()
	for spId, merged in pairs(self.dcLists) do
		local dcList = g_dcLists[spId]
		if (dcList) then
			for order, list in pairs(merged) do
				list:CommitCurrent()
				dcList:AddSubList(list)
				merged[order] = nil
			end
		end
	end
	
	return true, crCpu, crGpuNew
end

function SceneWidget:GetDrawcall(spId, mergeType, order)
	local o = self.dcLists[spId]
	if (not o) then
		o = {}
		self.dcLists[spId] = o
	end
	local s = o[mergeType]
	if (not s) then
		s = {}
		o[mergeType] = s
	end
	local list = s[order]
	if (not list) then
		list = g_fp:NewSubList()
		s[order] = list
	end
	if (not list.reset) then
		list:Reset(g_input)
	end
	return list
end

function SceneWidget:Render()
end

-----Scene3D-----
Scene3D = class(SceneWidget)

function Scene3D:ctor(camera)
	self.camera = camera or Object3D()
	self.mProj = CMatrix3D()
end

function Scene3D:SceneObjects()
	return g_sceneObjects()
end

function Scene3D:Render()
	cameraRes = self.camera
	local objects = self:SceneObjects()
	for obj in objects:pairs(self.Filter) do
		
	end
end
