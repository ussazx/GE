#pragma once
#include "Graphic.h"
#include <vulkan/vulkan.h>
#include <vulkan/vk_sdk_platform.h>
#include <vector>
#include <unordered_map>
#include <unordered_set>

class VKObject
{
public:
	virtual ~VKObject() {}
};

class VKSizeGroup;

class VKSwapchain : public Swapchain, public VKObject
{
public:
	VKSwapchain(HWND hwnd, uint32_t numImages, SizeGroup* sizeGroup);
	~VKSwapchain();

	void CreateSurface();
	void Resize();

	VkWin32SurfaceCreateInfoKHR m_sci{};
	std::vector<VkImage> m_vImage;
	std::vector<VkImageView> m_vImageView;
	std::vector<VkSemaphore> m_vSemaphore;
	uint32_t m_semaIndex;
	VkSwapchainKHR m_swapchain{};
	VkSurfaceCapabilitiesKHR m_caps;
	VkSwapchainCreateInfoKHR m_scci;
	VkImageViewCreateInfo m_vci;
	VKSizeGroup* m_size;
};

class VKFrameBuffer : public FrameBuffer, public VKObject
{
public:
	VKFrameBuffer(Swapchain* sc);
	~VKFrameBuffer();

	void Resize();

	std::vector<VkFramebuffer> m_fbs;
	VkRenderPass m_rp{};
	VKSwapchain* m_sc;
	VKSizeGroup* m_size;
};

class VKSizeGroup : public SizeGroup
{
public:
	VKSizeGroup(uint32_t width, uint32_t height) : m_width(width), m_height(height) {}
	void Resize(uint32_t width, uint32_t height) override;

	std::unordered_set<VKSwapchain*> m_sc;
	std::unordered_set<VKFrameBuffer*> m_fb;
	uint32_t m_width;
	uint32_t m_height;
};

class VKGraphic : public Graphic
{
public:
	void Initialize(HINSTANCE hInst) override;
	~VKGraphic();

	Swapchain* CreateSwapchain(HWND hwnd, uint32_t numImages, SizeGroup* sizeGroup) override
	{
		return new VKSwapchain(hwnd, numImages, sizeGroup);
	}
	FrameBuffer* CreateFrameBuffer(Swapchain* sc) override
	{
		return new VKFrameBuffer(sc);
	}
	SizeGroup* CreateSizeGroup(uint32_t width, uint32_t height) override
	{
		return new VKSizeGroup(width, height);
	}

	VkColorSpaceKHR GetColorSpace(VkFormat format)
	{
		auto it = formats.find(format);
		return it != formats.end() ? it->second : VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
	}

	void RenderFrameBuffer(FrameBuffer* fb) override;

	VkInstance inst;
	VkSurfaceKHR baseSurface;
	VkDevice device;
	VkCommandPool cmdPool;
	VkDebugReportCallbackEXT debugReport;

	VkCommandBuffer cmd;
	VkSemaphore cmdSema;
	VkFence cmdFence;

	VkPhysicalDevice gpu;
	VkPhysicalDeviceMemoryProperties gpuMemProps;
	VkQueue queueG;
	VkQueue queueP;
	uint32_t gpuQFs;
	uint32_t queueIdxG;
	uint32_t queueIdxP;
	VkSampleCountFlagBits sampleCount = VK_SAMPLE_COUNT_1_BIT;
	std::unordered_map<VkFormat, VkColorSpaceKHR> formats;
	VkFormat defaultFormat;
#ifdef WIN32
	HINSTANCE hInst;
#endif

private:
	void CreateInstance();
	void CreateDebugCallback();
	void GetGPU();
	void CreateDevice();
	void GetSupportedSufaceFormats();
};