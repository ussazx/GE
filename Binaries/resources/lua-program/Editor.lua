---Editor---
require 'window'
require 'presets'

local savedList = {}
local savedListPath
local scenePanelSet = {panels = {}}
g_proj = {}

function WindowRecord(w, redo, o)
	if (redo) then
		w:AddChild(o)
	else
		w:RemoveChild(o)
	end
end

function LoadSavedList(w)
	local layout = VBoxLayout()
	w:AddChild(layout)
	
	--local t = UiTextInput(0, uiFont.fontSize)
	--layout:AddChild(t, nil, 20, 10, true, 20, 20)
	
	local grid = GridLayout()
	local scrollPanel = UiScrollPanel()
	scrollPanel:SetWidget(grid)
	layout:AddChild(scrollPanel, 1, 10, 10, true, 10, 10)
	for _, v in pairs(savedList) do
		local ww = UiWidget(150, 150)
		ww.color:set(100, 100, 100, 100)
		grid:AddChild(ww, 5, 5, 10, 10)
		--ww.gpuClip = true
		
		local layout = VBoxLayout()
		ww:AddChild(layout)
		ww:AddChild(layout)
		
		ww = UiPolyIcon(g_iconFolder, true, 80, 45)
		layout:AddChild(ww, 1)
		
		-- ww = UiPolyIcon(g_iconFolder, true)
		-- layout:AddChild(ww, 1, Layout.ALIGN_LEFT|Layout.ALIGN_RIGHT|Layout.ALIGN_TOP|Layout.ALIGN_BOTTOM, 5, 5, 5, 5)
		
		--ww = UiPolyIcon(g_iconFolder)
		--layout:AddChild(ww, 1, 0, 5, 5, 5, 5)
		
		local t = UiText('abcdef')
		layout:AddChild(t)
	end
	local layoutBottom = HBoxLayout()
	w.idle_cost = 0
	w.idleText = UiText('--')
	layoutBottom:AddChild(w.idleText)
	layoutBottom:AddChild(UiButton(100, 30, _('Load')), 1, nil, 0, false)
	layout:AddChild(layoutBottom, nil, 10, 10, true, 10, 10)
	--layout:AddChild(UiButton(0, 0, 100, 30, _('Load')), 0, Layout.ALIGN_RIGHT|Layout.ALIGN_TOP|Layout.ALIGN_BOTTOM, 0, 10, 10, 10)
end

function NewCommonWindow()
	local w = Window()
end

local function SaveProject(path)
	if (not path) then
		path = cTerminal.NewFileDialog('', '', '')
		if (path:length() == 0) then
			return false
		end
		cTerminal.NewDirectory(path)
		local f = CNewFileOutput()
		f:Open(path .. '\\project', false)
		g_proj.panelLayout = scenePanelSet.nb:SaveLayout()
		f:WriteUtf8('return' .. SerializeToTableText(g_proj))
		f:Close()
		cTerminal.NewDirectory(path .. '\\Content')
		cTerminal.NewDirectory(path .. '\\Config')
		local name = path:substr(path:rfind('\\') + 1, -1)
		g_projPath = path
		table.insert(savedList, {name = name, path = path})
		f:Open(savedListPath, false)
		f:WriteUtf8('return' .. SerializeToTableText(savedList))
		f:Close()
	end
	return true
end

function LoadProject()
	--LoadAssets(g_projPath)
end

function LoadContent(path)
	-- local o = LoadLuaFile(path, isBin)
	-- if (o) then
		-- o = o() or {}
		
	-- else
		-- Print('error')
	-- end
	
	--// traverse content and load assets
	local f = cTerminal.NewFileFinder()
	local found = f:FindFirst(path .. '\\*')
	while (found) do
		local name = f:GetName()
		if (f:IsDirectory()) then
			if (name:ch(0) ~= '.') then
				LoadContent(path .. '\\' .. name)
			end
		elseif (name:rfind('.xasset') ~= -1) then
			CLoadAsset(path .. '\\' .. name)
		end
		found = f:FindNext()
	end
	--// Open windows
	
end

SceneView = class(Scene3D)
SceneView.EvtPicked = {}
function SceneView:ctor(scene)
	self.scene = scene
	self.focus = SceneObject(scene)
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

---FrameBufferPanel---
FrameBufferPanel = class(VBoxLayout)

