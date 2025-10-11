---global---
require 'graphic'
require 'utility'
require 'geometry'

g_recorder = Recorder()

g_content = {}

g_scene = SceneObject()

g_innerPolyVB = CMBuffer(1024)
g_innerPolyVB.offset = 0
g_innerPolyIB = CMBuffer(1024)
g_innerPolyIB.offset = 0

local function AddPolyVertex2D(o, t, i, x, y, nv, nvc)
	local v = t[i]
	nv = nv + 1
	nvc = nvc + 1
	CAddFloat3(v[1], v[2], Z_2D, g_innerPolyVB, APPEND, 1)
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
	local replaces
	local color = Color()
	color.wp = 0
	local cwp = 0
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
				table.insert(color.aa, {SIZE_UINT1 * cwp, nv / 2 - 1})
				cwp = cwp + nv
				nv = WritePolyIndex2D(o, nv, replaces)
			end
		end
		if (t.color) then
			color:copy(t.color)
			color.nvc = nvc
			table.insert(o.colors, color)
			color = Color()
			color.wp = SIZE_FLOAT3 * o.vtx_count
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

g_iconFolder = AddPoly2D(true, { {5, 0}, {50, 0}, {55, 5},
						{110, 5}, {110, 10}, {0, 10}, {0, 5} },
						{ {0, 12}, {110, 12}, {110, 62}, {0, 62} })

local s = 12
local h = s / 2
g_iconArrowHeadL = AddPoly2D(true, { {0, h}, {s, 0}, {s, s} })
g_iconTriangleR = AddPoly2D(true, { {0, 0}, {s, h}, {0, s} })
g_iconTriangleU = AddPoly2D(true, { {h, 0}, {s, s}, {0, s} })
g_iconTriangleD = AddPoly2D(true, { {0, 0}, {s, 0}, {h, s} })
g_iconTriangleDR = AddPoly2D(true, { {0, s}, {s, 0}, {s, s} })

local r = 8
g_iconMagnifier = AddPoly2D(true, DrawLine(3, false, true, MakeCircle(0, 0, r, 16)), 
	DrawLine(4, true, false, {r + r - 4, r + r - 4}, {r + 15, r + 15}))

g_iconLine = AddPoly2D(true, DrawLine(5, false, false, {0, 0}, {100, 0}, {50, 100}, {366, 210}, {500, 710}))

local o = DrawLine(4, false, true, MakeCircle(0, 0, 10, 6))
o.color = Color(255, 255, 255, 255)
g_iconPreset = AddPoly2D(true, o)

g_iconLine1 = AddPoly2D(false, DrawLine(10, false, false, {100, 100}, {100, 300}))

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

g_idVb = cGI:NewBuffer(SIZE_WRITE_ID * ID_NUM_MAX)
for i = 0, ID_NUM_MAX do
	AddVertexID(i, g_idVb, APPEND, 1)
end
g_idVbSet = cGI:NewBufferSet({g_idVb}, 1)

local f = CNewFileInput(false)
f:Open('Resources/shaders/'..cGI:Type()..'/ui.vsc', true)
ui_vs = cGI:NewShaderModule(f)
f:Open('Resources/shaders/'..cGI:Type()..'/ui.psc', true)
ui_ps = cGI:NewShaderModule(f)
f:Open('Resources/shaders/'..cGI:Type()..'/ui_id.vsc', true)
ui_id_vs = cGI:NewShaderModule(f)

f:Open('Resources/shaders/'..cGI:Type()..'/object3d.vsc', true)
object3d_vs = cGI:NewShaderModule(f)
f:Open('Resources/shaders/'..cGI:Type()..'/object3d.psc', true)
object3d_ps = cGI:NewShaderModule(f)
f:Open('Resources/shaders/'..cGI:Type()..'/object3d_id.vsc', true)
object3d_id_vs = cGI:NewShaderModule(f)

f:Open('Resources/shaders/'..cGI:Type()..'/plane3d.vsc', true)
plane3d_vs = cGI:NewShaderModule(f)
f:Open('Resources/shaders/'..cGI:Type()..'/plane3d.psc', true)
plane3d_ps = cGI:NewShaderModule(f)

