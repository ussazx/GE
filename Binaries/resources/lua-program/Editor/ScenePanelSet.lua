-----ScenePanelSet-----
require 'ContentPanel'
require 'presets'

--scenePanelSet.panels.content2 = ContentWindow()
--scenePanelSet.panels.content2.title = _('内容2')

sceneLayout = 'notebook_layout0/1<presets>0|2<viewport>0|3<content*logMessage>0|4<hirachey>0|5<inspector>0|*|layout2|name=dummy;caption=;state=2098174;dir=3;layer=0;row=0;pos=0;prop=100000;bestw=225;besth=225;minw=225;minh=225;maxw=-1;maxh=-1;floatx=-1;floaty=-1;floatw=-1;floath=-1|name=1;caption=;state=2098172;dir=5;layer=0;row=0;pos=0;prop=100000;bestw=250;besth=250;minw=-1;minh=-1;maxw=-1;maxh=-1;floatx=-1;floaty=-1;floatw=-1;floath=-1|name=2;caption=;state=2098172;dir=2;layer=0;row=1;pos=0;prop=100000;bestw=540;besth=346;minw=-1;minh=-1;maxw=-1;maxh=-1;floatx=-1;floaty=-1;floatw=-1;floath=-1|name=3;caption=;state=2098172;dir=3;layer=1;row=0;pos=0;prop=100000;bestw=225;besth=225;minw=-1;minh=-1;maxw=-1;maxh=-1;floatx=-1;floaty=-1;floatw=-1;floath=-1|name=4;caption=;state=2098172;dir=2;layer=2;row=0;pos=0;prop=100000;bestw=225;besth=225;minw=-1;minh=-1;maxw=-1;maxh=-1;floatx=-1;floaty=-1;floatw=-1;floath=-1|name=5;caption=;state=2098172;dir=2;layer=2;row=0;pos=1;prop=100000;bestw=225;besth=225;minw=-1;minh=-1;maxw=-1;maxh=-1;floatx=-1;floaty=-1;floatw=-1;floath=-1|dock_size(5,0,0)=18|dock_size(2,0,1)=634|dock_size(3,1,0)=227|dock_size(2,2,0)=227|/'

PresetsWindow = class(Window)
function PresetsWindow:ctor()
	local v = VBoxLayout()
	self:AddChild(v)
	local sp = UiScrollPanel()
	v:AddChild(sp, 1, 0, 0, true)
	self.vLayout = VBoxLayout()
	sp:SetWidget(self.vLayout)
	self:AddPresetItem(g_cube, _('立方体'))
	self:AddPresetItem(g_sphere, _('球体'))
	self:AddPresetItem(g_plane3d, _('平面'))
end

function PresetsWindow:AddPresetItem(item, text)
	local o = UiButton(0, 28)
	o.color0:set(0, 0, 0, 0)
	o.color2 = o.color1
	local icon = UiPolyIcon(g_iconGeom1)
	icon:EnableWriteId(false)
	
	o.layout:AddChild(icon, nil, 7)
	o.text:SetText(text)
	o.layout:AddChild(o.text, 1, 7, 5)
	self.vLayout:AddChild(o, nil, 0, 0, true, 0, 0)
	
	o.item = item
	o:bind_event(EVT.LEFT_DOWN, self, PresetsWindow.OnItemLeftDown)
end

function PresetsWindow:OnItemLeftDown(e)
	self:Drag(PresetsWindow, EVT.obj.item)
end

local gridLineLen = 1000
local lineSpace = 1
local fadePerCount = 10 
local fov = 45

