---global---
require 'graphic'
require 'utility'

g_recorder = Recorder()

g_assets = {}

g_content = {}

g_innerPolyVB = CMBuffer(1024)
g_innerPolyVB.offset = 0
g_innerPolyIB = CMBuffer(1024)
g_innerPolyIB.offset = 0

function AddPoly2D(...)
	local o = {colors = {}}
	o.w = 0
	o.h = 0
	o.vtx_count = 0
	o.idx_count = 0
	o.vb_offset = g_innerPolyVB.offset
	o.ib_offset = g_innerPolyIB.offset
	local x = 0
	local y = 0
	for _, v in pairs({...}) do
		local nv = 0
		local nvc = 0
		local vv = v.vertices or v
		for i = 1, #vv  do
			local w = vv[i]
			if (w == 1) then
				--CAddConvexPolyIndex(g_innerPolyIB, APPEND, 1, o.vtx_count, nv)
				o.idx_count = o.idx_count + CAddPolyIndex(1, g_innerPolyVB, 
					o.vb_offset + SIZE_FLOAT3 * o.vtx_count, nv, g_innerPolyIB, APPEND, o.vtx_count)
				o.vtx_count = o.vtx_count + nv
				nvc = nvc + nv
				nv = 0
			else
				nv = nv + 1
				CAddFloat3(g_innerPolyVB, APPEND, 1, w[1], w[2], Z_2D)
				if (w[1] < x) then
					x = w[1]
				elseif (w[1] > o.w) then
					o.w = w[1]
				end
				if (w[2] < y) then
					y = w[2]
				elseif (w[2] > o.h) then
					o.h = w[2]
				end
			end
		end
		if (nv > 0) then
			o.idx_count = o.idx_count + CAddPolyIndex(1, g_innerPolyVB, 
				o.vb_offset + SIZE_FLOAT3 * o.vtx_count, nv, g_innerPolyIB, APPEND, o.vtx_count)
			o.vtx_count = o.vtx_count + nv
			nvc = nvc + nv
		end
		table.insert(o.colors, {nvc, v.color or Color(150, 150, 150, 255)})
	end
	if (x < 0 or y < 0) then
		if (x < 0) then
			x = -x
		else
			x = 0
		end
		if (y < 0) then
			y = -y
		else
			y = 0
		end
		CMoveFloat3(g_innerPolyVB, g_innerPolyVB.offset, o.vtx_count, x, y, 0, g_innerPolyVB, g_innerPolyVB.offset)
	end
	g_innerPolyVB.offset = g_innerPolyVB.offset + SIZE_FLOAT3 * o.vtx_count
	g_innerPolyIB.offset = g_innerPolyIB.offset + SIZE_UINT1 * o.idx_count
	return o
end

g_iconFolder = AddPoly2D({{5, 0}, {50, 0}, {55, 5},
						{110, 5}, {110, 10}, {0, 10}, {0, 5}, 1,
						{0, 12}, {110, 12}, {110, 62}, {0, 62}})

local v = 10
local w = v * v
g_iconFold = AddPoly2D({{0, 0}, {math.sqrt(w - math.sqrt(2 * w) / 2), math.sqrt(2 * w) / 2}, {0, math.sqrt(2 * w)}})
g_iconExpand = AddPoly2D({{0, v}, {v, 0}, {v, v}})

local nn = DrawLines(10, false, {200, 10}, {150, 100}, {10, 200})

--g_iconLine = AddPoly2D({vertices = DrawLines(10, false, {10, 10}, {100, 100}, {10, 190})})
--g_iconLine = AddPoly2D({vertices = DrawLines(10, false, {10, 10}, {100, 100}, {200, 110}, {100, 200})})
g_iconLine = AddPoly2D({vertices = nn}, {vertices = DrawOutLine(nn, 2, 0), color = Color(150, 150, 150, 50)})
--g_iconLine = AddPoly2D({vertices = DrawLines(10, false, {100, 10}, {100, 100}, {10, 100})})
--g_iconLine = AddPoly2D({vertices = DrawLines(10, false, {100, 10}, {100, 100}, {200, 100})})

--g_iconLine = AddPoly2D({vertices = DrawLines(10, true, {10, 10}, {100, 10}, {100, 100})})

-- TargetViewDescs = {view1 = {samples = 1, format = FORMAT}}

-- FrameBufferDesc1 = {rp = '', views = {-1, 1, 2, 3, 4}}
-- CreatedViews = {}

g_idVb = cGI:NewBuffer(SIZE_WRITE_ID * ID_NUM_MAX)
for i = 0, ID_NUM_MAX do
	AddVertexID(g_idVb, APPEND, 1, i)
end
g_idVbSet = cGI:NewBufferSet({g_idVb}, 1)

local f = CNewFileInput(false)
f:Open('Resources/shaders/'..cGI:Type()..'/ui_vs.sc', true)
ui_vs = cGI:NewShaderModule(f)
f:Open('Resources/shaders/'..cGI:Type()..'/ui_ps.sc', true)
ui_ps = cGI:NewShaderModule(f)

f:Open('Resources/shaders/'..cGI:Type()..'/id_ui_vs.sc', true)
id_ui_vs = cGI:NewShaderModule(f)
f:Open('Resources/shaders/'..cGI:Type()..'/id_ui_ps.sc', true)
id_ui_ps = cGI:NewShaderModule(f)

f:Open('Resources/atlas2', true)
uiFont = CLoadFontAtlas(f, 256)

f:Close()

