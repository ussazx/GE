#include "GraphicVulkan.h"
#include <assert.h>

#define CLAMP(a, min, max) if ((a) < (min)) (a) = (min); if ((a) > (max)) (a) = (max)

static PFN_vkDestroyDebugReportCallbackEXT vkDestroyDebugReportCallback;

static VKGraphic g;

VKSwapchain::VKSwapchain(HWND hwnd, uint32_t numImages, SizeGroup* sizeGroup)
{
	m_size = (VKSizeGroup*)sizeGroup;
	m_size->m_sc.insert(this);
	m_semaIndex = 0;

#ifdef WIN32
	m_sci.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	m_sci.hwnd = hwnd;
	m_sci.hinstance = g.hInst;
#endif

	m_scci.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	m_scci.imageFormat = g.defaultFormat;
	m_scci.imageColorSpace = g.GetColorSpace(g.defaultFormat);
	m_scci.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	m_scci.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	m_scci.imageArrayLayers = 1;
	m_scci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	m_scci.presentMode = VK_PRESENT_MODE_FIFO_KHR;
	if (g.queueIdxG != g.queueIdxP)
	{
		uint32_t queueIdx[] = { g.queueIdxG, g.queueIdxP };
		m_scci.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		m_scci.pQueueFamilyIndices = queueIdx;
		m_scci.queueFamilyIndexCount = 2;
	}

	m_vci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	m_vci.viewType = VK_IMAGE_VIEW_TYPE_2D;
	m_vci.format = g.defaultFormat;
	m_vci.components = { VK_COMPONENT_SWIZZLE_IDENTITY };
	m_vci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	m_vci.subresourceRange.levelCount = 1;
	m_vci.subresourceRange.layerCount = 1;

	CreateSurface();
	Resize();
}
	
VKSwapchain::~VKSwapchain()
{
	vkDeviceWaitIdle(g.device);
	m_size->m_sc.erase(this);
	for (size_t i = 0; i < m_vImageView.size(); i++)
	{
		vkDestroyImageView(g.device, m_vImageView[i], {});
		vkDestroySemaphore(g.device, m_vSemaphore[i], {});
	}
	vkDestroySwapchainKHR(g.device, m_swapchain, {});
	vkDestroySurfaceKHR(g.inst, m_scci.surface, {});
}

void VKSwapchain::CreateSurface()
{
#ifdef WIN32
	vkCreateWin32SurfaceKHR(g.inst, &m_sci, {}, &m_scci.surface);
#endif
	VkBool32 flag = VK_FALSE;
	vkGetPhysicalDeviceSurfaceSupportKHR(g.gpu, g.queueIdxP, m_scci.surface, &flag);
	assert(flag);
}

void VKSwapchain::Resize()
{
	vkDeviceWaitIdle(g.device);

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(g.gpu, m_scci.surface, &m_caps);
	m_scci.minImageCount = m_caps.minImageCount;
	m_scci.preTransform = m_caps.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR ? VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR : m_caps.currentTransform;
	if (m_caps.currentExtent.width == UINT32_MAX)
	{
		m_scci.imageExtent = { m_size->m_width, m_size->m_height };
		CLAMP(m_scci.imageExtent.width, m_caps.minImageExtent.width, m_caps.maxImageExtent.width);
		CLAMP(m_scci.imageExtent.height, m_caps.minImageExtent.height, m_caps.maxImageExtent.height);
	}
	else
	{
		m_scci.imageExtent.width = m_caps.currentExtent.width;
		m_scci.imageExtent.height = m_caps.currentExtent.height;
	}

	m_scci.oldSwapchain = m_swapchain;
	vkCreateSwapchainKHR(g.device, &m_scci, {}, &m_swapchain);
	assert(m_swapchain);

	if (m_scci.oldSwapchain)
		vkDestroySwapchainKHR(g.device, m_scci.oldSwapchain, {});

	vkGetSwapchainImagesKHR(g.device, m_swapchain, &m_scci.minImageCount, NULL);
	m_vImage.resize(m_scci.minImageCount);
	vkGetSwapchainImagesKHR(g.device, m_swapchain, &m_scci.minImageCount, m_vImage.data());

	for (size_t i = 0; i < m_vImageView.size(); i++)
		vkDestroyImageView(g.device, m_vImageView[i], {});

	m_vImageView.resize(m_scci.minImageCount);
	for (size_t i = 0; i < m_scci.minImageCount; i++)
	{
		m_vci.image = m_vImage[i];
		vkCreateImageView(g.device, &m_vci, {}, &m_vImageView[i]);
	}

	if (m_scci.minImageCount > m_vSemaphore.size())
	{
		VkSemaphoreCreateInfo sci{};
		sci.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		uint32_t n = m_vSemaphore.size();
		m_vSemaphore.resize(m_scci.minImageCount);
		for (uint32_t i = n; i < m_vSemaphore.size(); i++)
			vkCreateSemaphore(g.device, &sci, {}, &m_vSemaphore[i]);
	}
}

