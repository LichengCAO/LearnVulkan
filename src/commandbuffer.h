#pragma once
#include "common.h"
#include "pipeline_io.h";
struct WaitInformation
{
	VkSemaphore          waitSamaphore = VK_NULL_HANDLE;
	VkPipelineStageFlags waitStage = VkPipelineStageFlagBits::VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
};
class CommandSubmission
{
private:
	VkSemaphore m_vkSemaphore = VK_NULL_HANDLE;
	std::vector<VkSemaphore> m_vkWaitSemaphores;
	std::vector<VkPipelineStageFlags> m_vkWaitStages;
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
	void StartCommands(const std::vector<WaitInformation>& _waitInfos);
	void StartRenderPass(const RenderPass* pRenderPass, const Framebuffer* pFramebuffer);
	void EndRenderPass();
	void StartOneTimeCommands(const std::vector<WaitInformation>& _waitInfos);
	VkSemaphore SubmitCommands();
};