#pragma once
#include "../../include/EngineBase.h"
#include "../../include/EngineInterface.h"
#include "../Functional.h"

struct ParamRenderPass;
struct ParamResourceLayout;
struct ParamPipeline;
struct ParamFrameBuffer;
struct ParamSampler;

class Texture
{
public:
	virtual ~Texture() {}

	virtual bool SetData(LuacObj<Engine::StreamInput> stream) = 0;

	virtual bool Resize(uint32_t w, uint32_t h) = 0;

	virtual void* GetData() = 0;

	virtual uint32_t GetRowPitch() = 0;

	virtual uint32_t GetRows() = 0;

	Lua_wrap_cpp_class(Texture, Lua_abstract, Lua_mf(Resize), Lua_mf(SetData));
};
Lua_global_add_cpp_class(Texture)

class Swapchain
{
public:
	virtual ~Swapchain() {}

	virtual bool Resize(uint32_t w, uint32_t h) = 0;

	virtual bool Acquire() = 0;

	Lua_wrap_cpp_class(Swapchain, Lua_abstract, Lua_mf(Resize), Lua_mf(Acquire));
};
Lua_global_add_cpp_class(Swapchain)

class RenderPass
{
public:
	virtual ~RenderPass() {}
	Lua_wrap_cpp_class(RenderPass, Lua_abstract);
};
Lua_global_add_cpp_class(RenderPass)

class FrameBuffer
{
public:	
	virtual ~FrameBuffer() {}

	virtual bool Resize(uint32_t w, uint32_t h) = 0;

	virtual void ClearSwapchain(float r, float g, float b, float a) = 0;

	virtual void ClearViewFloat4(uint32_t idx, float r, float g, float b, float a) = 0;

	virtual void ClearViewUint4(uint32_t idx, uint32_t r, uint32_t g, uint32_t b, uint32_t a) = 0;

	virtual void ClearViewInt4(uint32_t idx, int r, int g, int b, int a) = 0;

	virtual void ClearDepthStencil(uint32_t idx, float d, uint32_t s) = 0;

	Lua_wrap_cpp_class(FrameBuffer, Lua_abstract,
		Lua_mf(Resize),
		Lua_mf(ClearSwapchain),
		Lua_mf(ClearViewFloat4),
		Lua_mf(ClearViewUint4),
		Lua_mf(ClearViewInt4),
		Lua_mf(ClearDepthStencil));
};
Lua_global_add_cpp_class(FrameBuffer)

class ShaderModule
{
public:
	virtual ~ShaderModule() {}
	Lua_wrap_cpp_class(ShaderModule, Lua_abstract);
};
Lua_global_add_cpp_class(ShaderModule)

class Sampler
{
public:
	virtual ~Sampler() {}
	Lua_wrap_cpp_class(Sampler, Lua_abstract);
};
Lua_global_add_cpp_class(Sampler)

class ResourceSet
{
public:
	virtual ~ResourceSet() {}

	virtual void BindBuffer(LuacObj<CBuffer> buffer, uint32_t binding, uint32_t bufferType) = 0;
	
	virtual void BindImageWithSampler(LuacObj<Texture> image, LuacObj<Sampler> sampler, uint32_t binding) = 0;
	
	virtual void BindInputAttachment(LuacObj<Texture> image, uint32_t binding) = 0;

	Lua_wrap_cpp_class(ResourceSet, Lua_abstract,
		Lua_mf(BindBuffer),
		Lua_mf(BindImageWithSampler),
		Lua_mf(BindInputAttachment));
};
Lua_global_add_cpp_class(ResourceSet)

class ResourceLayout
{
public:
	virtual ~ResourceLayout() {}

	virtual LuacObjNew<ResourceSet> NewResourceSet() = 0;

	Lua_wrap_cpp_class(ResourceLayout, Lua_abstract, Lua_mf(NewResourceSet));
};
Lua_global_add_cpp_class(ResourceLayout)

