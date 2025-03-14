#pragma once
#include "common.h"
#include "pipeline_io.h";
struct WaitInformation
{
	VkSemaphore          waitSamaphore = VK_NULL_HANDLE;
	VkPipelineStageFlags waitStage = VkPipelineStageFlagBits::VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
};
class ImageBarrierBuilder
{
private:
	const void* pNext = nullptr;
	uint32_t                   m_srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	uint32_t                   m_dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	VkImageSubresourceRange    m_subresourceRange{};
public:
	ImageBarrierBuilder();
	void Reset();
	void SetMipLevelRange(uint32_t baseMipLevel, uint32_t levelCount = 1);
	void SetArrayLayerRange(uint32_t baseArrayLayer, uint32_t layerCount = 1);
	void SetAspect(VkImageAspectFlags aspectMask);
	VkImageMemoryBarrier NewBarrier(VkImage _image, VkImageLayout _oldLayout, VkImageLayout _newLayout, VkAccessFlags _srcAccessMask, VkAccessFlags _dstAccessMask) const;
};
class BlitRegionBuilder
{
	VkImageSubresourceLayers m_srcSubresourceLayers{};
	VkImageSubresourceLayers m_dstSubresourceLayers{};
public:
	BlitRegionBuilder();
	void Reset();
	void SetSrcAspect(VkImageAspectFlags aspectMask);
	void SetDstAspect(VkImageAspectFlags aspectMask);
	void SetSrcArrayLayerRange(uint32_t baseArrayLayer, uint32_t layerCount = 1);
	void SetDstArrayLayerRange(uint32_t baseArrayLayer, uint32_t layerCount = 1);
	VkImageBlit NewBlit(VkOffset3D srcOffsetUL, VkOffset3D srcOffsetLR, uint32_t srcMipLevel, VkOffset3D dstOffsetUL, VkOffset3D dstOffsetLR, uint32_t dstMipLevel) const;
	VkImageBlit NewBlit(VkOffset2D srcOffsetLR, uint32_t srcMipLevel, VkOffset2D dstOffsetLR, uint32_t dstMipLevel) const;
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
		const std::vector<VkImageMemoryBarrier>& imageBarriers);
	void AddPipelineBarrier(
		VkPipelineStageFlags srcStageMask,
		VkPipelineStageFlags dstStageMask,
		const std::vector<VkMemoryBarrier>& memoryBarriers);
	void AddPipelineBarrier(
		VkPipelineStageFlags srcStageMask,
		VkPipelineStageFlags dstStageMask,
		const std::vector<VkMemoryBarrier>& memoryBarriers, 
		const std::vector<VkImageMemoryBarrier>& imageBarriers);

	void FillImageView(const ImageView* pImageView, VkClearColorValue clearValue) const;
	void FillBuffer(const Buffer* pBuffer, uint32_t _data) const;
	void FillBuffer(VkBuffer vkBuffer, VkDeviceSize offset, VkDeviceSize size, uint32_t data) const;
	void BlitImage(
		VkImage srcImage, VkImageLayout srcLayout,
		VkImage dstImage, VkImageLayout dstLayout,
		const std::vector<VkImageBlit>& regions, VkFilter filter = VK_FILTER_LINEAR) const;
	VkImageLayout GetImageLayout(const ImageView* pImageView) const;
};