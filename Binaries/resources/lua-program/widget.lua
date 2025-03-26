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
	if (self.OnAddChild) then
		return self:OnAddChild(widget, ...)
	end
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
			self.inLayout.update = true
		end
		self.show = show
	end
end

function Widget2D:Update(...)
	if (self.show) then
		self.abortUpdate = false
		self.location.x = self.rect.x
		self.location.y = self.rect.y
		if (self.parent) then
			self.window = self.parent.window
			self.location:move(self.parent.location.x, self.parent.location.y)
			if (self.moved == false) then
				self.moved = self.parent.moved
			end
		end
		self:UpdateChildren(self:DoUpdate(...))
		if (self.abortUpdate) then
			return
		end
		self.moved = false
		self.sized = false
	else
		self:SetWindow()
	end
end

function Widget2D:SetWindow()
	if (self.parent and self.window ~= self.parent.window) then
		self.window = self.parent.window
		for w in self.children:pairs() do
			w:SetWindow()
		end
	end
end

function Widget2D:DoUpdate(...)
	return ...
end

function Widget2D:UpdateChildren(...)
	if (self.abortUpdate) then
		return
	end
	for v in self:ChildrenPairs() do
		v:Update(...)
	end
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
		self.moved = true
		if (self.OnMoved) then
			self:OnMoved()
		end
	end
end

function Widget2D:Move(x, y)
	if (self.inLayout and self.inLayout.update == false) then
		return
	end
	self:DoMove(x, y)
	if (self.window) then
		self.window.update = true
	end
end

function Widget2D:DoMove(x, y)
	x = x or 0
	y = y or 0
	if (x ~= 0 or y ~= 0) then
		self.rect.x = self.rect.x + x
		self.rect.y = self.rect.y + y
		self.moved = true
		if (self.OnMoved) then
			self:OnMoved()
		end
	end
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
	self.sized = true
	
	self:process_event(EVT.SIZE, w0, h0)
	
	if (self.inLayout) then
		self.inLayout.update = true
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
	self.update = true
	local o = {}
	self.props[w] = o
	self.InitProp(o, ...)
	return o
end

function Layout:OnRemoveChild(w)
	self.props[w] = nil
	self.update = true
end

function Layout:DoUpdate(...)
	if (self.parent.sized) then
		self:SetSize(self.parent.rect.w, self.parent.rect.h)
	elseif(self.update) then
		self:SetSize()
	end
	return ...
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
		
		local rh = prop.ratio // total * n
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
		
		local rw = prop.ratio // total * n
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
UiWidget.doClip = true
UiWidget.bakeCount = 4
UiWidget.writeId = true
UiWidget.drawClipRect = false
UiWidget.acceptFocus = true

function UiWidget:ctor(x, y, w, h)
	self.rect:set(x, y, w, h)
	self.crColor = Color()
	self.color = Color(255, 255, 255, 255)
	self.show = true
	self.gpuClip = false
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
		CAddRectFloat2(vbPos, wpPos, self.cr.x, self.cr.y, self.cr.w, self.cr.h)
	else
		CAddRectFloat2(vbPos, wpPos, self.location.x, self.location.y, self.rect.w, self.rect.h)
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

function UiWidget:DoUpdate(gpuClip, cr)
	local crNew
	local crCpu
	if (self.doClip) then
		crNew = Rect(self.location.x, self.location.y, self.rect.w, self.rect.h)
		if (cr ~= nil and ((gpuClip and self.gpuClip) or not gpuClip)) then
			crNew = crNew:intersect(cr)
			if (crNew == nil) then
				self.abortUpdate = true
				return
			end
		end
		if (self.gpuClip) then
			DrawcallList.cr = crNew
		else
			crCpu = crNew
			if (cr and gpuClip) then
				DrawcallList.cr = cr
			end
		end
	elseif (not gpuClip) then
		crCpu = cr
	elseif (cr) then
		DrawcallList.cr = cr
	end
	
	local changed = self.mesh.doCache and (self.mesh.update or (self.moved or self.sized or
	((self.cr or crCpu) and ((self.cr == nil and crCpu) or (self.cr and crCpu == nil) or
	(self.cr.x ~= crCpu.x or self.cr.y ~= crCpu.y or self.cr.w ~= crCpu.w or self.cr.h ~= crCpu.h)))))
	
	self.cr = crCpu
	
	local d
	if (self.writeId ~= true) then
		d = self.renderDisables
	end
	
	if (self.doClip and self.drawClipRect) then
		self.rcMesh:Render(d)
	end
	self.mesh.update = changed
	self.mesh:Render(d)
	
	if (self.doClip) then
		return self.gpuClip, crNew
	end
	return gpuClip, cr
end

-----Button-----
UiButton = class(UiWidget)

function UiButton:ctor(x, y, w, h, s, font)
	self.color:set(70, 70, 70, 255)
	self.rect:set(x, y, w, h)
	self.layout = BoxLayout()
	self:AddChild(self.layout)
	self.text = UiText(0, 0, s, font)
	self.text.writeId = false
	self.layout:AddChild(self.text, 1)
	
	self:bind_event(EVT.MOVE_IN, self, UiButton.OnMouse)
	self:bind_event(EVT.MOVE_OUT, self, UiButton.OnMouse)
	self:bind_event(EVT.LEFT_DOWN, self, UiButton.OnMouse)
	self:bind_event(EVT.LEFT_UP, self, UiButton.OnMouse)
end

function UiButton:OnMouse(e)
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
		self.color:set(150, 150, 150, 255)
	elseif (self.hovering) then
		self.color:set(100, 100, 100, 255)
	else
		self.color:set(70, 70, 70, 255)
	end
	
	self:Refresh()
end

