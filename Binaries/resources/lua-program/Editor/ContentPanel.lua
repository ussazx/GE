---ContentPanel---
require 'window'
require 'presets'

local nameHintBegin = _('名称不能以空格开头')
local nameHintChar = _('名称不能包含下列字符：') .. '\\/:*?\"<>|'
local nameHintExist = _('名称已存在')

function CheckName(s)
	if (s:length() > 0) then
		if (s:find(' ') == 0) then
			return nameHintBegin
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

SearchInput = class(UiWidget)
function SearchInput:ctor(w, font)
	self.drawSelf = false
	local h = HBoxLayout()
	self:AddChild(h)
	
	w = w or 0
	font = font or uiFont
	local th = font.maxHeight + 4
	
	local mag = UiPolyIcon(g_iconMagnifier)
	local ww = UiWidget(mag.rect.w + 10, th)
	ww:AddChild(mag, 5, math.ceil((th - mag.rect.h) / 2))

	self.input = UiTextInput(w, th, '', font)
	ww.color:copy(self.input.crColor)
	
	h:AddChild(ww, nil, 0)
	h:AddChild(self.input, 1, 0, 0)
	h:SetSize()
	self:SetSize(h.rect.w, h.rect.h)
end

local content = Object()
content.FOLDER = 1
local icons = {}
icons[content.FOLDER] = g_iconFolder

Folder = class()
Folder.tempName = LString('')

function Folder:ctor(meta)
	self.meta = meta
	self.files = SortedMap()
	self.fileNames = {}
	self.folders = SortedMap()
	self.folderNames = {}
end

function Folder:Add(panel, o, nonFresh)
	local d, n
	if (o.type == content.FOLDER) then
		o.data = o.data or Folder(o)
		o.data.parent = self
		d = self.folders
		n = self.folderNames
	else
		d = self.files
		n = self.fileNames
	end
	local op = content.OP_ADD
	if (n[o]) then
		op = content.OP_RENAME
		d[n[o]] = nil
	end
	d[o.lname] = o
	n[o] = o.lname
	if (not nonFresh) then
		content:Update(panel, self, o, op)
	end
end

function Folder:Remove(panel, o, nonFresh)
	local t, f
	if (o.type == content.FOLDER) then
		o.data:Clear(true)
		f = o.data
		self.folders[o.lname] = nil
		self.folderNames[o] = nil
		content:Update(panel, nil, o, content.OP_REMOVE)
	else
		t = self.files
		self.files[o.lname] = nil
		self.fileNames[o] = nil
	end
	if (not nonFresh) then
		content:Update(panel, self)
	end
	-- for i, v in pairs(t) do
		-- if (v == o) then
			-- if (f) then
				-- f:Clear(true)
			-- end
			-- table.remove(t, i)
			-- if (t == self.folders) then
				-- content:Update(panel, nil, v, content.OP_REMOVE)
			-- end
			-- if (not nonFresh) then
				-- content:Update(panel, self)
			-- end
			-- break
		-- end
	-- end
end

function Folder:Clear(nonFresh)
	for _, v in self.folders:pairs() do
		v.data:Clear(nonFresh)
		--remove
		content:Update(nil, nil, v, content.OP_REMOVE)
	end
	for _, v in self.files:pairs() do
		--remove
		content:Update(nil, nil, v, content.OP_REMOVE)
	end
	self.folders = SortedMap()
	self.folderNames = {}
	self.files = SortedMap()
	self.fileNames = {}
	if (not nonFresh) then
		content:Update(nil, self)
	end
end

function Folder:CheckNameExists(name, data)
	local t = self.files
	if (data.type == content.FOLDER) then
		t = self.folders
	end
	return t[name] ~= nil and t[name] ~= data
end
	

local o = {type = content.FOLDER, name = _('内容')}
o.data = Folder(o)
content.folder = o.data
content.EVT = EVT.new()
content.OP_ADD = 1
content.OP_RENAME = 2
content.OP_REMOVE = 3

function content:Update(panel, folder, meta, op)
	content:process_event(content.EVT, panel, folder, meta, op)
end

