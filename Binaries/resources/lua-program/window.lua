---window---
require 'event'
require 'widget'
require 'global'

Window = class(UiWidget)
Window.acceptFocus = true
Window.cursor = SYS.CURSOR_ARROW
Window.recycle = true

function Window:ctor()
	self.window = self
	self:EnableActive(true)
	self.time = 0
	self.timers = ObjectArray()
	self.sysCaptured = false
	self.keyDowns = {}
	
	self.defaultRQ = {}
	self.renderQueue = self.defaultRQ
	self.cmd = Command.NewRenderCmd()
	 
	self.res = ResourceHub(g_rlUB)
	self.rbWnd = self.res:BindResBuffer(0, SIZE_FLOAT3)
	
	--self.cmdList = CmdList()
	
	if (EVT.focus_id == nil) then
		EVT.focus_id = self.id
	end
end

function Window:dtor()
	if (Window.recycle) then
		RenderCommand.Recycle(self.cmd)
	end
end

function Window:SetFocus(w, flag)
	if (not w.acceptFocus) then
	return end
	if (flag and EVT.focus_id ~= w.id) then
		local o = get_object(EVT.focus_id)
		EVT.focus_id = w.id
		if (o) then
			o:process_event(EVT.FOCUS_OUT)
		end
		w:process_event(EVT.FOCUS_IN)
		self.update = true
	elseif (not flag and EVT.focus_id == w.id) then
		EVT.focus_id = nil
		w:process_event(EVT.FOCUS_OUT)
		self.update = true
	end
end	

function Window:SetActive(w, flag)
	w = w.active
	if  (not w) then
	return end
	if (flag and EVT.active_id ~= w.id) then
		local o = get_object(EVT.active_id)
		EVT.active_id = w.id
		if (o) then
			o:process_event(EVT.INACTIVE)
		end
		w:process_event(EVT.ACTIVE)
		self.update = true
	elseif (not flag and EVT.active_id == w.id) then
		EVT.active_id = nil
		w:process_event(EVT.INACTIVE)
		self.update = true
	end
end

function Window:OnWidgetShow(w, show)
	if (not show) then
		if (self.captured == w) then
			w:process_event(EVT.CAPTURE_LOST)
			self.captured = nil
		end
		if (EVT.entered_id == w.id) then
			w:process_event(EVT.MOVE_OUT)
			EVT.moved_in = nil
		end
		self:SetFocus(w, false)
		self:SetActive(w, false)
	end
	self.update = true
end

function Window:OnWidgetDtor(w)
	if (self.captured == w) then
		self.captured = nil
	end
	if (self.entered_id == w.id) then
		self.moved_in = nil
	end
	if (self.focus_id == w.id) then
		self.focus_id = nil
	end
	if (self.active_id == w.id) then
		self.active_id = nil
	end
end

function Window:SetTime(t)
	self.time = t
end

function Window:TimerPeriod()
	local curTimer = self.timers[1]
	local t = 0
	if (curTimer ~= self.prevTimer) then
		if (curTimer == nil) then
			t = -1
		else
			t = curTimer.t - self.time
			if (t < 1) then
				t = 1
			end
		end
	end
	self.prevTimer = curTimer
	return t
end