class Pipeline
{
public:
	virtual ~Pipeline() {}
	Lua_wrap_cpp_class(Pipeline, Lua_abstract);
};
Lua_global_add_cpp_class(Pipeline)

class BufferSet
{
public:
	virtual ~BufferSet() {}

	virtual void Add(LuacObj<CBuffer> buffer) = 0;

	virtual void SetWritePos(uint32_t pos) = 0;

	virtual void SetDrawOffset(uint32_t offset) = 0;

	virtual void AddDrawOffset(LuacObj<Pipeline> pipeline, uint32_t offset) = 0;

	Lua_wrap_cpp_class(BufferSet, Lua_abstract, Lua_mf(Add), Lua_mf(SetWritePos), Lua_mf(SetDrawOffset));
};
Lua_global_add_cpp_class(BufferSet);

class DrawIndirectCmd
{
public:
	virtual ~DrawIndirectCmd() {}

	virtual void Reset() = 0;

	virtual void AddDrawIndexed(int32_t vertexOffset, uint32_t firstIndex, uint32_t indexCount, uint32_t firstInstance, uint32_t instanceCount) = 0;

	Lua_wrap_cpp_class(DrawIndirectCmd, Lua_abstract, Lua_mf(Reset), Lua_mf(AddDrawIndexed));
};
Lua_global_add_cpp_class(DrawIndirectCmd);

class Command
{
public:
	virtual ~Command() {}

	virtual void Wait() = 0;

	virtual void RenderBegin(LuacObj<FrameBuffer> fb, bool secondary) = 0;

	virtual void NextSubpass(bool secondary) = 0;

	virtual void ExecuteCommand(LuacObj<Command> secondaryCmd) = 0;

	virtual void ExecuteCommandList(LuacObj<CList> list, uint32_t count) = 0;

	virtual void RenderEnd() = 0;

	virtual void SetViewport(float x, float y, float w, float h, float minDepth, float maxDepth) = 0;

	virtual void SetClipRect(int x, int y, uint32_t width, uint32_t height) = 0;

	virtual void SetResourceSet(LuacObj<ResourceSet> set, uint32_t idx) = 0;

	virtual void DrawIndexed(LuacObj<Pipeline> pipeline, LuacObj<BufferSet> vbSet, LuacObj<CBuffer> ib, int32_t vtxOffset, uint32_t firstIndex, uint32_t indexCount, uint32_t doneOffset) = 0;

	virtual void DrawIndexedIndirect(LuacObj<Pipeline> pipeline, LuacObj<BufferSet> vbSet, LuacObj<CBuffer> ib, LuacObj<DrawIndirectCmd> indirect, uint32_t start, uint32_t count) = 0;

	virtual void CopyImage(LuacObj<Texture> src, int src_base_layer, int src_x, int src_y,
		LuacObj<Texture> dst, int dst_base_layer, int dst_x, int dst_y, int num_layers, uint32_t w, uint32_t h) = 0;
	
	virtual void Execute() = 0;

	Lua_wrap_cpp_class(Command, Lua_abstract, 
		Lua_mf(Wait), 
		Lua_mf(RenderBegin),
		Lua_mf(RenderEnd),
		Lua_mf(NextSubpass), 
		Lua_mf(ExecuteCommand),
		Lua_mf(ExecuteCommandList),
		Lua_mf(Execute),
		Lua_mf(SetViewport),
		Lua_mf(SetClipRect),
		Lua_mf(SetResourceSet),
		Lua_mf(DrawIndexed),
		Lua_mf(DrawIndexedIndirect),
		Lua_mf(CopyImage));
};
Lua_global_add_cpp_class(Command)

class Graphic
{
public:
	enum ImageType
	{
		Image2D,
		ImageCube
	};

	enum AccessType
	{
		CPU_RW_GPU_R,
		GPU_RW
	};

	virtual ~Graphic() {}

#ifdef WIN32
	static bool InitializeVulkan(HINSTANCE inst);
#endif
	static Graphic* Vulkan();

