#pragma once
#include "Param.h"
#include "GraphicVulkan.h"
#include <vector>
#include <map>

#define MAX_RT 8

struct Subpass
{
	VkAttachmentReference inputAtt[MAX_RT]{};
	VkAttachmentReference outputAtt[MAX_RT]{};
	VkAttachmentReference resolvedAtt[MAX_RT]{};
	VkAttachmentReference depthStencilAtt[1]{};
};

struct VKParamRenderPass : public ParamRenderPass
{
	void Reset(bool sc, bool scLoad) override
	{
		subpassDescs.clear();
		viewDescs.clear();
		dependencies.clear();
		subpasses.clear();
		dep.clear();
		samples.clear();
		startIdx = 0;
		hasSwapchain = sc;
		if (hasSwapchain)
		{
			startIdx = 1;
			viewDescs.resize(viewDescs.size() + 1);
			viewDescs.back().format = g->defaultFormat;
			viewDescs.back().samples = VK_SAMPLE_COUNT_1_BIT;
			viewDescs.back().loadOp = scLoad ? VK_ATTACHMENT_LOAD_OP_LOAD : VK_ATTACHMENT_LOAD_OP_CLEAR;
			viewDescs.back().storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			viewDescs.back().initialLayout = scLoad ? VK_IMAGE_LAYOUT_PRESENT_SRC_KHR : VK_IMAGE_LAYOUT_UNDEFINED;
			viewDescs.back().finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		}
	}

	void AddViewDesc(uint32_t format, uint32_t sample, bool load, bool store, bool stencilLoad, bool stencilStore) override
	{
		auto layout = VK_IMAGE_LAYOUT_UNDEFINED;
		if (load)
			if (format == VK_FORMAT_D24_UNORM_S8_UINT)
				layout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
			else
				layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		viewDescs.resize(viewDescs.size() + 1);
		viewDescs.back().format = VkFormat(format);
		viewDescs.back().samples = VkSampleCountFlagBits(sample);
		viewDescs.back().loadOp = load ? VK_ATTACHMENT_LOAD_OP_LOAD : VK_ATTACHMENT_LOAD_OP_CLEAR;
		viewDescs.back().storeOp = store ? VK_ATTACHMENT_STORE_OP_STORE : VK_ATTACHMENT_STORE_OP_DONT_CARE;
		viewDescs.back().stencilLoadOp = stencilLoad ? VK_ATTACHMENT_LOAD_OP_LOAD : VK_ATTACHMENT_LOAD_OP_CLEAR;
		viewDescs.back().stencilStoreOp = stencilStore ? VK_ATTACHMENT_STORE_OP_STORE : VK_ATTACHMENT_STORE_OP_DONT_CARE;
		viewDescs.back().initialLayout = layout;
		viewDescs.back().finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	}

	void AddSwapchainOutput(uint32_t subpass, bool isResolved, uint32_t resolvedIdx) override
	{
		ResizeSubpasses(subpass);
		if (isResolved)
			subpasses[subpass].resolvedAtt[resolvedIdx].attachment = 0;
		else
			subpasses[subpass].outputAtt[subpassDescs[subpass].colorAttachmentCount++].attachment = 0;
		dep[0][subpass] = true;
	}

	void AddViewOutput(uint32_t targetIdx, uint32_t subpass, bool isResolved, uint32_t resolvedIdx) override
	{
		targetIdx += startIdx;
		ResizeSubpasses(subpass);
		if (isResolved)
			subpasses[subpass].resolvedAtt[resolvedIdx].attachment = targetIdx;
		else
			subpasses[subpass].outputAtt[subpassDescs[subpass].colorAttachmentCount++].attachment = targetIdx;
		dep[targetIdx][subpass] = true;
	}

	void SetDepthStencilOutput(uint32_t targetIdx, uint32_t subpass) override
	{
		targetIdx += startIdx;
		ResizeSubpasses(subpass);
		subpasses[subpass].depthStencilAtt[0].attachment = targetIdx;
		dep[targetIdx][subpass] = true;
	}

	void AddViewInput(uint32_t targetIdx, uint32_t subpass) override
	{
		targetIdx += startIdx;
		ResizeSubpasses(subpass);
		subpasses[subpass].inputAtt[subpassDescs[subpass].inputAttachmentCount++].attachment = targetIdx;
		dep[targetIdx][subpass] = false;
	}

