---graphic.lua---
SIZE_FLOAT1 = 4
SIZE_FLOAT2 = 8
SIZE_FLOAT3 = 12
SIZE_FLOAT4 = 16
--SIZE_CG_HANDLE = CGraphic.HandleSize()

VB_ELEM_FLOAT1 = 0x1
VB_ELEM_FLOAT2 = 0x10
VB_ELEM_FLOAT3 = 0x100
VB_ELEM_FLOAT4 = 0x1000
VB_ELEM_INT1 = 0x10000

VB_ELEM_MAX_COUNT = 4

VB_RESERVE_COUNT = 1024

local function CreateVB(elem_idx, elem_num, new_buf_method, vbo)
	local vb = {}
	if (elem_num > 0) then
		for i = 0, math.min(elem_num, VB_ELEM_MAX_COUNT) - 1 do
			vb.co = new_buf_method(vbo.vb_meta[elem_idx + i].stride * vbo.capacity)
			vb.ptr = vb.co:GetPtr(0)
			vbo.vb[elem_idx + i] = vb
		end
	end
end

---VBO---
VBO = class()
VBO.vb_meta = {}
for i = 0, VB_ELEM_MAX_COUNT - 1 do
	local n = VB_ELEM_FLOAT1 + i
	VBO.vb_meta[n] = {}
	VBO.vb_meta[n].set_val = CSetFloat1
	VBO.vb_meta[n].get_val = CGetFloat1
	VBO.vb_meta[n].stride = SIZE_FLOAT1

	n = VB_ELEM_FLOAT2 + i
	VBO.vb_meta[n] = {}
	VBO.vb_meta[n].set_val = CSetFloat2
	VBO.vb_meta[n].get_val = CGetFloat2
	VBO.vb_meta[n].stride = SIZE_FLOAT2

	n = VB_ELEM_FLOAT3 + i
	VBO.vb_meta[n] = {}
	VBO.vb_meta[n].set_val = CSetFloat3
	VBO.vb_meta[n].get_val = CGetFloat3
	VBO.vb_meta[n].stride = SIZE_FLOAT3

	n = VB_ELEM_FLOAT4 + i
	VBO.vb_meta[n] = {}
	VBO.vb_meta[n].set_val = CSetFloat4
	VBO.vb_meta[n].get_val = CGetFloat4
	VBO.vb_meta[n].stride = SIZE_FLOAT4

	n = VB_ELEM_INT1 + i
	VBO.vb_meta[n] = {}
	VBO.vb_meta[n].set_val = CSetInt1
	VBO.vb_meta[n].get_val = CGetInt1
	VBO.vb_meta[n].stride = SIZE_INT1
end

function VBO:ctor(s)
	self.current = 0
	self.offset = 0
	self.offset_max = 0
	self.capacity = s.vtx_count or 0
	self.reserve = s.reserve or 0
	self.vb = {}
	
	if (self.is_gpu) then
		new_buf_method = CGraphic.NewVertexInput
		self.vb_input_set = CBuffer(SIZE_GC_HANDLE)
	else
		new_buf_method = CBuffer
	end
	if (type(s[VB_ELEM_FLOAT1]) == 'number' and s[VB_ELEM_FLOAT1] > 0) then
		CreateVB(VB_ELEM_FLOAT1, s[VB_ELEM_FLOAT1], new_buf_method, self)
	end
	if (type(s[VB_ELEM_FLOAT2]) == 'number' and s[VB_ELEM_FLOAT2] > 0) then
		CreateVB(VB_ELEM_FLOAT2, s[VB_ELEM_FLOAT2], new_buf_method, self)
	end
	if (type(s[VB_ELEM_FLOAT3]) == 'number' and s[VB_ELEM_FLOAT3] > 0) then
		CreateVB(VB_ELEM_FLOAT3, s[VB_ELEM_FLOAT3], new_buf_method, self)
	end
	if (type(s[VB_ELEM_FLOAT4]) == 'number' and s[VB_ELEM_FLOAT4] > 0) then
		CreateVB(VB_ELEM_FLOAT4, s[VB_ELEM_FLOAT4], new_buf_method, self)
	end
	if (type(s[VB_ELEM_INT1]) == 'number' and s[VB_ELEM_INT1] > 0) then
		CreateVB(VB_ELEM_INT1, s[VB_ELEM_INT1], new_buf_method, self)
	end
	return self
end

function VBO:at(offset)
	if (self.offset_max < offset) then
		self.offset_max = offset
	end
	self.offset = self.current + offset
	if (self.offset >= self.capacity) then
		self.capacity = self.offset + math.max(1, self.reserve)
		for i, vb in pairs(self.vb) do
			vb.ptr = vb.co:Resize(self.capacity * self.vb_meta[i].stride)
		end
	end
end

function VBO:skip_used()
	self.current = self.current + self.offset_max
	self.offset = 0
	self.offset_max = 0
end

function VBO:set_val(elem_idx, ...)
	local vb = self.vb[elem_idx]
	local vb_meta = self.vb_meta[elem_idx]
	vb_meta.set_val(vb.ptr, self.offset * vb_meta.stride, ...)
end

function VBO:get_val(elem_idx)
	local vb = self.vb[elem_idx]
	local vb_meta = self.vb_meta[elem_idx]
	return vb_meta.get_val(vb.ptr, self.offset * vb_meta.stride)
end

--Renderer--
Renderer = class()
function Renderer:ctor()
	self.vbo = VBO()
	self.ibo = IBO()
	self.vb = nil
	self.ib = nil
end