	static void RegisterVulkanDefines(LuaState& lua);

	virtual const char* Type() = 0;

	virtual LuacObjNew<Swapchain> NewSwapchain(HWND hwnd, uint32_t imageCount, uint32_t w, uint32_t h) = 0;

	virtual LuacObjNew<RenderPass> NewRenderPass(LuacObj<ParamRenderPass> param) = 0;

	virtual LuacObjNew<Texture> NewTargetView(uint32_t type, uint32_t format, uint32_t sampleCount2D, uint32_t w, uint32_t h) = 0;

	virtual LuacObjNew<Texture> NewDepthStencilView(uint32_t sampleCount, uint32_t w, uint32_t h) = 0;

	virtual LuacObjNew<Texture> NewTexture(uint32_t type, uint32_t format, uint32_t w, uint32_t h) = 0;

	virtual LuacObjNew<FrameBuffer> NewFrameBuffer(LuacObj<RenderPass> pass, LuacObj<ParamFrameBuffer> param, uint32_t w, uint32_t h) = 0;

	virtual LuacObjNew<Sampler> NewSampler(LuacObj<ParamSampler> param) = 0;

	virtual LuacObjNew<ResourceLayout> NewResourceLayout(LuacObj<ParamResourceLayout> param) = 0;

	virtual LuacObjNew<ShaderModule> NewShaderModule(LuacObj<Engine::StreamInput> stream) = 0;

	virtual LuacObjNew<Pipeline> NewPipeline(LuacObj<RenderPass> pass, uint32_t subpass, uint32_t targets, LuacObj<ShaderModule> vs, const char* vsn, LuacObj<ShaderModule> ps, const char* psn, LuacObj<ParamPipeline> param) = 0;

	virtual LuacObjNew<Command> NewCommand(bool secondary) = 0;

	virtual LuacObjNew<CBuffer> NewBuffer(uint32_t size) = 0;

	virtual LuacObjNew<CBuffer> NewTexelBuffer(uint32_t size, uint32_t format) = 0;

	virtual LuacObjNew<DrawIndirectCmd> NewDrawIndirectCmd(uint32_t n) = 0;

	virtual LuacObjNew<BufferSet> NewBufferSet(LuaIdx set, uint32_t n) = 0;

	virtual void DeviceWaitIdle() = 0;

	virtual uint32_t GetDefined(const char* name) = 0;

	Lua_wrap_cpp_class(Graphic, Lua_abstract, 
		Lua_mf(Type), 
		Lua_mf(NewSwapchain), 
		Lua_mf(NewRenderPass),
		Lua_mf(NewTargetView),
		Lua_mf(NewTexture),
		Lua_mf(NewDepthStencilView),
		Lua_mf(NewFrameBuffer),
		Lua_mf(NewResourceLayout),
		Lua_mf(NewShaderModule),
		Lua_mf(NewPipeline),
		Lua_mf(NewCommand),
		Lua_mf(NewDrawIndirectCmd),
		Lua_mf(NewBuffer),
		Lua_mf(NewBufferSet),
		Lua_mf(DeviceWaitIdle));
};

inline uint32_t CPickByTextureU16(LuacObj<Texture> map, uint32_t x, uint32_t y)
{
	if (map->GetData() && x < map->GetRowPitch() / 2 && y < map->GetRows())
		return *((uint16_t*)map->GetData() + x + map->GetRowPitch() * y / 2);
	return 0;
}
Lua_global_add_cfunc(CPickByTextureU16);

inline uint32_t CPickByTexture(LuacObj<Texture> map, uint32_t x, uint32_t y)
{
	if (map->GetData() && x < map->GetRowPitch() / 4 && y < map->GetRows())
		return *((uint32_t*)map->GetData() + x + map->GetRowPitch() * y / 4) & 0xffff;
	return 0;
}
Lua_global_add_cfunc(CPickByTexture);