function FrameBufferPanel:ctor()
	local combo = UiCombo()
	combo:AddItem('zzzz')
	combo:AddItem('zzzz')
	combo:AddItem('zzzzzzzzzzzzzzzzzzzzzzzzzzzzz')
	combo:AddItem('zzzz')
	combo:AddItem('zzzz')
	combo:SetDefault(5)
	self:AddChild(combo, nil, 100, 0, false, 100)
end

local function NewScene()
	
end

local function NewUI()
	
end

---ContentPanel---
ContentPanel = class(HSizerLayout)

function ContentPanel:ctor()
	self.list = UiTreeList()
	self:AddChild(UiScrollPanel(self.list), 1, 0, 0, true)
	
	local mag = UiPolyIcon(g_iconMagnifier)
	self.searcher = UiTextInput(0, uiFont.fontSize)
	local h = HBoxLayout()
	h:AddChild(mag, nil, 5, 5, false)
	h:AddChild(self.searcher, 1, 0, 0, false)
	h:SetSize()
	
	local bkg = UiWidget(0, h.rect.h + 2)
	bkg.color:copy(self.searcher.crColor)
	bkg:AddChild(h)
	
	local v = VBoxLayout()
	v:AddChild(bkg, nil, 5, 0, true)
	
	self.grid = GridLayout()
	local panel = UiScrollPanel(self.grid)
	panel:bind_event(EVT.RIGHT_UP, self, ContentPanel.OnMouse)
	v:AddChild(panel, 1, 8, 0, true)
	
	local w = UiWidget()
	w.gpuClip = true
	w.drawSelf = false
	w:AddChild(v)
	
	self:AddChild(w, 3, 0, 0, true)
	
	self.menu = CMenu()
	self.menu:AddItem(1, _('新建场景'))
	self.menu:AddItem(2, _('新建UI'))
	local m = self.menu:AddSubMenu(_('Sub'))
	m:AddItem(3, 'zz')
end

function ContentPanel:OnMouse(e)
	if (e == EVT.RIGHT_UP) then
		local b, id = self.menu:Popup(g_actWindow)
		if (not b) then return end
		if (id == 1) then
			NewScene()
		end
	else
	end
end

function ContentPanel:ScanDirectory(d)
	local n = self.list:AddNode(nil, g_iconFolder, 'main')
	n = self.list:AddNode(n, g_iconFolder, 'sub')
	self.list:AddNode(n, g_iconFolder, 'sub111111111111111')
end

function ContentPanel:OnDropFile(x, y, files)
	for _, f in pairs(files) do
		Print(f)
	end
end

function OnLoadButton(w)
	cTerminal.OpenFileDialog(_('加载项目'), '', 'project')
end

function NewWindow_LoadProj()
	local w = Window()
	w.color:set(70, 70, 70, 255)
	
	savedListPath = cTerminal.currentPath .. 'saved'
	local c = LoadFile(nil, savedListPath, {})
	if (c) then
		savedList = c() or savedList
		local layout = VBoxLayout()
		w:AddChild(layout)
	
		local t = UiTextInput(0, uiFont.fontSize)
		layout:AddChild(t, nil, 20, 10, true, 20, 20)
	
		local grid = GridLayout()
		local scrollPanel = UiScrollPanel()
		scrollPanel:SetWidget(grid)
		scrollPanel.color:set(0, 0, 0, 0)
		layout:AddChild(scrollPanel, 1, 10, 10, true, 15, 15)
		
		for _, v in pairs(savedList) do
			local ww = UiWidget(150, 150)
			ww.color:set(100, 100, 100, 100)
			grid:AddChild(ww, 5, 5, 10, 10)
			
			local layout = VBoxLayout()
			ww:AddChild(layout)
			
			ww = UiPolyIcon(g_iconFolder, true, 80, 45)
			layout:AddChild(ww, 1)
			
			-- ww = UiPolyIcon(g_iconFolder, true)
			-- layout:AddChild(ww, 1, Layout.ALIGN_LEFT|Layout.ALIGN_RIGHT|Layout.ALIGN_TOP|Layout.ALIGN_BOTTOM, 5, 5, 5, 5)
			
			--ww = UiPolyIcon(g_iconFolder)
			--layout:AddChild(ww, 1, 0, 5, 5, 5, 5)
			
			local t = UiText(v.name)
			layout:AddChild(t)
		end
	else
		local v = VBoxLayout()
		v:AddChild(UiText(_('未找到本地项目')), 1)
		local b = UiButton(100, 30, _('加载...'))
		b:bind_event(EVT.LEFT_DOWN, w, OnLoadButton)
		v:AddChild(b, nil, 10, 20, false, nil, 20)
		w:AddChild(v)
		return w
	end
	
	return w
