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
	local layout = BoxLayout(true)
	w:AddChild(layout)
	
	layout:AddChild(UiTextInput(0, 0, 0, uiFont.fontSize), 0, Layout.ALIGN_LEFT|Layout.ALIGN_RIGHT|Layout.ALIGN_TOP|Layout.ALIGN_BOTTOM, 20, 20, 20, 10)
	
	-- local sb = UiSlideBar(nil, false, 0, 0, 0, 20)
	-- sb:SetScale(5, 1)
	-- layout:AddChild(sb, 0, Layout.ALIGN_LEFT|Layout.ALIGN_RIGHT|Layout.ALIGN_TOP|Layout.ALIGN_BOTTOM, 20, 20, 20, 10)
	
	
	local grid = GridLayout()
	local scrollPanel = UiScrollPanel(grid)
	layout:AddChild(scrollPanel, 1, Layout.ALIGN_LEFT|Layout.ALIGN_RIGHT|Layout.ALIGN_TOP|Layout.ALIGN_BOTTOM, 10, 10, 10, 10)
	for i = 1, 100 do
		local ww = UiWidget(0, 0, 150, 150)
		ww.color:set(100, 100, 100, 100)
		grid:AddChild(ww, 5, 5, 10, 10)
		--ww.gpuClip = true
		
		local layout = BoxLayout(true)
		ww:AddChild(layout)
		
		ww = UiWidget(0, 0, 150, 130)
		ww.color:set(90, 90, 90, 100)
		layout:AddChild(ww, 1, Layout.ALIGN_LEFT|Layout.ALIGN_RIGHT|Layout.ALIGN_TOP|Layout.ALIGN_BOTTOM, 5, 5, 5, 5)
		local t = UiText(0, 0, 'abcdef')
		layout:AddChild(t)
	end
	local layoutBottom = BoxLayout()
	w.idle_cost = 0
	w.idleText = UiText(0, 0, '--')
	layoutBottom:AddChild(w.idleText, 0, 0, 0, 0, 0, 0)
	layoutBottom:AddChild(UiButton(0, 0, 100, 30, _('Load')), 1, Layout.ALIGN_RIGHT, 0, 0, 0, 0)
	layout:AddChild(layoutBottom, 0, Layout.ALIGN_LEFT|Layout.ALIGN_RIGHT|Layout.ALIGN_TOP|Layout.ALIGN_BOTTOM, 10, 10, 10, 10)
	--layout:AddChild(UiButton(0, 0, 100, 30, _('Load')), 0, Layout.ALIGN_RIGHT|Layout.ALIGN_TOP|Layout.ALIGN_BOTTOM, 0, 10, 10, 10)
end

function NewCommonWindow()
	local w = Window()
end

function OnCreateProj()
	local s = cTerminal:NewFileDialog(_('Create Project'), _("new"), '')
	if (s == nil or s == '') then
		return
	end
	cTerminal:NewDirectory(s)
	g_projLoc = s
end

function LoadProj(path, isBin)
	-- local o = LoadLuaFile(path, isBin)
	-- if (o) then
		-- o = o() or {}
		
	-- else
		-- Print('error')
	-- end
	
	--// parse content and load assets
	
	--// Open windows
end

function NewWindow_CreateProj()
	local w = Window()
	
	local b = UiButton(20, 20, 100 ,30, _('Create'))
	b:bind_event(EVT.LEFT_UP, nil, OnCreateProj)
	w:AddChild(b)
	return w
end

function NewWindow_LoadProj()
	local w = Window()
	
	--t0:bind_event(EVT.TIMER, t0, t0.Func)
	--t1:bind_event(EVT.TIMER, t1, t1.Func)
	--t0:Start(w, 300, true)
	--t1:Start(w, 10, true)
	
	--VLayoutTest(w)
	--HLayoutTest(w)
	GridLayoutTest(w)
	--w:AddChild(UiWidget(0, 0, 150, 130))
	--w:AddChild(UiWidget(200, 0, 150, 130))
	
	--w:AddChild(UiTextInput(10, 10, 100, uiFont.fontSize))
	
	--w:AddChild(UiTextInput(10, 100, 100, uiFont.fontSize))
	
	--w:AddChild(UiText(0, 0, 'abcdef'))
	--w:AddChild(UiText(100, 0, 'abcdef'))
	
	w.OnLeftDown = WindowOnLeftDown
	--w:bind_event(EVT.LEFT_DOWN, w, w.OnLeftDown)
	
	return w
end

function LoadEntrance()
	cEntrance:AddPageWindow('load_proj', 'Load Project', NewWindow_LoadProj())
	cEntrance:AddPageWindow('new_proj', 'New Project', NewWindow_CreateProj())
	
	LoadProj('out', true)
end

function LoadMainFrame()
	cMainFrame:AddPageWindow('page0', 'page0', Window())
	cMainFrame:AddPageWindow('page1', 'page1', Window())
	cMainFrame:AddPageWindow('page2', 'page2', Window())
	cMainFrame:AddPageWindow('panel', 'page3', Window())
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
			








	