local itemMenus = {}
local m = CMenu()
m:AddItem(1, _('重命名'))
m:AddItem(2, _('删除'))
itemMenus[content.FOLDER] = m

local contentMenu = CMenu()
contentMenu:AddItem(1, _('新建文件夹'))
--contentMenu:AddItem(2, _('新建FrameBuffer'))

ContentPanel = class(Window)
ContentPanel.fMax = 20

function ContentPanel:ctor()
	self.color:set(70, 70, 70, 255)

	local hsLayout = HSizerLayout()
	hsLayout:SetBarDftColor(70, 70, 70, 255)
	self:AddChild(hsLayout)
	
	self.tree = UiTreeList()
	self.tree.selector:bind_event(Selector.EVT_CHANGED, self, self.OnTreeSelected)
	hsLayout:AddChild(UiScrollPanel(self.tree), 1, 2, 0, true, 5)
	
	local h = HBoxLayout()
	
	self.bkwd = UiPolyIcon(g_iconLNavi)
	self.bkwd:SetDefaultColor(1, 150, 150, 150, 255)
	self.bkwd:SetHoverringColor(1, 200, 200, 200, 255)
	self.bkwd:SetPressingColor(1, 230, 230, 230, 255)
	self.bkwd:SetDisableColor(1, 90, 90, 90, 255)
	self.bkwd:Enable(false)
	self.bkwd:bind_event(EVT.LEFT_UP, self, self.OnBackward)
	
	self.frwd = UiPolyIcon(g_iconRNavi)
	self.frwd:SetDefaultColor(1, 150, 150, 150, 255)
	self.frwd:SetHoverringColor(1, 200, 200, 200, 255)
	self.frwd:SetPressingColor(1, 230, 230, 230, 255)
	self.frwd:SetDisableColor(1, 90, 90, 90, 255)
	self.frwd:Enable(false)
	self.frwd:bind_event(EVT.LEFT_UP, self, self.OnForward)
	
	h:AddChild(self.bkwd, nil, 10, 0, false, 0)
	h:AddChild(self.frwd, nil, 15, 0, false, 0)
	
	self.pathLayout = HBoxLayout()
	self.pathScroll = UiScrollPanel(self.pathLayout)
	self.pathScroll.drawSelf = false
	self.pathScroll.sliderOut = true
	self.pathScroll:SetSliderWidth(8)
	self.pathLayout:bind_event(EVT.SIZE, self, self.OnPathSized)
	h:AddChild(self.pathScroll, nil, 20, 20, false, 0)
	
	local searcher = SearchInput()
	h:AddChild(searcher, 1, 12, 12, false, 0)
	
	local v = VBoxLayout()
	v:AddChild(h, nil, 10, 0, true)
	
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
	
	self.color:copy(self.pane.color)
	
	w = UiWidget()
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
	
	self.fIdx = 0
	self.fRecord = WeakTable()
	
	content:bind_event(content.EVT, self, self.OnContentUpdated)
	
	self.tid = {}
	self.tid[content.folder] = self.tree:AddNode(nil, g_iconFolder, _('内容'), content.folder.meta)
	self:LoadFolder(content.folder)
end

function ContentPanel:OnTreeSelected()
	local o = self.tree.selector:GetSelection()
	if (o) then
		self:LoadFolder(o.data)
	end
end

function ContentPanel:OnPathSized()
	local w, h = self.pathLayout.rect.w, self.pathLayout.rect.h
	self.pathScroll:SetSize(math.min(500, w), math.max(self.pathScroll.rect.h, h))
end

function ContentPanel:OnForward()
	if (self.fIdx < self.fSaved) then
		self.fIdx = self.fIdx + 1
		self:LoadFolder(self.fRecord[self.fIdx], true)
	end
end

function ContentPanel:OnBackward()
	if (self.fIdx > 1) then
		self.fIdx = self.fIdx - 1
		self:LoadFolder(self.fRecord[self.fIdx], true)
	end
end

