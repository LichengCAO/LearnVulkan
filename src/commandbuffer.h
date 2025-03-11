#pragma once
#include "common.h"
#include "pipeline_io.h";
struct WaitInformation
{
	VkSemaphore          waitSamaphore = VK_NULL_HANDLE;
	VkPipelineStageFlags waitStage = VkPipelineStageFlagBits::VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
};
struct ImageBarrierInformation
{
	const ImageView* pImageView = nullptr;
	VkImageLayout oldLayout = VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED;
	VkImageLayout newLayout = VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED;
	VkAccessFlags srcAccessMask = VkAccessFlagBits::VK_ACCESS_NONE;
	VkAccessFlags dstAccessMask = VkAccessFlagBits::VK_ACCESS_NONE;
};
class CommandSubmission
{
private:
	VkSemaphore m_vkSemaphore = VK_NULL_HANDLE;
	std::vector<VkSemaphore> m_vkWaitSemaphores;
	std::vector<VkPipelineStageFlags> m_vkWaitStages;
	std::optional<uint32_t> m_optQueueFamilyIndex;
	std::unordered_map<const ImageView*, VkImageLayout> m_mapImageLayout;
	VkQueue m_vkQueue = VK_NULL_HANDLE;
	bool m_isRecording = false;
	bool m_isInRenderpass = false;
	const RenderPass* m_pCurRenderpass = nullptr;
	const Framebuffer* m_pCurFramebuffer = nullptr;

	void _CreateSynchronizeObjects();
	VkImageMemoryBarrier _NewImageBarrier(const ImageBarrierInformation& _info) const;
public:
	VkCommandBuffer vkCommandBuffer = VK_NULL_HANDLE;
	VkFence vkFence = VK_NULL_HANDLE;

	void SetQueueFamilyIndex(uint32_t _queueFamilyIndex);
	std::optional<uint32_t> GetQueueFamilyIndex() const;
	VkQueue GetVkQueue() const;

	void Init();
	void Uninit();

	void WaitTillAvailable() const; // make sure ALL values used in this command is update AFTER this call or we may access the value while the device still using it
	void StartCommands(const std::vector<WaitInformation>& _waitInfos);
	void StartRenderPass(const RenderPass* pRenderPass, const Framebuffer* pFramebuffer);
	void EndRenderPass();
	void StartOneTimeCommands(const std::vector<WaitInformation>& _waitInfos);
	VkSemaphore SubmitCommands();

	void AddPipelineBarrier(
		VkPipelineStageFlags srcStageMask,
		VkPipelineStageFlags dstStageMask,
		const std::vector<VkMemoryBarrier>& memoryBarriers, 
		const std::vector<ImageBarrierInformation>& imageBarriers);
	void FillImageView(const ImageView* pImageView, VkClearColorValue clearValue) const;
	void FillBuffer(const Buffer* pBuffer, uint32_t _data) const;
	VkImageLayout GetImageLayout(const ImageView* pImageView) const;
};