local function GridFunc(grid, vbLineInfo, lwp, vbFadeInfo, fwp, vbRange, rwp, vbColor, cwp, ib, iwp, ibStart)
	local gridLineLenH = gridLineLen / 2
	local count = math.ceil(gridLineLenH / lineSpace)
	local lenH = lineSpace * count + lineSpace / 2
	local len = lenH * 2
	
	if (count ~= grid.count) then
		local b = g_gridSeqInst()
		b:SetWritePos(0)
		for i = -count, count do
			CMulAddInt1(1, b, APPEND, i)
		end
		grid:SetMeshInstArgs(1, g_gridSeqInst, 0, count * 2 + 1)
		grid.count = count
	end

	local x, y, z = grid.camera.mWorld:GetPosition()
	y = math.abs(y)
	local t = math.tan(fov * 0.5 * degToArc)
	local n = math.ceil(y * t * 2 / lineSpace)
	local fadeLevel = math.floor(math.log(n, fadePerCount))
	local noneFadeCount = fadePerCount ^ fadeLevel
	local fadeCount = 0
	local fade = 0
	if (fadeLevel > 0) then
		fadeCount = noneFadeCount / fadePerCount
		local y0 = noneFadeCount * lineSpace * 0.5 / t
		local y1 = math.max(y, y0 * fadePerCount * 0.25)
		fade = 1 - (y - y0) / (y1 - y0)
	end
	
	local ceiling = gridLineLenH / t
	
	CMulAddFloat4(1, vbLineInfo, lwp, -lenH, x, z, 1)
	CMulAddFloat4(1, vbLineInfo, APPEND, lenH, x, z, 1)
	CMulAddFloat4(1, vbLineInfo, APPEND, lenH, x, z, 0)
	CMulAddFloat4(1, vbLineInfo, APPEND, -lenH, x, z, 0)
	CMulAddFloat4(4, vbFadeInfo, fwp, lineSpace, fadeCount, noneFadeCount, fade)
	CMulAddFloat1(4, vbRange, rwp, math.min(500, math.max(50, y * t)))
	CMulAddUByte4(4, vbColor, cwp, 150, 150, 150, 80)
	
	CAddLineListIndex(4, ib, iwp, ibStart)
	return 4, 4
end

SceneView = class(Scene3D)
SceneView.EvtPicked = {}
function SceneView:ctor(scene)
	self.scene = scene or self.scene
	self.focus = SceneObject(self.scene)
	self.focus:Move(0, 5, 0)
	self.camera:Attach(self.focus)
	self.camera:Move(0, 0, -10)
	
	EVT.BindMouseAll(self, self, SceneView.OnSceneMouse)
end

function SceneView:RotateView(yaw, pitch)
	yaw = yaw / 2
	pitch = pitch / 2
	local m = self.focus
	m:Rotate(0, 1, 0, yaw, false)
	m:RotateLocalX(pitch, false)
end

function SceneView:MoveView(x, y, z)
	x = x / 20
	y = y / 20
	if (z > 0) then
		z = 1
	elseif (z < 0) then
		z = -1
	end
	local _, d, _ = self.camera.mWorld:GetPosition()
	local d = 1 + math.min(math.abs(d) * 0.1, 10)
	self.focus:MoveLocal(x * d, y * d, z * d)
end

function SceneView:OnSceneMouse(e, x, y, w, m)
	if (e == EVT.LEFT_DOWN) then
		g_actWindow:CaptureMouse(self)
		self.mx = x
		self.my = y
	elseif (e == EVT.LEFT_UP) then
		g_actWindow:ReleaseCaptured(self)
		if (not self.viewing) then
			if (g_sceneModel) then
				self.picked = g_sceneModel
				self:process_event(SceneView.EvtPicked, g_sceneModel)
			elseif (self.picked) then
				self:process_event(SceneView.EvtPicked)
			end
		end
		self.viewing = false
	elseif (e == EVT.MOTION) then
		if (g_actWindow.captured == self) then
			self.viewing = true
			if (m) then
				self:MoveView(self.mx - x, y - self.my, 0)
			else
				self:RotateView(x - self.mx, y - self.my)
			end
		end
		self.mx = x
		self.my = y
	elseif (e == EVT.MIDDLE_DOWN) then
		g_actWindow:CaptureMouse(self)
		self.mx = x
		self.my = y
	elseif (e == EVT.MOUSEWHEEL) then
		self:MoveView(0, 0, w)
	elseif (e == EVT.MIDDLE_UP) then
		g_actWindow:ReleaseCaptured(self)
		self.viewing = false
	end
	self:Refresh()
end

