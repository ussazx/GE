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
	if (g_actWindow) then
		g_actWindow:OnWidgetShow(self, show)
	end
end	

function Widget2D:AddChild(widget, ...)
	if (widget.parent) then
		if (widget.parent == self) then
			return
		end
		widget.parent:RemoveChild(widget)
	end
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
		widget.parent = nil
		widget.window = nil
		self.children:remove_idx(widget.w_idx)
		if (g_actWindow) then
			g_actWindow:OnWidgetShow(self, show)
		end
		if (self.OnRemoveChild) then
			self:OnRemoveChild(widget)
		end
	end
end

function Widget2D:Show(show)
	if (self.show ~= show) then
		if (g_actWindow) then
			g_actWindow:OnWidgetShow(self, show)
		elseif(self.window) then
			self.window.update = true
		end
		if (self.inLayout) then
			self.inLayout:SetUpdate()
		end
		self.show = show
	end
end

function Widget2D:Update(...)
	self.location.x = self.rect.x
	self.location.y = self.rect.y
	if (self.parent) then
		self.window = self.parent.window
		self.location:move(self.parent.location.x, self.parent.location.y)
		if (self.moved == false) then
			self.moved = self.parent.moved
		end
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
		self.inLayout:SetUpdate()
	end
	
	if (self.OnSized) then
		self:OnSized()
	end
end

--Layout--
Layout = class(Widget2D)
Layout.ALIGN_LEFT = 1
Layout.ALIGN_RIGHT = 2
Layout.ALIGN_TOP = 4
Layout.ALIGN_BOTTOM = 8

function Layout:ctor()
	self.props = setmetatable({}, {__mode = 'k'})
	self.update = true
	self.sized = false
end

function Layout:SetSize(w, h)
	w = w or self.rect.w
	h = h or self.rect.h
	if (w ~= self.rect.w or h ~= self.rect.h or self.update) then
		local w0, h0 = self.rect.w, self.rect.h
		self:Layout(w, h)
		self.update = false
		self.sized = false
		self:process_event(EVT.SIZE, w0, h0)
	end
end

function Layout:GetProp(w)
	return self.props[w]
end

function Layout:OnAddChild(w, ...)
	w.inLayout = self
	self:SetUpdate()
	local o = {}
	self.props[w] = o
	self.InitProp(o, ...)
	return o
end

function Layout:SetUpdate()
	if (not self.update) then
		self.update = true
		if (self.inLayout) then
			self.inLayout:SetUpdate()
		end
	end
end

function Layout:OnRemoveChild(w)
	self.props[w] = nil
	self:SetUpdate()
end

function Layout:DoUpdate(...)
	if (self.parent.sized) then
		self:SetSize(self.parent.rect.w, self.parent.rect.h)
	elseif(self.update) then
		self:SetSize()
	end
	return true, ...
end

local function InitBoxLayoutProp(o, ratio, align, gapLeft, gapRight, gapTop, gapBottom)
	o.ratio = ratio or 0
	o.align = align or 0
	o.gapLeft = gapLeft or 0
	o.gapRight = gapRight or 0
	o.gapTop = gapTop or 0
	o.gapBottom = gapBottom or 0
end

local function InitGridLayoutProp(o, gapLeft, gapRight, gapTop, gapBottom)
	o.gapLeft = gapLeft or 0
	o.gapRight = gapRight or 0
	o.gapTop = gapTop or 0
	o.gapBottom = gapBottom or 0
end

local function CaculateLayoutProp(prop)
	prop.left = prop.gapLeft * (prop.align & Layout.ALIGN_LEFT) // Layout.ALIGN_LEFT
	prop.top = prop.gapTop * (prop.align & Layout.ALIGN_TOP) // Layout.ALIGN_TOP
	prop.right = prop.gapRight * (prop.align & Layout.ALIGN_RIGHT) // Layout.ALIGN_RIGHT
	prop.bottom = prop.gapBottom * (prop.align & Layout.ALIGN_BOTTOM) // Layout.ALIGN_BOTTOM
	prop.h_expand = prop.align & (Layout.ALIGN_LEFT|Layout.ALIGN_RIGHT) == Layout.ALIGN_LEFT|Layout.ALIGN_RIGHT
	prop.v_expand = prop.align & (Layout.ALIGN_TOP|Layout.ALIGN_BOTTOM) == Layout.ALIGN_TOP|Layout.ALIGN_BOTTOM
