---Editor---
require 'PanelSet'
require 'ScenePanelSet'
require 'GeomPanelSet'

local savedList = {}
local savedListPath

local fileList = {}
local projModified = {}

local scenePanelSet
local panelSets = {}
local collect


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
	g_projPath = nil
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
	--local ssm = MtlSnapshot(100, 100)
	--self:AddChild(UiImage(ssm.view))
	
	--g_iconLine = AddPoly2D(true, DrawLine(5, false, false, {0, 0}, {100, 0}, {50, 100}, {366, 210}, {500, 710}))
	--self:AddChild(UiPolyIcon(g_iconLine), 100, 100)
	
	local r = 100
	circle = AddPoly2D(true, DrawLine(3, false, true, MakeCircle(0, 0, r, 32)))
	--self:AddChild(UiPolyIcon(circle), 100, 100)
	
	--local star = AddPoly2D(true, DrawLine(3, false, true, 100, ))
	--self:AddChild(UiPolyIcon(circle), 100, 100)
	
	local v = VBoxLayout()
	self:AddChild(v)
	
	local curve = {}
	local a, d = 50, 50
	for i = 0, 800, 2 do
		table.insert(curve, {i, a * math.sin(i / d * math.pi)})
	end
	v:AddChild(UiPolyIcon(AddPoly2D(true, DrawLine(3, false, false, curve))), nil, 100, 0, nil, 100)
	
	local curve2 = {}
	local a, d = 50, 50
	for i = 0, 600, 2 do
		table.insert(curve2, {i, a * math.sin(i / d * math.pi)})
	end
	
	for i = 600, 0, -2 do
		table.insert(curve2, {i, a * -math.sin(i / d * math.pi) + 150})
	end
	
	local icon = AddPoly2D(true, curve2)
	v:AddChild(UiPolyIcon(AddPoly2D(true, curve2)), nil, 100, 0, nil, 70)
	
	local icon = AddPoly2D(true, {{0, 125}, {100, 100}, {125, 0}, {150, 100}, {250, 125}, {150, 150}, {125, 250}, {100, 150}})
	self:AddChild(UiPolyIcon(icon), 700, 300)
	
	if (1) then return end
	
	local vLayout = VBoxLayout()
	self:AddChild(vLayout)
	
	savedListPath = cTerminal.currentPath .. 'saved'
	local f = CNewFileInput(false)
	local b, c = LoadFile(f, savedListPath, {}, true)
	local scrollPanel = UiScrollPanel()
	local n = 0
	if (b) then
		savedList = c or savedList
	
		local t = SearchInput()
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
	
	srlz_array[savedList] = true
	WriteTableToFile(savedList, savedListPath)
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
			srlz_array[savedList] = true
			WriteTableToFile(savedList, savedListPath)
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
		g_projPath = dir .. '\\'
		cTerminal.NewDirectory(g_projPath)
		
		table.insert(savedList, {path = g_projPath})
		srlz_array[savedList] = true
		WriteTableToFile(savedList, savedListPath)
	end
	g_proj.layout = cMainFrame:SaveLayout()
	g_proj.maximized = cMainFrame:IsMaximized()
	g_proj.w, g_proj.h = cMainFrame:GetSize()
	g_proj.sceneLayout = scenePanelSet.nb:SaveLayout()
	
	WriteTableToFile(g_proj, g_projPath .. 'project')
	
	local contentPath = g_projPath .. 'Content\\'
	local configPath = g_projPath .. 'Config\\'
	cTerminal.NewDirectory(contentPath)
	cTerminal.NewDirectory(configPath)
	
	if (scenePanelSet.modified) then
		local o = {}
		scenePanelSet:SaveProfile(o)
		srlz_array[o] = true
		WriteTableToFile(o, contentPath .. 'scene')
	end
	
	return true
end

function LoadEntrance()
	g_proj = {}
	g_recorder = Recorder()
	cEntrance:AddPageWindow('load_proj', _('加载项目'), LoadProjWindow())
	cEntrance:AddPageWindow('new_proj', _('新建项目'), CreateProjWindow())
end