local SceneWindow = class(Window)
function SceneWindow:ctor()
	self:EnableWriteId(false)
	self.drawSelf = false
	self.sceneView = SceneView()
	self.scene = self.sceneView.scene
	self.sceneView.fColor:set(0.25, 0.25, 0.25, 1)
	self.sceneView:bind_event(EVT.KEY_DOWN, self, SceneWindow.OnKeyDown)
	
	self.sceneView:bind_event(SceneView.EvtPicked, self, SceneWindow.OnPicked)
	
	local v = VBoxLayout()
	v:AddChild(self.sceneView, 1, 0, 0, true)
	self:AddChild(v)
	self:EnableDrop(PresetsWindow, true)
	
	self.grid = Model(g_grid3d)
	self.grid:EnableWriteId(false)
	self.grid:SetCustomMesh(1, GridFunc, self.grid)
	self.grid.camera = self.sceneView.camera
	self.grid:Attach(self.scene)
	
	self.o = Model(g_cube)
	--self.o:Attach(self.scene)
	self.o1 = Model(g_cube)
	self.o1:Move(-2.5, 0, 0)
	--self.o1:Attach(self.scene)
	
	self:bind_event(EVT.INNER_DRAG_ENTER, self, self.OnInnerDragEnter)
	self:bind_event(EVT.INNER_DRAGGING, self, self.OnInnerDragging)
	self:bind_event(EVT.INNER_DRAG_LEAVE, self, self.OnInnerDragLeave)
	self:bind_event(EVT.INNER_DROP, self, self.OnInnerDrop)
	
	self.objCoord = ObjectCoord(self.sceneView.camera)
	self.objCoord.arrowX.pickable[self.sceneView] = true
	self.objCoord.arrowY.pickable[self.sceneView] = true
	self.objCoord.arrowZ.pickable[self.sceneView] = true
	
	EVT.BindMouseAll(self.objCoord.arrowX, self, self.OnCoord, EVT.MOUSEWHEEL)
	EVT.BindMouseAll(self.objCoord.arrowY, self, self.OnCoord, EVT.MOUSEWHEEL)
	EVT.BindMouseAll(self.objCoord.arrowZ, self, self.OnCoord, EVT.MOUSEWHEEL)
	--self.objCoord:Attach(self.scene)
	
	--local w = UiPolyIcon(g_iconLine1)
	-- local w = UiText('abcdef')
	-- w:EnableWriteId(false)
	-- self.w = w
	-- w.mRoot = CMatrix()
	-- w.mtl = Material(g_mtlUi2)
	-- w.mtl.resModel = ResourceHub(g_rlUB)
	-- local buf = w.mtl.resModel:BindResBuffer(0, CMatrix._size)
	-- CAddMatrix(buf(), buf[1], w.mRoot)
	-- w.renderer:SetMaterial(w.mtl)
	--self.scene:AddChild(w, 100, 50)
	self.a = true
end

function SceneWindow:OnCoord(e, x, y, w, m)
	local c = self.objCoord
	local o = EVT.obj
	if (e == EVT.MOVE_IN) then
		if (o == c.arrowX) then
			c:SetColorX(255, 255, 0, 200)
		elseif (o == c.arrowY) then
			c:SetColorY(255, 255, 0, 200)
		elseif(o == c.arrowZ) then
			c:SetColorZ(255, 255, 0, 200)
		end
	elseif (e == EVT.MOVE_OUT) then
		c:RestoreColors(true, true, true)
	elseif (e == EVT.LEFT_DOWN) then
		g_actWindow:CaptureMouse(o)
		if (o == c.arrowX) then
			c:SetColorY(100, 100, 100, 100)
			c:SetColorZ(100, 100, 100, 100)
		elseif (o == c.arrowY) then
			c:SetColorX(100, 100, 100, 100)
			c:SetColorZ(100, 100, 100, 100)
		elseif(o == c.arrowZ) then
			c:SetColorX(100, 100, 100, 100)
			c:SetColorY(100, 100, 100, 100)
		end
		self.mx = x
		self.my = y
	elseif (e == EVT.MOTION) then
		if (g_actWindow.captured == o) then
			local x0, y0 = x - self.mx, y - self.my
			x0, y0 = self.sceneView:ScreenToViewVec(x0, y0)
			
			local x1, y1, z1 = c.attached.mWorld:GetPosition()
			local camera = self.sceneView.camera
			x1, y1, z1 = camera.mView:PointTransform(x1, y1, z1)
			x0 = x0 * z1
			y0 = y0 * z1
			
			local mView = camera.mView
			if (o == c.arrowX) then
				x1, y1, _ = mView:VectorTransform(1, 0, 0)
				c.attached:Move(Dot2D(x0, y0, Normalize2D(x1, y1)), 0, 0)
			elseif (o == c.arrowY) then
				x1, y1, _ = mView:VectorTransform(0, 1, 0)
				c.attached:Move(0, Dot2D(x0, y0, Normalize2D(x1, y1)), 0)
			elseif (o == c.arrowZ) then
				x1, y1, _ = mView:VectorTransform(0, 0, 1)
				c.attached:Move(0, 0, Dot2D(x0, y0, Normalize2D(x1, y1)))
			end
		end
		self.mx = x
		self.my = y
	elseif ( e == EVT.LEFT_UP) then
		g_actWindow:ReleaseCaptured(o)
		if (o == c.arrowX) then
			c:RestoreColors(false, true, true)
		elseif (o == c.arrowY) then
			c:RestoreColors(true, false, true)
		elseif(o == c.arrowZ) then
			c:RestoreColors(true, true, false)
		end
	end
	self:Refresh()