function ContentPanel:RemoveRecord(folder)
	local k, v = next(self.fRecord)
	while (v) do
		if (v == folder) then
			table.remove(self.fRecord, k)
			v = self.fRecord[k]
			if (self.fIdx >= k) then
				self.fIdx = self.fIdx - 1
			end
			self.fSaved = self.fSaved - 1
		else
			k, v = next(self.fRecord, k)
		end
	end
	
	local f = self.fRecord[self.fIdx]
	local k, v = next(self.fRecord, self.fIdx)
	while (v and self.fSaved > self.fIdx) do
		if (v ~= f) then
		break end
		table.remove(self.fRecord, k)
		v = self.fRecord[k]
		self.fSaved = self.fSaved - 1
	end
	while (self.fIdx > 1) do
		if (self.fRecord[self.fIdx - 1] ~= f) then
		break end
		self.fSaved = self.fSaved - 1
		self.fIdx = self.fIdx - 1
	end
		
	if (f ~= self.curFolder) then
		self:LoadFolder(f, true)
	else
		self.frwd:Enable(self.fIdx < self.fSaved)
		self.bkwd:Enable(self.fIdx > 1)
	end
end

function ContentPanel:Open(e)
	local o = EVT.obj.meta
	if (o.type == content.FOLDER) then
		self:LoadFolder(o.data)
	else
		
	end
end

function ContentPanel:SetPathButtons(folder)
	self.pathLayout:ClearChildren()
	local o = {}
	local n = 0
	local f = folder
	while (f) do
		local t = UiTextLabel(100, f.meta.name)
		t:EnableWriteId(false)
		local b = UiButton(t.rect.w + 10, t.rect.h + 4)
		b.text = t
		b.layout:AddChild(t, 1)
		b:SetDefaultColor(0, 0, 0, 0)
		b:bind_event(EVT.LEFT_UP, self, self.Open)
		b.meta = f.meta
		table.insert(o, b)
		f = f.parent
		n = n + 1
	end
	for i = n, 1, -1 do
		if (i < n) then
			local s = UiWidget(1, o[i].rect.h)
			s.color:set(100, 100, 100, 255)
			self.pathLayout:AddChild(s, nil, 0, 0)
		end
		self.pathLayout:AddChild(o[i])
	end
	self.pathLayout:SetSize()
end

function ContentPanel:LoadFolder(folder, recorded)
	self.grid:ClearChildren()
	self.selector = Selector()
	self.selector.showFocusOut = true
	self.selector:SetFocusOutColor(0, 0, 0, 0)
	for _, o in folder.folders:pairs() do
		self:AddItem(o)
	end
	
	if (self.curFolder ~= folder) then
		self:SetPathButtons(folder)
		if (not recorded) then
			self.fIdx = self.fIdx + 1
			if (self.fIdx > self.fMax) then
				self.fIdx = self.fMax
				table.remove(self.fRecord, 1)
			end
			table.insert(self.fRecord, self.fIdx, folder)
			self.fSaved = self.fIdx
		end
		self.tree:Select(self.tid[folder], false)
		self.frwd:Enable(self.fIdx < self.fSaved)
		self.bkwd:Enable(self.fIdx > 1)
		self.curFolder = folder
	end
end

function ContentPanel:OnPresetDrop(e, id, geom)
	
end

function ContentPanel:OnDropFile(e, files)
	for _, f in pairs(files) do
		Print(f)
	end
end

function ContentPanel:OnContentUpdated(e, panel, folder, meta, op)
	if (panel ~= self and folder == self.curFolder) then
		self:LoadFolder(folder)
	end
	if (not meta) then
	return end
	if (meta.type == content.FOLDER) then
		if (op == content.OP_ADD) then
			self.tid[meta.data] = self.tree:AddNode(self.tid[folder], g_iconFolder, meta.name, meta)
		elseif (op == content.OP_RENAME) then
			for _, v in self.pathLayout:ChildrenPairs() do
				if (meta == v.meta) then
					v.text:SetText(meta.name)
					v:SetSize(v.text.rect.w + 10)
					break
				end
			end
			self.tree:UpdateNode(self.tid[meta.data], nil, meta.name)
		elseif (op == content.OP_REMOVE) then
			self:RemoveRecord(meta.data)
			self.tree:RemoveNode(self.tid[meta.data])
		end
	end
