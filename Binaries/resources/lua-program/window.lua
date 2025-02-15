---window---
require 'event'
require 'widget'
require 'global'

Window = class(Widget2D)
Window.acceptFocus = true
Window.cursor = SYS.CURSOR_ARROW
Window.recycle = true

function Window:ctor()
	self.window = self
	self.time = 0
	self.timers = ObjectArray()
	self.sysCaptured = false
	self.keyDowns = {}
	
	self.cmd = Command.New()
	 
	self.cbWnd = ResBuffer(self.cmd, CAddFloat2)
	self.res_set = g_rl0:NewResourceSet()
	self.res_set:ResBuffer(self.cbWnd, 0)
	
	--self.cmdList = CmdList()
	
	self.dcLists = {[SubpassId(g_rp0, 0)] = DrawcallList(), [SubpassId(g_rp0, 1)] = DrawcallList()}
	
	if (EVT.focus_id == nil) then
		EVT.focus_id = self.id
	end
end

function Window:dtor()
	if (Window.recycle) then
		Command.Recycle(self.cmd)
	end
end

function Window:OnWidgetShow(w, show)
	if (show ~= true) then
		if (self.captured == w) then
			w:process_event(EVT.CAPTURE_LOST)
			self.captured = nil
		end
		if (EVT.focus_id == w.id) then
			w:process_event(EVT.FOCUS_OUT)
			EVT.focus_id = 0
		end
		if (EVT.entered_id == w.id) then
			w:process_event(EVT.MOVE_OUT)
			EVT.moved_in = 0
		end
	end
	self.update = true
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

function Window:init(t, hwnd, w, h)
	self:SetTime(t)

	self.rect:set(0, 0, w, h)
	self.sizegroup = SizeGroup(w, h)
	self.swapchain = cGI:NewSwapchain(hwnd, 0, w, h)
	
	--self.scTargetView = cGI:NewTargetView(cGI.IMAGE_TYPE_2D, cGI.FORMAT_SWAPCHAIN, cGI.SAMPLE_COUNT_4_BIT, w, h)
	self.idTargetView = cGI:NewTargetView(cGI.IMAGE_TYPE_2D, cGI.FORMAT_PICK_ID, cGI.SAMPLE_COUNT_1_BIT, w, h)
	self.idTexture = cGI:NewTexture(cGI.IMAGE_TYPE_2D, cGI.FORMAT_PICK_ID, w, h)
	
	cParamFrameBuffer:Reset()
	cParamFrameBuffer:SetSwapchain(self.swapchain)
	--cParamFrameBuffer:AddView(self.scTargetView, 0)
	cParamFrameBuffer:AddView(self.idTargetView, 0)
	self.frameBuffer = cGI:NewFrameBuffer(g_rp0, cParamFrameBuffer, w, h)
	
	self.frameBuffer:ClearSwapchain(0.15, 0.15, 0.15, 0.2)
	self.frameBuffer:ClearViewUint4(0, self.id, 0, 0, 0)
	-- self.frameBuffer:ClearViewFloat4(0, 0.15, 0.15, 0.15, 0.2)
	-- self.frameBuffer:ClearViewUint4(1, self.id, 0, 0, 0)
	
	self.frameBuffer.vp = self.rect
	self.frameBuffer.cr = self.rect
	
	--self.sizegroup:add_rtv(self.scTargetView)
	self.sizegroup:add_rtv(self.swapchain)
	self.sizegroup:add_fb(self.frameBuffer)
	self.sizegroup:add_rtv(self.idTargetView, true)
	self.sizegroup:add_rtv(self.idTexture, true)
	
	self.update = true
	
	return self:TimerPeriod()
end

function Window:on_idle(t, onTimer, show)
	self:SetTime(t)

	self:HandleTimer(onTimer, t)
		
	if(show and (self.update or self.sized)) then
		self.cmd:Flip()
		g_cmd = self.cmd
		
		ui_resourceSet = self.res_set
		
		DrawcallList.dvp = self.frameBuffer.vp
		DrawcallList.vp = self.frameBuffer.vp
		DrawcallList.dcr = self.frameBuffer.cr
		DrawcallList.cr = self.frameBuffer.cr
		self.dcLists[SubpassId(g_rp0, 0)]:Reset()
		self.dcLists[SubpassId(g_rp0, 1)]:Reset()
		g_dcLists = self.dcLists
		self:UpdateUI()
		self:render()
	end

	return self:TimerPeriod()
end

