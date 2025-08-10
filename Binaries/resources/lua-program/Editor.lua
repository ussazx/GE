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
	
	layout:AddChild(UiTextInput(0, uiFont.fontSize), nil, 20, 10, true, 20, 20)
	
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
	
	local ww = Scene3D(w.cmd)
	layout:AddChild(ww, 1, 10, 10, true, 10, 10)
	
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
	
	local bkg = UiWidget(0, h.rect.h + 8)
	bkg.gpuClip = true
	bkg.color:copy(self.searcher.crColor)
	bkg:AddChild(h)
	
	local v = VBoxLayout()
	v:AddChild(bkg, nil, 0, 0, true)
	
	self.grid = GridLayout()
	v:AddChild(UiScrollPanel(self.grid), 1, 8, 0, true)
	
	self:AddChild(v, 3, 0, 0, true)
end

function ContentPanel:ScanDirectory(d)
	local n = self.list:AddNode(nil, g_iconFolder, 'main')
	n = self.list:AddNode(n, g_iconFolder, 'sub')
	self.list:AddNode(n, g_iconFolder, 'sub111111111111111')
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

local function PaneWindow()
	local w = Window()
	w.color:set(70, 70, 70, 255)
	return w
end

function LoadEntrance()
	cEntrance:AddPageWindow('load_proj', 'Load Project', NewWindow_LoadProj())
	cEntrance:AddPageWindow('new_proj', 'New Project', NewWindow_CreateProj())
end

function LoadMainFrame()
	cMainFrame:AddPageWindow('page0', 'page0', PaneWindow())
	
	local w = PaneWindow()
	local layout = VBoxLayout()
	w:AddChild(layout)
	local cp = ContentPanel()
	cp:ScanDirectory()
	layout:AddChild(cp, 1, 0, 0, true)
	cMainFrame:AddPageWindow('page1', 'page1', w)
	
	cMainFrame:AddPageWindow('page2', 'page2', PaneWindow())
	
	w = PaneWindow()
	w:AddChild(FrameBufferPanel())
	cMainFrame:AddPageWindow('panel', 'page3', w)
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
			








	