end

local function OnCreateProjWndUpdate(w)
	local s = w.nameText
	if (s ~= w.nameInput.text) then
		local hint = nil
		s:set(w.nameInput.text)
		if (s:length() > 0) then
			if (s:find(' ') == 0) then
				hint = w.nameHint1
			elseif (s:length() > 0 and
				(s:find('\\') >= 0 or
				s:find('/') >= 0 or
				s:find(':') >= 0 or
				s:find('*') >= 0 or
				s:find('?') >= 0 or
				s:find('\"') >= 0 or
				s:find('<') >= 0 or
				s:find('>') >= 0 or
				s:find('|') >= 0)) then
				hint = w.nameHint2 
			end
		end
		if (hint) then
			w.hint:SetText(hint)
			w.hint:Show(true)
		else
			w.hint:Show(false)
		end
	end
	local loc = w.nameInput.location
	local rect = w.nameInput.rect
	w.hint:SetPos(loc.x, loc.y + rect.h + 10)
	local b = not hint and w.dirText:length() > 0 and s:length() > 0
	w.btnCreate:Enable(b)
end

local function OnCreateProjWndDirButton(w)
	local s = cTerminal.ChooseDirDialog('', '', true)
	if (s:length() > 0) then
		w.dirText = s
		w.dirButton.text:Show(false)
		w.dirButton.fzText:SetText(s)
	end
end

local function OnCreate()
	cEntrance:Accept()
end

function NewWindow_CreateProj()
	local w = Window()
	--w.UpdateBegin = OnCreateProjWndUpdate
	w.color:set(70, 70, 70, 255)
	w.dirText = LString('')
	w.nameText = LString('')
	w.finder = cTerminal.NewFileFinder()
	
	local v = VBoxLayout()
	w:AddChild(v)
	
	local h = HBoxLayout()
	v:AddChild(h, 1)
	
	-- h:AddChild(UiText(_('目录')))
	-- w.dirButton = UiButton(300, 30)
	-- w.dirButton.text:SetText('...')
	-- w.dirButton:SetDefaultColor(80, 80, 80, 255)
	-- w.dirButton:bind_event(EVT.LEFT_UP, w, OnCreateProjWndDirButton)
	-- w.dirButton.fzText = UiTextLabel(300)
	-- w.dirButton.fzText:EnableWriteId(false)
	-- w.dirButton.layout:AddChild(w.dirButton.fzText, 1, 0)
	
	-- h:AddChild(w.dirButton, nil, 10)

	-- h:AddChild(UiText(_('名称')), nil, 20)
	-- w.nameInput = UiTextInput(200, 30)
	-- h:AddChild(w.nameInput, nil, 10)		
	
	w.btnCreate = UiButton(100 ,30, _('新建'))
	--w.btnCreate:Enable(false)
	w.btnCreate:bind_event(EVT.LEFT_UP, nil, OnCreate)
	h:AddChild(w.btnCreate, nil, 5)
	
	--w.nameHint1 = _('项目名不能以空格开头')
	--w.nameHint2 = _('项目名不能包含下列字符：\\/:*?\"<>|')
	
	-- w.hint = UiText('')
	-- w.hint.color:set(255, 100, 100, 255)
	-- w.hint:Show(false)
	-- w:AddChild(w.hint)
	
	return w
end


PaneWindow = class(Window)
function PaneWindow:ctor()
	self.color:set(70, 70, 70, 255)
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