end

local function VerticalLayout(box, w, h)
	local ww = w 
	local y = 0
	local total = 0
	local expands = {}
	for v in box:ChildrenPairs() do
		local prop = box.props[v]
		CaculateLayoutProp(prop)
		
		if (prop.h_expand) then
			if (prop.ratio < 1) then
				table.insert(expands, v)
			else
				total = total + prop.ratio
			end
		else
			v:SetSize()
			ww = math.max(ww, prop.left + v.rect.w + prop.right)
			if (prop.ratio < 1) then
				y = y + v.rect.h
			else
				total = total + prop.ratio
			end
		end
		y = y + prop.top + prop.bottom
	end
	for _, v in pairs(expands) do
		local prop = box.props[v]
		v:SetSize(math.max(0, ww - prop.left - prop.right), nil)
		y = y + v.rect.h
	end
	local n = math.max(0, h - y)
	total = math.max(1, total)
	
	h = 0
	for v in box:ChildrenPairs() do
		local prop = box.props[v]
		
		local rh = math.floor(prop.ratio / total * n)
		if (prop.ratio > 0 and (prop.h_expand or prop.v_expand)) then
			local rw
			if (prop.h_expand) then
				rw = math.max(0, ww - prop.left - prop.right)
			end
			if (prop.v_expand) then
				v:SetSize(rw, rh)
			else
				v:SetSize(rw, nil)
			end
		end
		
		local x = 0
		if (prop.align & Layout.ALIGN_LEFT ~= 0) then
			x = prop.left
		elseif (prop.align & Layout.ALIGN_RIGHT ~= 0) then
			x = ww - v.rect.w - prop.right
		else
			x = (ww - v.rect.w) // 2
		end
		
		local d = math.max(0, rh - v.rect.h)
		if (prop.align & Layout.ALIGN_TOP ~= 0) then
			h = h + prop.top
		elseif (prop.align & Layout.ALIGN_BOTTOM ~= 0) then
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
	box.rect.w = ww
	box.rect.h = h
end

function HorizontalLayout(box, w, h)
	local hh = h
	local x = 0
	local total = 0
	local expands = {}
	for v in box:ChildrenPairs() do
		local prop = box.props[v]
		CaculateLayoutProp(prop)
		
		if (prop.v_expand) then
			if (prop.ratio < 1) then
				table.insert(expands, v)
			else
				total = total + prop.ratio
			end
		else
			v:SetSize()
			hh = math.max(hh, prop.top + v.rect.h + prop.bottom)
			if (prop.ratio < 1) then
				x = x + v.rect.w
			else
				total = total + prop.ratio
			end
		end
		x = x + prop.left + prop.right
	end
	for _, v in pairs(expands) do
		local prop = box.props[v]
		v:SetSize(nil, math.max(0, hh - prop.top - prop.bottom))
		x = x + v.rect.w
	end
	local n = math.max(0, w - x)
	total = math.max(1, total)
	
	w = 0
	for v in box:ChildrenPairs() do
		local prop = box.props[v]
		
		local rw = math.floor(prop.ratio / total * n)
		if (prop.ratio > 0 and (prop.h_expand or prop.v_expand)) then
			local rh
			if (prop.v_expand) then
				rh = math.max(0, hh - prop.top - prop.bottom)
			end
			if (prop.h_expand) then
				v:SetSize(rw, rh)
			else
				v:SetSize(nil, rh)
			end
		end
		
		local y = 0
		if (prop.align & Layout.ALIGN_TOP ~= 0) then
			y = prop.top
		elseif (prop.align & Layout.ALIGN_BOTTOM ~= 0) then
			y = hh - v.rect.h - prop.bottom
		else
			y = (hh - v.rect.h) // 2
		end
		
		local d = math.max(0, rw - v.rect.w)
		if (prop.align & Layout.ALIGN_LEFT ~= 0) then
			w = w + prop.left
		elseif (prop.align & Layout.ALIGN_RIGHT ~= 0) then
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
	box.rect.w = w
	box.rect.h = hh
end

