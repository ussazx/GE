-----GeomPanelSet-----
GeomPanelSet = class2(PanelSet)

function GeomPanelSet:ctor(nb)
	self.viewport = Window()
	self.viewport:AddChild(UiTextInput(100, 30))
	self:AddPanel('viewport', _('视口'), self.viewport)
end