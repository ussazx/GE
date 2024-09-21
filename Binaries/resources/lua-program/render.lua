-----render.lua-----
require 'graphic'
require 'global'

Color = class()
function Color:ctor(r, g, b, a)
	self.r = r or 0
	self.g = g or 0
	self.b = b or 0
	self.a = a or 0
end

function Color:read(color)
	self.r = color.r or self.r
	self.g = color.r or self.g
	self.b = color.r or self.b
	self.a = color.r or self.a
end

function Color:diff(color)
	return self.r ~= rect.r or self.g ~= rect.g or self.b ~= rect.b or self.a ~= rect.a
end

function Color:set(r, g, b, a)
	self.r = r or 0
	self.g = g or 0
	self.b = b or 0
	self.a = a or 0
end

function DrawRectUI(cmd, id, font, rect, color)
	CAddRectFloat2(ui_renderer.vb[VB_ELEM_FLOAT2_0], APPEND, rect.x, rect.y, rect.w, rect.h)
	CAddFloat3(ui_renderer.vb[VB_ELEM_FLOAT3_0], APPEND, 4, font.table.pixels, 0, 0)
	CAddUByte4(ui_renderer.vb[VB_ELEM_FLOAT4_0], APPEND, 4, color.r, color.g, color.b, color.a)
	AddVertexID(ui_renderer.vb[VB_ELEM_UINT1_0], APPEND, 4, id)
	local n_idx = CAddConvexPolyIndex(ui_renderer.ib, APPEND, 1, ui_renderer.ib.start, 4)
	ui_renderer.ib.start = ui_renderer.ib.start + 4
	
	cmd:AddResourceSet(ui_resourceSet)
	cmd:AddResourceSet(font.res)
	cmd:Render(ui_renderer, n_idx)
	--cmd:Render(ui_renderer, 4, n, ui_renderer.instStart, 1)
	--ui_renderer.instStart = ui_renderer.instStart + 1

	--dc.Draw(mesh.ibo.start, mesh.ibo.count, mesh.vbo.start)
end

function AddUiText(wp, vbPos, vbUVW, font, text, x, y, rect)		
	local n
	if (rect) then
		n, x = CAddTextClip(vbPos, vbUVW, wp, font.table,
		x, y, rect.x, rect.y, rect.w, rect.h, text)
	else
		n, x = CAddText(vbPos, vbUVW, wp, font.table, x, y, text)
	end
	return n, x
end

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