-----BoxLayout-----
BoxLayout = class(Layout)
BoxLayout.InitProp = InitBoxLayoutProp

function BoxLayout:ctor(vertical)
	if (vertical) then
		self.Layout = VerticalLayout
	else
		self.Layout = HorizontalLayout
	end
end

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

function UiWidget:ctor(w, h)
	self.rect:set(0, 0, w, h)
	self.crColor = Color()
	self.color = Color(255, 255, 255, 255)
	self.renderDisables = {[SubpassId(g_rp0, 1)] = true}
	
	self.mesh = Mesh(self.FillVB, self.FillIB, self, self.cached, 1|2|4)
	self.mesh:SetMaterial(g_mtlUi, {0, 1}, {self.id, 1})
	
	self.rcMesh = Mesh(self.FillClipRectVB, self.FillClipRectIB, self, false, 1|2|4)
	self.rcMesh:SetMaterial(g_mtlUi, {0, 1}, {self.id, 1})
end

function UiWidget:Refresh()
	self.mesh.update = true
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

function UiWidget:FillRectVB(color, vbPos, wpPos, vbUVW, wpUVW, vbColor, wpColor)
	if (self.cr) then
		CAddRectFloat3(vbPos, wpPos, self.cr.x, self.cr.y, self.cr.w, self.cr.h, Z_2D)
	else
		CAddRectFloat3(vbPos, wpPos, self.location.x, self.location.y, self.rect.w, self.rect.h, Z_2D)
	end
	CAddFloat3(vbUVW, wpUVW, 4, self.font.pixels, 0, 0)
	CAddUByte4(vbColor, wpColor, 4, color.r, color.g, color.b, color.a)
	return 4
end

function UiWidget:FillVB(vbPos, wpPos, vbUVW, wpUVW, vbColor, wpColor)
	return self:FillRectVB(self.color, vbPos, wpPos, vbUVW, wpUVW, vbColor, wpColor)
end

function UiWidget:FillIB(ib, ib_start, wp)
	return CAddConvexPolyIndex(ib, wp, 1, ib_start, 4)
end

function UiWidget:FillClipRectVB(vbPos, wpPos, vbUVW, wpUVW, vbColor, wpColor)
	return self:FillRectVB(self.crColor, vbPos, wpPos, vbUVW, wpUVW, vbColor, wpColor)
end

function UiWidget:FillClipRectIB(ib, ib_start, wp)
	return CAddConvexPolyIndex(ib, wp, 1, ib_start, 4)
end

function UiWidget:DoUpdate(crCpu, crGpu)
	local crCpuNew
	local crGpuNew
	if (self.cpuClip or self.gpuClip) then
		local crNew = Rect(self.location.x, self.location.y, self.rect.w, self.rect.h)
		if (self.cpuClip) then
			if (crCpu) then
				crCpuNew = crNew:intersect(crCpu)
				if (crCpuNew == nil) then
					return false
				end
			else
				crCpuNew = crNew
			end
		end
		if (self.gpuClip) then
			if (crCpuNew and (crCpu == crGpu or crGpu == nil)) then
				crGpuNew = crCpuNew
			elseif (crGpu or crCpu) then
				crGpuNew = crNew:intersect(crGpu or crCpu)
				if (crGpuNew == nil) then
					return false
				end
			else
				crGpuNew = crNew
			end
		end
	end
	crCpuNew = crCpuNew or crCpu
	crGpuNew = crGpuNew or crGpu

	if (crGpuNew) then
		DrawcallList.cr = crGpuNew
	end
	
	local changed = self.mesh.doCache and (self.mesh.update or (self.moved or self.sized or
	((self.cr or crCpuNew) and ((self.cr == nil and crCpuNew) or (self.cr and crCpuNew == nil) or
	(self.cr.x ~= crCpuNew.x or self.cr.y ~= crCpuNew.y or self.cr.w ~= crCpuNew.w or self.cr.h ~= crCpuNew.h)))))
	
	if (self.cpuClip) then
		self.cr = crCpuNew
	else
		self.cr = nil
	end
	
	local d
	if (not self.writeId) then
		d = self.renderDisables
	end
	
	if (self.drawClipRect) then
		self.rcMesh:Render(d)
	end
	self.mesh.update = changed
	self.mesh:Render(d)
	
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
		self.hovering = true
	elseif (e == EVT.MOVE_OUT) then
		self.down = false
		self.hovering = false
	end
	
	if (self.down) then
		self:OnPressing()
		
	elseif (self.hovering) then
		self:OnHovering()
	else
		self:OnDefault()
	end
	
	self:Refresh()
