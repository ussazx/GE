#pragma once
#ifdef WIN32
#include <windows.h>
#endif
#include <cstdint>

class Swapchain
{
public:
	virtual ~Swapchain() {}
};

class FrameBuffer
{
public:
	virtual ~FrameBuffer() {}
};

class SizeGroup
{
public:
	virtual ~SizeGroup() {}
	virtual void Resize(uint32_t width, uint32_t height) = 0;
};

class Command
{
public:
	virtual ~Command() {}

};

class Graphic
{
public:
	virtual ~Graphic() {}
#ifdef WIN32
	virtual void Initialize(HINSTANCE inst) = 0;
#endif
	virtual Swapchain* CreateSwapchain(HWND hwnd, uint32_t numImages, SizeGroup* sizeGroup) = 0;
	virtual FrameBuffer* CreateFrameBuffer(Swapchain* sc) = 0;
	virtual SizeGroup* CreateSizeGroup(uint32_t width, uint32_t height) = 0;
	static Graphic& Vulkan();

	virtual void RenderFrameBuffer(FrameBuffer* fb) = 0;
};

