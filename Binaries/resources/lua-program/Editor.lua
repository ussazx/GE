---Editor---
require 'window'

function WindowRecord(w, redo, o)
	if (redo) then
		w:AddChild(o)
	else
		w:RemoveChild(o)
	end
end

function GridLayoutTest(w)
	local layout = VBoxLayout()
	w:AddChild(layout)
	
	local t = UiTextInput(0, uiFont.fontSize)
	layout:AddChild(t, nil, 20, 10, true, 20, 20)
	
	if (1) then return end
	
	-- local sb = UiSlideBar(nil, false, 0, 0, 0, 20)
	-- sb:SetScale(5, 1)
	-- layout:AddChild(sb, 0, Layout.ALIGN_LEFT|Layout.ALIGN_RIGHT|Layout.ALIGN_TOP|Layout.ALIGN_BOTTOM, 20, 20, 20, 10)
	
	
	local grid = GridLayout()
	local scrollPanel = UiScrollPanel()
	scrollPanel:SetWidget(grid)
	layout:AddChild(scrollPanel, 1, 10, 10, true, 10, 10)
	for i = 1, 100 do
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

function OnCreateProj()
	local path = cTerminal.NewFileDialog(_('Create Project'), _("new"), '')
	if (path:length() == 0) then
		return
	end
	cTerminal.NewDirectory(path)
	local name = path .. path:substr(path:rfind('\\'), -1) .. '.proj'
	local f = CNewFileOutput()
	f:Open(name, true)
	--f:WriteUtf8('')
	f:Close()
	cTerminal.NewDirectory(path .. '\\Assets')
	cTerminal.NewDirectory(path .. '\\Configs')
	cEntrance:Accept()
	g_projPath = LString(path)
end

function LoadProject()
	--LoadAssets(g_projPath)
end

function LoadAssets(path)
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
				LoadAssets(path .. '\\' .. name)
			end
		elseif (name:rfind('.xasset') ~= -1) then
			CLoadAsset(path .. '\\' .. name)
		end
		found = f:FindNext()
	end
	--// Open windows
	
end

SceneView = class(Scene3D)
function SceneView:ctor(scene)
	self.scene = scene
	self.focus = SceneObject(scene)
	self.focus:Move(0, 5, 0)
	self.camera:Attach(self.focus)
	self.camera:Move(0, 0, -10)

	self:bind_event(EVT.LEFT_DOWN, self, SceneView.OnSceneMouse)
	self:bind_event(EVT.LEFT_UP, self, SceneView.OnSceneMouse)
	self:bind_event(EVT.MOTION, self, SceneView.OnSceneMouse)
	self:bind_event(EVT.MIDDLE_DOWN, self, SceneView.OnSceneMouse)
	self:bind_event(EVT.MOUSEWHEEL, self, SceneView.OnSceneMouse)
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
	local m = self.focus
	m:MoveLocal(x, y, z)
end

function SceneView:OnSceneMouse(e, x, y, w, m)
	if (e == EVT.LEFT_DOWN) then
		g_actWindow:CaptureMouse(self)
		self.mx = x
		self.my = y
		self.md = true
	elseif (e == EVT.LEFT_UP) then
		if (g_actWindow.captured == self) then
			g_actWindow:ReleaseCaptured()
			self.md = false
		elseif (g_sceneModel) then
		
		end
	elseif (e == EVT.MOTION) then
		if (self.md) then
			self:RotateView(x - self.mx, y - self.my)
		elseif (m) then
			self:MoveView(self.mx - x, y - self.my, 0)
		end
		self.mx = x
		self.my = y
	elseif (e == EVT.MIDDLE_DOWN) then
			self.mx = x
			self.my = y
	elseif (e == EVT.MOUSEWHEEL) then
		self:MoveView(0, 0, w)
	end
	self:Refresh()
end