end

function ContentPanel:OnTextFocusOut(e)
	local item = EVT.obj.item
	item.input:Show(false)
	item.name:Show(true)
	if (item.nameNew) then
		item.name:SetText(item.input.text)
		item.meta.name = item.input.text:utf8()
		item.meta.lname = item.input.text:lower_utf8()
	else
		item.input:SetText(item.name.text)
	end
	self.nameHint:Show(false)
	
	self.curFolder:Add(self, item.meta)
	--if (item.op == content.OP_ADD) then
		--self.curFolder:Add(self, item.meta)
	--else
		--content:Update(self, self.curFolder, item.meta, item.op)
	--end
end

function ContentPanel:OnTextKeyDown(e, k)
	if (k == SYS.VK_RETURN) then
		EVT.obj:Show(false)
	end
end

function ContentPanel:OnText(e)
	self.nameHint:Show(false)
	local item = EVT.obj.item
	item.nameNew = true
	local s = item.input.text
	local hint = CheckName(s)
	if (not hint and self.curFolder:CheckNameExists(s:lower_utf8(), item.meta)) then
		hint = nameHintExist
	end
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

function ContentPanel:AddItem(o)
	local item = self.selector:Add(100, 80, {})
	item:bind_event(EVT.LEFT_DCLICK, self, self.Open)
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
	
	item.name = UiTextLabel(100, o.name, uiFont2)
	item.name:EnableWriteId(false)
	layout:AddChild(item.name)
	return item
end

function ContentPanel:OnItemMenu(e)
	local item = EVT.obj
	local id = item.menu:Popup(self)
	if (not id) then
	return end
	
	if (item.meta.type == content.FOLDER) then
		if (id == 1) then
			item.name:Show(false)
			item.input:Show(true)
			item.input:SelectAll()
			item.nameNew = true
			item.op = content.OP_RENAME
			item.input:SetFocus(true)
		elseif (id == 2) then
			--DelistObject(item)
			self.grid:RemoveChild(item)
			self.selector:Remove(item)
			self.curFolder:Remove(self, item.meta)
		end
	end
	self.item = item
end

local fname = _('新建文件夹')
local lfname = LString('')
function ContentPanel:OnMouse(e)
	if (e == EVT.RIGHT_UP) then
		local id = contentMenu:Popup(self)
		if (not id) then return end
		
		local o = {}
		local name
		if (id == 1) then
			o.type = content.FOLDER
			name = fname
		end
		lfname:set(fname)
		local i = 1
		local s = lfname:lower_utf8()
		while (self.curFolder:CheckNameExists(s, o)) do
			lfname:set(fname)
			lfname = lfname .. string.format('_%q', i)
			s = lfname:lower_utf8()
			i = i + 1
		end
		o.name = lfname:utf8()
		o.lname = s
		local item = self:AddItem(o)
		item.op = content.OP_ADD
		item.nameNew = true
		item.input:SelectAll()
		item.input:Show(true)
		item.name:Show(false)
		self:SetFocus(item.input, true)
	else
	end
end

function ContentPanel:ScanDirectory(d)
	local n = self.list:AddNode(nil, g_iconFolder, 'main')
	n = self.list:AddNode(n, g_iconFolder, 'sub')
	self.list:AddNode(n, g_iconFolder, 'sub111111111111111')
end

---MtlSnapshot---
MtlSnapshot = class(Object)

function MtlSnapshot:ctor(w, h, mtl)
	self.scene = Scene3D()
	self.scene:SetSize(w, h)
	self.sphere = Model(g_sphere)
	self.sphere:Attach(self.scene.scene)
	if (mtl) then
		self.sphere:SetMaterial(1, mtl)
	end
	self.scene.camera:SetPosition(0, 0, -3)
	self.view = Snapshot(w, h)
	self.view:Render(self.scene)
	self.image = self.view.image
end

function MtlSnapshot:SetMaterial(mtl, render)
	self.sphere:SetMaterial(1, mtl)
	if (render) then
		self.view:Render(self.scene)
	end
end