function Window:init(hwnd, w, h)
	self.rect:set(0, 0, w, h)
	self.sizegroup = SizeGroup(w, h)
	self.swapchain = cGI:NewSwapchain(hwnd, 0, w, h)
	
	--self.scTargetView = cGI:NewTargetView(cGI.IMAGE_TYPE_2D, cGI.FORMAT_SWAPCHAIN, cGI.SAMPLE_COUNT_4_BIT, w, h)
	self.idTargetView = cGI:NewTargetView(cGI.IMAGE_TYPE_2D, cGI.FORMAT_PICK_ID, cGI.SAMPLE_COUNT_1_BIT, w, h)
	self.idTexture = cGI:NewTexture(cGI.IMAGE_TYPE_2D, cGI.FORMAT_PICK_ID, w, h)
	self.depthStencilView = cGI:NewDepthStencilView(cGI.SAMPLE_COUNT_1_BIT, w, h)
	
	cParamFrameBuffer:Reset()
	cParamFrameBuffer:SetSwapchain(self.swapchain)
	--cParamFrameBuffer:AddView(self.scTargetView, 0)
	cParamFrameBuffer:AddView(self.idTargetView, 0)
	cParamFrameBuffer:AddView(self.depthStencilView, 0)
	self.frameBuffer = cGI:NewFrameBuffer(g_rp0, cParamFrameBuffer, w, h)
	self.frameBuffer.rp = g_rp0
	self.frameBuffer.rt = {}
	self.frameBuffer.rt[1] = self.idTargetView
	
	self.frameBuffer:ClearSwapchain(0.15, 0.15, 0.15, 0.2)
	self.frameBuffer:ClearViewUint4(0, self.id, 0, 0, 0)
	self.frameBuffer:ClearDepthStencil(1, 0, 0)
	-- self.frameBuffer:ClearViewFloat4(0, 0.15, 0.15, 0.15, 0.2)
	-- self.frameBuffer:ClearViewUint4(1, self.id, 0, 0, 0)
	
	self.frameBuffer.vp = self.rect
	self.frameBuffer.cr = self.rect
	
	--self.sizegroup:add_rtv(self.scTargetView)
	self.sizegroup:add_rtv(self.swapchain)
	self.sizegroup:add_fb(self.frameBuffer)
	self.sizegroup:add_rtv(self.idTargetView, true)
	self.sizegroup:add_rtv(self.idTexture, true)
	self.sizegroup:add_rtv(self.depthStencilView, true)
	
	self.fp = FramePipeline()
	self.copyParam = {srcView = self.idTargetView, srcLayer = 0, src_x = 0, src_y = 0,
					dstView = self.idTexture, dstLayer = 0, dst_x = 0, dst_y = 0, 
					numLayers = 1, w = self.rect.w, h = self.rect.h}
	self.fp:AddFrameOutput(self.frameBuffer)
	self.fp:AddCopyImage(self.copyParam)
	self.fp:Bake()
	self.fp:SetSurface(self, g_rp0[1])
	self.fp:SetSurface(self, g_rp0[2])
	
	self.renderQueue[1] = self.fp
	
	self.update = true
	
	return self:TimerPeriod()
end

function Window:on_idle(onTimer, show)
	self:HandleTimer(onTimer, t)
	
	if (self.idleText) then
		self.idleText:SetText(self.idle_cost..'')
	end
	
	--if (g_gcStopped) then
		--collectgarbage('restart')
		--g_gcStopped = false
	--end
	
	self:Show(show)
	self:render()

	return self:TimerPeriod()
end

function Window:resize(w, h)
	local render = w <= self.rect.w and h <= self.rect.h
	self:SetSize(w, h)
	if (w < 1 or h < 1) then
		return
	end
	
	--for i = 1, 10000 do
		--local z = {}
	--end
	
	--if (not g_gcStopped) then
		--collectgarbage('stop')
		--g_gcStopped = true
	--end
	
	cGI:DeviceWaitIdle()
	CMulAddFloat3(1, self.rbWnd(), self.rbWnd[1], self.rect.w, self.rect.h, 1)
	self.sizegroup:resize(w, h)
	
	self.copyParam.w = w
	self.copyParam.h = h
	
	if (render) then
		self:render()
	end
end

function Window:render()
	--Print('---render---')
	if (not (self.show and self.rect.w > 0 and self.rect.h > 0)) then
	return end
	
	if(self.update or self.sized) then
		
		g_resWnd = self.res
		
		self.update = true
		while (self.update) do
			self.update = false
			self.fp:UpdateSurface(self.cmd:Reset())
		end
		self.sized = false
	end
	
	while (self.swapchain:Acquire() == false) do
		self.sizegroup:resize(self.rect.w, self.rect.h)
	end
	
	self.fp:FillCommand(self.cmd)
	self.cmd:Execute()
	
	self.swapchain:Present()
