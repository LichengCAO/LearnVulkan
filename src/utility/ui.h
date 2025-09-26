#pragma once
#define IMGUI_IMPL_VULKAN_USE_VOLK
#include "imgui.h"
#include "imgui_impl_vulkan.h"
#include "imgui_impl_glfw.h"
#include <string>

class MyGUI
{
public:
	void Init();
	void StartWindow(const std::string& _title);
	void EndWindow();
	void Uninit();
};