VkRenderPass VKCreateRenderPass()
{
	VkRenderPass rp{};

	VkAttachmentReference srcAtt{};
	srcAtt.attachment = 0;
	srcAtt.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpasses[1]{};
	subpasses[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpasses[0].pColorAttachments = &srcAtt;
	subpasses[0].colorAttachmentCount = 1;

	VkAttachmentDescription attDesc[1]{};
	attDesc[0].format = g.defaultFormat;
	attDesc[0].samples = VK_SAMPLE_COUNT_1_BIT;
	attDesc[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attDesc[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attDesc[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attDesc[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attDesc[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attDesc[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkRenderPassCreateInfo ci{};
	ci.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	ci.pSubpasses = subpasses;
	ci.subpassCount = ARRAYSIZE(subpasses);
	ci.pAttachments = attDesc;
	ci.attachmentCount = ARRAYSIZE(attDesc);

	vkCreateRenderPass(g.device, &ci, {}, &rp);
	return rp;
}

VKFrameBuffer::VKFrameBuffer(Swapchain* sc)
{
	m_sc = (VKSwapchain*)sc;
	m_size = m_sc->m_size;
	m_size->m_fb.insert(this);
	m_rp = VKCreateRenderPass();

	Resize();
}

VKFrameBuffer::~VKFrameBuffer()
{
	vkDeviceWaitIdle(g.device);
	m_size->m_fb.erase(this);
	for (auto i : m_fbs)
		vkDestroyFramebuffer(g.device, i, {});
	vkDestroyRenderPass(g.device, m_rp, {});
}

void VKFrameBuffer::Resize()
{
	VkFramebufferCreateInfo fbci{};
	fbci.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	fbci.renderPass = m_rp;
	fbci.width = m_size->m_width;
	fbci.height = m_size->m_height;
	fbci.layers = 1;
	fbci.attachmentCount = 1;

	for (auto i : m_fbs)
		vkDestroyFramebuffer(g.device, i, {});

	m_fbs.resize(m_sc->m_vImage.size());
	for (uint32_t i = 0; i < m_fbs.size(); i++)
	{
		fbci.pAttachments = &m_sc->m_vImageView[i];
		vkCreateFramebuffer(g.device, &fbci, {}, &m_fbs[i]);
	}
}

#ifdef WIN32
void VKGraphic::Initialize(HINSTANCE hInstance)
{
	if (inst)
		return;
	hInst = hInstance;
	CreateInstance();
	CreateDebugCallback();
	GetGPU();
	CreateDevice();
	GetSupportedSufaceFormats();

	VkCommandBufferAllocateInfo bai{};
	bai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	bai.commandPool = cmdPool;
	bai.commandBufferCount = 1;
	vkAllocateCommandBuffers(device, &bai, &cmd);

	VkSemaphoreCreateInfo sci{};
	sci.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	vkCreateSemaphore(device, &sci, {}, &cmdSema);

	VkFenceCreateInfo fci{};
	fci.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fci.flags = VK_FENCE_CREATE_SIGNALED_BIT;
	vkCreateFence(device, &fci, {}, &cmdFence);
}
#endif

VKGraphic::~VKGraphic()
{
	if (!inst)
		return;

	vkDestroyCommandPool(device, cmdPool, {});
	
	vkDestroySemaphore(device, cmdSema, {});
	vkDestroyFence(device, cmdFence, {});

	vkDestroyDevice(device, {});
	vkDestroySurfaceKHR(inst, baseSurface, {});
	vkDestroyDebugReportCallback(inst, debugReport, {});
	vkDestroyInstance(inst, {});
}

void VKGraphic::RenderFrameBuffer(FrameBuffer* fb)
{
	VKFrameBuffer* vfb = (VKFrameBuffer*)fb;
	if (vfb->m_size->m_width == 0 || vfb->m_size->m_height == 0)
		return;

	vkWaitForFences(device, 1, &cmdFence, VK_TRUE, UINT64_MAX);
	vkResetFences(device, 1, &cmdFence);

	uint32_t scImageIdx;
	VkResult res;
	do
	{
		res = vkAcquireNextImageKHR(device, vfb->m_sc->m_swapchain, UINT64_MAX,
			vfb->m_sc->m_vSemaphore[vfb->m_sc->m_semaIndex], VK_NULL_HANDLE, &scImageIdx);
		if (res == VK_ERROR_OUT_OF_DATE_KHR)
			vfb->m_size->Resize(vfb->m_size->m_width, vfb->m_size->m_height);
		else if (res == VK_ERROR_SURFACE_LOST_KHR)
		{
			vfb->m_sc->CreateSurface();
			vfb->m_size->Resize(vfb->m_size->m_width, vfb->m_size->m_height);
		}
	} while (res != VK_SUBOPTIMAL_KHR && res != VK_SUCCESS);

	VkCommandBufferBeginInfo cbbi{};
	cbbi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	cbbi.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

	VkClearValue cv[1]{};
	cv[0].color = { 0.1f, 0.1f, 0.2f, 0.2f };

	VkRenderPassBeginInfo rpbi{};
	rpbi.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	rpbi.renderPass = vfb->m_rp;
	rpbi.framebuffer = vfb->m_fbs[scImageIdx];
	rpbi.renderArea.extent = { vfb->m_size->m_width, vfb->m_size->m_height };
	rpbi.pClearValues = cv;
	rpbi.clearValueCount = 1;

	VkViewport vp{};
	vp.height = vfb->m_size->m_width;
	vp.width = vfb->m_size->m_height;
	vp.minDepth = (float)0.0f;
	vp.maxDepth = (float)1.0f;

	VkRect2D r2d{};
	r2d.extent.width = vp.width;
	r2d.extent.height = vp.height;

	vkBeginCommandBuffer(cmd, &cbbi);

	vkCmdSetViewport(cmd, 0, 1, &vp);
	vkCmdSetScissor(cmd, 0, 1, &r2d);

	vkCmdBeginRenderPass(cmd, &rpbi, VK_SUBPASS_CONTENTS_INLINE);
	vkCmdEndRenderPass(cmd);

	vkEndCommandBuffer(cmd);

	VkPipelineStageFlags psf = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	VkSubmitInfo si{};
	si.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	si.pWaitDstStageMask = &psf;
	si.pWaitSemaphores = &vfb->m_sc->m_vSemaphore[vfb->m_sc->m_semaIndex];
	si.waitSemaphoreCount = 1;
	si.pCommandBuffers = &cmd;
	si.commandBufferCount = 1;
	si.pSignalSemaphores = &cmdSema;
	si.signalSemaphoreCount = 1;
	vkQueueSubmit(queueG, 1, &si, cmdFence);

	VkPresentInfoKHR pi{};
	pi.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	pi.pWaitSemaphores = &cmdSema;
	pi.waitSemaphoreCount = 1;
	pi.pSwapchains = &vfb->m_sc->m_swapchain;
	pi.swapchainCount = 1;
	pi.pImageIndices = &scImageIdx;
	res = vkQueuePresentKHR(queueP, &pi);

	++vfb->m_sc->m_semaIndex %= vfb->m_sc->m_vSemaphore.size();

	if (res == VK_ERROR_OUT_OF_DATE_KHR)
		vfb->m_size->Resize(vfb->m_size->m_width, vfb->m_size->m_height);
	else if (res == VK_SUBOPTIMAL_KHR)
	{
		VkSurfaceCapabilitiesKHR sc{};
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(gpu, vfb->m_sc->m_scci.surface, &sc);
		if (sc.currentExtent.width != vfb->m_size->m_width || sc.currentExtent.height != vfb->m_size->m_height)
			vfb->m_size->Resize(sc.currentExtent.width, sc.currentExtent.height);
	}
	else if (res == VK_ERROR_SURFACE_LOST_KHR)
	{
		vfb->m_sc->CreateSurface();
		vfb->m_size->Resize(vfb->m_size->m_width, vfb->m_size->m_height);
	}
}

void VKGraphic::CreateInstance()
{
	//uint32_t n = 0;
	//vkEnumerateInstanceExtensionProperties(NULL, &n, NULL);
	//VkExtensionProperties *ep = (VkExtensionProperties*)alloca(sizeof(VkExtensionProperties) * n);
	//vkEnumerateInstanceExtensionProperties(NULL, &n, ep);
	//for (size_t i = 0; i < n; i++)
	//	Log(ep[i].extensionName);

	vkDestroyInstance(inst, {});

	const char* layers[] = { "VK_LAYER_KHRONOS_validation" };
	const char* instanceExt[] =
	{ VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
		VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
		VK_KHR_SURFACE_EXTENSION_NAME };

	VkApplicationInfo ai{};
	ai.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	ai.pApplicationName = "vkapp";
	ai.pEngineName = "vkapp";
	ai.apiVersion = VK_API_VERSION_1_0;

	VkInstanceCreateInfo ci{};
	ci.pApplicationInfo = &ai;
	ci.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	ci.ppEnabledLayerNames = layers;
	ci.enabledLayerCount = ARRAYSIZE(layers);
	ci.ppEnabledExtensionNames = instanceExt;
	ci.enabledExtensionCount = ARRAYSIZE(instanceExt);

	vkCreateInstance(&ci, {}, &inst);

	assert(inst);
}

VKAPI_ATTR VkBool32 VKAPI_CALL DebugReportCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType, uint64_t object, size_t location, int32_t messageCode, const char* pLayerPrefix, const char* pMessage, void* pUserData)
{
	OutputDebugStringA(pMessage);
	OutputDebugStringA("\n");
	DebugBreak();

	exit(0);
}

void VKGraphic::CreateDebugCallback()
{
	PFN_vkCreateDebugReportCallbackEXT vkCreateDebugReportCallbackEXT = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(inst, "vkCreateDebugReportCallbackEXT");
	vkDestroyDebugReportCallback = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(inst, "vkDestroyDebugReportCallbackEXT");
	if (!vkCreateDebugReportCallbackEXT || !vkDestroyDebugReportCallback)
	{
		OutputDebugStringA("vkCreateDebugReportCallbackEXT not found!\n");
		exit(0);
	}
	VkDebugReportCallbackCreateInfoEXT cci{};
	cci.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
	cci.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
	cci.pfnCallback = DebugReportCallback;
	vkCreateDebugReportCallbackEXT(inst, &cci, {}, &debugReport);
}

void VKGraphic::GetGPU()
{
	uint32_t count = 0;
	vkEnumeratePhysicalDevices(inst, &count, {});
	std::vector<VkPhysicalDevice> GPU(count);
	vkEnumeratePhysicalDevices(inst, &count, GPU.data());

	std::unordered_map<VkPhysicalDeviceType, uint32_t> devTypeMap;
	VkPhysicalDeviceProperties devProp;
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
		vkGetPhysicalDeviceMemoryProperties(gpu, &gpuMemProps);
		VkSampleCountFlags flags = min(devProp.limits.framebufferColorSampleCounts, devProp.limits.framebufferDepthSampleCounts);
		static VkSampleCountFlagBits bits[] = { VK_SAMPLE_COUNT_16_BIT, VK_SAMPLE_COUNT_8_BIT,VK_SAMPLE_COUNT_4_BIT,VK_SAMPLE_COUNT_2_BIT, VK_SAMPLE_COUNT_1_BIT };
		uint32_t i = 0;
		for (; i < ARRAYSIZE(bits) - 1 && !(bits[i] & flags); i++) {}
		sampleCount = bits[i];
		return;
	}

	OutputDebugStringA("GPU not found!\n");
	exit(0);
}

void VKGraphic::CreateDevice()
{
	vkDestroyDevice(device, {});

	vkGetPhysicalDeviceQueueFamilyProperties(gpu, &gpuQFs, {});
	std::vector<VkQueueFamilyProperties> props(gpuQFs);
	vkGetPhysicalDeviceQueueFamilyProperties(gpu, &gpuQFs, props.data());

	for (queueIdxG = 0; queueIdxG < gpuQFs && !(props[queueIdxG].queueFlags & VK_QUEUE_GRAPHICS_BIT); queueIdxG++) {}
	assert(queueIdxG < gpuQFs);

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

	VkDeviceCreateInfo dci{};
	dci.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	dci.queueCreateInfoCount = 1;
	dci.pQueueCreateInfos = &qci;
	dci.enabledExtensionCount = ARRAYSIZE(deviceExt);
	dci.ppEnabledExtensionNames = deviceExt;
	dci.pEnabledFeatures = &pdf;
	vkCreateDevice(gpu, &dci, {}, &device);

	vkGetDeviceQueue(device, queueIdxG, 0, &queueG);
	if (queueIdxG != queueIdxP)
		vkGetDeviceQueue(device, queueIdxP, 0, &queueP);
	else
		queueP = queueG;

	VkCommandPoolCreateInfo pci{};
	pci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	pci.queueFamilyIndex = queueIdxG;
	pci.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	vkCreateCommandPool(device, &pci, {}, &cmdPool);
}

void VKGraphic::GetSupportedSufaceFormats()
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
	assert(count > 0);

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
}

Graphic& Graphic::Vulkan()
{
	return g;
}

void VKSizeGroup::Resize(uint32_t width, uint32_t height)
{
	if (width == 0 || height == 0 || (m_width == width && m_height == height))
		return;

	m_width = width;
	m_height = height;

	for (auto i : m_sc)
		i->Resize();
	for (auto i : m_fb)
		i->Resize();
}