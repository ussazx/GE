#pragma once
#include "Graphic.h"
#include "vulkan_wrapper/vulkan_wrapper.h"
#include <unordered_map>
#include <unordered_set>

#define NewAndReturn(class_type, ...) \
class_type* p = new class_type; \
if (!p->Init(__VA_ARGS__)) delete p, p = {}; \
return p;

#define New(p, class_type, ...) \
class_type* p = new class_type; \
p->Init(__VA_ARGS__)

struct VKParamFrameBuffer;
struct VKParamResourceLayout;
struct VKParamRenderPass;
struct VKParamPipeline;
struct VKParamSampler;

class VKTexture : public Texture
{
public:
	~VKTexture();
	bool Init(Graphic::ImageType type, Graphic::AccessType usage, VkFormat format, VkSampleCountFlagBits sampleCount, uint32_t w, uint32_t h);

	bool SetData(LuacObj<Engine::StreamInput> stream) override;

	bool Resize(uint32_t w, uint32_t h) override;

	void* GetData() override { return m_data; }

	uint32_t GetRowPitch() override { return m_rowPitch; }

	uint32_t GetRows() override { return m_ci.extent.height; }

	void Cleanup();

	VkClearValue m_cv{};
	VkImageCreateInfo m_ci{};
	VkImageViewCreateInfo m_vci{};
	std::vector<VkImageView> m_rtv;
	VkImageView m_srv{};
	uint32_t m_rowPitch{};
	uint32_t m_memTypeIndex{};
	VkDeviceMemory m_mem{};
	void* m_data{};
};

struct VKView
{
	VKTexture* t;
	uint32_t layer;
};

class VKSwapchain : public Swapchain
{
public:
	~VKSwapchain();
#ifdef WIN32
	bool Init(HWND hwnd, uint32_t imageCount, uint32_t w, uint32_t h);
#endif
	
	bool CreateSurface();
	bool Resize(uint32_t w, uint32_t h) override;
	bool Acquire() override;
	void Present() override;

	void AddCmdSemaphore(VkSemaphore sema);

	VkWin32SurfaceCreateInfoKHR m_sci{};
	std::vector<VkImage> m_vImage;
	std::vector<VkImageView> m_vImageView;
	std::vector<VkSemaphore> m_vSemaphore;
	uint32_t m_imageIndex{};
	uint32_t m_semaIndex{};
	VkSwapchainKHR m_swapchain{};
	VkSurfaceCapabilitiesKHR m_caps{};
	VkSwapchainCreateInfoKHR m_scci{};
	VkImageViewCreateInfo m_vci{};

	std::vector<VkSemaphore> m_cmdSemas;
	uint32_t m_cmdCount = 0;
	uint32_t m_cmdCap = 0;
};

class VKRenderPass : public RenderPass
{
public:
	~VKRenderPass();
	bool Init(VKParamRenderPass* param);

	std::vector<VkSampleCountFlagBits> m_samples;
	VkRenderPass m_pass;
};

class VKFrameBuffer : public FrameBuffer
{
public:
	~VKFrameBuffer();
	bool Init(VKRenderPass* pass, VKParamFrameBuffer* param, uint32_t w, uint32_t h);

	void ClearSwapchain(float r, float g, float b, float a)
	{
		if (m_swapchain)
		{
			m_cv[0].color.float32[0] = r;
			m_cv[0].color.float32[1] = g;
			m_cv[0].color.float32[2] = b;
			m_cv[0].color.float32[3] = a;
		}
	}

	void ClearViewFloat4(uint32_t idx, float r, float g, float b, float a) override
	{
		idx += m_swapchain ? 1 : 0;
		m_cv[idx].color.float32[0] = r;
		m_cv[idx].color.float32[1] = g;
		m_cv[idx].color.float32[2] = b;
		m_cv[idx].color.float32[3] = a;
	}

	void ClearViewUint4(uint32_t idx, uint32_t r, uint32_t g, uint32_t b, uint32_t a) override
	{
		idx += m_swapchain ? 1 : 0;
		m_cv[idx].color.uint32[0] = r;
		m_cv[idx].color.uint32[1] = g;
		m_cv[idx].color.uint32[2] = b;
		m_cv[idx].color.uint32[3] = a;
	}