end

function Window:CaptureMouse(w)
	g_actWindow = self
	if (self.captured and self.captured ~= w) then
		self.captured:process_event(EVT.CAPTURE_LOST)
	end
	self.captured = w
	if (self.sysCaptured == false) then
		self:Capture(true)
		self.sysCaptured = true
	end
	g_actWindow = nil
end

function Window:ReleaseCaptured()
	self.captured = nil
	if (self.sysCaptured) then
		self:Capture(false)
		self.sysCaptured = false
	end
end

function Window:on_capture_lost(t)
	g_actWindow = self

	if (self.captured) then
		self.captured:process_event(EVT.CAPTURE_LOST)
	end
	self.sysCaptured = false
	
	g_actWindow = nil
	return self:TimerPeriod()
end

local function FindScene(w, x, y)
	local loc = w.location
	local rect = w.rect
	if ((x < loc.x or x > loc.x + rect.w) and (y < loc.y or y > loc.y + rect.h)) then
		return
	end
	for i = w.children.n, 1, -1 do
		local v = FindScene(w.children[i], x, y)
		if (v) then
			return v
		end
	end
	if (w[SceneWidget]) then
		return w
	end
end

local function CheckScene(obj, w, x, y)
	g_sceneModel = nil
	if (obj[Model]) then
		g_sceneModel = obj
		return FindScene(w, x, y)
	end
	return obj
end

function Window:on_mouse(e, x, y, w, m)
	g_actWindow = self
	
	local id 
	local obj
	if (self.captured) then
		id = self.captured.id
		obj = self.captured
	else
		id = PickByTexture(self.idTexture, x, y)
		obj = get_object(id) or self
		obj = CheckScene(obj, self, x, y)
	end
		
	if (EVT.entered_id ~= id) then
		local last_obj = get_object(EVT.entered_id)
		if (last_obj) then
			last_obj:process_event(EVT.MOVE_OUT)
		end
		if (obj) then
			obj:process_event(EVT.MOVE_IN)
		end
		EVT.entered_id = id
	end
	
	if (e == EVT.LEFT_DOWN or e == EVT.RIGHT_DOWN) then
		if (obj) then
			self:SetFocus(obj, true)
			self:SetActive(obj, true)
		end
	end
	
	if (e == EVT.MOUSEWHEEL) then
		while (obj and obj.event_table_obj[e] == nil) do
			obj = obj.parent
		end
	end
	if (obj) then
		obj:process_event(e, x, y, w, m)
	end
	g_actWindow = nil
	return self:TimerPeriod(), self.cursor
end

function Window:on_char(c)
	g_actWindow = self

	local w = get_object(EVT.focus_id)
	if (w) then
		w:process_event(EVT.CHAR, c)
	end
	
	g_actWindow = nil
	return self:TimerPeriod()
end

function Window:on_acc_key(k)
	g_actWindow = self
	
	if (k == SYS.VK_CTRL_Z) then
		g_recorder:Undo()
	elseif (k == SYS.VK_CTRL_Y) then
		g_recorder:Redo()
	else
		local w = get_object(EVT.focus_id)
		if (w) then
			w:process_event(EVT.ACC_KEY, k)
		end
	end
	
	g_actWindow = nil
	return self:TimerPeriod()
end

function Window:on_key_down(k, left, right)
	g_actWindow = self
	
	self.keyDowns[k] = true

	local w = get_object(EVT.focus_id)
	if (w) then
		w:process_event(EVT.KEY_DOWN, k, left, right)
	end
	
	g_actWindow = nil
	return self:TimerPeriod()
end	

function Window:on_key_up(k)
	g_actWindow = self
	
	self.keyDowns[k] = false

	local w = get_object(EVT.focus_id)
	if (w) then
		w:process_event(EVT,KEY_UP, k)
	end
	
	g_actWindow = nil
	return self:TimerPeriod()