function NewWindow_CreateProj()
	local w = Window()
	w.name = 'create'
	w.color:set(70, 70, 70, 255)
	
	local layout = VSizerLayout()
	w:AddChild(layout)
	
	local b = UiButton(100 ,30, _('Create'))
	b:bind_event(EVT.LEFT_UP, nil, OnCreateProj)
	layout:AddChild(b, nil, 10, 10, false, 10)
	
	-- local ww = UiWidget()
	-- ww.color:set(40, 40, 40, 255)
	-- layout:AddChild(ww, 1, 10, 10, true, 10, 10)
	
	-- local combo = UiCombo()
	-- combo:AddItem('zzzz')
	-- combo:AddItem('12345')
	-- combo:AddItem('zzzzzzzzzzzzzzzzzzzzzzzzzzzzz')
	-- combo:AddItem('zzzz')
	-- combo:AddItem('zzzz')
	-- combo:SetDefault(5)
	-- combo:ShowOutline(true, Color(150, 150, 150, 255))
	-- ww:AddChild(combo, 100, 100)
	
	--local ww = NewSceneViewport(w)
	--layout:AddChild(ww, 1, 10, 10, true, 10, 10)
	
	local cp = ContentPanel()
	layout:AddChild(cp, 1, 0, 10, true, 10, 10)
	
	cp:ScanDirectory()
	
	--local vs = HSizerLayout()
	--local w0 = UiWidget(200, 100)
	--w0.color:set(150, 150, 150, 100)
	--local w1 = UiWidget(200, 100)
	--w1.color:set(40, 40, 40, 255)
	--local w2 = UiWidget(200, 100)
	-- w2.color:set(40, 40, 40, 255)
	-- vs:AddChild(t, 1, Layout.ALIGN_LEFT|Layout.ALIGN_RIGHT|Layout.ALIGN_TOP|Layout.ALIGN_BOTTOM)
	-- vs:AddChild(w1, nil, Layout.ALIGN_LEFT|Layout.ALIGN_RIGHT|Layout.ALIGN_TOP|Layout.ALIGN_BOTTOM)
	-- vs:AddChild(w2, 1, Layout.ALIGN_LEFT|Layout.ALIGN_RIGHT|Layout.ALIGN_TOP|Layout.ALIGN_BOTTOM)
	-- layout:AddChild(vs, 1, Layout.ALIGN_LEFT|Layout.ALIGN_RIGHT|Layout.ALIGN_TOP|Layout.ALIGN_BOTTOM)
	
	return w
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

function NewWindow_LoadProj()
	local w = Window()
	w.name = 'load'
	w.color:set(40, 40, 40, 255)
	--t0:bind_event(EVT.TIMER, t0, t0.Func)
	--t1:bind_event(EVT.TIMER, t1, t1.Func)
	--t0:Start(w, 300, true)
	--t1:Start(w, 10, true)
	
	--VLayoutTest(w)
	--HLayoutTest(w)
	GridLayoutTest(w)
	--w:AddChild(UiWidget(150, 130), 0, 0)
	--w:AddChild(UiWidget(150, 130), 200, 0)
	
	--w:AddChild(UiTextInput(100, uiFont.fontSize), 10, 10)
	--w:AddChild(UiTextInput(100, uiFont.fontSize), 10, 100)
	
	--w:AddChild(UiText('abcdef'), 0, 0)
	--w:AddChild(UiText('abcdef'), 100, 0)
	--w:AddChild(UiText('abcdef'), 200, 0)
	
	--w:AddChild(UiPolyIcon(g_iconLine), 200, 200)
	--w:AddChild(UiPolyIcon(g_iconMagnifier), 200, 100)
	
	w.OnLeftDown = WindowOnLeftDown
	--w:bind_event(EVT.LEFT_DOWN, w, w.OnLeftDown)
	
	return w
end


PaneWindow = class(Window)
function PaneWindow:ctor()
	self.color:set(70, 70, 70, 255)
end

