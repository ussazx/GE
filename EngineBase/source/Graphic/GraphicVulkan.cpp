#include "GraphicVulkan.h"
#include "ParamVulkan.h"
#include <memory>

#define CLAMP(a, min, max) if ((a) < (min)) (a) = (min); else if ((a) > (max)) (a) = (max)

#define SET_DEFINE(name, value) \
lua.SetValue("cGI", name, value); \
g->defines[name] = value;

VKGraphic* g;

static bool GetMemTypeIndex(uint32_t bits, VkMemoryPropertyFlags flag, uint32_t& index)
{
	for (index = 0; index < g->gpuMemProps.memoryTypeCount; index++, bits >>= 1)
	{
		if (bits & 1 && (g->gpuMemProps.memoryTypes[index].propertyFlags & flag) == flag)
			return true;
	}
	return false;
}

VKSwapchain::~VKSwapchain()
{
	for (size_t i = 0; i < m_vImageView.size(); i++)
	{
		vkDestroyImageView(g->device, m_vImageView[i], {});
		vkDestroySemaphore(g->device, m_vSemaphore[i], {});
	}
	vkDestroySwapchainKHR(g->device, m_swapchain, {});
	vkDestroySurfaceKHR(g->inst, m_scci.surface, {});
}

bool VKSwapchain::Init(HWND hwnd, uint32_t imageCount, uint32_t w, uint32_t h)
{
#ifdef WIN32
	m_sci.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	m_sci.hwnd = hwnd;
	m_sci.hinstance = g->hInst;
#endif

	m_scci.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	m_scci.imageFormat = g->defaultFormat;
	m_scci.imageColorSpace = g->GetColorSpace(g->defaultFormat);
	m_scci.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	m_scci.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	m_scci.imageArrayLayers = 1;
	m_scci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	m_scci.presentMode = VK_PRESENT_MODE_FIFO_KHR;
	m_scci.clipped = VK_TRUE;
	m_scci.minImageCount = imageCount;
	if (g->queueIdxG != g->queueIdxP)
	{
		uint32_t queueIdx[] = { g->queueIdxG, g->queueIdxP };
		m_scci.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		m_scci.pQueueFamilyIndices = queueIdx;
		m_scci.queueFamilyIndexCount = 2;
	}

	m_vci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	m_vci.viewType = VK_IMAGE_VIEW_TYPE_2D;
	m_vci.format = g->defaultFormat;
	m_vci.components = { VK_COMPONENT_SWIZZLE_IDENTITY };
	m_vci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	m_vci.subresourceRange.levelCount = 1;
	m_vci.subresourceRange.layerCount = 1;

	return CreateSurface() && Resize(w, h);
}

bool VKSwapchain::CreateSurface()
{
#ifdef WIN32
	vkCreateWin32SurfaceKHR(g->inst, &m_sci, {}, &m_scci.surface);
#endif
	VkBool32 flag = VK_FALSE;
	return vkGetPhysicalDeviceSurfaceSupportKHR(g->gpu, g->queueIdxP, m_scci.surface, &flag) == VK_SUCCESS;
}

bool VKSwapchain::Resize(uint32_t w, uint32_t h)
{
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(g->gpu, m_scci.surface, &m_caps);
	m_scci.preTransform = m_caps.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR ? VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR : m_caps.currentTransform;
	CLAMP(m_scci.minImageCount, m_caps.minImageCount, m_caps.maxImageCount);
	if (m_caps.currentExtent.width == UINT32_MAX)
	{
		m_scci.imageExtent = { w, h };
		CLAMP(m_scci.imageExtent.width, m_caps.minImageExtent.width, m_caps.maxImageExtent.width);
		CLAMP(m_scci.imageExtent.height, m_caps.minImageExtent.height, m_caps.maxImageExtent.height);
	}
	else
	{
		m_scci.imageExtent.width = m_caps.currentExtent.width;
		m_scci.imageExtent.height = m_caps.currentExtent.height;
	}

	m_scci.oldSwapchain = m_swapchain;
	if (vkCreateSwapchainKHR(g->device, &m_scci, {}, &m_swapchain) != VK_SUCCESS)
		return false;

	if (m_scci.oldSwapchain)
		vkDestroySwapchainKHR(g->device, m_scci.oldSwapchain, {});

	vkGetSwapchainImagesKHR(g->device, m_swapchain, &m_scci.minImageCount, NULL);
	m_vImage.resize(m_scci.minImageCount);
	vkGetSwapchainImagesKHR(g->device, m_swapchain, &m_scci.minImageCount, m_vImage.data());

	for (size_t i = 0; i < m_vImageView.size(); i++)
		vkDestroyImageView(g->device, m_vImageView[i], {});

	m_vImageView.resize(m_scci.minImageCount);
	for (size_t i = 0; i < m_scci.minImageCount; i++)
	{
		m_vci.image = m_vImage[i];
		vkCreateImageView(g->device, &m_vci, {}, &m_vImageView[i]);
	}

	if (m_scci.minImageCount > m_vSemaphore.size())
	{
		VkSemaphoreCreateInfo sci{};
		sci.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		uint32_t n = m_vSemaphore.size();
		m_vSemaphore.resize(m_scci.minImageCount);
		for (uint32_t i = n; i < m_vSemaphore.size(); i++)
			vkCreateSemaphore(g->device, &sci, {}, &m_vSemaphore[i]);
	}

	return true;
}

bool VKSwapchain::Acquire()
{
	VkResult res = vkAcquireNextImageKHR(g->device, m_swapchain, UINT64_MAX,
		m_vSemaphore[++m_semaIndex %= m_scci.minImageCount], VK_NULL_HANDLE, &m_imageIndex);

	char s[128]{};
	sprintf_s(s, "---Acquire %u %d\n", m_imageIndex, res);
	OutputDebugStringA(s);

	if (res == VK_ERROR_OUT_OF_DATE_KHR)
		return false;
	if (res == VK_ERROR_SURFACE_LOST_KHR)
	{
		CreateSurface();
		return false;
	}
	if (res == VK_SUBOPTIMAL_KHR)
	{
		VkSurfaceCapabilitiesKHR sc{};
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(g->gpu, m_scci.surface, &sc);
		if (sc.currentExtent.width != m_scci.imageExtent.width || sc.currentExtent.height != m_scci.imageExtent.height)
			return false;
	}
	
	return res == VK_SUCCESS;
}

VKTexture::~VKTexture()
{
	Cleanup();
}

void VKTexture::Cleanup()
{
	vkDestroyImage(g->device, m_vci.image, {});
	if (m_ci.tiling == VK_IMAGE_TILING_LINEAR || m_ci.flags & VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT)
		vkDestroyImageView(g->device, m_srv, {});
	for (auto& i : m_rtv)
	{
		vkDestroyImageView(g->device, i, {});
		i = {};
	}
	vkFreeMemory(g->device, m_mem, {});
	m_vci.image = {};
	m_srv = {};
	m_mem = {};
	m_data = {};
}