end

function UiButtonBase:OnDefault()
end

function UiButtonBase:OnHovering()
end

function UiButtonBase:OnPressing()
end

-----UiButton-----
UiButton = class(UiButtonBase)

function UiButton:ctor(w, h, s, font)
	self.rect:set(0, 0, w, h)
	self.color0 = Color(70, 70, 70, 255)
	self.color1 = Color(100, 100, 100, 255)
	self.color2 = Color(150, 150, 150, 255)
	self.color = self.color0
	self.layout = BoxLayout()
	self:AddChild(self.layout)
	self.text = UiText(s, font)
	self.text.writeId = false
	self.layout:AddChild(self.text, 1)
end

function UiButton:OnDefault()
	self.color = self.color0
end

function UiButton:OnHovering()
	self.color = self.color1
end

function UiButton:OnPressing()
	self.color = self.color2
end

function UiButton:SetDefaultColor(r, g, b, a)
	self.color0:set(r, g, b, a)
end

function UiButton:SetHoveringColor(r, g, b, a)
	self.color1:set(r, g, b, a)
end

function UiButton:SetPressingColor(r, g, b, a)
	self.color2:set(r, g, b, a)
end

-----Text-----
UiText = class(UiWidget)
UiText.cached = true
UiText.cpuClip = false

function UiText:ctor(s, font)
	self.text = LString('')
	self:SetText(s, font)
end

function UiText:SetText(s, font)
	s = s or ''
	font = font or uiFont
	self:Show(s ~= '')
	if (self.text ~= s or self.font ~= font) then
		self.text:set(s)
		self.font = font
		self:SetSize(CMeasureText(s, -1, -1, font), font.fontSize)
		self:Refresh()
	end
end

function UiText:FillVB(vbPos, wpPos, vbUVW, wpUVW, vbColor, wpColor)
	local n
	if (self.cr) then
		n = CAddTextClip(vbPos, wpPos, vbUVW, wpUVW, self.font,
		self.location.x - self.cr.x, self.location.y - self.cr.y + self.font.fontSize + self.font.descender,
			self.cr.x, self.cr.y, self.cr.w, self.cr.h, Z_2D, self.text)
	else
		n = CAddText(vbPos, wpPos, vbUVW, wpUVW, self.font, self.location.x, self.location.y + self.rect.h + self.font.descender, Z_2D, self.text)
	end
	
	CAddUByte4(vbColor, wpColor, 4 * n, self.color.r, self.color.g, self.color.b, self.color.a)
	self.nText = n
	return 4 * n
end

function UiText:FillIB(ib, ib_start, wp)
	return CAddConvexPolyIndex(ib, wp, self.nText, ib_start, 4)
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
	
	self.crColor:set(100, 100, 100, 100)
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

function UiTextInput:FillVB(vbPos, wpPos, vbUVW, wpUVW, vbColor, wpColor)
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
	self.nText = n
	return 4 * n
end

function UiTextInput:FillIB(ib, ib_start, wp)
	return CAddConvexPolyIndex(ib, wp, self.nText, ib_start, 4)
end