SceneWindow = class(PaneWindow)
function SceneWindow:ctor()
	self.drawSelf = false
	self.scene = SceneObject()
	self.sceneView = SceneView(self.scene)
	self.sceneView.fColor:set(0.25, 0.25, 0.25, 1)
	self.sceneView:bind_event(EVT.KEY_DOWN, self, SceneWindow.OnKeyDown)
	
	local v = VBoxLayout()
	v:AddChild(self.sceneView, 1, 0, 0, true)
	self:AddChild(v)
	self:EnableDrop(PresetsWindow, true)
	
	self.grid = Model(g_grid3d)
	self.grid:Attach(self.scene)
	
	Model(g_w3d):Attach(self.scene)
	
	self.o = Model(g_cube)
	self.o:Attach(self.scene)
	
	self.o1 = Model(g_cube)
	self.o1:Move(-2.5, 0, 0)
	self.o1:Attach(self.scene)
	self.a = true
	
	--local w = UiPolyIcon(g_iconLine1)
	local w = UiText('abcdef')
	w:EnableWriteId(false)
	self.w = w
	w.mRoot = CMatrix3D()
	w.mtl = Material(g_mtlUi2)
	w.mtl.resModel = ResourceHub(g_rlUB)
	local buf = w.mtl.resModel:BindResBuffer(0, CMatrix3D._size)
	CAddMatrix(w.mRoot, buf(), buf[1])
	w.renderer:SetMaterial(w.mtl)
	
	--self.scene:AddChild(w, 100, 50)
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
			self.o1:Attach(self.o, self.o1.ATTACH_ROT_AFFECT_POS_ROT, self.o1.ATTACH_WORLD)
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

function SceneWindow:Render(...)
	local x, y, z = self.sceneView.camera.mWorld:GetPosition()
	--self.grid.mRoot:SetPosition(x, 0, z)
	return SceneWindow._base.Render(self, ...)
end

function SceneWindow:UpdateDragging(x, y, m)
	if (self.x == x and self.y == y and self.dragging == m) then
		return false
	end
	self.x, self.y, self.dragging = x, y, m
	local z
	x, y, z = CScreenToViewPos(x, y, 45, self.rect.w, self.rect.h)
	x, y, z = CVec3NormalizeScale(x, y, z, 20)
	x, y, z = CVec3Transform(x, y, z, self.sceneView.camera.mWorld)
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

local scenePanelSet = {panels = {}}
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

function LoadMainFrame()
	scenePanelSet.nb = cMainFrame:AddPageNotebook('scene', _('场景'))
	scenePanelSet.layout = 'notebook_layout0/1<presets>0|2<viewport>0|3<content*logMessage>0|4<hirachey>0|5<inspector>0|*|layout2|name=dummy;caption=;state=2098174;dir=3;layer=0;row=0;pos=0;prop=100000;bestw=225;besth=225;minw=225;minh=225;maxw=-1;maxh=-1;floatx=-1;floaty=-1;floatw=-1;floath=-1|name=1;caption=;state=2098172;dir=5;layer=0;row=0;pos=0;prop=100000;bestw=250;besth=250;minw=-1;minh=-1;maxw=-1;maxh=-1;floatx=-1;floaty=-1;floatw=-1;floath=-1|name=2;caption=;state=2098172;dir=2;layer=0;row=1;pos=0;prop=100000;bestw=540;besth=346;minw=-1;minh=-1;maxw=-1;maxh=-1;floatx=-1;floaty=-1;floatw=-1;floath=-1|name=3;caption=;state=2098172;dir=3;layer=1;row=0;pos=0;prop=100000;bestw=225;besth=225;minw=-1;minh=-1;maxw=-1;maxh=-1;floatx=-1;floaty=-1;floatw=-1;floath=-1|name=4;caption=;state=2098172;dir=2;layer=2;row=0;pos=0;prop=100000;bestw=225;besth=225;minw=-1;minh=-1;maxw=-1;maxh=-1;floatx=-1;floaty=-1;floatw=-1;floath=-1|name=5;caption=;state=2098172;dir=2;layer=2;row=0;pos=1;prop=100000;bestw=225;besth=225;minw=-1;minh=-1;maxw=-1;maxh=-1;floatx=-1;floaty=-1;floatw=-1;floath=-1|dock_size(5,0,0)=18|dock_size(2,0,1)=634|dock_size(3,1,0)=227|dock_size(2,2,0)=227|/'
	
	local mb = CMenuBar()
	local m = mb:Add('menu', SaveLoadLayout(scenePanelSet))
	m:AddItem(1, 'save')
	m:AddItem(2, 'load')
	
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
			








	