f:Open('Resources/shaders/'..cGI:Type()..'/grid3d.vsc', true)
grid3d_vs = cGI:NewShaderModule(f)

f:Open('Resources/shaders/'..cGI:Type()..'/ui2.vsc', true)
ui2_vs = cGI:NewShaderModule(f)

f:Open('Resources/shaders/'..cGI:Type()..'/id.psc', true)
id_ps = cGI:NewShaderModule(f)

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
cParamRenderPass:AddViewDesc(cGI.FORMAT_D24_UNORM_S8_UINT, cGI.SAMPLE_COUNT_1_BIT, false, false, false, false)
cParamRenderPass:AddSwapchainOutput(0, false, 0)
cParamRenderPass:AddViewOutput(0, 1, false, 0)
cParamRenderPass:SetDepthStencilOutput(1, 0)
cParamRenderPass:SetDepthStencilOutput(1, 1)
-- cParamRenderPass:AddViewDesc(cGI.FORMAT_PRESENT, cGI.SAMPLE_COUNT_4_BIT, false, false, false, false)
-- cParamRenderPass:AddViewDesc(cGI.FORMAT_PICK_ID, cGI.SAMPLE_COUNT_1_BIT, false, true, false, false)
-- cParamRenderPass:AddViewOutput(0, 0, false, 0)
-- cParamRenderPass:AddSwapchainOutput(0, true, 0)
-- cParamRenderPass:AddViewOutput(1, 1, false, 0)

g_rp0 = cGI:NewRenderPass(cParamRenderPass)
g_rp0[1] = SubpassId(g_rp0, 1)
g_rp0[2] = SubpassId(g_rp0, 2)

g_idView = {}
g_idView[g_rp0[2]] = 0

--resource layout
cParamResourceLayout:Reset()
cParamResourceLayout:Add(cGI.RESOURCE_TYPE_UNIFORM_BUFFER, 0, 1, cGI.SHADER_STAGE_VERTEX_BIT)
g_rlUB = cGI:NewResourceLayout(cParamResourceLayout)

cParamResourceLayout:Reset()
cParamResourceLayout:Add(cGI.RESOURCE_TYPE_UNIFORM_TEXEL_BUFFER, 0, 1, cGI.SHADER_STAGE_FRAGMENT_BIT)
g_rlTB = cGI:NewResourceLayout(cParamResourceLayout)

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

--ui2 pipeline
cParamPipeline:AddResourceLayout(g_rlUB)
cParamPipeline:AddResourceLayout(g_rlUB)
g_plUi2 = cGI:NewPipeline(g_rp0, 0, 1, ui2_vs, 'main', ui_ps, 'main', cParamPipeline)

cParamPipeline:Reset()
cParamPipeline:AddResourceLayout(g_rlUB)
cParamPipeline:SetRasterizerStates(cGI.PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, cGI.POLYGON_MODE_FILL, cGI.CULL_MODE_NONE, true, false, false, false)
cParamPipeline:SetDethStencilStates(false, false, true, cGI.COMPARE_OP_GREATER_OR_EQUAL, false)
cParamPipeline:AddVertexElement(0, 0, cGI.FORMAT_R32G32B32_SFLOAT, SIZE_FLOAT3)
cParamPipeline:AddVertexElement(3, 1, cGI.FORMAT_WRITE_ID, SIZE_WRITE_ID)
cParamPipeline:SetVertexInputRate(3, true)
g_plId2D = cGI:NewPipeline(g_rp0, 1, 1, ui_id_vs, 'main', id_ps, 'main', cParamPipeline)

--ui materal
g_mtlUi = {}
g_mtlUi.resFont = ResourceHub(g_rlTB)
g_mtlUi.resFont:BindTexelView(uiFont.view, 0)

g_mtlUi.vbLayout = NewVBLayout(1|2|4, SIZE_FLOAT3, SIZE_FLOAT3, SIZE_UINT1)
g_mtlUi.insSlot = {{g_rp0[1], 0, 1}}
g_mtlUi.idSlot = {g_rp0[2]}

g_mtlUi.func = {}