UiSlideBar = class(UiWidget)

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
	self.slider:SetHoveringColor(200, 200, 200, 255)
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
		self:process_event(EVT.SLIDE_BAR, self.scale * d // self.length, self.scale)
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

function UiScrollPanel:ctor(w, h)
	self:SetSize(w, h)
	self.color:set(0 ,0, 0, 0)
	self:bind_event(EVT.MOUSEWHEEL, self, UiScrollPanel.OnMouseWheel)
	
	local vLayout = BoxLayout(true)
	self:AddChild(vLayout)
	
	local hLayout = BoxLayout(false)
	vLayout:AddChild(hLayout, 1, Layout.ALIGN_LEFT|Layout.ALIGN_RIGHT|Layout.ALIGN_TOP|Layout.ALIGN_BOTTOM)
	
	self.plate = UiWidget()
	self.plate.color:set(0, 0, 0, 0)
	self.plate.gpuClip = true
	hLayout:AddChild(self.plate, 1, Layout.ALIGN_LEFT|Layout.ALIGN_RIGHT|Layout.ALIGN_TOP|Layout.ALIGN_BOTTOM)
	
	self.vScrollBar = UiSlideBar(true, 0, UiScrollPanel.barWidth)
	self.vScrollBar:bind_event(EVT.SLIDE_BAR, self, UiScrollPanel.OnVScroll)
	self.vScrollBar:Show(false)
	hLayout:AddChild(self.vScrollBar, 0, Layout.ALIGN_RIGHT|Layout.ALIGN_TOP|Layout.ALIGN_BOTTOM, 0, 0, 0, 0)
	
	self.hScrollBar = UiSlideBar(false, 0, UiScrollPanel.barWidth)
	self.hScrollBar:bind_event(EVT.SLIDE_BAR, self, UiScrollPanel.OnHScroll)
	self.hScrollBar:Show(false)
	vLayout:AddChild(self.hScrollBar, 0, Layout.ALIGN_LEFT|Layout.ALIGN_RIGHT|Layout.ALIGN_BOTTOM, 0, UiScrollPanel.barWidth, 0, 0)
	
	self:SetSize()
end

function UiScrollPanel:SetWidget(widget)
	if (self.widget) then
		self.widget:unbind_event(EVT.SIZE, self, UiScrollPanel.OnWidgetSize)
		self.plate:RemoveChild(widget)
	end
	self.widget = widget
	self.widget:bind_event(EVT.SIZE, self, UiScrollPanel.OnWidgetSize)
	self.plate:AddChild(widget)
end

function UiScrollPanel:OnWidgetSize()
	self.vScrollBar:Show(self.widget.rect.h > self.plate.rect.h)
	self.hScrollBar:Show(self.widget.rect.w > self.plate.rect.w)
	self.vScrollBar:SetScale(self.widget.rect.h, self.plate.rect.h)
	self.hScrollBar:SetScale(self.widget.rect.w, self.plate.rect.w)
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
		self:SetSize(iconPoly.w, iconPoly.h)
	end
	self.crColor:set(0, 0, 0, 0)
end

function UiPolyIcon:OnDefault()
	self.colors = self.colors0
end

function UiPolyIcon:OnHovering()
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

function UiPolyIcon:SetHoveringColor(idx, r, g, b, a)
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

function UiPolyIcon:FillVB(vbPos, wpPos, vbUVW, wpUVW, vbColor, wpColor)
	local n = self.poly.vtx_count
	if (self.scale) then
		if (self.sized) then
			self.mat3d:SetD0(self.rect.w / self.poly.w, 0, 0)
			self.mat3d:SetD1(0, self.rect.h / self.poly.h, 0)
		end
		if (self.moved) then
			self.mat3d:SetD3(self.location.x, self.location.y, 0) 			
		end
		CTransformFloat3(g_innerPolyVB, self.poly.vb_offset, n, self.mat3d, vbPos, wpPos)
	else
		CMoveFloat3(g_innerPolyVB, self.poly.vb_offset, n, self.location.x, self.location.y, 0, vbPos, wpPos)
	end
	CAddFloat3(vbUVW, wpUVW, n, self.font.pixels, 0, 0)
	for k, v in pairs(self.poly.colors) do
		if (k > 1) then
			wpColor = APPEND
		end
		local c = self.colors[k] or v[2]
		CAddUByte4(vbColor, wpColor, v[1], c.r, c.g, c.b, c.a)
	end
	return n
end

function UiPolyIcon:FillIB(ib, ib_start, wp)
	CCopyIndexBuffer(g_innerPolyIB, self.poly.ib_offset, self.poly.idx_count, ib_start, ib, wp)
	return self.poly.idx_count
end

-----UiTreeList-----
UiTreeList = class(UiScrollPanel)

function UiTreeList:ctor(w, h)
	self:SetSize(w, h)
	
	self.vScrollBar:bind_event(EVT.SLIDE_BAR, self, UiTreeList.OnVScroll)
	
	self.highLight = UiWidget()
	self.highLight.writeId = false
	self.highLight.color:set(0, 130, 255, 100)
	self.highLight:Show(false)
	self.plate:AddChild(self.highLight)
	self.plate:bind_event(EVT.LEFT_DOWN, self, UiTreeList.OnMouse)
	
	self.list = BoxLayout(true)
	self:SetWidget(self.list)
	
	self.nodes = {}
end

function UiTreeList:OnVScroll(e, pos)
	if (self.selected) then
		self.highLight:Move(0, -pos - self.hpos)
		self.hpos = -pos
	end
end

function UiTreeList.Pick(list, y)
	for node in list:ChildrenPairs() do
		local ny = node.title.location.y
		if (y >= ny and y <= ny + node.title.rect.h) then
			return node
		end
		if (node.list.show) then
			node = UiTreeList.Pick(node.list, y)
			if (node) then
				return node
			end
		end
	end
end

function UiTreeList:OnMouse(e, x, y, n)
	local node = self.Pick(self.list, y)
	if (node == nil) then
		return
	end
	if (e == EVT.LEFT_DOWN) then
		self.hpos = self.list.rect.y
		self.highLight:SetPos(0, node.title.location.y - self.plate.location.y)
		self.highLight:SetSize(self.plate.rect.w, node.title.rect.h)
		self.highLight:Show(true)
		self.selected = node
	end
end

function UiTreeList:AddNode(nodeId, icon, text)
	local node = BoxLayout(true)
	node.fold = true
	local superior = self.nodes[nodeId]
	if (superior) then
		superior.list:AddChild(node)
		if (superior.fold) then
			superior.iconFold:Show(true)
			superior.spacer:Show(false)
		end
	else
		self.list:AddChild(node, 0, Layout.ALIGN_LEFT)
	end
	self.nodes[node.id] = node
	
	node.title = BoxLayout(false)
	node:AddChild(node.title, 0, Layout.ALIGN_LEFT|Layout.ALIGN_TOP|Layout.ALIGN_BOTTOM, 0, 0, 1, 1)
	
	node.iconFold = UiPolyIcon(g_iconFold, true, 12, 12)
	node.iconFold.node = node
	node.iconFold:SetDefaultColor(1, 150, 150, 150, 255)
	node.iconFold:SetHoveringColor(1, 200, 200, 200, 255)
	node.iconFold:SetPressingColor(1, 230, 230, 230, 255)
	node.iconFold:Show(false)
	node.iconFold.Expand = UiTreeList.Expand
	node.iconFold:bind_event(EVT.LEFT_UP, node.iconFold, node.iconFold.Expand)
	node.title:AddChild(node.iconFold, 0, Layout.ALIGN_LEFT, 2, 0, 0, 0)
	
	node.iconExpand = UiPolyIcon(g_iconExpand, true, 12, 12)
	node.iconExpand.node = node
	node.iconExpand:SetDefaultColor(1, 150, 150, 150, 255)
	node.iconExpand:SetHoveringColor(1, 200, 200, 200, 255)
	node.iconExpand:SetPressingColor(1, 230, 230, 230, 255)
	node.iconExpand:Show(false)
	node.iconExpand.Fold = UiTreeList.Fold
	node.iconExpand:bind_event(EVT.LEFT_UP, node.iconExpand, node.iconExpand.Fold)
	node.title:AddChild(node.iconExpand, 0, Layout.ALIGN_LEFT, 2, 0, 0, 0)
	
	node.spacer = BoxLayout()
	node.spacer:Show(true)
	node.title:AddChild(node.spacer, 0, Layout.ALIGN_LEFT, 12, 0, 0, 0)
	
	node.icon = UiPolyIcon(icon, true, 20, 14)
	node.icon.writeId = false
	node.title:AddChild(node.icon, 0, Layout.ALIGN_LEFT|Layout.ALIGN_BOTTOM, 7, 0, 0, 4)
	
	node.text = UiText(text)
	node.text.writeId = false
	node.title:AddChild(node.text, 0, Layout.ALIGN_LEFT, 7, 0, 0, 0)
	
	node.list = BoxLayout(true)
	node.list:Show(false)
	node:AddChild(node.list, 0, Layout.ALIGN_LEFT|Layout.ALIGN_TOP|Layout.ALIGN_BOTTOM, 17, 0, 1, 1)
	
	return node.id
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
	
	
	
	
	
	
	
	