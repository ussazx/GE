---entrance---
require 'window'

wnd_load_proj = Window()

wnd_new_proj = Window()

function wnd_load_proj:init()
	--self init
	--...
	self:add_control(Control(nil, '', nil, 0, 0, w, h))
	local o = CLoad(projs)

	--self load o
end

function wnd_new_proj:OnLoadProject(name)

end

function wndNewProject:Init()
	--self init
	--...
end

function wndNewProject:OnNewProject(name)

end