g_mtlUi.func[g_rp0[1]] = {func = function(mtl, dcList)
	dcList:AddResourceSet(g_resWnd)
	dcList:AddResourceSet(mtl.resFont)
	dcList:SetPipeline(g_plUi, g_mtlUi.vbLayout, 0)
end, mergeType = DC_DEFAULT}

g_mtlUi.func[g_rp0[2]] = {func = function(mtl, dcList)
	dcList:AddResourceSet(g_resWnd)
	dcList:SetPipeline(g_plId2D, g_mtlUi.vbLayout, 0)
	dcList:SetInsVB(g_idVbSet, 3)
end, mergeType = DC_DEFAULT}

--ui2 material
g_mtlUi2 = {}
g_mtlUi2.matModel = CMatrix3D()
g_mtlUi2.resModel = ResourceHub(g_rlUB)
local buf = g_mtlUi2.resModel:BindResBuffer(0, CMatrix3D._size)
CAddMatrix(g_mtlUi2.matModel, buf(), buf[1])
g_mtlUi2.resFont = ResourceHub(g_rlTB)
g_mtlUi2.resFont:BindTexelView(uiFont.view, 0)

g_mtlUi2.vbLayout = NewVBLayout(1|2|4, SIZE_FLOAT3, SIZE_FLOAT3, SIZE_UINT1)
g_mtlUi2.insSlot = {{g_rp0[1], 0, 1}}
g_mtlUi2.idSlot = {g_rp0[2]}

g_mtlUi2.func = {}

g_mtlUi2.func[g_rp0[1]] = {func = function(mtl, dcList)
	dcList:AddResourceSet(g_resWnd)
	dcList:AddResourceSet(mtl.resFont)
	dcList:AddResourceSet(mtl.resModel)
	dcList:AddResourceSet(g_resCamera)
	dcList:SetPipeline(g_plUi2, g_mtlUi2.vbLayout, 0)
end, mergeType = DC_DEFAULT}

g_mtlUi2.func[g_rp0[2]] = {func = function(mtl, dcList)
	dcList:AddResourceSet(g_resWnd)
	dcList:AddResourceSet(mtl.resModel)
	dcList:AddResourceSet(g_resCamera)
	dcList:SetPipeline(g_plId2D, g_mtlUi2.vbLayout, 0)
	dcList:SetInsVB(g_idVbSet, 3)
end, mergeType = DC_DEFAULT}

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
g_pl3D = cGI:NewPipeline(g_rp0, 0, 1, object3d_vs, 'main', object3d_ps, 'main', cParamPipeline)

cParamPipeline:SetRasterizerStates(cGI.PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, cGI.POLYGON_MODE_FILL, cGI.CULL_MODE_NONE, true, false, false, false)
cParamPipeline:SetDethStencilStates(true, false, false, cGI.COMPARE_OP_GREATER_OR_EQUAL, false)
g_plPlane3D = cGI:NewPipeline(g_rp0, 0, 1, plane3d_vs, 'main', plane3d_ps, 'main', cParamPipeline)

cParamPipeline:SetRasterizerStates(cGI.PRIMITIVE_TOPOLOGY_LINE_LIST, cGI.POLYGON_MODE_FILL, cGI.CULL_MODE_NONE, true, false, false, false)
g_plGrid3D = cGI:NewPipeline(g_rp0, 0, 1, grid3d_vs, 'main', object3d_ps, 'main', cParamPipeline)

cParamPipeline:Reset()
cParamPipeline:AddResourceLayout(g_rlUB)
cParamPipeline:SetRasterizerStates(cGI.PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, cGI.POLYGON_MODE_FILL, cGI.CULL_MODE_BACK_BIT, true, false, false, false)
cParamPipeline:SetDethStencilStates(true, false, true, cGI.COMPARE_OP_GREATER_OR_EQUAL, false)
cParamPipeline:AddVertexElement(0, 0, cGI.FORMAT_R32G32B32_SFLOAT, SIZE_FLOAT3)
cParamPipeline:AddVertexElement(3, 1, cGI.FORMAT_WRITE_ID, SIZE_WRITE_ID)
cParamPipeline:SetVertexInputRate(3, true)
g_plId3D = cGI:NewPipeline(g_rp0, 1, 1, object3d_id_vs, 'main', id_ps, 'main', cParamPipeline)

