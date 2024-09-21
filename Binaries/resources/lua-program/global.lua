---global---
require 'graphic'
require 'utility'

-- TargetViewDescs = {view1 = {samples = 1, format = FORMAT}}

-- FrameBufferDesc1 = {rp = '', views = {-1, 1, 2, 3, 4}}
-- CreatedViews = {}

g_idVb = cGI:NewBuffer(SIZE_WRITE_ID * ID_NUM_MAX)
for i = 0, ID_NUM_MAX do
	AddVertexID(g_idVb, APPEND, 1, i)
end

function NewVSInput2D(n)
	n = n or 1024
	local o = {}
	o[VB_ELEM_FLOAT2_0] = cGI:NewBuffer(n * SIZE_FLOAT2)
	o[VB_ELEM_FLOAT3_0] = cGI:NewBuffer(n * SIZE_FLOAT3)
	o[VB_ELEM_FLOAT4_0] = cGI:NewBuffer(n * SIZE_UINT1)
	o[VB_ELEM_ID] = g_idVb
	o.vbSet = cGI:NewBufferSet(nil, 0)
	o.vbSet:Add(o[VB_ELEM_FLOAT2_0])
	o.vbSet:Add(o[VB_ELEM_FLOAT3_0])
	o.vbSet:Add(o[VB_ELEM_FLOAT4_0])
	o.vbSet:Add(o[VB_ELEM_ID])
	o.ib = cGI:NewBuffer(n * SIZE_UINT1)
	ResetVSInput2D(o)
	return o
end

function ResetVSInput2D(o)
	o.vbSet:SetWritePos(0)
	o.vbSet:SetDrawOffset(0)
	o.ib:SetWritePos(0)
end

f = {}
CNewFileInput(f, false)
f:Open('Resources/shaders/'..cGI:Type()..'/ui_vs.sc', true)
ui_vs = cGI:NewShaderModule(f)
f:Open('Resources/shaders/'..cGI:Type()..'/ui_ps.sc', true)
ui_ps = cGI:NewShaderModule(f)

f:Open('Resources/shaders/'..cGI:Type()..'/id_ui_vs.sc', true)
id_ui_vs = cGI:NewShaderModule(f)
f:Open('Resources/shaders/'..cGI:Type()..'/id_ui_ps.sc', true)
id_ui_ps = cGI:NewShaderModule(f)

f:Open('Resources/atlas2', true)
uiFont = {}
CLoadFontAtlas(uiFont, f, 256)

f:Close()

--pass
cParamRenderPass:Reset(true, false)
-- cParamRenderPass:AddSwapchainOutput(0, false, 0)
-- cParamRenderPass:AddViewDesc(cGI.FORMAT_PICK_ID, cGI.SAMPLE_COUNT_1_BIT, false, true, false, false)
-- cParamRenderPass:AddViewOutput(0, 1, false, 0)

cParamRenderPass:AddViewDesc(cGI.FORMAT_SWAPCHAIN, cGI.SAMPLE_COUNT_4_BIT, false, false, false, false)
cParamRenderPass:AddViewOutput(0, 0, false, 0)
cParamRenderPass:AddSwapchainOutput(0, true, 0)
cParamRenderPass:AddViewDesc(cGI.FORMAT_PICK_ID, cGI.SAMPLE_COUNT_1_BIT, false, true, false, false)
cParamRenderPass:AddViewOutput(1, 1, false, 0)
g_rp0 = cGI:NewRenderPass(cParamRenderPass)

--resource layout
cParamResourceLayout:Reset()
cParamResourceLayout:Add(cGI.RESOURCE_TYPE_UNIFORM_BUFFER, 0, 1, cGI.SHADER_STAGE_VERTEX_BIT)
g_rl0 = cGI:NewResourceLayout(cParamResourceLayout)

cParamResourceLayout:Reset()
cParamResourceLayout:Add(cGI.RESOURCE_UNIFORM_TEXEL_BUFFER, 0, 1, cGI.SHADER_STAGE_FRAGMENT_BIT)
g_rl1 = cGI:NewResourceLayout(cParamResourceLayout)

--pipeline
cParamPipeline:Reset()
cParamPipeline:AddResourceLayout(g_rl0)
cParamPipeline:AddResourceLayout(g_rl1)
cParamPipeline:SetRasterizerStates(cGI.PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, cGI.POLYGON_MODE_FILL, cGI.CULL_MODE_BACK_BIT, true, false, false, false)
cParamPipeline:SetDethStencilStates(true, true, true, cGI.COMPARE_OP_LESS_OR_EQUAL, false)
cParamPipeline:SetBlendState(0, true)
cParamPipeline:SetBsColorBlendOp(0, cGI.BLEND_FACTOR_SRC_ALPHA, cGI.BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, cGI.BLEND_OP_ADD)
cParamPipeline:AddVertexElement(0, VB_ELEM_FLOAT2_0.LOC, cGI.FORMAT_R32G32_SFLOAT, SIZE_FLOAT2)
cParamPipeline:AddVertexElement(1, VB_ELEM_FLOAT3_0.LOC, cGI.FORMAT_R32G32B32_SFLOAT, SIZE_FLOAT3)
cParamPipeline:AddVertexElement(2, VB_ELEM_FLOAT4_0.LOC, cGI.FORMAT_R8G8B8A8_UNORM, SIZE_UINT1)
g_plUi = cGI:NewPipeline(g_rp0, 0, 1, ui_vs, 'main', ui_ps, 'main', cParamPipeline)

cParamPipeline:Reset()
cParamPipeline:AddResourceLayout(g_rl0)
cParamPipeline:SetRasterizerStates(cGI.PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, cGI.POLYGON_MODE_FILL, cGI.CULL_MODE_BACK_BIT, true, false, false, false)
cParamPipeline:SetDethStencilStates(true, true, true, cGI.COMPARE_OP_LESS_OR_EQUAL, false)
cParamPipeline:AddVertexElement(0, VB_ELEM_FLOAT2_0.LOC, cGI.FORMAT_R32G32B32_SFLOAT, SIZE_FLOAT2)
cParamPipeline:AddVertexElement(3, VB_ELEM_ID.LOC, cGI.FORMAT_WRITE_ID, SIZE_WRITE_ID)
cParamPipeline:SetVertexInputRate(3, true)
g_plId2D = cGI:NewPipeline(g_rp0, 1, 1, id_ui_vs, 'main', id_ui_ps, 'main', cParamPipeline)

uiFont.res = g_rl1:NewResourceSet()
uiFont.res:BindBuffer(uiFont.view, 0, cGI.RESOURCE_UNIFORM_TEXEL_BUFFER)

-- cParamResourceLayout:Reset()
-- cParamResourceLayout:Add(RESOURCE_TYPE_UNIFORM_BUFFER, 0, 1, SHADER_STAGE_VERTEX_BIT)
-- self.resourceLayout = cGI:NewResourceLayout(cParamResourceLayout)
-- self.resourceSet = self.resourceLayout:NewResourceSet()