function LoadMainFrame()
	if (not g_proj.name) then
		g_proj.name = _('未命名')
	else
		cMainFrame:SetTitle(g_proj.name)
	end
	cMainFrame.OnClose = OnMainFrameClose
	cMainFrame.OnPageChanged = OnPageChanged
	cMainFrame.OnPageDestroy = OnPageDestroy

	if (g_proj.maximized) then
		cMainFrame:Maximize(true)
	elseif (g_proj.w and g_proj.h) then
		cMainFrame:SetSize(g_proj.w, g_proj.h)
	end
	
	local nbScene = cMainFrame:AddPageNotebook('scene', _('场景'))
	scenePanelSet = ScenePanelSet(nbScene)
	scenePanelSet.nb:LoadLayout(g_proj.sceneLayout or ScenePanelSet.layout)
	if (g_projPath) then
		local contentPath = g_projPath .. 'Content\\'
		local configPath = g_projPath .. 'Config\\'
		local b, o = LoadFile(nil, contentPath .. 'scene', {}, true)
		if (b) then
			scenePanelSet:LoadProfile(o)
		else
			Print(o)
		end
	end
	cMainFrame:SetMenuBar(scenePanelSet.mb)
	g_recorder = scenePanelSet.recorder
	
	--local nbGeom = cMainFrame:AddPageNotebook('geom', _('geom'))
	--geomPanelSet = GeomPanelSet(nbGeom)
	
	--nb = cMainFrame:AddPageNotebook('test', 'test')
	--nb:AddPage('1', '1', Window())
	--nb:AddPage('2', '2', Window())
	if (g_proj.nbLayout) then
		--nb:LoadLayout(g_proj.nbLayout)
	end
	--cMainFrame:SetPageNotebookTabStyle(nbScene, CNotebook.NB_CLOSE_BUTTON, false)
	--cMainFrame:SetPageNotebookTabStyle(nb, CNotebook.NB_CLOSE_BUTTON, false)
	if (g_proj.layout) then
		--cMainFrame:LoadLayout(g_proj.layout)
	end
	--cMainFrame:SetMenuBar(scenePanelSet.mb)
end

function OnPageChanged(w)
	local ps = PanelSet[w]
	if (ps) then
		if (ps == scenePanelSet) then
			cMainFrame:SetNotebookStyleFlag(CNotebook.NB_CLOSE_ON_ACTIVE_TAB, false)
		else
			cMainFrame:SetNotebookStyleFlag(CNotebook.NB_CLOSE_ON_ACTIVE_TAB, true)
		end
		if (ps.mb) then
			cMainFrame:SetMenuBar(ps.mb)
		end
		g_recorder = ps.recorder
	end
end

function OnPageDestroy(w)
	collect = true
	PanelSet[w] = nil
end

function ProjModified(panelSet, flag)
	if (flag) then
		projModified[panelSet] = panelSet
		cMainFrame:SetPageNotebookTitle(panelSet.nb, '*'..panelSet.title)
	else
		projModified[panelSet] = nil
		cMainFrame:SetPageNotebookTitle(panelSet.nb, panelSet.title)
	end
	local _, v = next(projModified)
	if (v) then
		cMainFrame:SetTitle('*' .. g_proj.name)
	else
		cMainFrame:SetTitle(g_proj.name)
	end
end

function SaveAll()
	local k, v = next(projModified)
	while (v) do
	end
end

function OnMainFrameClose()
	local k, v = next(projModified)
	if (v or not g_projPath) then
		local b = cTerminal:MessageDialog('', _('是否保存项目？'), _('保存'), _('不保存'), _('取消'))
		if (b) then
			return SaveProject(g_projPath)
		elseif (b == false) then
			return true
		end
		return false
	elseif (g_projPath) then
		local layout = cMainFrame:SaveLayout()
		local sceneLayout = scenePanelSet.nb:SaveLayout()
		local maximized = cMainFrame:IsMaximized()
		local w, h = cMainFrame:GetSize()
		if (g_proj.layout ~= layout or
			g_proj.sceneLayout ~= sceneLayout or
			g_proj.maximized ~= maximized or
			g_proj.w ~= w or g_proj.h ~= h) then
			g_proj.layout = layout
			g_proj.sceneLayout = sceneLayout
			g_proj.maximized = maximized
			g_proj.w, g_proj.h = w, h
			WriteTableToFile(g_proj, g_projPath .. 'project')
		end
		return true
	end
end

function AppCleanUp()
	cGI:DeviceWaitIdle()
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
			








	