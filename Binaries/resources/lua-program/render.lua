-----render.lua-----
require 'graphic'
require 'global'

fo_param_d = {pass = {}}
fo_param_d.pass[0] = {swapchain}
fo_param_d.pass[0][0] = rtv0
fo_param_d.pass[0][1] = rtv1
fo_param_d.pass[0].dsv = dsv0

FrameOutput = class()

function FrameOutput:ctor(fo_param)
	
end

function FrameOutput:Render()

end

function CreateFrameOutput(fo_cfg, swapchain)
	local fo = {}
	local fb = {}
	for i = 0, #fo_cfg.pass - 1 do
		table.insert(fb, fo_cfg.pass[i])
		if (fo_cfg.pass[i].split == true) then
			table.insert(fo, cGI:NewFrameBuffer(fb, swapchain))
			fb = {}
		end
	end
	return fo
end

ia = {}

--rtv_input connection changed
-- idx = fo[rtv_input]
-- if (idx ~= nil) then
	-- if (idx == cur_out_idx) then
		-- --error
	-- elseif (is_ia) then
		-- ia[rtv_input] = ia[rtv_input] or {fo = {}}
		-- table.insert(ia[rtv_input][fo], idx)
	-- else
		-- fo.rel[shader] = fo.rel[shader] or {}
		-- if (idx > cur_out_idx) then
			-- idx, cur_out_idx = swap(idx, cur_out_idx)
		-- end
		-- if (fo.rel[shader][idx] == nil or fo.rel[shader][idx] > cur_out_idx) then
			-- fo.rel[shader][idx] = cur_out_idx
		-- end
	-- end
-- end

--fo processing
function GetTopDst(idx)
	local dst
	for shader, _ in pairs(fo.rel) do
		if (dst == nil or dst > shader[idx]) then
			dst = shader[idx]
		end
	end
	return dst
end		

function FindSplit(idx, dst, ret_on_split)
	while (idx < dst) do
		if (fo.pass[idx].split == true) then
			if (ret_on_split) then
				return idx
			end
		else
			local dst2 = GetTopDst(idx)
			if (dst2 ~= nil and dst2 <= dst) then
				idx = FindSplit(idx, dst2 - 1, true)
				
				if (ret_on_split) then
					return idx
				end
			end
		end
		idx = idx + 1
	end
	fo.pass[idx].split = true
	return idx
end