end

function SceneWindow:OnPicked(e, m)
	local o = self.picked
	local b = not m and o
	if (m) then
		b = o and m ~= o
		self.objCoord:Attach(m, nil, SceneObject.ATTACH_ROT_IGNORE)
		self.picked = m
		m:ShowPicked(true)
	else
		self.objCoord:Detach(true)
		self.picked = nil
	end
	if (b) then
		o:ShowPicked(false)
	end
end

function SceneWindow:OnKeyDown(e, k)
	if (k == SYS.VK_UP) then
		self.o:Move(0, 0, 0.1)
	elseif (k == SYS.VK_DOWN) then
		self.o:Move(0, 0, -0.1)
	elseif (k == SYS.VK_LEFT) then
		self.o:Move(-0.1, 0, 0)
	elseif (k == SYS.VK_RIGHT) then
		self.o:Move(0.1, 0, 0)
	elseif (k == SYS.VK_A) then
		if (self.a) then
			self.o1:Attach(self.o, self.o1.ATTACH_WORLD, self.o1.ATTACH_ROT_AFFECT_POS_ROT)
		else
			self.o1:Detach()
		end
		self.a = not self.a
	elseif (k == SYS.VK_Q) then
		self.o:Rotate(0, 1, 0, 2, false)
	elseif (k == SYS.VK_E) then
		self.o:Rotate(0, 1, 0, -2, false)
	end
	self:Refresh()
end

function SceneWindow:UpdateDragging(x, y, m)
	if (self.x == x and self.y == y and self.dragging == m) then
		return false
	end
	self.x, self.y, self.dragging = x, y, m
	local z
	x, y, z = self.sceneView:ScreenToViewPos(x, y)
	x, y, z = CVec3NormalizeScale(x, y, z, 20)
	x, y, z = self.sceneView.camera.mWorld:PointTransform(x, y, z)
	m:SetPosition(x, y, z)
	return true
end

function SceneWindow:OnInnerDragEnter(e, id, data, x, y)
	if (not g_previews[data]) then
		g_previews[data] = Model(data)
	end
	local m = g_previews[data]
	if (self.dragging ~= m) then
		m:EnableWriteId(false)
		m:Attach(self.scene)
	end
	if (self:UpdateDragging(x, y, m)) then
		self:Refresh()
		self:render()
	end
end
	
function SceneWindow:OnInnerDragging(e, id, data, x, y)
	local m = self.dragging
	if (self:UpdateDragging(x, y, m)) then
		self:Refresh()
		self:render()
	end
end

function SceneWindow:OnInnerDragLeave()
	self.dragging:Detach(true)
	self.dragging = nil
	self:Refresh()
	self:render()
end

function SceneWindow:OnInnerDrop(e, id, data, x, y)
	self.dragging:EnableWriteId(true)
	g_previews[data] = nil
	self:Refresh()
end

function ScenePanelSet(funcAdd)
	funcAdd('presets', _('预设'), PresetsWindow())
	funcAdd('viewport', _('视口'), SceneWindow())
	funcAdd('hirachey', _('大纲'), Window())
	funcAdd('inspector', _('细节'), Window())
	funcAdd('logMessage', _('日志消息'), Window())
	funcAdd('content', _('内容'), ContentPanel())
end