	void ClearViewInt4(uint32_t idx, int r, int g, int b, int a) override
	{
		idx += m_swapchain ? 1 : 0;
		m_cv[idx].color.int32[0] = r;
		m_cv[idx].color.int32[1] = g;
		m_cv[idx].color.int32[2] = b;
		m_cv[idx].color.int32[3] = a;
	}

	void ClearDepthStencil(uint32_t idx, float d, uint32_t s) override
	{
		idx += m_swapchain ? 1 : 0;
		m_cv[idx].depthStencil = { d, s };
	}

	bool Resize(uint32_t w, uint32_t h) override;
	
	VKSwapchain* m_swapchain{};
	std::vector<VKView> m_views;
	std::vector<VkFramebuffer> m_fb;
	std::vector<VkClearValue> m_cv;
	VkFramebufferCreateInfo m_fbci{};
};

class VKSampler : public Sampler
{
public:
	virtual ~VKSampler();
	bool Init(VKParamSampler* param);
	VkSampler m_sampler;
};

class VKResourceLayout : public ResourceLayout
{
public:
	~VKResourceLayout();
	bool Init(VKParamResourceLayout* param);

	LuacObjNew<ResourceSet> NewResourceSet() override;

	VkDescriptorSetLayout m_layout{};
	std::vector<VkDescriptorPoolSize> m_dps;
};

class VKBuffer;
class VKBufferHolder
{
public:
	virtual void OnBufferResized(VKBuffer*) = 0;
};

class VKResourceSet : public ResourceSet, public VKBufferHolder
{
public:
	~VKResourceSet();
	void BindBuffer(LuacObj<CBuffer> buffer, uint64_t offset, uint64_t range, uint32_t binding, uint32_t bufferType) override;
	void BindImageWithSampler(LuacObj<Texture> image, LuacObj<Sampler> sampler, uint32_t binding) override;
	void BindInputAttachment(LuacObj<Texture> image, uint32_t binding) override;
	void OnBufferResized(VKBuffer* buffer) override;

	VkDescriptorPool m_pool{};
	VkDescriptorSet m_set{};
	std::unordered_map<VKBuffer*, std::unordered_map<uint32_t, std::tuple<VkDescriptorBufferInfo, VkWriteDescriptorSet>>> m_bufferInfo;
};

class VKShaderModule : public ShaderModule
{
public:
	~VKShaderModule();
	bool Init(Engine::StreamInput& stream);

	VkShaderModule m_module{};
};

class VKPipeline : public Pipeline
{
public:
	~VKPipeline();
	bool Init(VKRenderPass* pass, uint32_t subpass, uint32_t targets, VKShaderModule* vs, const char* vsn, VKShaderModule* ps, const char* psn, VKParamPipeline* param);

	std::vector<VkVertexInputBindingDescription> m_vibd;
	VkPipelineLayout m_layout{};
	VkPipeline m_pipeline{};
	uint32_t m_resCount{};
};

class VKBuffer : public CBuffer
{
public:
	~VKBuffer();
	bool Init(uint32_t size, VkBufferUsageFlags usage, VkFormat format);

	void Cleanup();

	bool Resize(uint32_t size) override;

	size_t GetSize() override;

	byte* GetPtr() override;

	uint32_t m_size{};
	VkBuffer m_buffer{};
	VkBufferView m_view{};
	VkFormat m_format{};
	uint32_t m_memTypeIndex{};
	VkDeviceMemory m_mem{};
	byte* m_ptr{};

	std::unordered_set<VKBufferHolder*> m_holders;

	VkBufferCreateInfo m_bci;
	static VkMemoryRequirements m_mem_reqs;
	static VkMemoryAllocateInfo m_mem_alloc;
};

class VKBufferSet : public BufferSet, public VKBufferHolder
{
public:
	void OnBufferResized(VKBuffer* b) override
	{
		m_set[m_buffers[b]] = b->m_buffer;
	}
	void Add(LuacObj<CBuffer> buffer) override
	{
		VKBuffer* b = (VKBuffer*)buffer;
		b->m_holders.insert(this);
		m_buffers[buffer] = m_set.size();
		m_set.push_back(b->m_buffer);
		m_offsets.resize(m_set.size());
	}
	void SetWritePos(uint32_t pos) override
	{
		for (auto& i : m_buffers)
			i.first->SetWritePos(pos);
	}
	std::unordered_map<VKBuffer*, size_t> m_buffers;
	std::vector<VkBuffer> m_set;
	std::vector<VkDeviceSize> m_offsets;
};