function Window:resize(w, h)
	local render = w <= self.rect.w and h <= self.rect.h

	self:SetSize(w, h)
	
	cGI:DeviceWaitIdle()
	self.cbWnd:Set(1, self.rect.w, self.rect.h)
	self.sizegroup:resize(w, h)
	
	if (render) then
		self:render()
	end
end

function Window:render()
	--Print('---render---')
	while (self.swapchain:Acquire() == false) do
		self.sizegroup:resize(self.rect.w, self.rect.h)
	end
	
	g_cmd = self.cmd
	
	self.cmd:PrepareRender()

	self.cmd:RenderBegin(self.frameBuffer, false)
	
	self.dcLists[SubpassId(g_rp0, 0)]:SetupDrawcalls()
	
	self.cmd:NextSubpass(false)
	
	self.dcLists[SubpassId(g_rp0, 1)]:SetupDrawcalls()
	
	self.cmd:RenderEnd()
	
	self.cmd:CopyImage(self.idTargetView, 0, 0, 0, self.idTexture, 0, 0, 0, 1, self.rect.w, self.rect.h)
	
	self.cmd:Execute()
end
	
function Window:UpdateUI()
	self.update = false
	self:Update()
	self.sized = false
end

function Window:CaptureMouse(w)
	g_focusing = self
	if (self.captured and self.captured ~= w) then
		self.captured:process_event(EVT.CAPTURE_LOST)
	end
	self.captured = w
	if (self.sysCaptured == false) then
		self:Capture(true)
		self.sysCaptured = true
	end
	g_focusing = nil
end

function Window:ReleaseCaptured()
	self.captured = nil
	if (self.sysCaptured) then
		self:Capture(false)
		self.sysCaptured = false
	end
end

function Window:on_capture_lost(t)
	g_focusing = self
	self:SetTime(t)

	if (self.captured) then
		self.captured:process_event(EVT.CAPTURE_LOST)
	end
	self.sysCaptured = false
	
	g_focusing = nil
	return self:TimerPeriod()
end

function Window:on_mouse(t, e, x, y, n)
	g_focusing = self
	self:SetTime(t)
	
	local id 
	local obj
	if (self.captured) then
		id = self.captured.id
		obj = self.captured
	else
		id = PickByTexture(self.idTexture, x, y)
		obj = get_object(id) or self
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
		if (EVT.focus_id ~= id and obj and obj.acceptFocus) then
			local last_obj = get_object(EVT.focus_id)
			if (last_obj) then
				last_obj:process_event(EVT.FOCUS_OUT)
			end
			obj:process_event(EVT.FOCUS_IN)
			EVT.focus_id = id
		end
	end
	
	if (e == EVT.MOUSEWHEEL) then
		while (obj and obj.event_table[e] == nil) do
			obj = obj.parent
		end
	end
	if (obj) then
		obj:process_event(e, x, y, n)
	end
	
	g_focusing = nil
	return self:TimerPeriod(), self.cursor
end

function Window:on_char(t, c)
	g_focusing = self
	self:SetTime(t)

	local w = get_object(EVT.focus_id)
	if (w) then
		w:process_event(EVT.CHAR, c)
	end
	
	g_focusing = nil
	return self:TimerPeriod()
end

function Window:on_acc_key(t, k)
	g_focusing = self
	self:SetTime(t)
	
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
	
	g_focusing = nil
	return self:TimerPeriod()
end

function Window:on_key_down(t, k, left, right)
	g_focusing = self
	self:SetTime(t)
	
	self.keyDowns[k] = true

	local w = get_object(EVT.focus_id)
	if (w) then
		w:process_event(EVT.KEY_DOWN, k, left, right)
	end
	
	g_focusing = nil
	return self:TimerPeriod()
end	

function Window:on_key_up(t, k)
	g_focusing = self
	self:SetTime(t)
	
	self.keyDowns[k] = false

	local w = get_object(EVT.focus_id)
	if (w) then
		w:process_event(EVT,KEY_UP, k)
	end
	
	g_focusing = nil
	return self:TimerPeriod()
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
	self.timers:insert(t, i)
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

Timer = class(Object)

function Timer:ctor()
	self.prev = 0
end

function Timer:dtor()
	if (self.window) then
		self.window:RemoveTimer(self)
	end
end

function Timer:Start(window, d, loop)
	if (self.on) then
		self.window:RemoveTimer(self)
	end
	self.prev = window.time
	self.window = window
	self.d = d
	self.loop = loop
	self.on = true
	self.window:AddTimer(self)
end

function Timer:Stop()
	if (self.on) then
		self.on = false
		self.window:RemoveTimer(self)
	end
end