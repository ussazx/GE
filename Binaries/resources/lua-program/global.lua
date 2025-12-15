---global---
require 'render'
require 'utility'
require 'geometry'

g_recorder = Recorder()

g_content = {}
g_previews = {}

g_scene = SceneObject()

g_perObjInstance = PerObjectInstance()

g_perObjInst1 = PerObjectInstance(CMulAddUByte4)
g_perObjInst1:SetDefaultValue(1, 255, 255, 255, 255)

g_innerPolyVB = CMBuffer(1024)
g_innerPolyVB.offset = 0
g_innerPolyIB = CMBuffer(1024)
g_innerPolyIB.offset = 0

local function AddPolyVertex2D(o, t, i, x, y, nv, nvc)
	local v = t[i]
	nv = nv + 1
	nvc = nvc + 1
	CMulAddFloat3(1, g_innerPolyVB, APPEND, v[1], v[2], Z_2D)
	local replaces = {}
	if (v.combined and i ~= v.combined[1]) then
		table.insert(replaces, {i - 1, v.combined[1] - 1})
	end
	if (v[1] < x) then
		x = v[1]
	elseif (v[1] > o.w) then
		o.w = v[1]
	end
	if (v[2] < y) then
		y = v[2]
	elseif (v[2] > o.h) then
		o.h = v[2]
	end
	-- if (v.color) then
		-- table.insert(o.colors, {nvc, v.color})
		-- nvc = 0
	-- end
	return replaces, x, y, nv, nvc
end

local function WritePolyIndex2D(o, nv, replaces)
	if (nv > 0) then
		local ib_offset = o.ib_offset + SIZE_UINT1 * o.idx_count
		local ni = CAddPolyIndex(g_innerPolyVB, 
			o.vb_offset + SIZE_FLOAT3 * o.vtx_count, nv, g_innerPolyIB, ib_offset, o.vtx_count, 1)
			
		for _, v in pairs(replaces) do
			CReplaceIndex(g_innerPolyIB, ib_offset, ni, v[1] + o.vtx_count, v[2] + o.vtx_count)
		end
		o.vtx_count = o.vtx_count + nv
		o.idx_count = o.idx_count + ni
	end
	return 0
end