--3d material
g_mtl3d = {}
g_mtl3d.vbLayout = NewVBLayout(1|2|4, SIZE_FLOAT3, SIZE_FLOAT2, SIZE_UINT1)
g_mtl3d.insSlot = {{g_rp0[1], 0, 1}}
g_mtl3d.idSlot = {g_rp0[2]}

g_mtl3d.func = {}

g_mtl3d.func[g_rp0[1]] = {func = function(mtl, dcList)
	dcList:AddResourceSet(g_resCamera)
	dcList:SetPipeline(g_pl3D, g_mtl3d.vbLayout, 0)
end, mergeType = DC_MTL_MERGED, order = g_mtl3d}

g_mtl3d.func[g_rp0[2]] = {func = function(mtl, dcList)
	dcList:AddResourceSet(g_resCamera)
	dcList:SetPipeline(g_plId3D, g_mtl3d.vbLayout, 0)
	dcList:SetInsVB(g_idVbSet, 3)
end, mergeType = DC_MTL_MERGED, order = g_mtl3d}

--plane3d material
g_mtlPlane3d = {}
g_mtlPlane3d.vbLayout = NewVBLayout(1|2|4, SIZE_FLOAT3, SIZE_FLOAT2, SIZE_UINT1)
g_mtlPlane3d.insSlot = {{g_rp0[1], 0, 1}}
g_mtlPlane3d.idSlot = {}

g_mtlPlane3d.func = {}

g_mtlPlane3d.func[g_rp0[1]] = {func = function(mtl, dcList)
	dcList:AddResourceSet(g_resCamera)
	dcList:SetPipeline(g_plPlane3D, g_mtlPlane3d.vbLayout, 0)
end, mergeType = DC_SORTED_2, order = 1}

--grid3d material
g_mtlGrid3d = {}
g_mtlGrid3d.vbLayout = NewVBLayout(1|2|4, SIZE_FLOAT3, SIZE_FLOAT2, SIZE_UINT1)
g_mtlGrid3d.insSlot = {{g_rp0[1], 0, 1}}
g_mtlGrid3d.idSlot = {}

g_mtlGrid3d.func = {}
g_mtlGrid3d.func[g_rp0[1]] = {func = function(mtl, dcList)
	dcList:AddResourceSet(g_resCamera)
	dcList:SetLineWidth(2)
	dcList:SetPipeline(g_plGrid3D, g_mtlGrid3d.vbLayout, 0)
end, mergeType = DC_SORTED_2, order = 1}

g_previews = {}

---Cube---
local vb = CMBuffer(1)
local ub = CMBuffer(1)
local cb = CMBuffer(1)
local ib = CMBuffer(1)
CAddCube(vb, 0, ub, 0, ib, 0)
CAddUByte4(255, 0, 0, 255, cb, APPEND, 4)
CAddUByte4(0, 255, 0, 255, cb, APPEND, 4)
CAddUByte4(0, 0, 255, 255, cb, APPEND, 4)
CAddUByte4(255, 255, 0, 255, cb, APPEND, 4)
CAddUByte4(255, 0, 255, 255, cb, APPEND, 4)
CAddUByte4(0, 255, 255, 255, cb, APPEND, 4)

local geoInfo = {}
geoInfo.layout = 1|2|4
geoInfo.vb = {}
geoInfo.vb[1] = {vb, Geometry.TRANS_DEFAULT}
geoInfo.vb[2] = {ub, Geometry.TRANS_NONE}
geoInfo.vb[4] = {cb, Geometry.TRANS_NONE}
geoInfo.ib = ib
geoInfo.meshes = {}
geoInfo.meshes[1] = {0, 36, g_mtl3d}
g_cube = Geometry(geoInfo)