function Renderer:CopyVB(vbo)
	

---DrawCmd---
local function CompareViewport(vp, x, y, width, height, min_depth, max_depth)
	return vp.x == x and
	vp.y == y and 
	vp.width == width and
	vp.height == height and
	vp.min_depth == min_depth and 
	vp.max_depth == max_depth
end

local function InitViewport(vp, x, y, width, height, min_depth, max_depth)
	vp.x = x
	vp.y = y
	vp.width = width
	vp.height = height
	vp.min_depth = min_depth
	vp.max_depth = max_depth
end

local function CompareClipRect(cr, x, y, width, height)
	return cr.x == x and
	cr.y == y and 
	cr.width == width and
	cr.height == height
end

local function InitClipRect(cr, x, y, width, height)
	cr.x = x
	cr.y = y
	cr.width = width
	cr.height = height
end

dc_renderer = 1
dc_viewport = 2
dc_cliprect = 3
dc_blend_const = 4
dc_stencil_ref = 5
dc_vbo = 6
dc_ibo = 7
dc_states_max = 7

local function CommitStates(dc)
	local s = dc[dc_renderer]
	if (s and s.s) then

	end
	s = dc[dc_viewport]
	if (s and s.s) then

	end
	s = dc[dc_cliprect]
	if (s and s.s) then

	end
	s = dc[dc_blend_const]
	if (s and s.s) then

	end
	s = dc[dc_stencil_ref]
	if (s and s.s) then

	end
end

local function AddDrawCall(cmd)
	cmd.drawcalls.max = cmd.drawcalls.max + 1
	cmd.drawcalls[cmd.drawcalls.max] = cmd.drawcalls[cmd.drawcalls.max] or {}
	
	local dc = cmd.drawcalls[cmd.drawcalls.max]
	for i = 1, dc_states_max do
		dc[i] = cmd[i].top
	end
	dc.vb_start = 0
	dc.vb_count = 0

	cmd.drawcalls.top = dc
end

local function UpdateDrawCmd(cmd, dc_state, s)
	local top_dc = cmd.drawcalls.top
	if (top_dc.vb_count > 0 and top_dc[dc_state] ~= s) then
		AddDrawCall(cmd)
	end
	top_dc[dc_state] = s
end

local function PushState(cmd, dc_state, compare_func, init_func, ...)
	local t = cmd[dc_state]
	local ttop = t.top
	if (ttop and ((compare_func and compare_func(ttop.s, ...)) or ttop.s == ...)) then
		ttop.count = ttop.count + 1
	else
		t.max = t.max + 1
		t[t.max] = t[t.max] or {s = {}}
		ttop = t[t.max]
		ttop.count = 1

		if (init_func) then
			init_func(ttop.s, ...)
		else
			ttop.s = ...
		end
		UpdateDrawCmd(cmd, dc_state, ttop)
		t.top = ttop
	end
end

local function PopState(cmd, dc_state)
	local t = cmd[dc_state]
	if (t.top) then
		if (t.top.count > 0) then
			t.top.count = t.top.count - 1
			if (t.top.count == 0) then
				t.max = t.max - 1
				t.top = t[t.max]
				UpdateDrawCmd(cmd, dc_state, t.top)
			end
		end
	end
end

DrawCmd = class()
function DrawCmd:ctor()
	self.drawcalls = {}
	for i = 1, dc_states_max do
		self[i] = {}
	end
	self:Reset()
end

function DrawCmd:Reset()
	self.drawcalls.max = 0
	AddDrawCall(self)

	for i = 1, dc_states_max do
		self[i].top = nil
		self[i].max = 0
	end
end

function DrawCmd:Draw(vb_start, vb_count)
	if (vb_start ~= self.top_dc.vb_start + 1) then
end

function DrawCmd:DrawIndexed()

end

function DrawCmd:Commit()
	local dc
	for i = 1, self.drawcalls.max do
		dc = self.drawcalls[i]
		CommitStates(dc)
	end
end

function DrawCmd:Draw(vbo)

end

function DrawCmd:PushRenderer(rdr)
	PushState(self, dc_renderer, nil, nil, rdr)
end

function DrawCmd:PopRenderer()
	PopState(self, dc_renderer)
end

function DrawCmd:PushViewport(x, y, width, height, min_depth, max_depth)
	PushState(self, dc_viewport, CompareViewport, InitViewport, x, y, width, height, min_depth, max_depth)
end

function DrawCmd:PopViewport()
	PopState(self, dc_viewport)
end

function DrawCmd:PushClipRect(x, y, width, height)
	PushState(self, dc_cliprect, CompareClipRect, InitClipRect, x, y, width, height)
end

function DrawCmd:PopClipRect()
	PopState(self, dc_cliprect)
end

function DrawCmd:PushBlendConst(bc)
	PushState(self, dc_blend_const, nil, nil, bc)
end

function DrawCmd:PopBlendConst()
	PopState(self, dc_blend_const)
end

function DrawCmd:PushStencilRef(sr)
	PushState(self, dc_stencil_ref, nil, nil, sr)
end

function DrawCmd:PopStencilRef()
	PopState(self, dc_stencil_ref)
end

function DrawCmd:PushVBO(vbo)
	PushState(self, dc_vbo, nil, nil, vbo)
end

function DrawCmd:PopVBO()
	PopState(self, dc_vbo)
end

function DrawCmd:PushIBO(ibo)
	PushState(self, dc_ibo, nil, nil, ibo)
end

function DrawCmd:PopIBO()
	PopState(self, dc_ibo)
end
