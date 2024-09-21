#include "ParamVulkan.h"

ParamRenderPass& ParamRenderPass::GetVulkanParam()
{
	static VKParamRenderPass param;
	return param;
}

ParamFrameBuffer& ParamFrameBuffer::GetVulkanParam()
{
	static VKParamFrameBuffer param;
	return param;
}

ParamResourceLayout& ParamResourceLayout::GetVulkanParam()
{
	static VKParamResourceLayout param;
	return param;
}

ParamPipeline& ParamPipeline::GetVulkanParam()
{
	static VKParamPipeline param;
	return param;
}

ParamSampler& ParamSampler::GetVulkanParam()
{
	static VKParamSampler param;
	return param;
}