SceneWindow = class(PaneWindow)
function SceneWindow:ctor()
	self:EnableWriteId(false)
	self.drawSelf = false
	self.scene = ObjectScene()
	self.sceneView = SceneView(self.scene)
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
			local x0, y0 = x - self.mx, self.my - y
			local mView = self.sceneView.camera.mView
			if (o == c.arrowX) then
				local x1, y1, _ = mView:VectorTransform(1, 0, 0)
				c.attached:Move(Dot2D(x0, y0, Normalize2D(x1, y1)) / 20, 0, 0)
			elseif (o == c.arrowY) then
				local x1, y1, _ = mView:VectorTransform(0, 1, 0)
				c.attached:Move(0, Dot2D(x0, y0, Normalize2D(x1, y1)) / 20, 0)
			elseif (o == c.arrowZ) then
				local x1, y1, _ = mView:VectorTransform(0, 0, 1)
				c.attached:Move(0, 0, Dot2D(x0, y0, Normalize2D(x1, y1)) / 20)
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
	x, y, z = CScreenToViewPos(x, y, 45, self.rect.w, self.rect.h)
	x, y, z = CVec3NormalizeScale(x, y, z, 20)
	x, y, z = self.sceneView.camera.mWorld:PointTransform(x, y, z)
	m:SetPosition(x, y, z)
	return true
end

function SceneWindow:OnInnerDragEnter(x, y, id, data)
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
	
function SceneWindow:OnInnerDragging(x, y)
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

function SceneWindow:OnInnerDrop(x, y, id, data)
	self.dragging:EnableWriteId(true)
	g_previews[data] = Model(data)
	self:Refresh()
end

PresetsWindow = class(PaneWindow)
function PresetsWindow:ctor()
	local v = VBoxLayout()
	self:AddChild(v)
	local sp = UiScrollPanel()
	v:AddChild(sp, 1, 0, 0, true)
	self.vLayout = VBoxLayout()
	sp:SetWidget(self.vLayout)
	self:AddPresetItem(g_cube, _('立方体'))
	self:AddPresetItem(g_plane3d, _('平面'))
end

function PresetsWindow:AddPresetItem(item, text)
	local o = UiButton(0, 28)
	o.color2 = o.color1
	local icon = UiPolyIcon(g_iconPreset)
	icon:EnableWriteId(false)
	
	o.layout:AddChild(icon, nil, 7)
	o.text:SetText(text)
	o.layout:AddChild(o.text, 1, 7, 5)
	self.vLayout:AddChild(o, nil, 0, 0, true, 0, 0)
	
	o.item = item
	o:bind_event(EVT.LEFT_DOWN, self, PresetsWindow.OnItemLeftDown)
end

function PresetsWindow:OnItemLeftDown()
	self:Drag(PresetsWindow, EVT.obj.item)
end

function LoadEntrance()
	cEntrance:AddPageWindow('load_proj', _('加载项目'), NewWindow_LoadProj())
	cEntrance:AddPageWindow('new_proj', _('新建项目'), NewWindow_CreateProj())
end

scenePanelSet.panels.presets = PresetsWindow()
scenePanelSet.panels.presets.title = _('预设')
scenePanelSet.panels.viewport = SceneWindow()
scenePanelSet.panels.viewport.title = _('视口')
scenePanelSet.panels.hirachey = PaneWindow()
scenePanelSet.panels.hirachey.title = _('大纲')
scenePanelSet.panels.inspector = PaneWindow()
scenePanelSet.panels.inspector.title = _('细节')
scenePanelSet.panels.logMessage = PaneWindow()
scenePanelSet.panels.logMessage.title = _('日志消息')
scenePanelSet.panels.content = PaneWindow()
scenePanelSet.panels.content:AddChild(ContentPanel())
scenePanelSet.panels.content.title = _('内容')

local function SaveLoadLayout(set)
	return function (id)
		if (id == 1) then
			set.layout = set.nb:SaveLayout()
		elseif (id == 2) then
			set.nb:LoadLayout(set.layout)
		end
	end
end

local function ShowPanel(set)
	return function (id)
		set.nb:ShowPage(get_object(id))
	end
end

local function OnShowPanelOpen(set)
	return function ()
		for _, v in pairs(set.panels) do
			CMenu.Check(v.menuItem, set.nb:IsPageShown(v))
		end
	end
end

local function LoadPanelSetLayout(panelSet, menu)
	local nb = panelSet.nb
	for k, v in pairs(panelSet.panels) do
		nb:AddPage(k, v.title, v)
		v.menuItem = menu:AddCheckItem(v.id, v.title)
	end
	menu:BindOnOpen(OnShowPanelOpen(panelSet))
	nb:LoadLayout(panelSet.layout)
end

local function OnMainFrameClose(w)
	if (not g_projPath or modified) then
		local b = cTerminal:MessageDialog('', _('是否保存项目？'), _('保存'), _('不保存'), _('取消'))
		if (b) then
			return SaveProject()
		elseif (b == false) then
			return true
		end
		return false
	else
		return true
	end
