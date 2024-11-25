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

f = CNewFileInput(false)
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
g_plUi = {}
cParamPipeline:Reset()
cParamPipeline:AddResourceLayout(g_rl0)
cParamPipeline:AddResourceLayout(g_rl1)
cParamPipeline:SetRasterizerStates(cGI.PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, cGI.POLYGON_MODE_FILL, cGI.CULL_MODE_BACK_BIT, true, false, false, false)
cParamPipeline:SetDethStencilStates(true, true, true, cGI.COMPARE_OP_LESS_OR_EQUAL, false)
cParamPipeline:SetBlendState(0, true)
cParamPipeline:SetBsColorBlendOp(0, cGI.BLEND_FACTOR_SRC_ALPHA, cGI.BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, cGI.BLEND_OP_ADD)
cParamPipeline:AddVertexElement(0, 0, cGI.FORMAT_R32G32_SFLOAT, SIZE_FLOAT2)
cParamPipeline:AddVertexElement(1, 1, cGI.FORMAT_R32G32B32_SFLOAT, SIZE_FLOAT3)
cParamPipeline:AddVertexElement(2, 2, cGI.FORMAT_R8G8B8A8_UNORM, SIZE_UINT1)
g_plUi.pl = cGI:NewPipeline(g_rp0, 0, 1, ui_vs, 'main', ui_ps, 'main', cParamPipeline)
g_plUi.nElems = 3
g_plUi.slot = {}
g_plUi.slot[0] = 0
g_plUi.slot[1] = 1
g_plUi.slot[2] = 2
g_plUi.stride = {}
g_plUi.stride[0] = SIZE_FLOAT2
g_plUi.stride[1] = SIZE_FLOAT3
g_plUi.stride[2] = SIZE_UINT1

g_plId2D = {}
cParamPipeline:Reset()
cParamPipeline:AddResourceLayout(g_rl0)
cParamPipeline:SetRasterizerStates(cGI.PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, cGI.POLYGON_MODE_FILL, cGI.CULL_MODE_BACK_BIT, true, false, false, false)
cParamPipeline:SetDethStencilStates(true, true, true, cGI.COMPARE_OP_LESS_OR_EQUAL, false)
cParamPipeline:AddVertexElement(0, 0, cGI.FORMAT_R32G32B32_SFLOAT, SIZE_FLOAT2)
cParamPipeline:AddVertexElement(1, 1, cGI.FORMAT_WRITE_ID, SIZE_WRITE_ID)
cParamPipeline:SetVertexInputRate(1, true)
g_plId2D.pl = cGI:NewPipeline(g_rp0, 1, 1, id_ui_vs, 'main', id_ui_ps, 'main', cParamPipeline)
g_plId2D.nElems = 2
g_plId2D.slot = {}
g_plId2D.slot[0] = 0
g_plId2D.slot[1] = 1
g_plId2D.stride = {}
g_plId2D.stride[0] = SIZE_FLOAT2
g_plId2D.stride[1] = SIZE_WRITE_ID

uiFont.res = g_rl1:NewResourceSet()
uiFont.res:BindBuffer(uiFont.view, 0, cGI.WHOLE_SIZE, 0, cGI.RESOURCE_TYPE_UNIFORM_TEXEL_BUFFER)

g_vsInput2 = NewVSInput2()

-- cParamResourceLayout:Reset()
-- cParamResourceLayout:Add(RESOURCE_TYPE_UNIFORM_BUFFER, 0, 1, SHADER_STAGE_VERTEX_BIT)
-- self.resourceLayout = cGI:NewResourceLayout(cParamResourceLayout)
-- self.resourceSet = self.resourceLayout:NewResourceSet()