	void ArrangeDependencies()
	{
		for (size_t i = 0; i < subpasses.size(); i++)
		{
			subpassDescs[i].pColorAttachments = subpasses[i].outputAtt;
			subpassDescs[i].pResolveAttachments = subpasses[i].resolvedAtt;
			subpassDescs[i].pInputAttachments = subpasses[i].inputAtt;
			subpassDescs[i].pDepthStencilAttachment = subpasses[i].depthStencilAtt;
		}

		for (size_t i = 0; i < samples.size(); i++)
		{
			uint32_t viewIdx = subpasses[i].outputAtt[0].attachment;
			samples[i] = viewIdx == 0 && hasSwapchain ? VK_SAMPLE_COUNT_1_BIT : viewDescs[viewIdx].samples;
		}

		for (auto& i : dep)
		{
			size_t src = VK_SUBPASS_EXTERNAL;
			std::vector<uint32_t> reads;
			for (auto j = i.second.begin(); j != i.second.end(); j++)
			{
				//read -> write
				if (reads.size() > 0 && j->second)
				{
					for (auto k : reads)
					{
						dependencies.resize(dependencies.size() + 1);
						dependencies.back().srcSubpass = k;
						dependencies.back().srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
						dependencies.back().srcAccessMask = VK_ACCESS_INPUT_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
						dependencies.back().dstSubpass = j->first;
						dependencies.back().dstStageMask = dependencies.back().srcStageMask;
						dependencies.back().dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
					}

					src = j->first;
					reads.clear();
					continue;
				}

				dependencies.resize(dependencies.size() + 1);
				dependencies.back().srcSubpass = src;
				dependencies.back().srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
				dependencies.back().srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
				dependencies.back().dstSubpass = j->first;
				dependencies.back().dstStageMask = dependencies.back().srcStageMask;
				//write -> write
				if (j->second)
				{
					dependencies.back().dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
					src = j->first;
				}
				//write -> read
				else
				{
					dependencies.back().dstAccessMask = VK_ACCESS_INPUT_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
					reads.push_back(j->first);
				}
			}
		}
	}

