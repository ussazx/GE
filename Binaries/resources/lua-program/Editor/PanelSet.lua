-----PanelSet-----
require 'window'

PanelSet = class()

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

function PanelSet.Add(name, title, panel)
	local set = PanelSet.set
	set.nb:AddPage(name, title, panel)
	panel.mShow = set.mShowPanel:AddCheckItem(panel.id, title)
	table.insert(set.panels, panel)
end

function PanelSet:ctor(nb, addPanels, layout)
	self.mb = CMenuBar()
	-- local m = mb:Add('menu', SaveLoadLayout(self))
	-- m:AddItem(1, 'save')
	-- m:AddItem(2, 'load')
	self.mShowPanel = self.mb:Add(_('面板'), ShowPanel(self))
	self.nb = nb
	self.panels = {}
	PanelSet.set = self
	addPanels(PanelSet.Add)
	self.mShowPanel:BindOnOpen(OnShowPanelOpen(self))
	if (layout) then
		self.nb:LoadLayout(layout)
	end
end