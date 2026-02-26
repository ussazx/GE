-----PanelSet-----
require 'window'

PanelSet = class(Object)

local function ShowPanel(set)
	return function (id)
		set.nb:ShowPage(get_object(id))
	end
end

local function OnShowPanelOpen(set)
	return function ()
		for _, v in pairs(set.panels) do
			CMenu.Check(v.mShow, set.nb:IsPageShown(v))
		end
	end
end

local function SaveLoadLayout(set)
	return function (id)
		if (id == 1) then
			set.layout = set.nb:SaveLayout()
		elseif (id == 2) then
			set.nb:LoadLayout(set.layout)
		end
	end
end

function PanelSet:AddPanel(name, title, panel)
	self.title = title
	self.nb:AddPage(name, title, panel)
	panel.mShow = self.mShowPanel:AddCheckItem(panel.id, title)
	table.insert(self.panels, panel)
end

function PanelSet:ctor(nb)
	PanelSet[nb:GetNB()] = self
	self.mb = CMenuBar()
	-- local m = mb:Add('menu', SaveLoadLayout(self))
	-- m:AddItem(1, 'save')
	-- m:AddItem(2, 'load')
	self.mShowPanel = self.mb:Add(_('面板'), ShowPanel(self))
	self.nb = nb
	self.panels = {}
	PanelSet.set = self
	self.mShowPanel:BindOnOpen(OnShowPanelOpen(self))
	self.recorder = Recorder()
	self.recorder:SetSaveHandler(self, PanelSet.SaveRecord)
	self.recObjFunc = {}
end

function PanelSet:HandleRecord(undo, data)
	data.func(data.obj, undo, data.data)
	self.recData = data
	self.recUndo = undo
	if (self.modified and data == self.savedData and undo == self.savedUndo) then
		ProjModified(self, false)
		self.modified = false
	elseif (not self.modified and (data ~= self.savedData or undo == not self.savedUndo)) then
		ProjModified(self, true)
		self.modified = true
	end
end

function PanelSet:Record(obj, func, data)
	data = {obj = obj, func = func, data = data}
	self.recorder:Record(self, PanelSet.HandleRecord, data, PanelSet.SaveRecord)
	if (not self.savedData) then
		self.savedData = self.recData
		self.savedUndo = true
	elseif (self.recData == self.savedData and self.recUndo == self.savedUndo) then
		self.savedData = data
		self.savedUndo = true
	end
	
	self.recData = data
	self.recUndo = false
	if (not self.modified) then
		ProjModified(self, true)
		self.modified = true
	end
end

function PanelSet:SaveRecord(all)
	
end

function PanelSet:LoadProfile(o)
	self:OnLoadProfile(o)
end

function PanelSet:SaveProfile(o)
	if (self.modified) then
		self:OnSaveProfile(o)
		self.savedData = self.recData
		self.savedUndo = self.recUndo
		ProjModified(self, false)
		self.modified = false
	end
end