class VKDrawIndirectCmd : public DrawIndirectCmd
{
public:
	~VKDrawIndirectCmd() {}

	bool Init(uint32_t n);

	void Reset() override;

	void AddDrawIndexed(int32_t vertexOffset, uint32_t firstIndex, uint32_t indexCount, uint32_t firstInstance, uint32_t instanceCount) override;

	bool b{};
	VKBuffer m_buffer;
	uint32_t m_pos{};
	uint32_t m_capacity{};
};

class VKCommand : public Command
{
public:
	~VKCommand();
	bool Init(bool secondary);

	void Wait() override;

	void RenderBegin(LuacObj<FrameBuffer> fb, bool secondary) override;

	void NextSubpass(bool secondary) override;

	void ExecuteCommand(LuacObj<Command> secondaryCmd) override;

	void ExecuteCommandList(LuacObj<CList> list, uint32_t count) override;

	void RenderEnd() override;

	void ClearSwapchain(int x, int y, uint32_t w, uint32_t h, float r, float g, float b, float a) override;

	void ClearViewFloat4(size_t idx, int x, int y, uint32_t w, uint32_t h, float r, float g, float b, float a) override;

	void ClearViewUint4(size_t idx, int x, int y, uint32_t w, uint32_t h, uint32_t r, uint32_t g, uint32_t b, uint32_t a) override;

	void ClearDepthStencil(size_t idx, int x, int y, uint32_t w, uint32_t h, float d, uint32_t s) override;

	void SetViewport(float x, float y, float w, float h, float minDepth, float maxDepth) override;

	void SetScissor(int x, int y, uint32_t width, uint32_t height) override;

	virtual void SetLineWidth(float width) override;

	void SetResourceSet(LuacObj<ResourceSet> set, uint32_t idx) override;

	void SetVertexBuffers(LuacObj<BufferSet> vbSet, uint32_t firstBinding) override;


	void DrawIndexed(LuacObj<Pipeline> pipeline, LuacObj<CBuffer> ib, int32_t vtxOffset, uint32_t firstIndex, uint32_t indexCount, uint32_t firstInst, uint32_t instCount) override;

	void DrawIndexedIndirect(LuacObj<Pipeline> pipeline, LuacObj<CBuffer> ib, LuacObj<DrawIndirectCmd> indirect, uint32_t start, uint32_t count) override;

	void CopyImage(LuacObj<Texture> src, int src_base_layer, int src_x, int src_y,
		LuacObj<Texture> dst, int dst_base_layer, int dst_x, int dst_y, int num_layers, uint32_t w, uint32_t h) override;

	void Execute() override;

	bool m_executing{};
	VkFence m_fence{};
	VkCommandBuffer m_cmd{};
	VkSemaphore m_completeSema;
	std::vector<VkDescriptorSet> m_resources;
	VKFrameBuffer* m_fb;
	VkSubmitInfo m_si{};

	std::vector<VKSwapchain*> m_swapchains;
	std::vector<VkSwapchainKHR> m_scKHRs;
	std::vector<VkSemaphore> m_scSemas;
	std::vector<uint32_t> m_scIndecies;
	std::vector<VkPipelineStageFlags> m_plStageFlags;

	static VkImageMemoryBarrier m_imb[2];
	static VkImageCopy m_copy;

	uint32_t m_scCap = 0;
};

class VKGraphic : public Graphic
{
public:
	VKGraphic();
	~VKGraphic();
#ifdef WIN32
	bool Init(HINSTANCE hInst);
#endif

	const char* Type() override;

	VkColorSpaceKHR GetColorSpace(VkFormat format)
	{
		auto it = formats.find(format);
		return it != formats.end() ? it->second : VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
	}

#ifdef WIN32
	LuacObjNew<Swapchain> NewSwapchain(HWND hwnd, uint32_t imageCount, uint32_t w, uint32_t h) override
	{
		NewAndReturn(VKSwapchain, hwnd, imageCount, w, h);
	}
#endif

	LuacObjNew<RenderPass> NewRenderPass(LuacObj<ParamRenderPass> param) override
	{
		NewAndReturn(VKRenderPass, (VKParamRenderPass*)param);
	}

	LuacObjNew<Texture> NewTargetView(uint32_t type, uint32_t format, uint32_t sampleCount2D, uint32_t w, uint32_t h) override
	{
		NewAndReturn(VKTexture, (ImageType)type, GPU_RW, (VkFormat)format, (VkSampleCountFlagBits)sampleCount2D, w, h);
	}