bool VKTexture::Init(Graphic::ImageType type, Graphic::AccessType usage, VkFormat format, VkSampleCountFlagBits sampleCount2D, uint32_t w, uint32_t h)
{
	VkImageTiling tiling{};
	VkImageUsageFlags usageFlags = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	if (usage == Graphic::GPU_RW || type == Graphic::ImageCube)
	{
		tiling = VK_IMAGE_TILING_OPTIMAL;
		usageFlags |= VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT |
			(format == VK_FORMAT_D24_UNORM_S8_UINT ? VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT : VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
	}
	else if (usage == Graphic::CPU_RW_GPU_R)
		tiling = VK_IMAGE_TILING_LINEAR;
	else
		return false;

	m_ci.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	m_ci.flags = type == Graphic::ImageCube ? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : 0;
	m_ci.imageType = VK_IMAGE_TYPE_2D;
	m_ci.format = format;
	m_ci.mipLevels = 1;
	m_ci.arrayLayers = usage == Graphic::ImageCube ? 6 : 1;
	m_ci.samples = sampleCount2D;
	m_ci.tiling = tiling;
	m_ci.usage = usageFlags;
	m_ci.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	m_vci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	m_vci.viewType = VK_IMAGE_VIEW_TYPE_2D;
	m_vci.subresourceRange.layerCount = 1;
	m_vci.subresourceRange.baseArrayLayer = 0;
	m_vci.format = m_ci.format;
	m_vci.components = { VK_COMPONENT_SWIZZLE_IDENTITY };
	m_vci.subresourceRange.aspectMask = m_ci.format == VK_FORMAT_D24_UNORM_S8_UINT ? VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
	m_vci.subresourceRange.levelCount = 1;

	return Resize(w, h);
}

bool VKTexture::Resize(uint32_t w, uint32_t h)
{
	bool firstCreate = m_vci.image == VK_NULL_HANDLE;
	Cleanup();

	m_ci.extent = { w, h, 1 };
	if (vkCreateImage(g->device, &m_ci, {}, &m_vci.image) != VK_SUCCESS)
		return false;

	VkMemoryRequirements req{};
	vkGetImageMemoryRequirements(g->device, m_vci.image, &req);
	VkMemoryPropertyFlags pf = m_ci.tiling == VK_IMAGE_TILING_LINEAR ? VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT : VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	if (firstCreate && !GetMemTypeIndex(req.memoryTypeBits, pf, m_memTypeIndex))
		return false;

	VkMemoryAllocateInfo mai{};
	mai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	mai.allocationSize = req.size;
	mai.memoryTypeIndex = m_memTypeIndex;

	vkAllocateMemory(g->device, &mai, {}, &m_mem);
	vkBindImageMemory(g->device, m_vci.image, m_mem, 0);
	if (m_ci.tiling == VK_IMAGE_TILING_LINEAR)
	{
		vkMapMemory(g->device, m_mem, 0, req.size, 0, &m_data);
		VkImageSubresource subres{};
		VkSubresourceLayout layout{};
		subres.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		vkGetImageSubresourceLayout(g->device, m_vci.image, &subres, &layout);
		m_rowPitch = layout.rowPitch;
	}

	if (m_ci.flags & VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT)
	{
		m_vci.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
		m_vci.subresourceRange.layerCount = 6;
		m_vci.subresourceRange.baseArrayLayer = 0;
	}
	if (vkCreateImageView(g->device, &m_vci, {}, &m_srv) != VK_SUCCESS)
		return false;

	if (m_ci.tiling == VK_IMAGE_TILING_OPTIMAL)
	{
		if (m_ci.flags & VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT)
		{
			m_rtv.resize(m_ci.arrayLayers);
			m_vci.viewType = VK_IMAGE_VIEW_TYPE_2D;
			m_vci.subresourceRange.layerCount = 1;
			for (size_t i = 0; i < m_ci.arrayLayers; i++)
			{
				m_vci.subresourceRange.baseArrayLayer = i;
				if (vkCreateImageView(g->device, &m_vci, {}, &m_rtv[i]) != VK_SUCCESS)
					return false;
			}
		}
		else
		{
			m_rtv.resize(1);
			m_rtv[0] = m_srv;
		}
	}

	return true;
}

bool VKTexture::SetData(LuacObj<Engine::StreamInput> data)
{
	return true;
}

VKRenderPass::~VKRenderPass()
{
	vkDestroyRenderPass(g->device, m_pass, {});
}

bool VKRenderPass::Init(VKParamRenderPass* param)
{
	param->ArrangeDependencies();

	m_samples = param->samples;
	VkRenderPassCreateInfo rpci{};
	rpci.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	rpci.pSubpasses = param->subpassDescs.data();
	rpci.subpassCount = param->subpassDescs.size();
	rpci.pAttachments = param->viewDescs.data();
	rpci.attachmentCount = param->viewDescs.size();
	rpci.pDependencies = param->dependencies.data();
	rpci.dependencyCount = param->dependencies.size();
	return vkCreateRenderPass(g->device, &rpci, {}, &m_pass) == VK_SUCCESS;
}

VKFrameBuffer::~VKFrameBuffer()
{
	for (size_t i = 0; i < m_fb.size(); i++)
		vkDestroyFramebuffer(g->device, m_fb[i], {});
}

bool VKFrameBuffer::Init(VKRenderPass* pass, VKParamFrameBuffer* param, uint32_t w, uint32_t h)
{
	m_swapchain = param->swapchain;
	m_views = param->views;

	m_fbci.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	m_fbci.layers = 1;
	m_fbci.renderPass = pass->m_pass;

	m_cv.resize(param->views.size() + (m_swapchain ? 1 : 0), {});

	return Resize(w, h);
}

bool VKFrameBuffer::Resize(uint32_t w, uint32_t h)
{
	size_t n = m_swapchain ? 1 : 0;
	std::vector<VkImageView> views(m_views.size() + n);
	for (size_t i = n, j = 0; i < views.size(); i++, j++)
		views[i] = m_views[j].t->m_rtv[m_views[j].layer];

	m_fbci.width = w;
	m_fbci.height = h;
	m_fbci.pAttachments = views.data();
	m_fbci.attachmentCount = views.size();

	m_fb.resize(m_swapchain ? m_swapchain->m_vImageView.size() : 1);
	for (size_t i = 0; i < m_fb.size(); i++)
	{
		vkDestroyFramebuffer(g->device, m_fb[i], {});

		if (m_swapchain)
			views[0] = m_swapchain->m_vImageView[i];
		if (vkCreateFramebuffer(g->device, &m_fbci, {}, &m_fb[i]) != VK_SUCCESS)
			return false;
	}
	return true;
}

VKSampler::~VKSampler()
{
	vkDestroySampler(g->device, m_sampler, {});
}

bool VKSampler::Init(VKParamSampler* param)
{
	return vkCreateSampler(g->device, &param->m_sci, {}, &m_sampler) == VK_SUCCESS;
}

VKShaderModule::~VKShaderModule()
{
	vkDestroyShaderModule(g->device, m_module, {});
}

bool VKShaderModule::Init(Engine::StreamInput& stream)
{
	VkShaderModuleCreateInfo mci{};
	mci.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	mci.codeSize = stream.GetSize();
	mci.pCode = (uint32_t*)stream.GetData();
	return vkCreateShaderModule(g->device, &mci, {}, &m_module) == VK_SUCCESS;
}

VKResourceLayout::~VKResourceLayout()
{
	vkDestroyDescriptorSetLayout(g->device, m_layout, {});
}

bool VKResourceLayout::Init(VKParamResourceLayout* param)
{
	for (auto& i : param->dslb)
	{
		m_dps.resize(m_dps.size() + 1);
		VkDescriptorPoolSize& d = m_dps.back();
		d.descriptorCount = i.descriptorCount;
		d.type = i.descriptorType;
	}
	return vkCreateDescriptorSetLayout(g->device, &param->dslci, {}, &m_layout) == VK_SUCCESS;
}

LuacObjNew<ResourceSet> VKResourceLayout::NewResourceSet()
{
	VKResourceSet* set = new VKResourceSet{};

	VkDescriptorPoolCreateInfo dpci{};
	dpci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	dpci.pPoolSizes = m_dps.data();
	dpci.poolSizeCount = m_dps.size();
	dpci.maxSets = 1;
	vkCreateDescriptorPool(g->device, &dpci, nullptr, &set->m_pool);

	VkDescriptorSetAllocateInfo dsai{};
	dsai.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	dsai.descriptorSetCount = 1;
	dsai.descriptorPool = set->m_pool;
	dsai.pSetLayouts = &m_layout;
	vkAllocateDescriptorSets(g->device, &dsai, &set->m_set);

	return set;
}

VKResourceSet::~VKResourceSet()
{
	vkDestroyDescriptorPool(g->device, m_pool, {});
}

void VKResourceSet::BindBuffer(LuacObj<CBuffer> buffer, uint64_t offset, uint64_t range, uint32_t binding, uint32_t bufferType)
{
	VKBuffer* b = (VKBuffer*)buffer;
	auto& info = m_bufferInfo[b][binding];

	VkDescriptorBufferInfo& dbi = std::get<0>(info);
	dbi = {};
	dbi.offset = offset;
	dbi.range = range;
	dbi.buffer = b->m_buffer;
	VkWriteDescriptorSet& wds = std::get<1>(info);
	wds = {};
	wds.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	wds.descriptorType = (VkDescriptorType)bufferType;
	wds.dstBinding = binding;
	wds.pBufferInfo = &dbi;
	wds.descriptorCount = 1;
	wds.dstSet = m_set;
	if (wds.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER ||
		wds.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER)
		wds.pTexelBufferView = &b->m_view;
	vkUpdateDescriptorSets(g->device, 1, &wds, 0, NULL);

	b->m_holders.insert(this);
}

void VKResourceSet::OnBufferResized(VKBuffer* b)
{
	for (auto& i : m_bufferInfo[b])
	{
		std::get<0>(i.second).buffer = b->m_buffer;
		vkUpdateDescriptorSets(g->device, 1, &std::get<1>(i.second), 0, NULL);
	}
}

void VKResourceSet::BindImageWithSampler(LuacObj<Texture> image, LuacObj<Sampler> sampler, uint32_t binding)
{
	VkDescriptorImageInfo dii{};
	//dii.imageView = ((VKTexture*)image)->m_vci.image;
	dii.sampler = ((VKSampler*)sampler)->m_sampler;
	dii.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	VkWriteDescriptorSet wds[1]{};
	wds[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	wds[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	wds[0].dstBinding = binding;
	wds[0].descriptorCount = 1;
	wds[0].pImageInfo = &dii;
	wds[0].dstSet = m_set;
	vkUpdateDescriptorSets(g->device, 1, wds, 0, NULL);
}

void VKResourceSet::BindInputAttachment(LuacObj<Texture> image, uint32_t binding)
{
	VkDescriptorImageInfo dii{};
	//dii.imageView = ((VKTexture*)image)->m_vci.image;
	dii.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	VkWriteDescriptorSet wds[1]{};
	wds[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	wds[0].descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
	wds[0].dstBinding = binding;
	wds[0].descriptorCount = 1;
	wds[0].pImageInfo = &dii;
	wds[0].dstSet = m_set;
	vkUpdateDescriptorSets(g->device, 1, wds, 0, NULL);
}

VKPipeline::~VKPipeline()
{
	vkDestroyPipeline(g->device, m_pipeline, {});
	vkDestroyPipelineLayout(g->device, m_layout, {});
}

bool VKPipeline::Init(VKRenderPass* pass, uint32_t subpass, uint32_t targets, VKShaderModule* vs, const char* vsn, VKShaderModule* ps, const char* psn, VKParamPipeline* param)
{
	if (vkCreatePipelineLayout(g->device, &param->lci, {}, &m_layout) != VK_SUCCESS)
		return false;
	m_resCount = param->lci.setLayoutCount;
	m_vibd = param->vibd;

	VkPipelineMultisampleStateCreateInfo msci{};
	msci.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	msci.rasterizationSamples = pass->m_samples[subpass];
	msci.sampleShadingEnable = VK_TRUE;
	msci.minSampleShading = 0.2f;

	VkPipelineShaderStageCreateInfo ssci[2]{};
	ssci[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	ssci[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	ssci[0].module = vs->m_module;
	ssci[0].pName = vsn;

	ssci[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	ssci[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	ssci[1].module = ps->m_module;
	ssci[1].pName = psn;

	VkPipelineViewportStateCreateInfo vpsci{};
	vpsci.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	vpsci.viewportCount = 1;
	vpsci.scissorCount = 1;

	param->cbsci.attachmentCount = min(8, targets);
	VkGraphicsPipelineCreateInfo pci{};
	pci.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pci.subpass = subpass;
	pci.layout = m_layout;
	pci.pMultisampleState = &msci;
	pci.pViewportState = &vpsci;
	pci.pStages = ssci;
	pci.stageCount = ARRAYSIZE(ssci);
	pci.renderPass = pass->m_pass;
	pci.pDynamicState = &g->dsci;
	pci.pVertexInputState = &param->visci;
	pci.pInputAssemblyState = &param->iasci;
	pci.pRasterizationState = &param->rsci;
	pci.pColorBlendState = &param->cbsci;
	pci.pDepthStencilState = &param->dssci;
	return vkCreateGraphicsPipelines(g->device, {}, 1, &pci, nullptr, &m_pipeline) == VK_SUCCESS;
}

bool VKDrawIndirectCmd::Init(uint32_t n)
{
	m_capacity = n;
	return m_buffer.Init(sizeof(VkDrawIndexedIndirectCommand) * n, VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT, VK_FORMAT_UNDEFINED);
}

void VKDrawIndirectCmd::Reset()
{
	m_pos = 0;
}

void VKDrawIndirectCmd::AddDrawIndexed(int32_t vertexOffset, uint32_t firstIndex, uint32_t indexCount, uint32_t firstInstance, uint32_t instanceCount)
{
	if (m_pos >= m_capacity)
		m_buffer.Resize((m_capacity = m_pos + 1) * sizeof(VkDrawIndexedIndirectCommand));
	VkDrawIndexedIndirectCommand* c = (VkDrawIndexedIndirectCommand*)m_buffer.m_ptr + m_pos++;
	c->vertexOffset = vertexOffset;
	c->firstIndex = firstIndex;
	c->indexCount = indexCount;
	c->firstInstance = firstInstance;
	c->instanceCount = instanceCount;
}

VkImageMemoryBarrier VKCommand::m_imb[2];
VkImageCopy VKCommand::m_copy;

VKCommand::~VKCommand()
{
	vkDestroyFence(g->device, m_fence, {});
	vkDestroySemaphore(g->device, m_completeSema, {});
}

bool VKCommand::Init(bool secondary)
{
	VkCommandBufferAllocateInfo cbai{};
	cbai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	cbai.commandPool = g->cmdPool;
	cbai.commandBufferCount = 1;
	if (secondary)
	{
		cbai.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
		vkAllocateCommandBuffers(g->device, &cbai, &m_cmd);
	}
	else
	{
		vkAllocateCommandBuffers(g->device, &cbai, &m_cmd);
		VkFenceCreateInfo fci{};
		fci.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		vkCreateFence(g->device, &fci, {}, &m_fence);

		VkSemaphoreCreateInfo sci{};
		sci.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		vkCreateSemaphore(g->device, &sci, {}, &m_completeSema);

		VkCommandBufferBeginInfo cbbi{};
		cbbi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		cbbi.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
		vkBeginCommandBuffer(m_cmd, &cbbi);
	}

	return true;
}

void VKCommand::Wait()
{
	if (m_executing)
	{
		vkWaitForFences(g->device, 1, &m_fence, VK_TRUE, UINT64_MAX);
		vkResetFences(g->device, 1, &m_fence);

		VkCommandBufferBeginInfo cbbi{};
		cbbi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		cbbi.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
		vkBeginCommandBuffer(m_cmd, &cbbi);

		m_swapchains.clear();
		m_executing = false;
	}
}

void VKCommand::RenderBegin(LuacObj<FrameBuffer> fb, bool secondary)
{
	Wait();

	VKFrameBuffer* vfb = (VKFrameBuffer*)fb;

	if (vfb->m_swapchain)
		m_swapchains.insert(vfb->m_swapchain);

	VkRenderPassBeginInfo rpbi{};
	rpbi.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	rpbi.renderPass = vfb->m_fbci.renderPass;
	rpbi.framebuffer = vfb->m_fb[vfb->m_swapchain ? vfb->m_swapchain->m_imageIndex : 0];
	rpbi.renderArea.extent = { vfb->m_fbci.width, vfb->m_fbci.height };
	rpbi.pClearValues = vfb->m_cv.data();
	rpbi.clearValueCount = vfb->m_cv.size();

	m_fb = vfb;

	vkCmdBeginRenderPass(m_cmd, &rpbi, secondary ? VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS : VK_SUBPASS_CONTENTS_INLINE);
}

void VKCommand::NextSubpass(bool secondary)
{
	vkCmdNextSubpass(m_cmd, secondary ? VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS : VK_SUBPASS_CONTENTS_INLINE);
}

void VKCommand::ExecuteCommand(LuacObj<Command> secondaryCmd)
{
	vkCmdExecuteCommands(m_cmd, 1, &((VKCommand*)secondaryCmd)->m_cmd);
}

void VKCommand::ExecuteCommandList(LuacObj<CList> list, uint32_t count)
{
	auto it = list->list.begin();
	for (size_t i = 0; i < count && it != list->list.end(); i++, it++)
		vkCmdExecuteCommands(m_cmd, 1, &((VKCommand*)*it)->m_cmd);
}

void VKCommand::RenderEnd()
{
	vkCmdEndRenderPass(m_cmd);
}

void VKCommand::SetViewport(float x, float y, float w, float h, float minDepth, float maxDepth)
{
	VkViewport vp{x, y, w, h, minDepth, maxDepth};
	vkCmdSetViewport(m_cmd, 0, 1, &vp);
}

void VKCommand::SetClipRect(int x, int y, uint32_t width, uint32_t height)
{
	VkRect2D rect{ {x, y}, {width, height} };
	vkCmdSetScissor(m_cmd, 0, 1, &rect);
}

void VKCommand::SetResourceSet(LuacObj<ResourceSet> set, uint32_t idx)
{
	if (m_resources.size() <= idx)
		m_resources.resize(idx + 1);
	m_resources[idx] = ((VKResourceSet*)set)->m_set;
}

void VKCommand::DrawIndexed(LuacObj<Pipeline> pipeline, LuacObj<BufferSet> vbSet, LuacObj<CBuffer> ib, int32_t vtxOffset, uint32_t firstIndex, uint32_t indexCount, uint32_t doneOffset)
{
	VKPipeline* p = (VKPipeline*)pipeline;
	vkCmdBindPipeline(m_cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, p->m_pipeline);
	if (p->m_resCount > 0 && m_resources.size() > 0)
		vkCmdBindDescriptorSets(m_cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, p->m_layout, 0, min(p->m_resCount, m_resources.size()), m_resources.data(), 0, NULL);

	VKBufferSet* s = (VKBufferSet*)vbSet;

	vkCmdBindVertexBuffers(m_cmd, 0, s->m_set.size(), s->m_set.data(), s->m_offsets.data());
	vkCmdBindIndexBuffer(m_cmd, ((VKBuffer*)ib)->m_buffer, 0, VK_INDEX_TYPE_UINT32);
	vkCmdDrawIndexed(m_cmd, indexCount, 1, firstIndex, 0, 0);

	s->AddDrawOffset(p, doneOffset);
}

void VKCommand::DrawIndexedIndirect(LuacObj<Pipeline> pipeline, LuacObj<BufferSet> vbSet, LuacObj<CBuffer> ib, LuacObj<DrawIndirectCmd> indirect, uint32_t start, uint32_t count)
{
	vkCmdBindPipeline(m_cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, ((VKPipeline*)pipeline)->m_pipeline);

	VKBufferSet* s = (VKBufferSet*)vbSet;
	VKDrawIndirectCmd* d = (VKDrawIndirectCmd*)indirect;

	vkCmdBindVertexBuffers(m_cmd, 0, s->m_set.size(), s->m_set.data(), s->m_offsets.data());
	vkCmdBindIndexBuffer(m_cmd, ((VKBuffer*)ib)->m_buffer, 0, VK_INDEX_TYPE_UINT32);
	vkCmdDrawIndexedIndirect(m_cmd, d->m_buffer.m_buffer, sizeof(VkDrawIndexedIndirectCommand) * start, count, sizeof(VkDrawIndexedIndirectCommand));
}

void VKCommand::CopyImage(LuacObj<Texture> src, int src_base_layer, int src_x, int src_y,
	LuacObj<Texture> dst, int dst_base_layer, int dst_x, int dst_y, int num_layers, uint32_t w, uint32_t h)
{
	Wait();

	VKTexture* s = (VKTexture*)src;
	VKTexture* d = (VKTexture*)dst;
	m_imb[0].oldLayout = m_imb[1].oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	m_imb[0].newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	m_imb[0].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	m_imb[0].dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
	m_imb[0].image = s->m_vci.image;
	m_imb[0].subresourceRange = s->m_vci.subresourceRange;
	m_imb[0].subresourceRange.baseArrayLayer = src_base_layer;
	m_imb[0].subresourceRange.layerCount = num_layers;

	m_imb[1].newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	m_imb[1].srcAccessMask = 0;
	m_imb[1].dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
	m_imb[1].image = d->m_vci.image;
	m_imb[1].subresourceRange = d->m_vci.subresourceRange;
	m_imb[1].subresourceRange.baseArrayLayer = dst_base_layer;
	m_imb[1].subresourceRange.layerCount = num_layers;
	vkCmdPipelineBarrier(m_cmd, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 0, NULL, 2, m_imb);

	m_copy.srcOffset = { src_x, src_y };
	m_copy.srcSubresource.aspectMask = m_imb[0].subresourceRange.aspectMask;
	m_copy.srcSubresource.baseArrayLayer = m_imb[0].subresourceRange.baseArrayLayer;
	m_copy.srcSubresource.layerCount = m_imb[0].subresourceRange.layerCount;
	m_copy.srcSubresource.mipLevel = m_imb[0].subresourceRange.baseMipLevel;
	m_copy.dstOffset = { dst_x, dst_y };
	m_copy.dstSubresource.aspectMask = m_imb[1].subresourceRange.aspectMask;
	m_copy.dstSubresource.baseArrayLayer = m_imb[1].subresourceRange.baseArrayLayer;
	m_copy.dstSubresource.layerCount = m_imb[1].subresourceRange.layerCount;
	m_copy.dstSubresource.mipLevel = m_imb[1].subresourceRange.baseMipLevel;
	m_copy.extent = { w, h, 1 };
	vkCmdCopyImage(m_cmd, s->m_vci.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, d->m_vci.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &m_copy);

	m_imb[0].oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	m_imb[0].newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	m_imb[0].srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	m_imb[0].dstAccessMask = 0;
	
	m_imb[1].oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	m_imb[1].newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	m_imb[1].srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
	m_imb[1].dstAccessMask = 0;
	vkCmdPipelineBarrier(m_cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0, 0, NULL, 0, NULL, 2, m_imb);
}

void VKCommand::Execute()
{
	Wait();
	vkEndCommandBuffer(m_cmd);
	m_executing = true;

	VkSubmitInfo si{};
	si.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	si.pCommandBuffers = &m_cmd;
	si.commandBufferCount = 1;

	if (m_swapchains.size() == 0)
		vkQueueSubmit(g->queueG, 1, &si, m_fence);
	else
	{
		std::vector<VkSwapchainKHR> swapchains(m_swapchains.size());
		std::vector<VkSemaphore> scSemas(m_swapchains.size());
		std::vector<uint32_t> scIndecies(m_swapchains.size());
		std::vector<VkPipelineStageFlags> plStageFlags(m_swapchains.size(), VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);

		size_t i = 0;
		for (auto j : m_swapchains)
		{
			swapchains[i] = j->m_swapchain;
			scSemas[i] = j->m_vSemaphore[j->m_semaIndex];
			scIndecies[i++] = j->m_imageIndex;
		}

		si.pWaitDstStageMask = plStageFlags.data();
		si.pWaitSemaphores = scSemas.data();
		si.waitSemaphoreCount = scSemas.size();
		si.pSignalSemaphores = &m_completeSema;
		si.signalSemaphoreCount = 1;
		vkQueueSubmit(g->queueG, 1, &si, m_fence);

		auto xx = m_fb;

		VkPresentInfoKHR pi{};
		pi.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		pi.pWaitSemaphores = &m_completeSema;
		pi.waitSemaphoreCount = 1;
		pi.pSwapchains = swapchains.data();
		pi.swapchainCount = swapchains.size();
		pi.pImageIndices = scIndecies.data();
		vkQueuePresentKHR(g->queueP, &pi);
	}
}

#ifdef WIN32
bool Graphic::InitializeVulkan(HINSTANCE hinst)
{
	delete g;
	g = new VKGraphic;
	if (!g->Init(hinst))
	{
		delete g;
		return false;
	}

	VKBuffer::m_mem_alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;

	VKCommand::m_imb[0].sType = VKCommand::m_imb[1].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	VKCommand::m_imb[0].srcQueueFamilyIndex = VKCommand::m_imb[1].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	VKCommand::m_imb[0].dstQueueFamilyIndex = VKCommand::m_imb[1].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

	return true;
}
#endif

Graphic* Graphic::Vulkan()
{
	return g;
}

VKGraphic::VKGraphic()
{
	ds[0] = VK_DYNAMIC_STATE_VIEWPORT;
	ds[1] = VK_DYNAMIC_STATE_SCISSOR;
	ds[2] = VK_DYNAMIC_STATE_LINE_WIDTH;
	ds[3] = VK_DYNAMIC_STATE_DEPTH_BIAS;
	ds[4] = VK_DYNAMIC_STATE_DEPTH_BOUNDS;
	ds[5] = VK_DYNAMIC_STATE_STENCIL_REFERENCE;
	ds[6] = VK_DYNAMIC_STATE_STENCIL_COMPARE_MASK;
	ds[7] = VK_DYNAMIC_STATE_STENCIL_WRITE_MASK;
	ds[8] = VK_DYNAMIC_STATE_BLEND_CONSTANTS;

	dsci.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dsci.pDynamicStates = ds;
	dsci.dynamicStateCount = 9;
}

VKGraphic::~VKGraphic()
{
	g = {};
	vkDestroyCommandPool(device, cmdPool, {});
	vkDestroyDevice(device, {});
	vkDestroySurfaceKHR(inst, baseSurface, {});
	if (debugReport && vkDestroyDebugReportCallbackEXT)
			vkDestroyDebugReportCallbackEXT(inst, debugReport, {});
	vkDestroyInstance(inst, {});
}

const char* VKGraphic::Type()
{
	return "vulkan";
}

#ifdef WIN32
bool VKGraphic::Init(HINSTANCE hinst)
{
	if (InitVulkan() == 0)
		return false;
	
	hInst = hinst;
	return CreateInstance() && GetGPU() && CreateDevice() && GetSupportedSufaceFormats();
}
#endif

bool VKGraphic::CreateInstance()
{
	const char* layers[] = { "VK_LAYER_KHRONOS_validation" };
	const char* instanceExt[] =
	{ VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
	VK_KHR_SURFACE_EXTENSION_NAME,
	VK_EXT_DEBUG_REPORT_EXTENSION_NAME };

	VkApplicationInfo ai{};
	ai.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	ai.pApplicationName = "vkapp";
	ai.pEngineName = "vkapp";
	ai.apiVersion = VK_API_VERSION_1_0;

	VkInstanceCreateInfo ci{};
	ci.pApplicationInfo = &ai;
	ci.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	ci.ppEnabledLayerNames = layers;
	ci.ppEnabledExtensionNames = instanceExt;
#ifdef DEBUG
	ci.enabledLayerCount = ARRAYSIZE(layers);
	ci.enabledExtensionCount = ARRAYSIZE(instanceExt);
#else
	ci.enabledLayerCount = ARRAYSIZE(layers) - 1;
	ci.enabledExtensionCount = ARRAYSIZE(instanceExt) - 1;
#endif

	if (vkCreateInstance(&ci, {}, &inst) != VK_SUCCESS)
		return false;

#ifdef DEBUG
	return CreateDebugCallback();
#else
	return true;
#endif
}

bool VKGraphic::GetGPU()
{
	uint32_t count = 0;
	vkEnumeratePhysicalDevices(inst, &count, {});
	std::vector<VkPhysicalDevice> GPU(count);
	vkEnumeratePhysicalDevices(inst, &count, GPU.data());

	std::unordered_map<VkPhysicalDeviceType, uint32_t> devTypeMap;
	for (uint32_t i = 0; i < count; i++)
	{
		vkGetPhysicalDeviceProperties(GPU[i], &devProp);
		if (devTypeMap.find(devProp.deviceType) == devTypeMap.end())
			devTypeMap[devProp.deviceType] = i;
	}

	auto it = devTypeMap.find(VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU);
	if (it != devTypeMap.end() ||
		(it = devTypeMap.find(VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)) != devTypeMap.end() ||
		(it = devTypeMap.find(VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU)) != devTypeMap.end() ||
		(it = devTypeMap.find(VK_PHYSICAL_DEVICE_TYPE_CPU)) != devTypeMap.end() ||
		(it = devTypeMap.find(VK_PHYSICAL_DEVICE_TYPE_OTHER)) != devTypeMap.end())
	{
		gpu = GPU[it->second];
		vkGetPhysicalDeviceProperties(gpu, &devProp);
		vkGetPhysicalDeviceMemoryProperties(gpu, &gpuMemProps);
		VkSampleCountFlags flags = min(devProp.limits.framebufferColorSampleCounts, devProp.limits.framebufferDepthSampleCounts);
		static VkSampleCountFlagBits bits[] = { VK_SAMPLE_COUNT_16_BIT, VK_SAMPLE_COUNT_8_BIT, VK_SAMPLE_COUNT_4_BIT, VK_SAMPLE_COUNT_2_BIT, VK_SAMPLE_COUNT_1_BIT };
		uint32_t i = 0;
		for (; i < ARRAYSIZE(bits) - 1 && !(bits[i] & flags); i++) {}
		maxSampleCount = bits[i];
		return true;
	}
	return false;
}

bool VKGraphic::CreateDevice()
{
	vkGetPhysicalDeviceQueueFamilyProperties(gpu, &gpuQFs, {});
	std::vector<VkQueueFamilyProperties> props(gpuQFs);
	vkGetPhysicalDeviceQueueFamilyProperties(gpu, &gpuQFs, props.data());

	for (queueIdxG = 0; queueIdxG < gpuQFs && !(props[queueIdxG].queueFlags & VK_QUEUE_GRAPHICS_BIT); queueIdxG++) {}
	if (queueIdxG >= gpuQFs)
		return false;

	float priorities[1] = { 0.0 };
	VkDeviceQueueCreateInfo qci{};
	qci.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	qci.queueFamilyIndex = queueIdxG;
	qci.queueCount = 1;
	qci.pQueuePriorities = priorities;

	const char* deviceExt[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

	VkPhysicalDeviceFeatures pdf{};
	pdf.sampleRateShading = VK_TRUE;
	pdf.fillModeNonSolid = VK_TRUE;
	pdf.wideLines = VK_TRUE;
	pdf.depthClamp = VK_TRUE;
	pdf.depthBiasClamp = VK_TRUE;
	pdf.independentBlend = VK_TRUE;
	pdf.multiDrawIndirect = VK_TRUE;

	VkDeviceCreateInfo dci{};
	dci.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	dci.queueCreateInfoCount = 1;
	dci.pQueueCreateInfos = &qci;
	dci.enabledExtensionCount = ARRAYSIZE(deviceExt);
	dci.ppEnabledExtensionNames = deviceExt;
	dci.pEnabledFeatures = &pdf;
	if (vkCreateDevice(gpu, &dci, {}, &device) != VK_SUCCESS)
		return false;

	vkGetDeviceQueue(device, queueIdxG, 0, &queueG);
	if (queueIdxG != queueIdxP)
		vkGetDeviceQueue(device, queueIdxP, 0, &queueP);
	else
		queueP = queueG;

	VkCommandPoolCreateInfo pci{};
	pci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	pci.queueFamilyIndex = queueIdxG;
	pci.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	return vkCreateCommandPool(device, &pci, {}, &cmdPool) == VK_SUCCESS;
}

bool VKGraphic::GetSupportedSufaceFormats()
{
	VkWin32SurfaceCreateInfoKHR sci{};
	sci.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
#ifdef WIN32
	sci.hinstance = hInst;
	sci.hwnd = NULL;
	vkCreateWin32SurfaceKHR(inst, &sci, {}, &baseSurface);
#endif

	uint32_t count = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, baseSurface, &count, NULL);
	if (count == 0)
		return false;

	std::vector<VkSurfaceFormatKHR> sf(count);
	vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, baseSurface, &count, sf.data());
	if (count == 1 && sf[0].format == VK_FORMAT_UNDEFINED)
		formats[VK_FORMAT_R8G8B8A8_UNORM] = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
	else
		for (auto& i : sf)
			formats[i.format] = i.colorSpace;

	auto it = formats.find(VK_FORMAT_R8G8B8A8_UNORM);
	if (it != formats.end()
		|| (it = formats.find(VK_FORMAT_B8G8R8A8_UNORM)) != formats.end()
		|| (it = formats.find(VK_FORMAT_A2B10G10R10_UNORM_PACK32)) != formats.end()
		|| (it = formats.find(VK_FORMAT_A2R10G10B10_UNORM_PACK32)) != formats.end()
		|| (it = formats.find(VK_FORMAT_R16G16B16A16_SFLOAT)) != formats.end())
		defaultFormat = it->first;
	else
		defaultFormat = formats.begin()->first;

	return true;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL DebugReportCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType,
	uint64_t object, size_t location, int32_t messageCode, const char* pLayerPrefix, const char* pMessage, void* pUserData)
{
#ifdef WIN32
	OutputDebugStringA(pMessage);
	OutputDebugStringA("\n");
	DebugBreak();
#endif
	return VK_TRUE;
}

bool VKGraphic::CreateDebugCallback()
{
	vkCreateDebugReportCallbackEXT = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(inst, "vkCreateDebugReportCallbackEXT");
	vkDestroyDebugReportCallbackEXT = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(inst, "vkDestroyDebugReportCallbackEXT");
	if (vkCreateDebugReportCallbackEXT)
	{
		VkDebugReportCallbackCreateInfoEXT cci{};
		cci.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
		cci.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
		cci.pfnCallback = DebugReportCallback;
		vkCreateDebugReportCallbackEXT(inst, &cci, {}, &debugReport);
		return true;
	}
	return false;
}

LuacObjNew<BufferSet> VKGraphic::NewBufferSet(LuaIdx t, uint32_t n)
{
	VKBufferSet* s = new VKBufferSet;
	for (size_t i = 1; i <= n; i++)
	{
		VKBuffer* b{};
		t.state.GetValue(t, i, 0, (void**)&b);
		s->Add(b);
	}
	return s;
}

VKBuffer::~VKBuffer()
{
	Cleanup();
}

bool VKBuffer::Init(uint32_t size, VkBufferUsageFlags usage, VkFormat format)
{
	m_bci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	m_bci.usage = usage;
	m_format = format;
	return Resize(size);
}

void VKBuffer::Cleanup()
{
	vkDestroyBuffer(g->device, m_buffer, {});
	vkFreeMemory(g->device, m_mem, {});
	vkDestroyBufferView(g->device, m_view, {});
	m_buffer = {};
	m_mem = {};
	m_view = {};
	m_ptr = {};
}

VkMemoryRequirements VKBuffer::m_mem_reqs;
VkMemoryAllocateInfo VKBuffer::m_mem_alloc;

bool VKBuffer::Resize(uint32_t size)
{
	VkBuffer newBuffer{};
	VkDeviceMemory newMem{};
	byte* newPtr{};

	size = max(1, size);
	m_bci.size = size;
	if (vkCreateBuffer(g->device, &m_bci, nullptr, &newBuffer) != VK_SUCCESS)
		return false;

	vkGetBufferMemoryRequirements(g->device, newBuffer, &m_mem_reqs);
	if (m_buffer == VK_NULL_HANDLE && !GetMemTypeIndex(m_mem_reqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, m_memTypeIndex))
		return false;

	m_mem_alloc.memoryTypeIndex = m_memTypeIndex;
	m_mem_alloc.allocationSize = m_mem_reqs.size;
	vkAllocateMemory(g->device, &m_mem_alloc, nullptr, &newMem);
	vkBindBufferMemory(g->device, newBuffer, newMem, 0);
	vkMapMemory(g->device, newMem, 0, size, 0, (void**)&newPtr);

	uint32_t minSize = min(m_size, size);
	if (minSize > 0)
		memcpy(newPtr, m_ptr, minSize);
	m_size = size;

	Cleanup();
	m_buffer = newBuffer;
	m_mem = newMem;
	m_ptr = newPtr;

	if (m_bci.usage & (VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT))
	{
		VkBufferViewCreateInfo bvci{};
		bvci.sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO;
		bvci.format = m_format;
		bvci.range = size;
		bvci.buffer = m_buffer;
		vkCreateBufferView(g->device, &bvci, {}, &m_view);
	}

	for (auto i : m_holders)
		i->OnBufferResized(this);

	return true;
}

size_t VKBuffer::GetSize()
{
	return m_size;
}

byte* VKBuffer::GetPtr()
{
	return m_ptr;
}

void Graphic::RegisterVulkanDefines(LuaState& lua)
{
	SET_DEFINE("IMAGE_TYPE_2D", Image2D);
	SET_DEFINE("IMAGE_TYPE_CUBE", ImageCube);

	SET_DEFINE("SHADER_STAGE_VERTEX_BIT", VK_SHADER_STAGE_VERTEX_BIT);
	SET_DEFINE("SHADER_STAGE_FRAGMENT_BIT", VK_SHADER_STAGE_FRAGMENT_BIT);

	SET_DEFINE("WHOLE_SIZE", VK_WHOLE_SIZE);

	SET_DEFINE("VERTEX_BUFFER", VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
	SET_DEFINE("INDEX_BUFFER", VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
	SET_DEFINE("UNIFORM_BUFFER", VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
	SET_DEFINE("STORAGE_BUFFER", VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
	SET_DEFINE("UNIFORM_TEXEL_BUFFER", VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT);
	SET_DEFINE("STORAGE_TEXEL_BUFFER", VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT);

	SET_DEFINE("SAMPLE_COUNT_1_BIT", VK_SAMPLE_COUNT_1_BIT);
	SET_DEFINE("SAMPLE_COUNT_2_BIT", VK_SAMPLE_COUNT_2_BIT);
	SET_DEFINE("SAMPLE_COUNT_4_BIT", VK_SAMPLE_COUNT_4_BIT);
	SET_DEFINE("SAMPLE_COUNT_8_BIT", VK_SAMPLE_COUNT_8_BIT);
	SET_DEFINE("VK_SAMPLE_COUNT_16_BIT", VK_SAMPLE_COUNT_16_BIT);
	SET_DEFINE("VK_SAMPLE_COUNT_32_BIT", VK_SAMPLE_COUNT_32_BIT);
	SET_DEFINE("VK_SAMPLE_COUNT_64_BIT", VK_SAMPLE_COUNT_64_BIT);
	
	SET_DEFINE("FORMAT_PRESENT", g->defaultFormat);
	SET_DEFINE("FORMAT_R16_UINT", VK_FORMAT_R16_UINT);
	SET_DEFINE("FORMAT_R32_SINT", VK_FORMAT_R32_SINT);
	SET_DEFINE("FORMAT_R32_SFLOAT", VK_FORMAT_R32_SFLOAT);
	SET_DEFINE("FORMAT_R32G32_UINT", VK_FORMAT_R32G32_UINT);
	SET_DEFINE("FORMAT_R32G32_SINT", VK_FORMAT_R32G32_SINT);
	SET_DEFINE("FORMAT_R32G32_SFLOAT", VK_FORMAT_R32G32_SFLOAT);
	SET_DEFINE("FORMAT_R32G32B32_UINT", VK_FORMAT_R32G32B32_UINT);
	SET_DEFINE("FORMAT_R32G32B32_SINT", VK_FORMAT_R32G32B32_SINT);
	SET_DEFINE("FORMAT_R32G32B32_SFLOAT", VK_FORMAT_R32G32B32_SFLOAT);
	SET_DEFINE("FORMAT_R32G32B32A32_UINT", VK_FORMAT_R32G32B32A32_UINT);
	SET_DEFINE("FORMAT_R32G32B32A32_SINT", VK_FORMAT_R32G32B32A32_SINT);
	SET_DEFINE("FORMAT_R32G32B32A32_SFLOAT", VK_FORMAT_R32G32B32A32_SFLOAT);
	SET_DEFINE("FORMAT_R8G8B8A8_UNORM", VK_FORMAT_R8G8B8A8_UNORM);
	SET_DEFINE("FORMAT_R8G8B8A8_UINT", VK_FORMAT_R8G8B8A8_UINT);
	SET_DEFINE("FORMAT_B8G8R8A8_UNORM", VK_FORMAT_B8G8R8A8_UNORM);

	SET_DEFINE("RESOURCE_TYPE_SAMPLER", VK_DESCRIPTOR_TYPE_SAMPLER);
	SET_DEFINE("RESOURCE_TYPE_COMBINED_IMAGE_SAMPLER", VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
	SET_DEFINE("RESOURCE_TYPE_SAMPLED_IMAGE", VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE);
	SET_DEFINE("RESOURCE_TYPE_STORAGE_IMAGE", VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
	SET_DEFINE("RESOURCE_TYPE_UNIFORM_TEXEL_BUFFER", VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER);
	SET_DEFINE("RESOURCE_TYPE_STORAGE_TEXEL_BUFFER", VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER);
	SET_DEFINE("RESOURCE_TYPE_UNIFORM_BUFFER", VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
	SET_DEFINE("RESOURCE_TYPE_STORAGE_BUFFER", VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
	SET_DEFINE("RESOURCE_TYPE_UNIFORM_BUFFER_DYNAMIC", VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC);
	SET_DEFINE("RESOURCE_TYPE_STORAGE_BUFFER_DYNAMIC", VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC);
	SET_DEFINE("RESOURCE_TYPE_INPUT_ATTACHMENT", VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT);

	SET_DEFINE("PRIMITIVE_TOPOLOGY_POINT_LIST", VK_PRIMITIVE_TOPOLOGY_POINT_LIST);
	SET_DEFINE("PRIMITIVE_TOPOLOGY_LINE_LIST", VK_PRIMITIVE_TOPOLOGY_LINE_LIST);
	SET_DEFINE("PRIMITIVE_TOPOLOGY_LINE_STRIP", VK_PRIMITIVE_TOPOLOGY_LINE_STRIP);
	SET_DEFINE("PRIMITIVE_TOPOLOGY_TRIANGLE_LIST", VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
	SET_DEFINE("PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP", VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP);
	SET_DEFINE("PRIMITIVE_TOPOLOGY_TRIANGLE_FAN", VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN);

	SET_DEFINE("POLYGON_MODE_FILL", VK_POLYGON_MODE_FILL);
	SET_DEFINE("POLYGON_MODE_LINE", VK_POLYGON_MODE_LINE);
	SET_DEFINE("POLYGON_MODE_POINT", VK_POLYGON_MODE_POINT);

	SET_DEFINE("CULL_MODE_NONE", VK_CULL_MODE_NONE);
	SET_DEFINE("CULL_MODE_FRONT_BIT", VK_CULL_MODE_FRONT_BIT);
	SET_DEFINE("CULL_MODE_BACK_BIT", VK_CULL_MODE_BACK_BIT);

	SET_DEFINE("COMPARE_OP_NEVER", VK_COMPARE_OP_NEVER);
	SET_DEFINE("COMPARE_OP_LESS", VK_COMPARE_OP_LESS);
	SET_DEFINE("COMPARE_OP_EQUAL", VK_COMPARE_OP_EQUAL);
	SET_DEFINE("COMPARE_OP_LESS_OR_EQUAL", VK_COMPARE_OP_LESS_OR_EQUAL);
	SET_DEFINE("COMPARE_OP_GREATER", VK_COMPARE_OP_GREATER);
	SET_DEFINE("COMPARE_OP_NOT_EQUAL", VK_COMPARE_OP_NOT_EQUAL);
	SET_DEFINE("COMPARE_OP_GREATER_OR_EQUAL", VK_COMPARE_OP_GREATER_OR_EQUAL);
	SET_DEFINE("COMPARE_OP_ALWAYS", VK_COMPARE_OP_ALWAYS);

	SET_DEFINE("BLEND_FACTOR_ZERO", VK_BLEND_FACTOR_ZERO);
	SET_DEFINE("BLEND_FACTOR_ONE", VK_BLEND_FACTOR_ONE);
	SET_DEFINE("BLEND_FACTOR_SRC_COLOR", VK_BLEND_FACTOR_SRC_COLOR);
	SET_DEFINE("BLEND_FACTOR_ONE_MINUS_SRC_COLOR", VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR);
	SET_DEFINE("BLEND_FACTOR_DST_COLOR", VK_BLEND_FACTOR_DST_COLOR);
	SET_DEFINE("BLEND_FACTOR_ONE_MINUS_DST_COLOR", VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR);
	SET_DEFINE("BLEND_FACTOR_SRC_ALPHA", VK_BLEND_FACTOR_SRC_ALPHA);
	SET_DEFINE("BLEND_FACTOR_ONE_MINUS_SRC_ALPHA", VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA);
	SET_DEFINE("BLEND_FACTOR_DST_ALPHA", VK_BLEND_FACTOR_DST_ALPHA);
	SET_DEFINE("BLEND_FACTOR_ONE_MINUS_DST_ALPHA", VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA);
	SET_DEFINE("BLEND_FACTOR_CONSTANT_COLOR", VK_BLEND_FACTOR_CONSTANT_COLOR);
	SET_DEFINE("BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR", VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR);
	SET_DEFINE("BLEND_FACTOR_CONSTANT_ALPHA", VK_BLEND_FACTOR_CONSTANT_ALPHA);
	SET_DEFINE("BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA", VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA);
	SET_DEFINE("BLEND_FACTOR_SRC_ALPHA_SATURATE", VK_BLEND_FACTOR_SRC_ALPHA_SATURATE);
	SET_DEFINE("BLEND_FACTOR_SRC1_COLOR", VK_BLEND_FACTOR_SRC1_COLOR);
	SET_DEFINE("BLEND_FACTOR_ONE_MINUS_SRC1_COLOR", VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR);
	SET_DEFINE("BLEND_FACTOR_SRC1_ALPHA", VK_BLEND_FACTOR_SRC1_ALPHA);
	SET_DEFINE("BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA", VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA);

	SET_DEFINE("BLEND_OP_ADD", VK_BLEND_OP_ADD);
	SET_DEFINE("BLEND_OP_SUBTRACT", VK_BLEND_OP_SUBTRACT);
	SET_DEFINE("BLEND_OP_REVERSE_SUBTRACT", VK_BLEND_OP_REVERSE_SUBTRACT);
	SET_DEFINE("BLEND_OP_MIN", VK_BLEND_OP_MIN);
	SET_DEFINE("BLEND_OP_MAX", VK_BLEND_OP_MAX);

}


