#pragma once
#include "trig_app.h"
class CommandPool
{
public:
	uint32_t queueFamilyIndex = 0;
	VkCommandPool vkCommandPool = VK_NULL_HANDLE;

	void Init();
	void Uninit();
};

class CommandBuffer
{
public:
	const CommandPool* pCommandPool = nullptr;
	VkCommandBuffer vkCommandBuffer;
};