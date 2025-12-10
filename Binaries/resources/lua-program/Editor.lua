---Editor---
require 'window'
require 'presets'

PaneWindow = class(Window)
function PaneWindow:ctor()
	self.color:set(70, 70, 70, 255)
end

local savedList = {}
local savedListPath
local scenePanelSet = {panels = {}}

local fileList = {}
local content = Object()

local nameHintBegin = _('名称不能以空格开头')
local nameHintChar = _('名称不能包含下列字符：') .. '\\/:*?\"<>|'
local nameHintExist = _('名称已存在')

local function CheckName(s)
	if (s:length() > 0) then
		if (s:find(' ') == 0) then
			return nameHintSpace
		elseif (s:find('\\') >= 0 or
			s:find('/') >= 0 or
			s:find(':') >= 0 or
			s:find('*') >= 0 or
			s:find('?') >= 0 or
			s:find('\"') >= 0 or
			s:find('<') >= 0 or
			s:find('>') >= 0 or
			s:find('|') >= 0) then
			return nameHintChar
		end
	end
end

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
	
	--local t = UiTextInput(0, uiFont.maxHeight)
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

---ContentWindow---
local CONT_FOLDER = 1
local icons = {}
icons[CONT_FOLDER] = g_iconFolder

Folder = class()

function Folder:ctor()
	self.files = {}
	self.folders = {}
end
function Folder:Add(o)
	if (o.type == CONT_FOLDER) then
		table.insert(self.folders, o)
	else
		table.insert(self.files, o)
	end
end

function Folder:Remove(o)
	local t
	if (o.type == CONT_FOLDER) then
		t = self.folders
	else
		t = self.files
	end
	for i, v in pairs(t) do
		if (v == o) then
			table.remove(t, i)
		end
	end
end

content.folder = Folder()
content.EVT = EVT.new()

function content:Update(panel, folder)
	content:process_event(content.EVT, panel, folder or panel.curFolder)
end

local itemMenus = {}
local m = CMenu()
m:AddItem(1, _('重命名'))
m:AddItem(2, _('删除'))
itemMenus[CONT_FOLDER] = m

local contentMenu = CMenu()
contentMenu:AddItem(1, _('新建文件夹'))
contentMenu:AddItem(2, _('新建FrameBuffer'))

ContentWindow = class(PaneWindow)

function ContentWindow:ctor()
	local hsLayout = HSizerLayout()
	hsLayout:SetBarDftColor(70, 70, 70, 255)
	self:AddChild(hsLayout)
	
	self.tree = UiTreeList()
	local o = hsLayout:AddChild(UiScrollPanel(self.tree), 1, 0, 0, true)
	o.limit = 300
	
	local mag = UiPolyIcon(g_iconMagnifier)
	self.searcher = UiTextInput(0, uiFont.maxHeight)
	local h = HBoxLayout()
	h:AddChild(mag, nil, 5, 0, false)
	h:AddChild(self.searcher, 1, 7, 12, false)
	h:SetSize()
	
	local v = VBoxLayout()
	v:AddChild(h, nil, 5, 0, true)
	
	self.grid = GridLayout()
	local vv = VBoxLayout()
	vv:AddChild(self.grid, nil, 0, 0, true, 5, 5)
	local pane = UiScrollPanel(vv)
	pane.acceptFile = true
	pane:EnableDrop(PresetsWindow, true)
	pane:bind_event(EVT.RIGHT_UP, self, self.OnMouse)
	pane:bind_event(EVT.INNER_DROP, self, self.OnPresetDrop)
	pane:bind_event(EVT.FILE_DROP, self, self.OnDropFile)
	v:AddChild(pane, 1, 8, 0, true)
	self.pane = pane
	self.selector = Selector()
	self.selector.showFocusOut = true
	self.selector:SetFocusOutColor(0, 0, 0, 0)
	self.curPath = ''
	
	self.color:copy(self.pane.color)
	
	local w = UiWidget()
	w.gpuClip = true
	w.drawSelf = false
	w:AddChild(v)
	
	hsLayout:AddChild(w, 3, 0, 0, true)
	
	--self.menu:AddItem(1, _('新建场景'))
	--self.menu:AddItem(2, _('新建UI'))
	--local m = self.menu:AddSubMenu(_('Sub'))
	--m:AddItem(3, 'zz')
	self.nameHint = UiText()
	self.nameHint.drawClipRect = true
	self.nameHint.crColor:set(160, 0, 0, 255)
	self.nameHint:Show(false)
	self:AddChild(self.nameHint)
	
	self.curFolder = content.folder
	content:bind_event(content.EVT, self, self.OnContentUpdated)
