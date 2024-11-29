---widget---
require 'object'
require 'render'
require 'utility'

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
end

function Widget2D:AddChild(widget, ...)
	if (widget.parent) then
		if (widget.parent == self) then
			return
		end
		widget.parent:RemoveChild(widget)
	end
	widget.parent = self
	self.children:insert(widget)
	widget:bind_event(EVT.DELISTED, self, Widget2D.OnChildDelisted)
	if (self.window) then
		self.window:OnAddChild(widget)
	end
	if (self.OnAddChild) then
		return self:OnAddChild(widget, ...)
	end
end

function Widget2D:RemoveChild(widget)
	if (widget.parent == self) then
		widget.parent = nil
		self.children:remove_obj(widget)
		if (self.OnRemoveChild) then
			self:OnRemoveChild(widget)
		end
	end
end

function Widget2D:OnChildDelisted(e, widget)
	if (widget.parent == self) then
		widget.parent = nil
		if (self.window) then
			self.window:OnRemoveChild(w)
		end
		if (self.OnRemoveChild) then
			self:OnRemoveChild(widget)
		end
	end
end

function Widget2D:Show(show)
	if (self.show ~= show) then
		if (self.window) then
			self.window.update = true
		end
		if (self.inLayout) then
			self.inLayout.update = true
		end		
		self.show = show
	end
end

function Widget2D:Update(...)
	if (self.show == false) then
		return
	end
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
	if (allowHided) then
		return true
	end
	return c.show
end

function Widget2D:ChildrenPairs()
	return self.children:pairs(FilterShown, allowHided)
end

function Widget2D:SetPos(x, y)
	if (self.inLayout and self.inLayout.update == false) then
		return
	end
	self:DoSetPos(x, y)
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
UiWidget.baked = false
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
	self.fill = true
	self.gpuClip = false
	
	if (self.baked) then
		self.vb = {}
		self.vb.pos = CMBuffer(SIZE_FLOAT2 * self.bakeCount)
		self.vb.uvw = CMBuffer(SIZE_FLOAT3 * self.bakeCount)
		self.vb.color = CMBuffer(SIZE_UINT1 * self.bakeCount)
		self.ib = CMBuffer(SIZE_UINT1 * self.bakeCount)
		
		self.vb[0] = self.vb.pos
		self.vb[1] = self.vb.uvw
		self.vb[2] = self.vb.color
	end
end

function UiWidget:Refresh()
	self.fill = true
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

function UiWidget:FillVertex(vbPos, vbUVW, vbColor, wp)
	if (self.clipRect) then
		CAddRectFloat2(vbPos, wp, self.clipRect.x, self.clipRect.y, self.clipRect.w, self.clipRect.h)
	else
		CAddRectFloat2(vbPos, wp, self.location.x, self.location.y, self.rect.w, self.rect.h)
	end
	CAddFloat3(vbUVW, wp, 4, self.font.pixels, 0, 0)
	CAddUByte4(vbColor, wp, 4, self.color.r, self.color.g, self.color.b, self.color.a)
	return 4
end

function UiWidget:FillIndex(ib, ib_start, wp)
	return CAddConvexPolyIndex(ib, wp, 1, ib_start, 4)
end

