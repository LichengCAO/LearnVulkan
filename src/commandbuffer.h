#pragma once
#include "trig_app.h"
//class CommandPool
//{
//public:
//	uint32_t queueFamilyIndex = 0;
//	VkCommandPool vkCommandPool = VK_NULL_HANDLE;
//
//	void Init();
//	void Uninit();
//};
//
//class CommandBuffer
//{
//public:
//	const CommandPool* pCommandPool = nullptr;
//	VkCommandBuffer vkCommandBuffer;
//};

class CommandSubmission
{
private:
	VkSemaphore m_vkSemaphore = VK_NULL_HANDLE;
	std::vector<VkSemaphore> m_vkWaitSemaphores;
	std::optional<uint32_t> m_optQueueFamilyIndex;
	VkQueue m_vkQueue = VK_NULL_HANDLE;
	bool m_isRecording = false;
	
	void _CreateSynchronizeObjects();
public:
	VkCommandBuffer vkCommandBuffer = VK_NULL_HANDLE;
	VkFence vkFence = VK_NULL_HANDLE;

	void SetQueueFamilyIndex(uint32_t _queueFamilyIndex);

	void Init();
	void Uninit();

	void WaitTillAvailable() const; // make sure ALL values used in this command is update AFTER this call or we may access the value while the device still using it
	void StartCommands(const std::vector<VkSemaphore>& _waitSemaphores);
	void StartOneTimeCommands(const std::vector<VkSemaphore>& _waitSemaphores);
	VkSemaphore SubmitCommands();
};