#include "Internal.h"
#include "Graphic/Param.h"

Graphic* g_graphic;

using namespace Engine;

bool Engine::Initialize(const InitParam& param)
{
#ifdef WIN32
	return Graphic::InitializeVulkan(param.hInst);
#elif defined ANDROID
	CFileReader::assetManager = param->assetManager;
#endif
}

void Engine::CleanUp()
{
	delete Graphic::Vulkan();
}

void Engine::LuaRegister(lua_State* L)
{
	LuaState lua(L);
	LuaRegGlobalReflected(&lua);

	Graphic::RegisterVulkanDefines(lua);
	g_graphic = Graphic::Vulkan();

#ifdef WIN32
	lua.SetValue("SCREEN_W", ::GetSystemMetrics(SM_CXSCREEN));
	lua.SetValue("SCREEN_H", ::GetSystemMetrics(SM_CYSCREEN));
#endif
	lua.SetValue("cGI", Lua_set_cobj(g_graphic));
	lua.SetValue("cParamRenderPass", Lua_set_cobj(&ParamRenderPass::GetVulkanParam()));
	lua.SetValue("cParamFrameBuffer", Lua_set_cobj(&ParamFrameBuffer::GetVulkanParam()));
	lua.SetValue("cParamResourceLayout", Lua_set_cobj(&ParamResourceLayout::GetVulkanParam()));
	lua.SetValue("cParamPipeline", Lua_set_cobj(&ParamPipeline::GetVulkanParam()));
	lua.SetValue("cParamSampler", Lua_set_cobj(&ParamSampler::GetVulkanParam()));

	lua.SetValue("SIZE_FLOAT1", 4);
	lua.SetValue("SIZE_FLOAT2", 8);
	lua.SetValue("SIZE_FLOAT3", 12);
	lua.SetValue("SIZE_FLOAT4", 16);
	lua.SetValue("SIZE_USHORT1", 2);
	lua.SetValue("SIZE_INT1", 4);
	lua.SetValue("SIZE_UINT1", 4);

#ifdef WIN32
	lua.SetValue("SYS", "CURSOR_IBEAM", ::LoadCursor(NULL, IDC_IBEAM));
	lua.SetValue("SYS", "CURSOR_ARROW", ::LoadCursor(NULL, IDC_ARROW));

	lua.SetValue("SYS", "VK_F1", VK_F1);
	lua.SetValue("SYS", "VK_F2", VK_F2);
	lua.SetValue("SYS", "VK_F3", VK_F2);
	lua.SetValue("SYS", "VK_F4", VK_F2);
	lua.SetValue("SYS", "VK_F5", VK_F2);
	lua.SetValue("SYS", "VK_F6", VK_F2);
	lua.SetValue("SYS", "VK_F7", VK_F2);
	lua.SetValue("SYS", "VK_F8", VK_F2);
	lua.SetValue("SYS", "VK_F9", VK_F2);
	lua.SetValue("SYS", "VK_F10", VK_F10);
	lua.SetValue("SYS", "VK_F11", VK_F11);
	lua.SetValue("SYS", "VK_F12", VK_F12);
	lua.SetValue("SYS", "VK_BACK", VK_BACK);
	lua.SetValue("SYS", "VK_TAB", VK_TAB);
	lua.SetValue("SYS", "VK_RETURN", VK_RETURN);
	lua.SetValue("SYS", "VK_SHIFT", VK_SHIFT);
	lua.SetValue("SYS", "VK_CONTROL", VK_CONTROL);
	lua.SetValue("SYS", "VK_MENU", VK_MENU);
	lua.SetValue("SYS", "VK_CAPITAL", VK_CAPITAL);
	lua.SetValue("SYS", "VK_ESCAPE", VK_ESCAPE);
	lua.SetValue("SYS", "VK_SPACE", VK_SPACE);
	lua.SetValue("SYS", "VK_PRIOR", VK_PRIOR);
	lua.SetValue("SYS", "VK_NEXT", VK_NEXT);
	lua.SetValue("SYS", "VK_END", VK_END);
	lua.SetValue("SYS", "VK_HOME", VK_HOME);
	lua.SetValue("SYS", "VK_LEFT", VK_LEFT);
	lua.SetValue("SYS", "VK_UP", VK_UP);
	lua.SetValue("SYS", "VK_RIGHT", VK_RIGHT);
	lua.SetValue("SYS", "VK_DOWN", VK_DOWN);
	lua.SetValue("SYS", "VK_PRINT", VK_PRINT);
	lua.SetValue("SYS", "VK_EXECUTE", VK_EXECUTE);
	lua.SetValue("SYS", "VK_SNAPSHOT", VK_SNAPSHOT);
	lua.SetValue("SYS", "VK_INSERT", VK_INSERT);
	lua.SetValue("SYS", "VK_DELETE", VK_DELETE);
	lua.SetValue("SYS", "VK_HELP", VK_HELP);
	lua.SetValue("SYS", "VK_NUMPAD0", VK_NUMPAD0);
	lua.SetValue("SYS", "VK_NUMPAD1", VK_NUMPAD1);
	lua.SetValue("SYS", "VK_NUMPAD2", VK_NUMPAD2);
	lua.SetValue("SYS", "VK_NUMPAD3", VK_NUMPAD3);
	lua.SetValue("SYS", "VK_NUMPAD4", VK_NUMPAD4);
	lua.SetValue("SYS", "VK_NUMPAD5", VK_NUMPAD5);
	lua.SetValue("SYS", "VK_NUMPAD6", VK_NUMPAD6);
	lua.SetValue("SYS", "VK_NUMPAD7", VK_NUMPAD7);
	lua.SetValue("SYS", "VK_NUMPAD8", VK_NUMPAD8);
	lua.SetValue("SYS", "VK_NUMPAD9", VK_NUMPAD9);
	lua.SetValue("SYS", "VK_MULTIPLY", VK_MULTIPLY);
	lua.SetValue("SYS", "VK_ADD", VK_ADD);
	lua.SetValue("SYS", "VK_SEPARATOR", VK_SEPARATOR);
	lua.SetValue("SYS", "VK_SUBTRACT", VK_SUBTRACT);
	lua.SetValue("SYS", "VK_DECIMAL", VK_DECIMAL);
	lua.SetValue("SYS", "VK_DIVIDE", VK_DIVIDE);
	lua.SetValue("SYS", "VK_NUMLOCK", VK_NUMLOCK);
	lua.SetValue("SYS", "VK_SCROLL", VK_SCROLL);

	lua.SetValue("SYS", "VK_CTRL_A", 1);
	lua.SetValue("SYS", "VK_CTRL_B", 2);
	lua.SetValue("SYS", "VK_CTRL_C", 3);
	lua.SetValue("SYS", "VK_CTRL_D", 4);
	lua.SetValue("SYS", "VK_CTRL_E", 5);
	lua.SetValue("SYS", "VK_CTRL_F", 6);
	lua.SetValue("SYS", "VK_CTRL_G", 7);
	lua.SetValue("SYS", "VK_CTRL_I", 9);
	lua.SetValue("SYS", "VK_CTRL_J", 10);
	lua.SetValue("SYS", "VK_CTRL_K", 11);
	lua.SetValue("SYS", "VK_CTRL_L", 12);
	lua.SetValue("SYS", "VK_CTRL_N", 14);
	lua.SetValue("SYS", "VK_CTRL_O", 15);
	lua.SetValue("SYS", "VK_CTRL_P", 16);
	lua.SetValue("SYS", "VK_CTRL_Q", 17);
	lua.SetValue("SYS", "VK_CTRL_R", 18);
	lua.SetValue("SYS", "VK_CTRL_S", 19);
	lua.SetValue("SYS", "VK_CTRL_T", 20);
	lua.SetValue("SYS", "VK_CTRL_U", 21);
	lua.SetValue("SYS", "VK_CTRL_V", 22);
	lua.SetValue("SYS", "VK_CTRL_W", 23);
	lua.SetValue("SYS", "VK_CTRL_X", 24);
	lua.SetValue("SYS", "VK_CTRL_Y", 25);
	lua.SetValue("SYS", "VK_CTRL_Z", 26);
	lua.SetValue("SYS", "VK_CTRL_[", 27);
	lua.SetValue("SYS", "VK_CTRL_\\", 28);
	lua.SetValue("SYS", "VK_CTRL_]", 29);

	for (size_t i = 0x30, j = 0; i <= 0x39; i++, j++)
	{
		char c[8]{};
		sprintf_s(c, "VK_%d", j);
		lua.SetValue("SYS", c, i);
	}
	for (size_t i = 0x41; i <= 0x5A; i++)
	{
		char c[8]{};
		sprintf_s(c, "VK_%c", i);
		lua.SetValue("SYS", c, i);
	}
#endif
}