function UiWidget:DoUpdate(clipRect)
	local clipRectNew
	if (self.doClip or clipRect) then
		clipRectNew = Rect(self.location.x, self.location.y, self.rect.w, self.rect.h)
		if (clipRect) then
			clipRectNew = clipRectNew:intersect(clipRect)
			if (clipRectNew == nil) then
				self.abortUpdate = true
				return
			end
		end
		if (self.gpuClip) then
			g_dcListUI:SetClipRect(clipRectNew)
			g_dcListId:SetClipRect(clipRectNew)
		end
		if (self.doClip) then
			clipRect = clipRectNew
		end
	end
	
	local clipRectChanged = false
	if (clipRectNew) then
		clipRectChanged = self.clipRect == nil or clipRectNew.x ~= self.clipRect.x or clipRectNew.y ~= self.clipRect.y
		or clipRectNew.w ~= self.clipRect.w or clipRectNew.h ~= self.clipRect.h
	elseif (self.clipRect) then
		clipRectChanged = true
	end
	self.clipRect = clipRectNew
	
	g_dcListUI:AddResourceSet(ui_resourceSet)
	g_dcListUI:AddResourceSet(self.font.res)
	g_dcListUI:SetPipeline(g_plUi)
	g_dcListUI:CommitStates()
	
	if (self.writeId) then
		g_dcListId:AddResourceSet(ui_resourceSet)
		g_dcListId:SetPipeline(g_plId2D)
		g_dcListId:CommitStates()
	end
	
	local slot = g_plUi.slot
	if (self.doClip and self.drawClipRect) then
		CAddRectFloat2(g_vsInput[slot[0]], APPEND, clipRectNew.x, clipRectNew.y, clipRectNew.w, clipRectNew.h)
		CAddFloat3(g_vsInput[slot[1]], APPEND, 4, self.font.pixels, 0, 0)
		CAddUByte4(g_vsInput[slot[2]], APPEND, 4, self.crColor.r, self.crColor.g, self.crColor.b, self.crColor.a)
		
		local n_idx = CAddConvexPolyIndex(g_dcListUI.ib, APPEND, 1, g_dcListUI.idxAddOn, 4)
		g_dcListUI:Draw(4, n_idx, 0, 1)
		
		if (self.writeId) then
			n_idx = CAddConvexPolyIndex(g_dcListId.ib, APPEND, 1, g_dcListId.idxAddOn, 4)
			g_dcListId:Draw(4, n_idx, self.id, 1)
		else
			g_dcListId:Skip(4)
		end
	end
	
	--draw self
	if (self.baked) then
		if (self.fill or self.moved or clipRectChanged) then
			self.n_vtx = self:FillVertex(self.vb.pos, self.vb.uvw, self.vb.color, 0, 0)
			self.n_idx = self:FillIndex(self.ib, 0, 0)
			self.fill = false
		end
		CBufferCopy(self.vb.pos, 0, self.n_vtx * SIZE_FLOAT2, g_vsInput[slot[0]], APPEND)
		CBufferCopy(self.vb.uvw, 0, self.n_vtx * SIZE_FLOAT3, g_vsInput[slot[1]], APPEND)
		CBufferCopy(self.vb.color, 0, self.n_vtx * SIZE_UINT1, g_vsInput[slot[2]], APPEND)
		
		CCopyIndexBuffer(self.ib, 0, self.n_idx, g_dcListUI.idxAddOn, g_dcListUI.ib, APPEND)
		if (self.writeId) then
			CCopyIndexBuffer(self.ib, 0, self.n_idx, g_dcListId.idxAddOn, g_dcListId.ib, APPEND)
		end
	else
		self.n_vtx = self:FillVertex(g_vsInput[slot[0]], g_vsInput[slot[1]], g_vsInput[slot[2]], APPEND)
		self.n_idx = self:FillIndex(g_dcListUI.ib, g_dcListUI.idxAddOn, APPEND)
		if (self.writeId) then
			self:FillIndex(g_dcListId.ib, g_dcListId.idxAddOn, APPEND)
		end
	end

	g_dcListUI:Draw(self.n_vtx, self.n_idx, 0, 1)
	if (self.writeId) then
		g_dcListId:Draw(self.n_vtx, self.n_idx, self.id, 1)
	else
		g_dcListId:Skip(self.n_vtx)
	end
	
	return clipRect
end

-----Button-----
UiButton = class(UiWidget)

function UiButton:ctor(x, y, w, h, s, font)
	self.color:set(70, 70, 70, 255)
	self.rect:set(x, y, w, h)
	self.layout = BoxLayout()
	self:AddChild(self.layout)
	self.text = UiText(0, 0, s, font)
	self.layout:AddChild(self.text, 1)
end

-----Text-----
UiText = class(UiWidget)
UiText.baked = true
UiText.doClip = false

function UiText:ctor(x, y, s, font)
	self:SetPos(x, y)
	self:SetText(s, font)
end

function UiText:SetText(s, font)
	s = s or ''
	font = font or uiFont
	if (self.text ~= s or self.font ~= font) then
		self.text = s
		self.font = font
		self:SetSize(CMeasureText(s, -1, -1, font), font.fontSize)
	end
end

function UiText:FillVertex(vbPos, vbUVW, vbColor, wp)
	local n
	if (self.clipRect) then
		n = CAddTextClip(vbPos, vbUVW, wp, self.font,
		self.location.x - self.clipRect.x, self.location.y - self.clipRect.y + self.font.fontSize + self.font.descender,
			self.clipRect.x, self.clipRect.y, self.clipRect.w, self.clipRect.h, self.text)
	else
		n = CAddText(vbPos, vbUVW, wp, self.font, self.location.x, self.location.y + self.rect.h + self.font.descender, self.text)
	end
	
	CAddUByte4(vbColor, wp, 4 * n, self.color.r, self.color.g, self.color.b, self.color.a)
	self.nText = n
	return 4 * n
end

function UiText:FillIndex(ib, ib_start, wp)
	return CAddConvexPolyIndex(ib, wp, self.nText, ib_start, 4)
end

-----TextInput-----
UiTextInput = class(UiWidget)
UiTextInput.baked = true
UiTextInput.drawClipRect = true

local function TextInputAssign(a, b)
	a.text = b.text
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
	self:SetText('', font or uiFont)
	
	self.timer = self:auto_del(Timer())
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
		self.window.cursor = SYS.CURSOR_IBEAM
	elseif (e == EVT.MOVE_OUT) then
		self.window.cursor = SYS.CURSOR_ARROW
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
		self.timer:Start(self.window, 500, true)
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
	if (self.text == '') then
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
		self.window:CaptureMouse(self)
	elseif(e == EVT.LEFT_DCLICK) then
		self:SelectAll()
	end
