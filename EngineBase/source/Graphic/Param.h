#pragma once
#include "Graphic.h"

struct ParamRenderPass
{
	virtual ~ParamRenderPass() {}

	virtual void Reset(bool hasSwapchain, bool scLoad) = 0;

	virtual void AddViewDesc(uint32_t format, uint32_t sample, bool load, bool store, bool stencilLoad, bool stencilStore) = 0;

	virtual void AddViewOutput(uint32_t targetIdx, uint32_t subpass, bool isResolved, uint32_t resolvedIdx) = 0;

	virtual void AddSwapchainOutput(uint32_t subpass, bool isResolved, uint32_t resolvedIdx) = 0;

	virtual void SetDepthStencilOutput(uint32_t targetIdx, uint32_t subpass) = 0;

	virtual void AddViewInput(uint32_t subpass, uint32_t idx) = 0;

	static ParamRenderPass& GetVulkanParam();

	Lua_wrap_cpp_class(ParamRenderPass, Lua_abstract,
		Lua_mf(Reset),
		Lua_mf(AddViewDesc),
		Lua_mf(AddViewOutput),
		Lua_mf(AddSwapchainOutput),
		Lua_mf(SetDepthStencilOutput),
		Lua_mf(AddViewInput));
};

struct ParamFrameBuffer
{
	virtual ~ParamFrameBuffer() {}

	virtual void Reset() = 0;

	virtual void SetSwapchain(LuacObj<Swapchain> swapchain) = 0;

	virtual void AddView(LuacObj<Texture> view, uint32_t layer) = 0;

	static ParamFrameBuffer& GetVulkanParam();

	Lua_wrap_cpp_class(ParamFrameBuffer, Lua_abstract, Lua_mf(Reset), Lua_mf(SetSwapchain), Lua_mf(AddView));
};

struct ParamResourceLayout
{
	virtual ~ParamResourceLayout() {}

	virtual void Reset() = 0;

	virtual void Add(uint32_t type, uint32_t binding, uint32_t count, uint32_t stageFlags) = 0;

	static ParamResourceLayout& GetVulkanParam();

	Lua_wrap_cpp_class(ParamResourceLayout, Lua_abstract, Lua_mf(Reset), Lua_mf(Add));
};

struct ParamPipeline
{
	virtual ~ParamPipeline() {}

	virtual void Reset() = 0;

	virtual void AddResourceLayout(LuacObj<ResourceLayout> resLayout) = 0;

	virtual void SetRasterizerStates(uint32_t topology, uint32_t polygonMode, uint32_t cullMode, bool clockwise, bool depthClampEnable, bool depthBiasEnable, bool discardEnable) = 0;

	virtual void SetDethStencilStates(bool depthTestEnable, bool depthBoundTestEnable, bool depthWriteEnable, uint32_t depthCompareOp, bool stencilTestEnble) = 0;

	virtual void SetDssStencilOpFail(uint32_t frontOp, uint32_t backOp) = 0;

	virtual void SetDssStencilOpPass(uint32_t frontOp, uint32_t backOp) = 0;

	virtual void SetDssStencilOpDepthFail(uint32_t frontOp, uint32_t backOp) = 0;

	virtual void SetDssStencilOpCompareOp(uint32_t frontOp, uint32_t backOp) = 0;


	virtual void SetBlendState(uint32_t index, bool enable) = 0;

	virtual void SetBsColorBlendOp(uint32_t index, uint32_t srcFactor, uint32_t dstFactor, uint32_t op) = 0;

	virtual void SetBsAlphaBlendOp(uint32_t index, uint32_t srcFactor, uint32_t dstFactor, uint32_t op) = 0;

	virtual void SetBsColorWriteMask(uint32_t index, bool r, bool g, bool b, bool a) = 0;


	virtual void SetVertexInputRate(uint32_t slot, bool perInstance) = 0;

	virtual void AddVertexElement(uint32_t slot, uint32_t location, uint32_t format, uint32_t stride) = 0;

	static ParamPipeline& GetVulkanParam();

	Lua_wrap_cpp_class(ParamPipeline, Lua_abstract,
		Lua_mf(Reset),
		Lua_mf(AddResourceLayout),
		Lua_mf(SetRasterizerStates),
		Lua_mf(SetDethStencilStates),
		Lua_mf(SetDssStencilOpFail),
		Lua_mf(SetDssStencilOpPass),
		Lua_mf(SetDssStencilOpDepthFail),
		Lua_mf(SetDssStencilOpCompareOp),
		Lua_mf(SetBlendState),
		Lua_mf(SetBsColorBlendOp),
		Lua_mf(SetBsAlphaBlendOp),
		Lua_mf(SetBsColorWriteMask),
		Lua_mf(SetVertexInputRate),
		Lua_mf(AddVertexElement));
};

struct ParamSampler
{
	virtual ~ParamSampler() {}

	virtual void Reset() = 0;

	virtual void SetFilterMode(uint32_t mag, uint32_t min) = 0;

	virtual void SetMipmapMode(uint32_t mode, float maxLod, float minLod, float bias) = 0;

	virtual void SetAddressMode(uint32_t mode_u, uint32_t mode_v, uint32_t mode_w, bool unnormalizedCoordinates) = 0;

	virtual void SetAnisotropyMode(bool enable, float maxAnisotropy) = 0;

	virtual void SetCompareMode(bool enable, uint32_t op) = 0;

	static ParamSampler& GetVulkanParam();

	Lua_wrap_cpp_class(ParamSampler, Lua_abstract,
		Lua_mf(Reset),
		Lua_mf(SetFilterMode),
		Lua_mf(SetMipmapMode),
		Lua_mf(SetAddressMode),
		Lua_mf(SetAnisotropyMode),
		Lua_mf(SetCompareMode));
};