	LuacObjNew<Texture> NewDepthStencilView(uint32_t sampleCount, uint32_t w, uint32_t h) override
	{
		NewAndReturn(VKTexture, Image2D, GPU_RW, VK_FORMAT_D24_UNORM_S8_UINT, (VkSampleCountFlagBits)sampleCount, w, h);
	}

	LuacObjNew<Texture> NewTexture(uint32_t type, uint32_t format, uint32_t w, uint32_t h) override
	{
		NewAndReturn(VKTexture, (ImageType)type, CPU_RW_GPU_R, (VkFormat)format, VK_SAMPLE_COUNT_1_BIT, w, h);
	}

	LuacObjNew<FrameBuffer> NewFrameBuffer(LuacObj<RenderPass> pass, LuacObj<ParamFrameBuffer> param, uint32_t w, uint32_t h) override
	{
		NewAndReturn(VKFrameBuffer, (VKRenderPass*)pass, (VKParamFrameBuffer*)param, w, h);
	}

	LuacObjNew<Command> NewCommand(bool secondary) override
	{
		NewAndReturn(VKCommand, secondary);
	}

	LuacObjNew<Sampler> NewSampler(LuacObj<ParamSampler> param) override
	{
		NewAndReturn(VKSampler, (VKParamSampler*)param);
	}

	LuacObjNew<ResourceLayout> NewResourceLayout(LuacObj<ParamResourceLayout> param) override
	{
		NewAndReturn(VKResourceLayout, (VKParamResourceLayout*)param);
	}

	LuacObjNew<ShaderModule> NewShaderModule(LuacObj<Engine::StreamInput> stream) override
	{
		NewAndReturn(VKShaderModule, *stream);
	}

	LuacObjNew<Pipeline> NewPipeline(LuacObj<RenderPass> pass, uint32_t subpass, uint32_t targets, 
		LuacObj<ShaderModule> vs, const char* vsn, LuacObj<ShaderModule> ps, const char* psn, LuacObj<ParamPipeline> param) override
	{
		NewAndReturn(VKPipeline, (VKRenderPass*)pass, subpass, targets, (VKShaderModule*)vs, vsn, (VKShaderModule*)ps, psn, (VKParamPipeline*)param);
	}

	LuacObjNew<CBuffer> NewBuffer(uint32_t size) override
	{
		NewAndReturn(VKBuffer, size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT
			| VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_FORMAT_UNDEFINED);
	}

	LuacObjNew<BufferSet> NewBufferSet(LuaIdx set, uint32_t n) override;

	LuacObjNew<CBuffer> NewTexelBuffer(uint32_t size, uint32_t format) override
	{
		NewAndReturn(VKBuffer, size, VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT, VkFormat(format));
	}

	LuacObjNew<DrawIndirectCmd> NewDrawIndirectCmd(uint32_t n) override
	{
		NewAndReturn(VKDrawIndirectCmd, n);
	}

	void DeviceWaitIdle() override
	{
		vkDeviceWaitIdle(device);
	}

	uint32_t GetDefined(const char* name) override
	{
		auto it = defines.find(name);
		return it != defines.end() ? it->second : 0;
	}

	VkDynamicState ds[9]{};
	VkPipelineDynamicStateCreateInfo dsci{};

	VkInstance inst{};
	VkSurfaceKHR baseSurface{};
	VkDevice device{};
	VkCommandPool cmdPool{};
	VkDebugReportCallbackEXT debugReport{};

	VkPhysicalDevice gpu{};
	VkPhysicalDeviceMemoryProperties gpuMemProps{};
	VkPhysicalDeviceProperties devProp{};
	VkQueue queueG{};
	VkQueue queueP{};
	uint32_t gpuQFs{};
	uint32_t queueIdxG{};
	uint32_t queueIdxP{};
	VkSampleCountFlagBits maxSampleCount = VK_SAMPLE_COUNT_1_BIT;
	std::unordered_map<VkFormat, VkColorSpaceKHR> formats;
	VkFormat defaultFormat{};
#ifdef WIN32
	HINSTANCE hInst{};
#endif
	std::unordered_map<std::string, uint32_t> defines;

private:
	bool CreateInstance();
	bool GetGPU();
	bool CreateDevice();
	bool CreateDebugCallback();
	bool GetSupportedSufaceFormats();
};

extern VKGraphic* g;