end

function ContentWindow:OnPresetDrop(e, id, geom)
	
end

function ContentWindow:OnDropFile(e, files)
	for _, f in pairs(files) do
		Print(f)
	end
end

function ContentWindow:OnContentUpdated(e, panel, folder)
	if (panel ~= self and folder == self.curFolder) then
		self.grid:ClearChildren()
		self.selector = Selector()
		self.selector.showFocusOut = true
		self.selector:SetFocusOutColor(0, 0, 0, 0)
		for _, item in pairs(folder.folders) do
			self:AddItem(item)
		end
	end
end

function ContentWindow:OnTextFocusOut(e)
	local item = e.obj.item
	item.input:Show(false)
	item.name:Show(true)
	if (item.nameNew) then
		item.name:SetText(item.input.text)
		item.meta.name = item.input.text:utf8()
	else
		item.input:SetText(item.name.text)
	end
	self.nameHint:Show(false)
	
	content:Update(self)
end

function ContentWindow:OnTextKeyDown(e, k)
	if (k == SYS.VK_RETURN) then
		self:OnTextFocusOut(e)
	end
end

function ContentWindow:OnText(e)
	self.nameHint:Show(false)
	local item = e.obj.item
	item.nameNew = true
	local s = item.input.text
	local hint = CheckName(s)
	if (hint) then
		item.nameNew = false
		local x = item.icon.location.x
		local y = item.input.location.y
		local h = item.input.rect.h
		self.nameHint:SetText(hint)
		self.nameHint:SetPos(x, y + h + 2)
		self.nameHint:Show(true)
	elseif (s:length() == 0) then
		item.nameNew = false
	end
end

function ContentWindow:AddItem(o)
	local item = self.selector:Add(100, 80, {})
	item:bind_event(EVT.RIGHT_UP, self, self.OnItemMenu)
	self.grid:AddChild(item, 0, 0, 5, 10)
	
	local layout = VBoxLayout()
	item:AddChild(layout)
	
	item.meta = o
	item.menu = itemMenus[o.type]
	item.icon = UiPolyIcon(icons[o.type], true)
	item.icon:EnableWriteId(false)
	layout:AddChild(item.icon, 1, 10, 10, true, 20, 20)
	
	item.input = UiTextInput(0, uiFont2.maxHeight, o.name, uiFont2)
	item.input:Show(false)
	item.input:FixTextSize(true, 80)
	item.input:bind_event(EVT.FOCUS_OUT, self, self.OnTextFocusOut)
	item.input:bind_event(EVT.KEY_DOWN, self, self.OnTextKeyDown)
	item.input:bind_event(UiTextInput.EVT, self, self.OnText)
	item.input.item = item
	layout:AddChild(item.input) 
	
	item.name = UiTextLabel(80, o.name, uiFont2)
	layout:AddChild(item.name)
	return item
end

function ContentWindow:OnItemMenu(e)
	local item = e.obj
	local id = item.menu:Popup(self)
	if (not id) then
	return end
	
	if (item.meta.type == CONT_FOLDER) then
		if (id == 1) then
			item.name:Show(false)
			item.input:Show(true)
			item.input:SelectAll()
			item.input:SetFocus(true)
		elseif (id == 2) then
			self.grid:RemoveChild(item)
			self.selector:Remove(item)
			self.curFolder:Remove(item.meta)
			content:Update(self)
		end
	end
	self.item = item
end

function ContentWindow:OnMouse(e)
	if (e == EVT.RIGHT_UP) then
		local id = contentMenu:Popup(self)
		if (not id) then return end
		
		local o = {}
		if (id == 1) then
			o.type = CONT_FOLDER
			o.name = _('新建文件夹')
			self.curFolder:Add(o)
		end
		local item = self:AddItem(o)
		item.input:SelectAll()
		item.input:Show(true)
		item.name:Show(false)
		self:SetFocus(item.input, true)
	else
	end
end

function ContentWindow:ScanDirectory(d)
	local n = self.list:AddNode(nil, g_iconFolder, 'main')
	n = self.list:AddNode(n, g_iconFolder, 'sub')
	self.list:AddNode(n, g_iconFolder, 'sub111111111111111')
end

---CreateProjWindow---
CreateProjWindow = class(Window)
function CreateProjWindow:ctor()
	self.color:set(50, 50, 50, 255)
	self.dirText = LString('')
	self.nameText = LString('')
	self.finder = cTerminal.NewFileFinder()
	
	local v = VBoxLayout()
	self:AddChild(v)
	
	local h = HBoxLayout()
	v:AddChild(h, 1)
	
	self.btnCreate = UiButton(100 ,30, _('新建'))
	self.btnCreate:SetDefaultColor(70, 70, 70, 255)
	self.btnCreate:bind_event(EVT.LEFT_UP, self, self.OnCreate)
	h:AddChild(self.btnCreate, nil, 5)