end

function Window:on_dragging(x, y, type, text)
	local id = PickByTexture(self.idTexture, x, y)
	obj = get_object(id) or self
	obj = CheckScene(obj, self, x, y)
	while (obj) do
		if (type == 1) then
			if (obj.OnDropFile) then
				return true
			end
		elseif (obj.dropId[Window.dragId]) then
			if (Window.dragOn ~= obj) then
				if (Window.dragOn) then
					Window.dragOn:OnInnerDragLeave(Window.dragId, Window.dragData)
				end
				obj:OnInnerDragEnter(x, y, Window.dragId, Window.dragData)
			end
			Window.dragOn = obj
			if (obj.OnInnerDragging) then
				local dx = obj.location.x
				local dy = obj.location.y
				obj:OnInnerDragging(x - dx, y - dy, Window.dragId, Window.dragData)
			end
			return true
		end
		obj = obj.parent
	end
	return false
end

function Window:on_drag_leave()
	if (Window.dragOn) then
		Window.dragOn:OnInnerDragLeave(Window.dragId, Window.dragData)
		Window.dragOn = nil
	end
end

function Window:on_drop(x, y, type, text)
	Window.dragOn = nil
	local id = PickByTexture(self.idTexture, x, y)
	obj = get_object(id) or self
	obj = CheckScene(obj, self, x, y)
	while (obj) do
		if (type == 1)then
			if (obj.OnDropFile) then
				obj:OnDropFile(x, y, load('return '..text, '', 't')())
			end
		elseif (obj.dropId[Window.dragId]) then
			local dx = obj.location.x
			local dy = obj.location.y
			obj:OnInnerDrop(x - dx, y - dy, Window.dragId, Window.dragData)
		end
		obj = obj.parent
	end
end

function Window:Drag(id, data)
	Window.dragOn = nil
	Window.dragId = id
	Window.dragData = data
	self:DoDragDrop()
end

function Window:HandleTimer(onTimer, t)
	local o = {}
	local b = false
	local v = self.timers[1]
	while (v) do
		if (onTimer == false and t < v.t) then
			break
		end
		onTimer = false
		b = true
		v:process_event(EVT.TIMER)
		v.prev = t
		
		self.timers:remove_idx(1)
		if (v.loop) then
			v.t = t + v.d
			table.insert(o, v)
		else
			v.on = false
		end
	end
	
	if (onTimer == false and b) then
		self.prevTimer = nil
	end
	
	for k, v in pairs(o) do
		self:AddTimer(v)
	end
end

function Window:AddTimer(t)
	t.t = self.time + t.d
	local i
	for v, k in self.timers:pairs() do
		if (v.t > t.t) then
			i = k
			break
		end
	end
	self.timers:insert(i)
end

function Window:RemoveTimer(t)
	for v, k in self.timers:pairs() do
		if (v == t) then
			self.timers:remove_idx(k)
			break
		end
	end
	self.prevTimer = self.timers[1]
end

-----Timer-----
Timer = class(Object)

function Timer:ctor()
	self.t = cTerminal.NewTimer(self.id)
end

function Timer.OnTimer(id)
	get_object(id):process_event(EVT.TIMER)
end

function Timer:Start(n, loop)
	loop = loop or true
	self.t:Start(n, not loop)
end

function Timer:Stop()
	self.t:Stop()
end

-- function Timer:ctor()
	-- self.prev = 0
-- end

-- function Timer:dtor()
	-- if (self.window) then
		-- self.window:RemoveTimer(self)
	-- end
-- end

-- function Timer:Start(window, d, loop)
	-- if (self.on) then
		-- self.window:RemoveTimer(self)
	-- end
	-- self.prev = window.time
	-- self.window = window
	-- self.d = d
	-- self.loop = loop
	-- self.on = true
	-- self.window:AddTimer(self)
-- end

-- function Timer:Stop()
	-- if (self.on) then
		-- self.on = false
		-- self.window:RemoveTimer(self)
	-- end
-- end