function AddPoly2D(antiAlias, ...)
	local o = {colors = {}, aa = {}, w = 0, h = 0, vtx_count = 0, idx_count = 0}
	o.vb_offset = g_innerPolyVB.offset
	o.ib_offset = g_innerPolyIB.offset
	
	local x = 0
	local y = 0
	local nv = 0
	local nvc = 0
	local cwp = 0
	local replaces
	local color = Color()
	color.wp = 0
	color.aa = {}
	for _, t in pairs({...}) do
		if (antiAlias and not t.has_normal) then
			BakePolyNormals2D(t)
		end
		for i = 1, #t do
			local v = t[i]
			if (antiAlias and (not v.combined or v.combined[1] == i)) then
				v[1], v[2] = v[1] - v.normal[1], v[2] - v.normal[2]
			end
			replaces, x, y, nv, nvc = AddPolyVertex2D(o, t, i, x, y, nv, nvc)
		end
		cwp = cwp + nv
		nv = WritePolyIndex2D(o, nv, replaces)
		if (antiAlias) then
			local aa = DrawPolyOutline(t, 1, 0)
			for _, t in pairs(aa) do
				for i = 1, #t do
					replaces, x, y, nv, nvc = AddPolyVertex2D(o, t, i, x, y, nv, nvc)
				end
				table.insert(color.aa, {SIZE_UINT1 * cwp, nv // 2 - 1})
				cwp = cwp + nv
				nv = WritePolyIndex2D(o, nv, replaces)
			end
		end
		if (t.color) then
			color:copy(t.color)
			color.nvc = nvc
			table.insert(o.colors, color)
			color = Color()
			color.wp = SIZE_UINT1 * o.vtx_count
			color.aa = {}
			nvc = 0
			cwp = 0
		end
	end
	if (nvc > 0) then
		color:set(150, 150, 150, 255)
		color.nvc = nvc
		table.insert(o.colors, color)
	end
	if (x < 0 or y < 0) then
		if (x < 0) then
			x = -x
			o.w = o.w + x
		else
			x = 0
		end
		if (y < 0) then
			y = -y
			o.h = o.h + y
		else
			y = 0
		end
		CMoveFloat3(g_innerPolyVB, g_innerPolyVB.offset, g_innerPolyVB, g_innerPolyVB.offset, o.vtx_count, x, y, 0)
	end
	g_innerPolyVB.offset = g_innerPolyVB.offset + SIZE_FLOAT3 * o.vtx_count
	g_innerPolyIB.offset = g_innerPolyIB.offset + SIZE_UINT1 * o.idx_count
	return o
end

--local vb = CMBuffer(1)
--local uvb = CMBuffer(1)
--local ib = CMBuffer(1)
--local n_vtx, n_idx = CAddCube(vb, 0, uvb, 0, ib, 0)
--g_cubeMesh = Mesh()

--local c = DrawLine(5, false, true, MakeCircle(0, 0, 100))
--c.AA = true
--g_iconLine = AddPoly2D(c)

--local nn2 = DrawOutLine(nn, 10, 2)
--nn2[#nn2 / 2 - 1].color = Color(150, 150, 150, 255)
--nn2[#nn2 - 1].color = Color(150, 150, 150, 255)
--nn2[#nn2].color = Color(150, 150, 150, 0)

--g_iconLine = AddPoly2D({vertices = DrawLines(10, false, {10, 10}, {100, 100}, {10, 190})})
--g_iconLine = AddPoly2D({vertices = DrawLines(10, false, {10, 10}, {100, 100}, {200, 110}, {100, 200})})
--g_iconLine = AddPoly2D(nn, nn2)
--g_iconLine = AddPoly2D({vertices = DrawLines(10, false, {100, 10}, {100, 100}, {10, 100})})
--g_iconLine = AddPoly2D({vertices = DrawLines(10, false, {100, 10}, {100, 100}, {200, 100})})

--g_iconLine = AddPoly2D({vertices = DrawLines(10, true, {10, 10}, {100, 10}, {100, 100})})

-- TargetViewDescs = {view1 = {samples = 1, format = FORMAT}}

-- FrameBufferDesc1 = {rp = '', views = {-1, 1, 2, 3, 4}}
-- CreatedViews = {}

local f = CNewFileInput(false)
f:Open('Resources/shaders/'..cGI:Type()..'/ui.vsc', true)
ui_vs = cGI:NewShaderModule(f)
f:Open('Resources/shaders/'..cGI:Type()..'/ui.psc', true)
ui_ps = cGI:NewShaderModule(f)
f:Open('Resources/shaders/'..cGI:Type()..'/ui_id.vsc', true)
ui_id_vs = cGI:NewShaderModule(f)

f:Open('Resources/shaders/'..cGI:Type()..'/object3d.vsc', true)
object3d_vs = cGI:NewShaderModule(f)
f:Open('Resources/shaders/'..cGI:Type()..'/object3dInst.vsc', true)
object3dInst_vs = cGI:NewShaderModule(f)
f:Open('Resources/shaders/'..cGI:Type()..'/object3d.psc', true)
object3d_ps = cGI:NewShaderModule(f)
f:Open('Resources/shaders/'..cGI:Type()..'/object3d_id.vsc', true)
object3d_id_vs = cGI:NewShaderModule(f)
f:Open('Resources/shaders/'..cGI:Type()..'/object3dInst_id.vsc', true)
object3dInst_id_vs = cGI:NewShaderModule(f)

f:Open('Resources/shaders/'..cGI:Type()..'/coord3d.vsc', true)
coord3d_vs = cGI:NewShaderModule(f)

f:Open('Resources/shaders/'..cGI:Type()..'/grid3d.vsc', true)
grid3d_vs = cGI:NewShaderModule(f)
f:Open('Resources/shaders/'..cGI:Type()..'/grid3d.psc', true)
grid3d_ps = cGI:NewShaderModule(f)

f:Open('Resources/shaders/'..cGI:Type()..'/ui3d.vsc', true)
ui3d_vs = cGI:NewShaderModule(f)

f:Open('Resources/shaders/'..cGI:Type()..'/id.psc', true)
id_ps = cGI:NewShaderModule(f)

f:Open('Resources/shaders/'..cGI:Type()..'/id_outline.psc', true)
id_outline_ps = cGI:NewShaderModule(f)

--pass
cParamRenderPass:Reset(true, false)
cParamRenderPass:AddViewDesc(cGI.FORMAT_PICK_ID, cGI.SAMPLE_COUNT_1_BIT, false, true, false, false)
cParamRenderPass:AddViewDesc(cGI.FORMAT_D24_UNORM_S8_UINT, cGI.SAMPLE_COUNT_1_BIT, false, false, false, false)
cParamRenderPass:AddViewDesc(cGI.FORMAT_PICK_ID, cGI.SAMPLE_COUNT_1_BIT, false, true, false, false)
cParamRenderPass:AddSwapchainOutput(0, false, 0)
cParamRenderPass:AddViewOutput(0, 1, false, 0)
cParamRenderPass:AddViewOutput(2, 2, false, 0)
cParamRenderPass:SetDepthStencilOutput(1, 0)
cParamRenderPass:SetDepthStencilOutput(1, 1)
-- cParamRenderPass:AddViewDesc(cGI.FORMAT_PRESENT, cGI.SAMPLE_COUNT_4_BIT, false, false, false, false)
-- cParamRenderPass:AddViewDesc(cGI.FORMAT_PICK_ID, cGI.SAMPLE_COUNT_1_BIT, false, true, false, false)
-- cParamRenderPass:AddViewOutput(0, 0, false, 0)
-- cParamRenderPass:AddSwapchainOutput(0, true, 0)
-- cParamRenderPass:AddViewOutput(1, 1, false, 0)

g_rp0 = cGI:NewRenderPass(cParamRenderPass)
g_rp0[1] = SubpassId(g_rp0, 1)
g_rp0[2] = SubpassId(g_rp0, 2, 0)
g_rp0[3] = SubpassId(g_rp0, 3)

cParamRenderPass:Reset(true, true)
cParamRenderPass:AddSwapchainOutput(0, false, 0)
g_rp1 = cGI:NewRenderPass(cParamRenderPass)
g_rp1[1] = SubpassId(g_rp1, 1)

--resource layout
cParamResourceLayout:Reset()
cParamResourceLayout:Add(cGI.RESOURCE_TYPE_UNIFORM_BUFFER, 0, 1, cGI.SHADER_STAGE_VERTEX_BIT)
g_rlUB = cGI:NewResourceLayout(cParamResourceLayout)

cParamResourceLayout:Reset()
cParamResourceLayout:Add(cGI.RESOURCE_TYPE_COMBINED_IMAGE_SAMPLER, 0, 1, cGI.SHADER_STAGE_FRAGMENT_BIT)
g_rlCIS = cGI:NewResourceLayout(cParamResourceLayout)

cParamResourceLayout:Reset()
cParamResourceLayout:Add(cGI.RESOURCE_TYPE_UNIFORM_TEXEL_BUFFER, 0, 1, cGI.SHADER_STAGE_FRAGMENT_BIT)
g_rlTB = cGI:NewResourceLayout(cParamResourceLayout)

f:Open('Resources/font/simsun18', true)
uiFont = CLoadFontAtlas(f, 256)
uiFont.res = ResourceHub(g_rlTB)
uiFont.res:BindTexelView(uiFont.view, 0)

f:Open('Resources/font/simsun15', true)
uiFont2 = CLoadFontAtlas(f, 256)
uiFont2.res = ResourceHub(g_rlTB)
uiFont2.res:BindTexelView(uiFont2.view, 0)

f:Close()

--sampler
cParamSampler:Reset()
cParamSampler:SetFilterMode(cGI.FILTER_NEAREST, cGI.FILTER_NEAREST)
cParamSampler:SetAddressMode(cGI.SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER, 
	cGI.SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER, 
	cGI.SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
	false)
g_idSampler = cGI:NewSampler(cParamSampler)

--ui pipeline
cParamPipeline:Reset()
cParamPipeline:AddResourceLayout(g_rlUB)
cParamPipeline:AddResourceLayout(g_rlTB)
cParamPipeline:SetRasterizerStates(cGI.PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, cGI.POLYGON_MODE_FILL, cGI.CULL_MODE_NONE, true, false, false, false)
cParamPipeline:SetDethStencilStates(false, false, true, cGI.COMPARE_OP_GREATER_OR_EQUAL, false)
cParamPipeline:SetBlendState(0, true)
cParamPipeline:SetBsColorBlendOp(0, cGI.BLEND_FACTOR_SRC_ALPHA, cGI.BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, cGI.BLEND_OP_ADD)
cParamPipeline:AddVertexElement(0, 0, cGI.FORMAT_R32G32B32_SFLOAT, SIZE_FLOAT3)
cParamPipeline:AddVertexElement(1, 1, cGI.FORMAT_R32G32B32_SFLOAT, SIZE_FLOAT3)
cParamPipeline:AddVertexElement(2, 2, cGI.FORMAT_R8G8B8A8_UNORM, SIZE_UINT1)
g_plUi = cGI:NewPipeline(g_rp0, 0, 1, ui_vs, 'main', ui_ps, 'main', cParamPipeline)

--ui3d pipeline
cParamPipeline:AddResourceLayout(g_rlUB)
cParamPipeline:AddResourceLayout(g_rlUB)
--g_plui3d = cGI:NewPipeline(g_rp0, 0, 1, ui3d_vs, 'main', ui_ps, 'main', cParamPipeline)

cParamPipeline:Reset()
cParamPipeline:AddResourceLayout(g_rlUB)
cParamPipeline:SetRasterizerStates(cGI.PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, cGI.POLYGON_MODE_FILL, cGI.CULL_MODE_NONE, true, false, false, false)
cParamPipeline:SetDethStencilStates(false, false, true, cGI.COMPARE_OP_GREATER_OR_EQUAL, false)
cParamPipeline:AddVertexElement(0, 0, cGI.FORMAT_R32G32B32_SFLOAT, SIZE_FLOAT3)
cParamPipeline:AddVertexElement(3, 1, cGI.FORMAT_WRITE_ID, SIZE_WRITE_ID)
cParamPipeline:SetVertexInputRate(3, true)
g_plId2D = cGI:NewPipeline(g_rp0, 1, 1, ui_id_vs, 'main', id_ps, 'main', cParamPipeline)

--ui materal
g_mtlUi = {inst = g_perObjInstance}
g_mtlUi.resFont = uiFont.res

g_mtlUi.vbLayout = NewVBLayout(1|2|4, false, SIZE_FLOAT3, SIZE_FLOAT3, SIZE_UINT1)

g_mtlUi.func = {}

g_mtlUi.func[g_rp0[1]] = {func = function(mtl, dcList)
	dcList:AddResourceSet(g_resWnd)
	dcList:AddResourceSet(mtl.resFont)
	dcList:SetPipeline(g_plUi, g_mtlUi.vbLayout, 0)
end, mergeType = DC_DEFAULT}

g_mtlUi.func[g_rp0[2]] = {func = function(mtl, dcList)
	dcList:AddResourceSet(g_resWnd)
	dcList:SetPipeline(g_plId2D, g_mtlUi.vbLayout, 0)
	dcList:SetInstVB(g_perObjInstance.vbId, 3)
end, mergeType = DC_DEFAULT}

--ui3d material
-- g_mtlui3d = {inst = {}}
-- g_mtlui3d.matModel = CMatrix()
-- g_mtlui3d.resModel = ResourceHub(g_rlUB)
-- local buf = g_mtlui3d.resModel:BindResBuffer(0, CMatrix._size)
-- CAddMatrix(buf(), buf[1], g_mtlui3d.matModel)
-- g_mtlui3d.resFont = ResourceHub(g_rlTB)
-- g_mtlui3d.resFont:BindTexelView(uiFont.view, 0)

-- g_mtlui3d.vbLayout = NewVBLayout(1|2|4, false, SIZE_FLOAT3, SIZE_FLOAT3, SIZE_UINT1)

-- g_mtlui3d.func = {}

-- g_mtlui3d.func[g_rp0[1]] = {func = function(mtl, dcList)
	-- dcList:AddResourceSet(g_resWnd)
	-- dcList:AddResourceSet(mtl.resFont)
	-- dcList:AddResourceSet(mtl.resModel)
	-- dcList:AddResourceSet(g_resCamera)
	-- dcList:SetPipeline(g_plui3d, g_mtlui3d.vbLayout, 0)
-- end, mergeType = DC_DEFAULT}

-- g_mtlui3d.func[g_rp0[2]] = {func = function(mtl, dcList)
	-- dcList:AddResourceSet(g_resWnd)
	-- dcList:AddResourceSet(mtl.resModel)
	-- dcList:AddResourceSet(g_resCamera)
	-- dcList:SetPipeline(g_plId2D, g_mtlui3d.vbLayout, 0)
	-- dcList:SetInsVB(g_idVbSet, 3)
-- end, mergeType = DC_DEFAULT}

--3d pipeline
cParamPipeline:Reset()
cParamPipeline:AddResourceLayout(g_rlUB)
cParamPipeline:SetRasterizerStates(cGI.PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, cGI.POLYGON_MODE_FILL, cGI.CULL_MODE_BACK_BIT, true, false, false, false)
cParamPipeline:SetDethStencilStates(true, false, true, cGI.COMPARE_OP_GREATER_OR_EQUAL, false)
cParamPipeline:SetBlendState(0, true)
cParamPipeline:SetBsColorBlendOp(0, cGI.BLEND_FACTOR_SRC_ALPHA, cGI.BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, cGI.BLEND_OP_ADD)
cParamPipeline:AddVertexElement(0, 0, cGI.FORMAT_R32G32B32_SFLOAT, SIZE_FLOAT3)
cParamPipeline:AddVertexElement(1, 1, cGI.FORMAT_R32G32_SFLOAT, SIZE_FLOAT2)
cParamPipeline:AddVertexElement(2, 2, cGI.FORMAT_R8G8B8A8_UNORM, SIZE_UINT1)
g_pl3d = cGI:NewPipeline(g_rp0, 0, 1, object3d_vs, 'main', object3d_ps, 'main', cParamPipeline)

cParamPipeline:AddVertexElement(3, 3, cGI.FORMAT_R32G32B32A32_SFLOAT, SIZE_FLOAT4)
cParamPipeline:AddVertexElement(3, 4, cGI.FORMAT_R32G32B32A32_SFLOAT, SIZE_FLOAT4)
cParamPipeline:AddVertexElement(3, 5, cGI.FORMAT_R32G32B32A32_SFLOAT, SIZE_FLOAT4)
cParamPipeline:AddVertexElement(3, 6, cGI.FORMAT_R32G32B32A32_SFLOAT, SIZE_FLOAT4)
cParamPipeline:SetVertexInputRate(3, true)
g_pl3dInst = cGI:NewPipeline(g_rp0, 0, 1, object3dInst_vs, 'main', object3d_ps, 'main', cParamPipeline)

cParamPipeline:Reset()
cParamPipeline:AddResourceLayout(g_rlUB)
cParamPipeline:SetRasterizerStates(cGI.PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, cGI.POLYGON_MODE_FILL, cGI.CULL_MODE_BACK_BIT, true, false, false, false)
cParamPipeline:SetDethStencilStates(true, false, true, cGI.COMPARE_OP_GREATER_OR_EQUAL, false)
cParamPipeline:AddVertexElement(0, 0, cGI.FORMAT_R32G32B32_SFLOAT, SIZE_FLOAT3)
cParamPipeline:AddVertexElement(3, 1, cGI.FORMAT_WRITE_ID, SIZE_WRITE_ID)
--cParamPipeline:SetVertexInputRate(3, true)
g_plId3d = cGI:NewPipeline(g_rp0, 1, 1, object3d_id_vs, 'main', id_ps, 'main', cParamPipeline)

cParamPipeline:AddVertexElement(4, 2, cGI.FORMAT_R32G32B32A32_SFLOAT, SIZE_FLOAT4)
cParamPipeline:AddVertexElement(4, 3, cGI.FORMAT_R32G32B32A32_SFLOAT, SIZE_FLOAT4)
cParamPipeline:AddVertexElement(4, 4, cGI.FORMAT_R32G32B32A32_SFLOAT, SIZE_FLOAT4)
cParamPipeline:AddVertexElement(4, 5, cGI.FORMAT_R32G32B32A32_SFLOAT, SIZE_FLOAT4)
cParamPipeline:SetVertexInputRate(3, true)
cParamPipeline:SetVertexInputRate(4, true)
g_plId3dInst = cGI:NewPipeline(g_rp0, 1, 1, object3dInst_id_vs, 'main', id_ps, 'main', cParamPipeline)
g_plRecId3dInst = cGI:NewPipeline(g_rp0, 2, 1, object3dInst_id_vs, 'main', id_ps, 'main', cParamPipeline)

cParamPipeline:SetDethStencilStates(false, false, false, cGI.COMPARE_OP_GREATER_OR_EQUAL, false)
g_plIdCoord3d = cGI:NewPipeline(g_rp0, 1, 1, object3dInst_id_vs, 'main', id_ps, 'main', cParamPipeline)

cParamPipeline:AddResourceLayout(g_rlCIS)
g_plIdOutline = cGI:NewPipeline(g_rp1, 0, 1, object3dInst_id_vs, 'main', id_outline_ps, 'main', cParamPipeline)

--grid3d pipeline
cParamPipeline:Reset()
cParamPipeline:AddResourceLayout(g_rlUB)
cParamPipeline:SetDethStencilStates(true, false, false, cGI.COMPARE_OP_GREATER_OR_EQUAL, false)
cParamPipeline:SetRasterizerStates(cGI.PRIMITIVE_TOPOLOGY_LINE_LIST, cGI.POLYGON_MODE_FILL, cGI.CULL_MODE_NONE, true, false, false, false)
cParamPipeline:SetBlendState(0, true)
cParamPipeline:SetBsColorBlendOp(0, cGI.BLEND_FACTOR_SRC_ALPHA, cGI.BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, cGI.BLEND_OP_ADD)
cParamPipeline:AddVertexElement(0, 0, cGI.FORMAT_R32G32B32A32_SFLOAT, SIZE_FLOAT4)
cParamPipeline:AddVertexElement(1, 1, cGI.FORMAT_R32G32B32A32_SFLOAT, SIZE_FLOAT4)
cParamPipeline:AddVertexElement(2, 2, cGI.FORMAT_R32_SFLOAT, SIZE_FLOAT1)
cParamPipeline:AddVertexElement(3, 3, cGI.FORMAT_R8G8B8A8_UNORM, SIZE_UINT1)
cParamPipeline:AddVertexElement(4, 4, cGI.FORMAT_R32_SINT, SIZE_INT1)
cParamPipeline:SetVertexInputRate(4, true)
g_plGrid3d = cGI:NewPipeline(g_rp0, 0, 1, grid3d_vs, 'main', grid3d_ps, 'main', cParamPipeline)

--object coord pipeline
cParamPipeline:Reset()
cParamPipeline:AddResourceLayout(g_rlUB)
cParamPipeline:SetRasterizerStates(cGI.PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, cGI.POLYGON_MODE_FILL, cGI.CULL_MODE_BACK_BIT, true, false, false, false)
cParamPipeline:SetDethStencilStates(false, false, false, cGI.COMPARE_OP_GREATER_OR_EQUAL, false)
cParamPipeline:SetBlendState(0, true)
cParamPipeline:SetBsColorBlendOp(0, cGI.BLEND_FACTOR_SRC_ALPHA, cGI.BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, cGI.BLEND_OP_ADD)
cParamPipeline:AddVertexElement(0, 0, cGI.FORMAT_R32G32B32_SFLOAT, SIZE_FLOAT3)
cParamPipeline:AddVertexElement(1, 1, cGI.FORMAT_R8G8B8A8_UNORM, SIZE_UINT1)
cParamPipeline:AddVertexElement(2, 2, cGI.FORMAT_R32G32B32A32_SFLOAT, SIZE_FLOAT4)
cParamPipeline:AddVertexElement(2, 3, cGI.FORMAT_R32G32B32A32_SFLOAT, SIZE_FLOAT4)
cParamPipeline:AddVertexElement(2, 4, cGI.FORMAT_R32G32B32A32_SFLOAT, SIZE_FLOAT4)
cParamPipeline:AddVertexElement(2, 5, cGI.FORMAT_R32G32B32A32_SFLOAT, SIZE_FLOAT4)
cParamPipeline:SetVertexInputRate(1, true)
cParamPipeline:SetVertexInputRate(2, true)
g_plCoord3d = cGI:NewPipeline(g_rp0, 0, 1, coord3d_vs, 'main', object3d_ps, 'main', cParamPipeline)

--3d material
g_mtl3d = {inst = {}}
g_mtl3d.vbLayout = NewVBLayout(1|2|4, true, SIZE_FLOAT3, SIZE_FLOAT2, SIZE_UINT1)

g_mtl3d.func = {}

g_mtl3d.func[g_rp0[1]] = {func = function(mtl, dcList)
	dcList:AddResourceSet(g_resCamera)
	dcList:SetPipeline(g_pl3d, g_mtl3d.vbLayout, 0)
end, mergeType = DC_MTL_MERGED, order = g_mtl3d}

g_mtl3d.func[g_rp0[2]] = {func = function(mtl, dcList)
	dcList:AddResourceSet(g_resCamera)
	dcList:SetPipeline(g_plId3d, g_mtl3d.vbLayout, 0)
end, mergeType = DC_MTL_MERGED, order = g_mtl3d}

g_mtl3d.func[g_rp0[3]] = {func = function(mtl, dcList)
	dcList:AddResourceSet(g_resCamera)
	dcList:SetPipeline(g_plId3d, g_mtl3d.vbLayout, 0)
end, mergeType = DC_MTL_MERGED, order = g_mtl3d}

g_mtl3d.func[g_rp1[1]] = {func = function(mtl, dcList)
	dcList:AddResourceSet(g_resCamera)
	dcList:AddResourceSet(g_resIdImage)
	dcList:SetPipeline(g_plIdOutline, g_mtl3d.vbLayout, 0)
end, mergeType = DC_MTL_MERGED, order = g_mtl3d}

g_mtl3dInst = {inst = g_perObjInstance}
g_mtl3dInst.vbLayout = NewVBLayout(1|2|4, false, SIZE_FLOAT3, SIZE_FLOAT2, SIZE_UINT1)

g_mtl3dInst.func = {}

g_mtl3dInst.func[g_rp0[1]] = {func = function(mtl, dcList)
	dcList:AddResourceSet(g_resCamera)
	dcList:SetPipeline(g_pl3dInst, g_mtl3dInst.vbLayout, 0)
	dcList:SetInstVB(g_perObjInstance.vbMtx, 3)
end, mergeType = DC_MTL_MERGED, order = g_mtl3dInst}

g_mtl3dInst.func[g_rp0[2]] = {func = function(mtl, dcList)
	dcList:AddResourceSet(g_resCamera)
	dcList:SetPipeline(g_plId3dInst, g_mtl3dInst.vbLayout, 0)
	dcList:SetInstVB(g_perObjInstance.vbId, 3)
	dcList:SetInstVB(g_perObjInstance.vbMtx, 4)
end, mergeType = DC_MTL_MERGED, order = g_mtl3dInst}

g_mtl3dInst.func[g_rp0[3]] = {func = function(mtl, dcList)
	dcList:AddResourceSet(g_resCamera)
	dcList:SetPipeline(g_plRecId3dInst, g_mtl3dInst.vbLayout, 0)
	dcList:SetInstVB(g_perObjInstance.vbId, 3)
	dcList:SetInstVB(g_perObjInstance.vbMtx, 4)
end, mergeType = DC_MTL_MERGED, order = g_mtl3dInst}

g_mtl3dInst.func[g_rp1[1]] = {func = function(mtl, dcList)
	dcList:AddResourceSet(g_resCamera)
	dcList:AddResourceSet(g_resIdImage)
	dcList:SetPipeline(g_plIdOutline, g_mtl3dInst.vbLayout, 0)
	dcList:SetInstVB(g_perObjInstance.vbId, 3)
	dcList:SetInstVB(g_perObjInstance.vbMtx, 4)
end, mergeType = DC_MTL_MERGED, order = g_mtl3dInst}

--coord3d material
g_mtlCoord3d = {inst = g_perObjInst1}
g_mtlCoord3d.vbLayout = NewVBLayout(1, false, SIZE_FLOAT3)
g_mtlCoord3d.func = {}

g_mtlCoord3d.func[g_rp0[1]] = {func = function(mtl, dcList)
	dcList:AddResourceSet(g_resCamera)
	dcList:SetPipeline(g_plCoord3d, g_mtlCoord3d.vbLayout, 0)
	dcList:SetInstVB(g_perObjInst1.vbExtra, 1)
	dcList:SetInstVB(g_perObjInst1.vbMtx, 2)
end, mergeType = DC_SORTED_EDITOR, order = 2}

g_mtlCoord3d.func[g_rp0[2]] = {func = function(mtl, dcList)
	dcList:AddResourceSet(g_resCamera)
	dcList:SetPipeline(g_plIdCoord3d, g_mtlCoord3d.vbLayout, 0)
	dcList:SetInstVB(g_perObjInst1.vbId, 3)
	dcList:SetInstVB(g_perObjInst1.vbMtx, 4)
end, mergeType = DC_SORTED_EDITOR, order = 2}

g_mtlOrigin3d = {inst = g_perObjInst1}
g_mtlOrigin3d.vbLayout = NewVBLayout(1, false, SIZE_FLOAT3)
g_mtlOrigin3d.func = {}

g_mtlOrigin3d.func[g_rp0[1]] = {func = function(mtl, dcList)
	dcList:AddResourceSet(g_resCamera)
	dcList:SetPipeline(g_plCoord3d, g_mtlCoord3d.vbLayout, 0)
	dcList:SetInstVB(g_perObjInst1.vbExtra, 1)
	dcList:SetInstVB(g_perObjInst1.vbMtx, 2)
end, mergeType = DC_SORTED_EDITOR, order = 3}

g_mtlOrigin3d.func[g_rp0[2]] = {func = function(mtl, dcList)
	dcList:AddResourceSet(g_resCamera)
	dcList:SetPipeline(g_plIdCoord3d, g_mtlCoord3d.vbLayout, 0)
	dcList:SetInstVB(g_perObjInst1.vbId, 3)
	dcList:SetInstVB(g_perObjInst1.vbMtx, 4)
end, mergeType = DC_SORTED_EDITOR, order = 3}

--grid3d material
g_gridSeqInst = InstanceBuffer(1000 * SIZE_INT1)

g_mtlGrid3d = {inst = g_gridSeqInst}
g_mtlGrid3d.vbLayout = NewVBLayout(1|2|4|8, false, SIZE_FLOAT4, SIZE_FLOAT4, SIZE_FLOAT1, SIZE_UINT1)

g_mtlGrid3d.func = {}
g_mtlGrid3d.func[g_rp0[1]] = {func = function(mtl, dcList)
	dcList:AddResourceSet(g_resCamera)
	dcList:SetLineWidth(2)
	dcList:SetPipeline(g_plGrid3d, g_mtlGrid3d.vbLayout, 0)
	dcList:SetInstVB(g_mtlGrid3d.inst, 4)
end, mergeType = DC_SORTED_2, order = 1}

---Cube---
local vb = CMBuffer(1)
local ub = CMBuffer(1)
local cb = CMBuffer(1)
local ib = CMBuffer(1)
CAddCube(vb, 0, ub, 0, ib, 0)
CMulAddUByte4(4, cb, APPEND, 255, 0, 0, 255)
CMulAddUByte4(4, cb, APPEND, 0, 255, 0, 255)
CMulAddUByte4(4, cb, APPEND, 0, 0, 255, 255)
CMulAddUByte4(4, cb, APPEND, 255, 255, 0, 255)
CMulAddUByte4(4, cb, APPEND, 255, 0, 255, 255)
CMulAddUByte4(4, cb, APPEND, 0, 255, 255, 255)

local geoInfo = {}
geoInfo.layout = 1|2|4
geoInfo.vbInfo = {}
geoInfo.vbInfo[1] = {Geometry.TRANS_DEFAULT}
geoInfo.vbInfo[2] = {Geometry.TRANS_NONE, SIZE_FLOAT2}
geoInfo.vbInfo[3] = {Geometry.TRANS_NONE, SIZE_UINT1}
geoInfo.vb = {vb, ub, cb}
geoInfo.ib = ib
geoInfo.meshes = {}
geoInfo.meshes[1] = {0, 36, g_mtl3dInst}
g_cube = Geometry(geoInfo)

---plane3d---
local vb = CMBuffer(1)
local ub = CMBuffer(1)
local cb = CMBuffer(1)
local ib = CMBuffer(1)
CMulAddFloat3(1, vb, APPEND, -10, 0, 10)
CMulAddFloat3(1, vb, APPEND, 10, 0, 10)
CMulAddFloat3(1, vb, APPEND, 10, 0, -10)
CMulAddFloat3(1, vb, APPEND, -10, 0, -10)
CMulAddFloat2(1, ub, APPEND, 0, 0)
CMulAddFloat2(1, ub, APPEND, 1, 0)
CMulAddFloat2(1, ub, APPEND, 1, 1)
CMulAddFloat2(1, ub, APPEND, 0, 1)
CMulAddUByte4(4, cb, APPEND, 150, 150, 150, 255)
CAddConvexPolyIndex(4, ib, APPEND, 0, 1)
geoInfo = {}
geoInfo.layout = 1|2|4
geoInfo.vbInfo = {}
geoInfo.vbInfo[1] = {Geometry.TRANS_DEFAULT}
geoInfo.vbInfo[2] = {Geometry.TRANS_NONE, SIZE_FLOAT2}
geoInfo.vbInfo[3] = {Geometry.TRANS_NONE, SIZE_UINT1}
geoInfo.vb = {vb, ub, cb}
geoInfo.ib = ib
geoInfo.meshes = {}
geoInfo.meshes[1] = {0, 6, g_mtl3dInst}
g_plane3d = Geometry(geoInfo)

---Grid3d---
geoInfo = {}
geoInfo.layout = 1|2|4|8
geoInfo.vbInfo = {}
geoInfo.vbInfo[1] = {Geometry.TRANS_NONE, SIZE_FLOAT4}
geoInfo.vbInfo[2] = {Geometry.TRANS_NONE, SIZE_FLOAT4}
geoInfo.vbInfo[3] = {Geometry.TRANS_NONE, SIZE_FLOAT1}
geoInfo.vbInfo[4] = {Geometry.TRANS_NONE, SIZE_UINT1}
geoInfo.meshes = {}
geoInfo.meshes[1] = {0, 0, g_mtlGrid3d}
g_grid3d = Geometry(geoInfo)

---base cube---
local vb = CMBuffer(1)
local ib = CMBuffer(1)

CMulAddFloat3(1, vb, APPEND, -0.05, 0.05, 0.05)
CMulAddFloat3(1, vb, APPEND, 0.05, 0.05, 0.05)
CMulAddFloat3(1, vb, APPEND, 0.05, 0.05, -0.05)
CMulAddFloat3(1, vb, APPEND, -0.05, 0.05, -0.05)

CMulAddFloat3(1, vb, APPEND, -0.05, -0.05, 0.05)
CMulAddFloat3(1, vb, APPEND, 0.05, -0.05, 0.05)
CMulAddFloat3(1, vb, APPEND, 0.05, -0.05, -0.05)
CMulAddFloat3(1, vb, APPEND, -0.05, -0.05, -0.05)

CMulAddUInt3(1, ib, APPEND, 0, 4, 5)
CMulAddUInt3(1, ib, APPEND, 0, 5, 1)
CMulAddUInt3(1, ib, APPEND, 1, 5, 6)
CMulAddUInt3(1, ib, APPEND, 1, 6, 2)
CMulAddUInt3(1, ib, APPEND, 2, 6, 7)
CMulAddUInt3(1, ib, APPEND, 2, 7, 3)
CMulAddUInt3(1, ib, APPEND, 3, 7, 4)
CMulAddUInt3(1, ib, APPEND, 3, 4, 0)
CMulAddUInt3(1, ib, APPEND, 0, 2, 3)
CMulAddUInt3(1, ib, APPEND, 0, 1, 2)
CMulAddUInt3(1, ib, APPEND, 4, 7, 6)
CMulAddUInt3(1, ib, APPEND, 4, 6, 5)

geoInfo = {}
geoInfo.layout = 1
geoInfo.vbInfo = {}
geoInfo.vbInfo[1] = {Geometry.TRANS_DEFAULT}
geoInfo.meshes = {}
geoInfo.meshes[1] = {0, 3 * 12, g_mtlOrigin3d}
geoInfo.vb = {vb}
geoInfo.ib = ib
g_baseCube = Geometry(geoInfo)

---arrow---
local vbY = CMBuffer(1)
local ib = CMBuffer(1)
CMulAddFloat3(1, vbY, APPEND, 0, 2, 0)

CMulAddFloat3(1, vbY, APPEND, -0.1, 1.7, 0.1)
CMulAddFloat3(1, vbY, APPEND, 0.1, 1.7, 0.1)
CMulAddFloat3(1, vbY, APPEND, 0.1, 1.7, -0.1)
CMulAddFloat3(1, vbY, APPEND, -0.1, 1.7, -0.1)

CMulAddFloat3(1, vbY, APPEND, -0.05, 1.7, 0.05)
CMulAddFloat3(1, vbY, APPEND, 0.05, 1.7, 0.05)
CMulAddFloat3(1, vbY, APPEND, 0.05, 1.7, -0.05)
CMulAddFloat3(1, vbY, APPEND, -0.05, 1.7, -0.05)

CMulAddFloat3(1, vbY, APPEND, -0.05, 0.05, 0.05)
CMulAddFloat3(1, vbY, APPEND, 0.05, 0.05, 0.05)
CMulAddFloat3(1, vbY, APPEND, 0.05, 0.05, -0.05)
CMulAddFloat3(1, vbY, APPEND, -0.05, 0.05, -0.05)

CMulAddUInt3(1, ib, APPEND, 0, 1, 2)
CMulAddUInt3(1, ib, APPEND, 0, 2, 3)
CMulAddUInt3(1, ib, APPEND, 0, 3, 4)
CMulAddUInt3(1, ib, APPEND, 0, 4, 1)

CMulAddUInt3(1, ib, APPEND, 1, 4, 3)
CMulAddUInt3(1, ib, APPEND, 1, 3, 2)

CMulAddUInt3(1, ib, APPEND, 5, 9, 10)
CMulAddUInt3(1, ib, APPEND, 5, 10, 6)
CMulAddUInt3(1, ib, APPEND, 6, 10, 11)
CMulAddUInt3(1, ib, APPEND, 6, 11, 7)
CMulAddUInt3(1, ib, APPEND, 7, 11, 12)
CMulAddUInt3(1, ib, APPEND, 7, 12, 8)
CMulAddUInt3(1, ib, APPEND, 8, 12, 9)
CMulAddUInt3(1, ib, APPEND, 8, 9, 5)

CMulAddUInt3(1, ib, APPEND, 9, 12, 11)
CMulAddUInt3(1, ib, APPEND, 9, 11, 10)

local rot = CMatrix()
local vbX = CMBuffer(1)
rot:SetRotation(0, 0, 1, -90)
CTransformFloat3(vbX, 0, vbY, 0, 13, rot)
local vbZ = CMBuffer(1)
rot:SetRotation(1, 0, 0, -90)
CTransformFloat3(vbZ, 0, vbY, 0, 13, rot)

geoInfo.meshes = {}
geoInfo.meshes[1] = {0, 3 * 16, g_mtlCoord3d}
geoInfo.vb = {vbY}
geoInfo.ib = ib
g_arrowY = Geometry(geoInfo)

geoInfo.vb = {vbX}
g_arrowX = Geometry(geoInfo)

geoInfo.vb = {vbZ}
g_arrowZ = Geometry(geoInfo)

-- cParamResourceLayout:Reset()
-- cParamResourceLayout:Add(RESOURCE_TYPE_UNIFORM_BUFFER, 0, 1, SHADER_STAGE_VERTEX_BIT)
-- self.resourceLayout = cGI:NewResourceLayout(cParamResourceLayout)
-- self.resourceSet = self.resourceLayout:NewResourceSet()