function LoadLuaFile(path, isBin)
	local f = CNewFileInput(false)
	if (not f:Open(path, isBin)) then
		return nil, false
	end
	local o = CLuaLoad(f)
	f:Close()
	return o, true
end

--pass
cParamRenderPass:Reset(true, false)
cParamRenderPass:AddViewDesc(cGI.FORMAT_PICK_ID, cGI.SAMPLE_COUNT_1_BIT, false, true, false, false)
cParamRenderPass:AddSwapchainOutput(0, false, 0)
cParamRenderPass:AddViewOutput(0, 1, false, 0)
-- cParamRenderPass:AddViewDesc(cGI.FORMAT_PRESENT, cGI.SAMPLE_COUNT_4_BIT, false, false, false, false)
-- cParamRenderPass:AddViewDesc(cGI.FORMAT_PICK_ID, cGI.SAMPLE_COUNT_1_BIT, false, true, false, false)
-- cParamRenderPass:AddViewOutput(0, 0, false, 0)
-- cParamRenderPass:AddSwapchainOutput(0, true, 0)
-- cParamRenderPass:AddViewOutput(1, 1, false, 0)

g_rp0 = cGI:NewRenderPass(cParamRenderPass)

--resource layout
cParamResourceLayout:Reset()
cParamResourceLayout:Add(cGI.RESOURCE_TYPE_UNIFORM_BUFFER, 0, 1, cGI.SHADER_STAGE_VERTEX_BIT)
g_rl0 = cGI:NewResourceLayout(cParamResourceLayout)

cParamResourceLayout:Reset()
cParamResourceLayout:Add(cGI.RESOURCE_TYPE_UNIFORM_TEXEL_BUFFER, 0, 1, cGI.SHADER_STAGE_FRAGMENT_BIT)
g_rl1 = cGI:NewResourceLayout(cParamResourceLayout)

--pipeline
cParamPipeline:Reset()
cParamPipeline:AddResourceLayout(g_rl0)
cParamPipeline:AddResourceLayout(g_rl1)
cParamPipeline:SetRasterizerStates(cGI.PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, cGI.POLYGON_MODE_FILL, cGI.CULL_MODE_BACK_BIT, true, false, false, false)
cParamPipeline:SetDethStencilStates(true, true, true, cGI.COMPARE_OP_LESS_OR_EQUAL, false)
cParamPipeline:SetBlendState(0, true)
cParamPipeline:SetBsColorBlendOp(0, cGI.BLEND_FACTOR_SRC_ALPHA, cGI.BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, cGI.BLEND_OP_ADD)
cParamPipeline:AddVertexElement(0, 0, cGI.FORMAT_R32G32B32_SFLOAT, SIZE_FLOAT3)
cParamPipeline:AddVertexElement(1, 1, cGI.FORMAT_R32G32B32_SFLOAT, SIZE_FLOAT3)
cParamPipeline:AddVertexElement(2, 2, cGI.FORMAT_R8G8B8A8_UNORM, SIZE_UINT1)
g_plUi = cGI:NewPipeline(g_rp0, 0, 1, ui_vs, 'main', ui_ps, 'main', cParamPipeline)

cParamPipeline:Reset()
cParamPipeline:AddResourceLayout(g_rl0)
cParamPipeline:SetRasterizerStates(cGI.PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, cGI.POLYGON_MODE_FILL, cGI.CULL_MODE_BACK_BIT, true, false, false, false)
cParamPipeline:SetDethStencilStates(true, true, true, cGI.COMPARE_OP_LESS_OR_EQUAL, false)
cParamPipeline:AddVertexElement(0, 0, cGI.FORMAT_R32G32B32_SFLOAT, SIZE_FLOAT3)
cParamPipeline:AddVertexElement(3, 1, cGI.FORMAT_WRITE_ID, SIZE_WRITE_ID)
cParamPipeline:SetVertexInputRate(3, true)
g_plId2D = cGI:NewPipeline(g_rp0, 1, 1, id_ui_vs, 'main', id_ui_ps, 'main', cParamPipeline)

g_mtlUi = {}
g_mtlUi.vtxLayout = NewVtxLayout(SIZE_FLOAT3, SIZE_FLOAT3, SIZE_UINT1)
g_mtlUi.slot = {}
g_mtlUi.slot[1] = 1
g_mtlUi.slot[2] = 2
g_mtlUi.slot[4] = 3
g_mtlUi.insSlot = {}
g_mtlUi.insSlot[SubpassId(g_rp0, 0)] = 1
g_mtlUi.insSlot[SubpassId(g_rp0, 1)] = 2

uiFont.res = ResourceSetNew(g_rl1)
uiFont.res:BindTexelView(uiFont.view)

g_mtlUi.func = {}

g_mtlUi.func[SubpassId(g_rp0, 0)] = function(dcList)
	dcList:AddResourceSet(ui_resourceSet)
	dcList:AddResourceSet(uiFont.res)
	dcList:SetPipeline(g_plUi, g_mtlUi.vtxLayout, 0)
end

g_mtlUi.func[SubpassId(g_rp0, 1)] = function(dcList)
	dcList:AddResourceSet(ui_resourceSet)  
	dcList:SetPipeline(g_plId2D, g_mtlUi.vtxLayout, 0)
	dcList:SetInsVB(g_idVbSet, 3)
end

-- cParamResourceLayout:Reset()
-- cParamResourceLayout:Add(RESOURCE_TYPE_UNIFORM_BUFFER, 0, 1, SHADER_STAGE_VERTEX_BIT)
-- self.resourceLayout = cGI:NewResourceLayout(cParamResourceLayout)
-- self.resourceSet = self.resourceLayout:NewResourceSet()