-----Text-----
UiText = class(UiWidget)
UiText.cached = true
UiText.doClip = false

function UiText:ctor(x, y, s, font)
	self:SetPos(x, y)
	self.text = LString('')
	self:SetText(s, font)
end

function UiText:SetText(s, font)
	s = s or ''
	font = font or uiFont
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
			self.cr.x, self.cr.y, self.cr.w, self.cr.h, self.text)
	else
		n = CAddText(vbPos, wpPos, vbUVW, wpUVW, self.font, self.location.x, self.location.y + self.rect.h + self.font.descender, self.text)
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
UiTextInput.doClip = true
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

function UiTextInput:ctor(x, y, w, h, font)
	self.rect:set(x, y, w, h)
	
	self.crColor:set(100, 100, 100, 100)
	self.selectedColor = Color(0, 130, 255, 100)
	
	self.caret = UiWidget(0, 0, 1, self.rect.h)
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
			cTerminal:SetClipboardText(s)
		end
	
	elseif (k == SYS.VK_CTRL_V) then
		local s = cTerminal:GetClipboardText()
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
	if (remainCaret ~= true) then
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
		CAddRectFloat2(vbPos, wpPos, math.max(self.location.x + x, rect.x), rect.y, w, rect.h)
		CAddFloat3(vbUVW, wpUVW, 4, self.font.pixels, 0, 0)
		CAddUByte4(vbColor, wpColor, 4, self.selectedColor.r, self.selectedColor.g, self.selectedColor.b, self.selectedColor.a)
		n0 = 1
		wpPos = APPEND
		wpUVW = APPEND
		wpColor = APPEND
	end
	local n = CAddTextClip(vbPos, wpPos, vbUVW, wpUVW, self.font, self.textOffset, self.rect.h + self.font.descender, 
	rect.x, rect.y, rect.w, rect.h, self.text)
	CAddUByte4(vbColor, wpColor, 4 * n, self.color.r, self.color.g, self.color.b, self.color.a)
	n = n + n0
	self.nText = n
	return 4 * n
end

function UiTextInput:FillIB(ib, ib_start, wp)
	return CAddConvexPolyIndex(ib, wp, self.nText, ib_start, 4)
end

UiSlideBar = class(UiWidget)

function UiSlideBar:ctor(vertical, x, y, length, width)
	self.color:set(100, 100, 100, 100)
	self:SetPos(x, y)
	self.scale = 1
	self.slScale = 1
	self.step = 1
	self.pos = 0
	self.vertical = vertical
	if (vertical) then
		self.slider = UiWidget(x, 0, math.max(1, width), 0)
		self:SetSize(width, length)
	else
		self.slider = UiWidget(0, y, 0, math.max(1, width))
		self:SetSize(length, width)
	end
	
	self:AddChild(self.slider)
	self.slider.color:set(150, 150, 150, 255)
	self.slider:bind_event(EVT.MOVE_IN, self, UiSlideBar.OnSlideBarHovered)
	self.slider:bind_event(EVT.MOVE_OUT, self, UiSlideBar.OnSlideBarHovered)
	self.slider:bind_event(EVT.LEFT_DOWN, self, UiSlideBar.OnSliderMouseButton)
	self.slider:bind_event(EVT.LEFT_UP, self, UiSlideBar.OnSliderMouseButton)
	self.slider:bind_event(EVT.MOTION, self, UiSlideBar.OnSliding)
end

function UiSlideBar:OnSlideBarHovered(e)
	if (e == EVT.MOVE_IN) then
		self.slider.color:set(200, 200, 200, 255)
	else
		self.slider.color:set(150, 150, 150, 255)
	end
	self:Refresh()
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
UiScrollPanel = class(BoxLayout)
UiScrollPanel.barWidth = 12

function UiScrollPanel:ctor(widget)
	self.Layout = VerticalLayout
	self:bind_event(EVT.MOUSEWHEEL, self, UiScrollPanel.OnMouseWheel)
	
	local hLayout = BoxLayout(false)
	Widget2D.AddChild(self, hLayout, 1, Layout.ALIGN_LEFT|Layout.ALIGN_RIGHT|Layout.ALIGN_TOP|Layout.ALIGN_BOTTOM)
	
	self.plate = UiWidget()
	self.plate.color:set(0, 0, 0, 0)
	hLayout:AddChild(self.plate, 1, Layout.ALIGN_LEFT|Layout.ALIGN_RIGHT|Layout.ALIGN_TOP|Layout.ALIGN_BOTTOM)
	
	self.widget = widget
	self.plate:AddChild(widget)
	self.widget:bind_event(EVT.SIZE, self, UiScrollPanel.OnWidgetSize)
	
	self.vScrollBar = UiSlideBar(true, 0, 0, 0, UiScrollPanel.barWidth)
	self.vScrollBar:bind_event(EVT.SLIDE_BAR, self, UiScrollPanel.OnVScroll)
	self.vScrollBar:Show(false)
	hLayout:AddChild(self.vScrollBar, 0, Layout.ALIGN_RIGHT|Layout.ALIGN_TOP|Layout.ALIGN_BOTTOM, 0, 0, 0, 0)
	
	self.hScrollBar = UiSlideBar(false, 0, 0, 0, UiScrollPanel.barWidth)
	self.hScrollBar:bind_event(EVT.SLIDE_BAR, self, UiScrollPanel.OnHScroll)
	self.hScrollBar:Show(false)
	Widget2D.AddChild(self, self.hScrollBar, 0, Layout.ALIGN_LEFT|Layout.ALIGN_RIGHT|Layout.ALIGN_BOTTOM, 0, UiScrollPanel.barWidth, 0, 0)
	
	self:SetSize()
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
	
	
	
	
	
	
	
	