end

function CreateProjWindow:OnCreate()
	cEntrance:Accept()
end

---LoadProjWindow---
LoadProjWindow = class(Window)

function LoadProjWindow:ctor()
	self.color:set(50, 50, 50, 255)
	self.selector = Selector()
	self.selector:bind_event(Selector.EVT_CHANGED, self, self.OnSelection)
	
	--self:AddChild(UiPolyIcon(g_iconGeom), 100, 100)
	--self:AddChild(UiPolyIcon(g_iconUnsaved), 100, 100)
	--local z = UiText('斑鸠')
	--z.cpuClip = false
	--self:AddChild(z, 100, 100)
	--self:AddChild(UiPolyIcon(g_iconLNavi), 100, 100)
	--self:AddChild(UiPolyIcon(g_iconRNavi), 100 + 30, 100)
	--if (1) then return end
	
	local vLayout = VBoxLayout()
	self:AddChild(vLayout)
	
	savedListPath = cTerminal.currentPath .. 'saved'
	local f = CNewFileInput(false)
	local b, c = LoadFile(f, savedListPath, {}, true)
	local scrollPanel = UiScrollPanel()
	local n = 0
	if (b) then
		savedList = c or savedList
	
		local t = UiTextInput(0, uiFont.maxHeight)
		vLayout:AddChild(t, nil, 20, 10, true, 20, 20)
	
		local grid = GridLayout()
		scrollPanel:SetWidget(grid)
		scrollPanel.color:set(0, 0, 0, 0)
		
		for k, v in pairs(savedList) do
			local b, c = LoadFile(f, v.path .. 'project', {}, true)
			if (b) then
				local ww = self.selector:Add(150, 150, {o = c, path = v.path})
				grid:AddChild(ww, 5, 5, 10, 10) 
				
				local layout = VBoxLayout()
				ww:AddChild(layout)
				
				ww = UiPolyIcon(g_iconFolder, true, 80, 45)
				ww:EnableWriteId(false)
				layout:AddChild(ww, 1)
				
				local t = UiTextLabel(50, c.name)
				t:EnableWriteId(false)
				layout:AddChild(t)
				n = n + 1
			else
				savedList[k] = nil
				Print(c)
			end
		end
	end
	
	local hLayout = HBoxLayout()
	local btn = UiButton(100, 30, _('浏览...'))
	btn:bind_event(EVT.LEFT_UP, self, self.OnBrowse)
	btn:SetDefaultColor(70, 70, 70, 255)
	hLayout:AddChild(btn)
	
	if (n > 0) then
		vLayout:AddChild(scrollPanel, 1, 10, 10, true, 15, 15)
		self.btn = UiButton(100, 30, _('加载'))
		self.btn:SetDefaultColor(70, 70, 70, 255)
		self.btn:bind_event(EVT.LEFT_UP, self, self.OnLoad)
		hLayout:AddChild(self.btn, nil, 20)
		self.btn:Enable(false)
	else
		vLayout:AddChild(UiText(_('未找到本地项目')), 1)
	end
	
	vLayout:AddChild(hLayout, nil, 10, 20, false, nil, 20)
	
	WriteTableToFile(savedList, true, savedListPath)
end

function LoadProjWindow:OnSelection()
	local o = self.selector:GetSelection()
	self.btn:Enable(o ~= nil)
	if (o.o) then
		g_proj = o.o
		g_projPath = o.path
	end
end

function LoadProjWindow:OnBrowse()
	local s = cTerminal.OpenFileDialog(_('加载项目'), '', 'project')
	if (s:length() > 0) then
		local f = CNewFileInput(false)
		local b, o = LoadFile(f, s, {}, true)
		if (not b) then
			Print(o)
		return end
		
		s:erase(s:rfind('\\') + 1, -1)
		g_projPath = s:utf8()
		
		local new = true
		for _, v in pairs(savedList) do
			if (v.path == g_projPath) then
				new = false
				break
			end
		end
		if (new) then
			table.insert(savedList, {path = g_projPath})
			WriteTableToFile(savedList, true, savedListPath)
		end
		
		g_proj = o
		self:OnLoad()
	end
end

function LoadProjWindow:OnLoad()
	cEntrance:Accept()
end