end

function UiTextInput:OnMouseUp(e, x)
	if (self.window.captured == self) then
		self.window:ReleaseCaptured()
	end
end

function UiTextInput:OnCaptureLost()
	self:ClearSelected()
end

function UiTextInput:OnMouseMotion(e, x)
	if (self.window.captured == self) then
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
	local w, n
	self.text, w, n = CTextInsert(self.text, self.insertIdx, c, self.font)
	self.textWidth = self.textWidth + w
	self.insertIdx = self.insertIdx + n
	self:RestrictCaretPos(self.caret.rect.x + w)
	self:ClearSelected()
	
	self:Record()
end

function UiTextInput:RemoveText(idx, count, recorded)
	self.text, d = CTextRemove(self.text, idx, count, self.font)
	if (d == 0) then
		return
	end
	
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
end

function UiTextInput:OnKeyDown(e, k)
	local x
	local shiftDown = self.window.keyDowns[SYS.VK_SHIFT] or false
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
		CSetClipboardText(CTextSubstr(self.text, idx, count))
		if (k == SYS.VK_CTRL_X) then
			self:RemoveText(idx, count)
		end
	
	elseif (k == SYS.VK_CTRL_V) then
		local s = CGetClipboardText()
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
		self.text = s
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

function UiTextInput:FillVertex(vbPos, vbUVW, vbColor, wp)
	local rect = self.clipRect or self.rect
	local wp0 = wp
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
		CAddRectFloat2(vbPos, wp, math.max(self.location.x + x, rect.x), rect.y, w, rect.h)
		CAddFloat3(vbUVW, wp, 4, self.font.pixels, 0, 0)
		CAddUByte4(vbColor, wp, 4, self.selectedColor.r, self.selectedColor.g, self.selectedColor.b, self.selectedColor.a)
		n0 = 1
		wp = APPEND
	end
	local n = CAddTextClip(vbPos, vbUVW, wp, self.font, self.textOffset, self.rect.h + self.font.descender, 
	rect.x, rect.y, rect.w, rect.h, self.text)
	CAddUByte4(vbColor, wp, 4 * n, self.color.r, self.color.g, self.color.b, self.color.a)
	n = n + n0
	self.nText = n
	return 4 * n
end

function UiTextInput:FillIndex(ib, ib_start, wp)
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
	self.slider:bind_event(EVT.MOVE_IN, self, self.OnSlideBarHovered)
	self.slider:bind_event(EVT.MOVE_OUT, self, self.OnSlideBarHovered)
	self.slider:bind_event(EVT.LEFT_DOWN, self, self.OnSliderMouseButton)
	self.slider:bind_event(EVT.LEFT_UP, self, self.OnSliderMouseButton)
	self.slider:bind_event(EVT.MOTION, self, self.OnSliding)
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
		self.window:CaptureMouse(self.slider)
	elseif (e == EVT.LEFT_UP) then
		if (self.window.captured == self.slider) then
			self.window:ReleaseCaptured()
		end
	end
end

function UiSlideBar:OnSliding(e, x, y)
	if (self.window.captured ~= self.slider) then
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
	self:bind_event(EVT.MOUSEWHEEL, self, self.OnMouseWheel)
	
	local hLayout = BoxLayout(false)
	Widget2D.AddChild(self, hLayout, 1, Layout.ALIGN_LEFT|Layout.ALIGN_RIGHT|Layout.ALIGN_TOP|Layout.ALIGN_BOTTOM)
	
	self.plate = UiWidget()
	self.plate.color:set(0, 0, 0, 0)
	hLayout:AddChild(self.plate, 1, Layout.ALIGN_LEFT|Layout.ALIGN_RIGHT|Layout.ALIGN_TOP|Layout.ALIGN_BOTTOM)
	
	self.widget = widget
	self.plate:AddChild(widget)
	self.widget:bind_event(EVT.SIZE, self, UiScrollPanel.OnWidgetSize)
	
	self.vScrollBar = UiSlideBar(true, 0, 0, 0, self.barWidth)
	self.vScrollBar:bind_event(EVT.SLIDE_BAR, self, self.OnVScroll)
	self.vScrollBar:Show(false)
	hLayout:AddChild(self.vScrollBar, 0, Layout.ALIGN_RIGHT|Layout.ALIGN_TOP|Layout.ALIGN_BOTTOM, 0, 0, 0, 0)
	
	self.hScrollBar = UiSlideBar(false, 0, 0, 0, self.barWidth)
	self.hScrollBar:bind_event(EVT.SLIDE_BAR, self, self.OnHScroll)
	self.hScrollBar:Show(false)
	Widget2D.AddChild(self, self.hScrollBar, 0, Layout.ALIGN_LEFT|Layout.ALIGN_RIGHT|Layout.ALIGN_BOTTOM, 0, self.barWidth, 0, 0)
	
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
	
	
	
	
	
	
	
	