end

function LoadMainFrame()
	cMainFrame:SetTitle(_('* 无标题'))
	cMainFrame.OnClose = OnMainFrameClose
	scenePanelSet.nb = cMainFrame:AddPageNotebook('scene', _('场景'))
	scenePanelSet.layout = 'notebook_layout0/1<presets>0|2<viewport>0|3<content*logMessage>0|4<hirachey>0|5<inspector>0|*|layout2|name=dummy;caption=;state=2098174;dir=3;layer=0;row=0;pos=0;prop=100000;bestw=225;besth=225;minw=225;minh=225;maxw=-1;maxh=-1;floatx=-1;floaty=-1;floatw=-1;floath=-1|name=1;caption=;state=2098172;dir=5;layer=0;row=0;pos=0;prop=100000;bestw=250;besth=250;minw=-1;minh=-1;maxw=-1;maxh=-1;floatx=-1;floaty=-1;floatw=-1;floath=-1|name=2;caption=;state=2098172;dir=2;layer=0;row=1;pos=0;prop=100000;bestw=540;besth=346;minw=-1;minh=-1;maxw=-1;maxh=-1;floatx=-1;floaty=-1;floatw=-1;floath=-1|name=3;caption=;state=2098172;dir=3;layer=1;row=0;pos=0;prop=100000;bestw=225;besth=225;minw=-1;minh=-1;maxw=-1;maxh=-1;floatx=-1;floaty=-1;floatw=-1;floath=-1|name=4;caption=;state=2098172;dir=2;layer=2;row=0;pos=0;prop=100000;bestw=225;besth=225;minw=-1;minh=-1;maxw=-1;maxh=-1;floatx=-1;floaty=-1;floatw=-1;floath=-1|name=5;caption=;state=2098172;dir=2;layer=2;row=0;pos=1;prop=100000;bestw=225;besth=225;minw=-1;minh=-1;maxw=-1;maxh=-1;floatx=-1;floaty=-1;floatw=-1;floath=-1|dock_size(5,0,0)=18|dock_size(2,0,1)=634|dock_size(3,1,0)=227|dock_size(2,2,0)=227|/'
	
	local mb = CMenuBar()
	-- local m = mb:Add('menu', SaveLoadLayout(scenePanelSet))
	-- m:AddItem(1, 'save')
	-- m:AddItem(2, 'load')
	
	m = mb:Add(_('面板'), ShowPanel(scenePanelSet))
	LoadPanelSetLayout(scenePanelSet, m)
	
	scenePanelSet.mb = mb
	cMainFrame:SetMenuBar(scenePanelSet.mb)
	
	if (1) then return end

	cMainFrame:AddPageWindow('presets', _('预设'), PaneWindow())
	
	cMainFrame:AddPageWindow('scene', _('场景'), SceneWindow())
	
	local w = PaneWindow()
	local layout = VBoxLayout()
	w:AddChild(layout)
	local cp = ContentPanel()
	cp:ScanDirectory()
	layout:AddChild(cp, 1, 0, 0, true)
	cMainFrame:AddPageWindow('content', _('内容'), w)
	
	cMainFrame:AddPageWindow('outline', '大纲', PaneWindow())
	
	w = PaneWindow()
	w:AddChild(FrameBufferPanel())
	cMainFrame:AddPageWindow('inspector', '细节', w)
end

function AppCleanUp()
	cGI:DeviceWaitIdle()
end

FileBrowser = class(UiWidget)
FileBrowser.drawSelf = false

function FileBrowser:ctor(w, h)
	self:SetSize(w, h)
	
	
	self.treeList = UiTreeList()
end

-- fo = {pass = {}}
-- fo.pass[0] = {w = 0, h = 0, ia_to = {}}
-- fo.pass[0][0] = rtv0
-- fo.pass[0][1] = rtv1
-- fo.pass[0].dsv = dsv0

-- fo.pass[1] = {w = 0, h = 0, ia_to = {}}
-- fo.pass[1][0] = rtv2
-- fo.pass[1][1] = rtv3
-- fo.pass[1].dsv = dsv1

-- fo[rtv0] = 0
-- fo[rtv1] = 0
-- fo[rtv2] = 1
-- fo[rtv3] = 1
-- fo.rel = {}
			








	