local function SaveProject(dir)
	if (not dir) then
		dir = cTerminal.NewFileDialog('', '', '')
		if (dir:length() == 0) then
			return false
		end
		
		local name = dir:substr(dir:rfind('\\') + 1, -1)
		g_proj.name = name
		g_proj.panelLayout = scenePanelSet.nb:SaveLayout()
		g_proj.maximized = cMainFrame:IsMaximized()
		g_proj.w, g_proj.h = cMainFrame:GetSize()
		
		g_projPath = dir .. '\\'
		cTerminal.NewDirectory(g_projPath)
		WriteTableToFile(g_proj, false, g_projPath .. 'project')
		cTerminal.NewDirectory(g_projPath .. 'Content')
		cTerminal.NewDirectory(g_projPath .. 'Config')
		
		table.insert(savedList, {path = g_projPath})
		WriteTableToFile(savedList, true, savedListPath)
	end
	return true
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
	local o = e.obj
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
	self:Drag(PresetsWindow, e.obj.item)
end

function LoadEntrance()
	g_proj = {}
	cEntrance:AddPageWindow('load_proj', _('加载项目'), LoadProjWindow())
	cEntrance:AddPageWindow('new_proj', _('新建项目'), CreateProjWindow())
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
scenePanelSet.panels.content = ContentWindow()
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
	elseif (g_projPath) then
		local layout = scenePanelSet.nb:SaveLayout()
		local maximized = cMainFrame:IsMaximized()
		local w, h = cMainFrame:GetSize()
		if (g_proj.panelLayout ~= layout or 
			g_proj.maximized ~= maximized or
			g_proj.w ~= w or g_proj.h ~= h) then
			g_proj.panelLayout = layout
			g_proj.maximized = maximized
			g_proj.w = w
			g_proj.h = h
			WriteTableToFile(g_proj, false, g_projPath .. 'project')
		end
		return true
	end
end

function LoadMainFrame()
	local name = g_proj.name
	local layout = g_proj.panelLayout or 'notebook_layout0/1<presets>0|2<viewport>0|3<content*logMessage>0|4<hirachey>0|5<inspector>0|*|layout2|name=dummy;caption=;state=2098174;dir=3;layer=0;row=0;pos=0;prop=100000;bestw=225;besth=225;minw=225;minh=225;maxw=-1;maxh=-1;floatx=-1;floaty=-1;floatw=-1;floath=-1|name=1;caption=;state=2098172;dir=5;layer=0;row=0;pos=0;prop=100000;bestw=250;besth=250;minw=-1;minh=-1;maxw=-1;maxh=-1;floatx=-1;floaty=-1;floatw=-1;floath=-1|name=2;caption=;state=2098172;dir=2;layer=0;row=1;pos=0;prop=100000;bestw=540;besth=346;minw=-1;minh=-1;maxw=-1;maxh=-1;floatx=-1;floaty=-1;floatw=-1;floath=-1|name=3;caption=;state=2098172;dir=3;layer=1;row=0;pos=0;prop=100000;bestw=225;besth=225;minw=-1;minh=-1;maxw=-1;maxh=-1;floatx=-1;floaty=-1;floatw=-1;floath=-1|name=4;caption=;state=2098172;dir=2;layer=2;row=0;pos=0;prop=100000;bestw=225;besth=225;minw=-1;minh=-1;maxw=-1;maxh=-1;floatx=-1;floaty=-1;floatw=-1;floath=-1|name=5;caption=;state=2098172;dir=2;layer=2;row=0;pos=1;prop=100000;bestw=225;besth=225;minw=-1;minh=-1;maxw=-1;maxh=-1;floatx=-1;floaty=-1;floatw=-1;floath=-1|dock_size(5,0,0)=18|dock_size(2,0,1)=634|dock_size(3,1,0)=227|dock_size(2,2,0)=227|/'
	cMainFrame:SetTitle(name or _('未命名') .. '*')
	cMainFrame.OnClose = OnMainFrameClose
	scenePanelSet.nb = cMainFrame:AddPageNotebook('scene', _('场景'))
	scenePanelSet.layout = layout
	if (g_proj.maximized) then
		cMainFrame:Maximize(true)
	elseif (g_proj.w and g_proj.h) then
		cMainFrame:SetSize(g_proj.w, g_proj.h)
	end
	
	local mb = CMenuBar()
	-- local m = mb:Add('menu', SaveLoadLayout(scenePanelSet))
	-- m:AddItem(1, 'save')
	-- m:AddItem(2, 'load')
	
	m = mb:Add(_('面板'), ShowPanel(scenePanelSet))
	LoadPanelSetLayout(scenePanelSet, m)
	
	scenePanelSet.mb = mb
	cMainFrame:SetMenuBar(scenePanelSet.mb)
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
			








	