---plane3d---
local vb = CMBuffer(1)
local ub = CMBuffer(1)
local cb = CMBuffer(1)
local ib = CMBuffer(1)
CAddFloat3(-10, 0, 10, vb, APPEND, 1)
CAddFloat3(10, 0, 10, vb, APPEND, 1)
CAddFloat3(10, 0, -10, vb, APPEND, 1)
CAddFloat3(-10, 0, -10, vb, APPEND, 1)
CAddFloat2(0, 0, ub, APPEND, 1)
CAddFloat2(1, 0, ub, APPEND, 1)
CAddFloat2(1, 1, ub, APPEND, 1)
CAddFloat2(0, 1, ub, APPEND, 1)
CAddUByte4(150, 150, 150, 255, cb, APPEND, 4)
CAddConvexPolyIndex(0, 4, ib, APPEND, 1)
geoInfo = {}
geoInfo.layout = 1|2|4
geoInfo.vb = {}
geoInfo.vb[1] = {vb, Geometry.TRANS_DEFAULT}
geoInfo.vb[2] = {ub, Geometry.TRANS_NONE}
geoInfo.vb[4] = {cb, Geometry.TRANS_NONE}
geoInfo.ib = ib
geoInfo.meshes = {}
geoInfo.meshes[1] = {0, 6, g_mtlPlane3d}
g_plane3d = Geometry(geoInfo)

---Grid3d---
local vb = CMBuffer(1)
local ub = CMBuffer(1)
local cb = CMBuffer(1)
local ib = CMBuffer(1)
CAddFloat3(-10, 0, 0, vb, APPEND, 1)
CAddFloat3(10, 0, 0, vb, APPEND, 1)
CAddFloat3(0, 0, -10, vb, APPEND, 1)
CAddFloat3(0, 0, 10, vb, APPEND, 1)
CAddFloat2(0, 0, ub, APPEND, 1)
CAddFloat2(1, 0, ub, APPEND, 1)
CAddFloat2(1, 1, ub, APPEND, 1)
CAddFloat2(0, 1, ub, APPEND, 1)
CAddUByte4(150, 150, 150, 100, cb, APPEND, 4)
CAddUInt1(0, ib, APPEND, 1)
CAddUInt1(1, ib, APPEND, 1)
CAddUInt1(2, ib, APPEND, 1)
CAddUInt1(3, ib, APPEND, 1)
geoInfo = {}
geoInfo.layout = 1|2|4
geoInfo.vb = {}
geoInfo.vb[1] = {vb, Geometry.TRANS_DEFAULT}
geoInfo.vb[2] = {ub, Geometry.TRANS_NONE}
geoInfo.vb[4] = {cb, Geometry.TRANS_NONE}
geoInfo.ib = ib
geoInfo.meshes = {}
geoInfo.meshes[1] = {0, 4, g_mtlGrid3d}
g_grid3d = Geometry(geoInfo)

---w3d---
local vb = CMBuffer(1)
local ub = CMBuffer(1)
local cb = CMBuffer(1)
local ib = CMBuffer(1)
CAddFloat3(-1, 0.35, 0, vb, APPEND, 1)
CAddFloat3(-0.5, 0.35, 0, vb, APPEND, 1)
CAddFloat3(-0.5, 0.3, -0, vb, APPEND, 1)
CAddFloat3(-1, 0.3, -0, vb, APPEND, 1)
CAddFloat2(0, 0, ub, APPEND, 1)
CAddFloat2(1, 0, ub, APPEND, 1)
CAddFloat2(1, 1, ub, APPEND, 1)
CAddFloat2(0, 1, ub, APPEND, 1)
CAddUByte4(150, 150, 150, 255, cb, APPEND, 4)
CAddConvexPolyIndex(0, 4, ib, APPEND, 1)
geoInfo = {}
geoInfo.layout = 1|2|4
geoInfo.vb = {}
geoInfo.vb[1] = {vb, Geometry.TRANS_DEFAULT}
geoInfo.vb[2] = {ub, Geometry.TRANS_NONE}
geoInfo.vb[4] = {cb, Geometry.TRANS_NONE}
geoInfo.ib = ib
geoInfo.meshes = {}
geoInfo.meshes[1] = {0, 6, g_mtlPlane3d}
g_w3d = Geometry(geoInfo)

-- cParamResourceLayout:Reset()
-- cParamResourceLayout:Add(RESOURCE_TYPE_UNIFORM_BUFFER, 0, 1, SHADER_STAGE_VERTEX_BIT)
-- self.resourceLayout = cGI:NewResourceLayout(cParamResourceLayout)
-- self.resourceSet = self.resourceLayout:NewResourceSet()