	bool hasSwapchain;
	std::vector<VkSampleCountFlagBits> samples;
	std::vector<VkSubpassDescription> subpassDescs;
	std::vector<VkAttachmentDescription> viewDescs;
	std::vector<VkSubpassDependency> dependencies;

private:
	void ResizeSubpasses(size_t n)
	{
		if (subpasses.size() <= n)
		{
			subpasses.resize(n + 1);
			subpassDescs.resize(subpasses.size());
			samples.resize(subpasses.size());

			for (size_t i = 0; i < MAX_RT; i++)
			{
				subpasses[n].outputAtt[i] = { VK_ATTACHMENT_UNUSED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
				subpasses[n].resolvedAtt[i] = { VK_ATTACHMENT_UNUSED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
				subpasses[n].inputAtt[i] = { VK_ATTACHMENT_UNUSED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
			}
			subpasses[n].depthStencilAtt[0] = { VK_ATTACHMENT_UNUSED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };
			subpassDescs[n].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		}
	}

	uint32_t startIdx{};
	std::vector<Subpass> subpasses;
	std::unordered_map<uint32_t, std::map<uint32_t, bool>> dep;
};

struct VKParamFrameBuffer : public ParamFrameBuffer
{
	void Reset() override
	{
		swapchain = {};
		views.clear();
	}

	void SetSwapchain(LuacObj<Swapchain> sc) override
	{
		swapchain = (VKSwapchain*)sc;
	}

	void AddView(LuacObj<Texture> view, uint32_t layer) override
	{
		views.push_back({ (VKTexture*)view, layer });
	}

	VKSwapchain* swapchain{};
	std::vector<VKView> views;
};

struct VKParamResourceLayout : public ParamResourceLayout
{
	void Reset() override
	{
		dslci = {};
		dslci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		dslb.clear();
	}
	void Add(uint32_t type, uint32_t binding, uint32_t count, uint32_t stageFlags) override
	{
		VkDescriptorSetLayoutBinding d{};
		d.descriptorType = (VkDescriptorType)type;
		d.binding = binding;
		d.descriptorCount = count;
		d.stageFlags = stageFlags;
		dslb.push_back(d);
		dslci.pBindings = dslb.data();
		dslci.bindingCount = dslb.size();
	}

	VkDescriptorSetLayoutCreateInfo dslci{};
	std::vector<VkDescriptorSetLayoutBinding> dslb;
};

struct VKParamPipeline : public ParamPipeline
{
	void Reset() override
	{
		for (size_t i = 0; i < 8; i++)
		{
			dsl[i] = {};
			cbas[i] = {};
			cbas[i].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		}
		lci = {};
		lci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		lci.pSetLayouts = dsl;
		
		cbsci = {};
		cbsci.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		cbsci.pAttachments = cbas;
		
		iasci = {};
		iasci.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		
		rsci = {};
		rsci.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		
		dssci = {};
		dssci.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;

		visci = {};
		visci.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		
		ve.clear();
		vibd.clear();
		viad.clear();
	}

	void AddResourceLayout(LuacObj<ResourceLayout> resLayout) override
	{
		lci.setLayoutCount &= 7;
		dsl[lci.setLayoutCount++] = ((VKResourceLayout*)resLayout)->m_layout;
	}

	void SetRasterizerStates(uint32_t topology, uint32_t polygonMode, uint32_t cullMode, bool clockwise, bool depthClampEnable, bool depthBiasEnable, bool discardEnable) override
	{
		iasci.topology = (VkPrimitiveTopology)topology;
		rsci.polygonMode = (VkPolygonMode)polygonMode;
		rsci.cullMode = cullMode;
		rsci.frontFace = clockwise ? VK_FRONT_FACE_CLOCKWISE : VK_FRONT_FACE_COUNTER_CLOCKWISE;
		rsci.depthClampEnable = depthClampEnable;
		rsci.depthBiasEnable = depthBiasEnable;
		rsci.depthBiasEnable = discardEnable;
	}

	void SetDethStencilStates(bool depthTestEnable, bool depthBoundTestEnable, bool depthWriteEnable, uint32_t depthCompareOp, bool stencilTestEnble) override
	{
		dssci.depthTestEnable = depthTestEnable;
		dssci.depthBoundsTestEnable = depthBoundTestEnable;
		dssci.depthWriteEnable = depthWriteEnable;
		dssci.depthCompareOp = (VkCompareOp)depthCompareOp;
		dssci.stencilTestEnable = stencilTestEnble;
	}
	void SetDssStencilOpPass(uint32_t frontOp, uint32_t backOp) override
	{
		dssci.front.passOp = (VkStencilOp)frontOp;
		dssci.back.passOp = (VkStencilOp)backOp;
	}
	void SetDssStencilOpFail(uint32_t frontOp, uint32_t backOp) override
	{
		dssci.front.failOp = (VkStencilOp)frontOp;
		dssci.back.failOp = (VkStencilOp)backOp;
	}
	void SetDssStencilOpDepthFail(uint32_t frontOp, uint32_t backOp) override
	{
		dssci.front.depthFailOp = (VkStencilOp)frontOp;
		dssci.back.depthFailOp = (VkStencilOp)backOp;
	}
	void SetDssStencilOpCompareOp(uint32_t frontOp, uint32_t backOp) override
	{
		dssci.front.compareOp = (VkCompareOp)frontOp;
		dssci.back.compareOp = (VkCompareOp)backOp;
	}

	void SetBlendState(uint32_t index, bool enable) override
	{
		cbsci.attachmentCount = max(cbsci.attachmentCount, (index &= 7) + 1);
		cbas[index].blendEnable = enable;
	}
	void SetBsColorBlendOp(uint32_t index, uint32_t srcFactor, uint32_t dstFactor, uint32_t op) override
	{
		cbsci.attachmentCount = max(cbsci.attachmentCount, (index &= 7) + 1);
		cbas[index].srcColorBlendFactor = (VkBlendFactor)srcFactor;
		cbas[index].dstColorBlendFactor = (VkBlendFactor)dstFactor;
		cbas[index].colorBlendOp = (VkBlendOp)op;
	}
	void SetBsAlphaBlendOp(uint32_t index, uint32_t srcFactor, uint32_t dstFactor, uint32_t op) override
	{
		cbsci.attachmentCount = max(cbsci.attachmentCount, (index &= 7) + 1);
		cbas[index].srcAlphaBlendFactor = (VkBlendFactor)srcFactor;
		cbas[index].dstAlphaBlendFactor = (VkBlendFactor)dstFactor;
		cbas[index].alphaBlendOp = (VkBlendOp)op;
	}
	void SetBsColorWriteMask(uint32_t index, bool r, bool g, bool b, bool a) override
	{
		cbsci.attachmentCount = max(cbsci.attachmentCount, (index &= 7) + 1);
		VkColorComponentFlags& c = cbas[index].colorWriteMask;
		c = r ? c | VK_COLOR_COMPONENT_R_BIT : c & ~VK_COLOR_COMPONENT_R_BIT;
		c = g ? c | VK_COLOR_COMPONENT_G_BIT : c & ~VK_COLOR_COMPONENT_G_BIT;
		c = b ? c | VK_COLOR_COMPONENT_B_BIT : c & ~VK_COLOR_COMPONENT_B_BIT;
		c = a ? c | VK_COLOR_COMPONENT_A_BIT : c & ~VK_COLOR_COMPONENT_A_BIT;
	}
	void SetVertexInputRate(uint32_t slot, bool perInstance) override
	{
		VkVertexInputBindingDescription& v = GetVibd(slot);
		v.inputRate = perInstance ? VK_VERTEX_INPUT_RATE_INSTANCE : VK_VERTEX_INPUT_RATE_VERTEX;
	}
	void AddVertexElement(uint32_t slot, uint32_t location, uint32_t format, uint32_t stride) override
	{
		VkVertexInputBindingDescription& v = GetVibd(slot);
	
		viad.resize(viad.size() + 1);
		viad.back().binding = slot;
		viad.back().location = location;
		viad.back().offset = v.stride;
		viad.back().format = (VkFormat)format;
		v.stride += stride;

		visci.pVertexAttributeDescriptions = viad.data();
		visci.vertexAttributeDescriptionCount = viad.size();
	}

	VkVertexInputBindingDescription& GetVibd(uint32_t slot)
	{
		VkVertexInputBindingDescription* pvibd{};
		auto it = ve.find(slot);
		if (it != ve.end())
			pvibd = it->second;
		else
		{
			vibd.resize(vibd.size() + 1);
			pvibd = ve[slot] = &vibd.back();
			pvibd->binding = slot;
		}
		visci.pVertexBindingDescriptions = vibd.data();
		visci.vertexBindingDescriptionCount = vibd.size();
		return *pvibd;
	}

	VkPipelineLayoutCreateInfo lci{};
	VkDescriptorSetLayout dsl[8]{};
	VkPipelineRasterizationStateCreateInfo rsci{};
	VkPipelineDepthStencilStateCreateInfo dssci{};
	VkPipelineColorBlendStateCreateInfo cbsci{};
	VkPipelineColorBlendAttachmentState cbas[8]{};

	std::unordered_map<uint32_t, VkVertexInputBindingDescription*> ve;
	VkPipelineVertexInputStateCreateInfo visci{};
	std::vector<VkVertexInputBindingDescription> vibd;
	std::vector<VkVertexInputAttributeDescription> viad;
	VkPipelineInputAssemblyStateCreateInfo iasci{};
};

struct VKParamSampler : public ParamSampler
{
	void Reset() override
	{
		m_sci = {};
		m_sci.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	}

	void SetFilterMode(uint32_t mag, uint32_t min) override
	{
		m_sci.magFilter = (VkFilter)mag;
		m_sci.minFilter = (VkFilter)min;
	}

	void SetMipmapMode(uint32_t mode, float maxLod, float minLod, float bias) override
	{
		m_sci.mipmapMode = (VkSamplerMipmapMode)mode;
		m_sci.maxLod = maxLod;
		m_sci.minLod = minLod;
		m_sci.mipLodBias = bias;
	}

	void SetAddressMode(uint32_t mode_u, uint32_t mode_v, uint32_t mode_w, bool unnormalizedCoordinates) override
	{
		m_sci.addressModeU = (VkSamplerAddressMode)mode_u;
		m_sci.addressModeV = (VkSamplerAddressMode)mode_v;
		m_sci.addressModeW = (VkSamplerAddressMode)mode_w;
		m_sci.unnormalizedCoordinates = unnormalizedCoordinates;
	}

	void SetAnisotropyMode(bool enable, float maxAnisotropy) override
	{
		m_sci.anisotropyEnable = enable;
		m_sci.maxAnisotropy = maxAnisotropy;
	}

	void SetCompareMode(bool enable, uint32_t op) override
	{
		m_sci.compareEnable = enable;
		m_sci.compareOp = (VkCompareOp)op;
	}

	VkSamplerCreateInfo m_sci{};
};