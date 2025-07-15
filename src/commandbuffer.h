#pragma once
#include "common.h"
#include "pipeline_io.h";
#include <queue>
// https://stackoverflow.com/questions/44105058/implementing-component-system-from-unity-in-c

class GraphicsPipeline;
class RenderPass;

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
	// _srcAccessMask: when the data is updated (available)
	// _dstAccessMask: when the data is visible
	// see: https://themaister.net/blog/2019/08/14/yet-another-blog-explaining-vulkan-synchronization/
	VkImageMemoryBarrier NewBarrier(
		VkImage _image,
		VkImageLayout _oldLayout,
		VkImageLayout _newLayout,
		VkAccessFlags _srcAccessMask,
		VkAccessFlags _dstAccessMask) const;
};

class ImageBlitBuilder
{
private:
	VkImageSubresourceLayers m_srcSubresourceLayers{};
	VkImageSubresourceLayers m_dstSubresourceLayers{};

public:
	ImageBlitBuilder();
	void Reset();
	void SetSrcAspect(VkImageAspectFlags aspectMask);
	void SetDstAspect(VkImageAspectFlags aspectMask);
	void SetSrcArrayLayerRange(uint32_t baseArrayLayer, uint32_t layerCount = 1);
	void SetDstArrayLayerRange(uint32_t baseArrayLayer, uint32_t layerCount = 1);
	VkImageBlit NewBlit(VkOffset3D srcOffsetUL, VkOffset3D srcOffsetLR, uint32_t srcMipLevel, VkOffset3D dstOffsetUL, VkOffset3D dstOffsetLR, uint32_t dstMipLevel) const;
	VkImageBlit NewBlit(VkOffset2D srcOffsetLR, uint32_t srcMipLevel, VkOffset2D dstOffsetLR, uint32_t dstMipLevel) const;
};

// class handles commandbuffer and queue submission, synchronization
class CommandSubmission
{
public:
	enum class CALLBACK_BINDING_POINT
	{
		END_RENDER_PASS,
		COMMANDS_DONE,
	};
	struct WaitInformation
	{
		VkSemaphore          waitSamaphore = VK_NULL_HANDLE;
		VkPipelineStageFlags waitStage = VkPipelineStageFlagBits::VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	};

private:
	VkSemaphore m_vkSemaphore = VK_NULL_HANDLE;
	std::vector<VkSemaphore> m_vkWaitSemaphores;
	std::vector<VkPipelineStageFlags> m_vkWaitStages;
	std::optional<uint32_t> m_optQueueFamilyIndex;
	std::unordered_map<CALLBACK_BINDING_POINT, std::queue<std::function<void(CommandSubmission*)>>> m_callbacks;
	VkQueue m_vkQueue = VK_NULL_HANDLE;
	bool m_isRecording = false;
	bool m_isInRenderpass = false;

public:
	VkCommandBuffer vkCommandBuffer = VK_NULL_HANDLE;
	VkFence vkFence = VK_NULL_HANDLE;

private:
	void _CreateSynchronizeObjects();

	void _UpdateImageLayout(VkImage vkImage, VkImageSubresourceRange range, VkImageLayout layout) const;

	// Called by RenderPass only
	void _BeginRenderPass(const VkRenderPassBeginInfo& info, VkSubpassContents content = VK_SUBPASS_CONTENTS_INLINE);

	// Do all callbacks in the queue and remove them
	void _DoCallbacks(CALLBACK_BINDING_POINT _bindingPoint);

	// Called by RayTracingAccelerationStructure
	void _AddPipelineBarrier2(const VkDependencyInfo& _dependency);

public:
	void SetQueueFamilyIndex(uint32_t _queueFamilyIndex);
	
	std::optional<uint32_t> GetQueueFamilyIndex() const;
	
	VkQueue GetVkQueue() const;

	void Init();
	
	void Uninit();

	void WaitTillAvailable(); // make sure ALL values used in this command is update AFTER this call or we may access the value while the device still using it
	
	void StartCommands(const std::vector<WaitInformation>& _waitInfos);	
	
	void StartRenderPass(const RenderPass* pRenderPass, const Framebuffer* pFramebuffer);
	
	void EndRenderPass(); // this will change image layout
	
	void StartOneTimeCommands(const std::vector<WaitInformation>& _waitInfos);
	
	VkSemaphore SubmitCommands();

	void AddPipelineBarrier(
		VkPipelineStageFlags srcStageMask,
		VkPipelineStageFlags dstStageMask,
		const std::vector<VkImageMemoryBarrier>& imageBarriers); // this will change image layout
	
	void AddPipelineBarrier(
		VkPipelineStageFlags srcStageMask,
		VkPipelineStageFlags dstStageMask,
		const std::vector<VkMemoryBarrier>& memoryBarriers); // this will change image layout
	
	void AddPipelineBarrier(
		VkPipelineStageFlags srcStageMask,
		VkPipelineStageFlags dstStageMask,
		const std::vector<VkMemoryBarrier>& memoryBarriers, 
		const std::vector<VkImageMemoryBarrier>& imageBarriers); // this will change image layout

	void ClearColorImage(
		VkImage vkImage,
		VkImageLayout vkImageLayout,
		const VkClearColorValue& clearColor,
		const std::vector<VkImageSubresourceRange>& ranges);
	
	void FillBuffer(
		VkBuffer vkBuffer, 
		VkDeviceSize offset, 
		VkDeviceSize size, 
		uint32_t data);
	
	void BlitImage(
		VkImage srcImage, VkImageLayout srcLayout,
		VkImage dstImage, VkImageLayout dstLayout,
		std::vector<VkImageBlit> const& regions,
		VkFilter filter = VK_FILTER_LINEAR);  // this will change image layout

	void CopyBuffer(VkBuffer vkBufferFrom, VkBuffer vkBufferTo, std::vector<VkBufferCopy> const& copies) const;

	void CopyBufferToImage(VkBuffer vkBuffer, VkImage vkImage, VkImageLayout layout, const std::vector<VkBufferImageCopy>& regions) const;

	void BuildAccelerationStructures(
		const std::vector<VkAccelerationStructureBuildGeometryInfoKHR>& buildGeomInfos,
		const std::vector<const VkAccelerationStructureBuildRangeInfoKHR*>& buildRangeInfoPtrs) const;

	void WriteAccelerationStructuresProperties(
		const std::vector<VkAccelerationStructureKHR>& vkAccelerationStructs,
		VkQueryType queryType,
		VkQueryPool queryPool,
		uint32_t firstQuery) const;

	void CopyAccelerationStructure(const VkCopyAccelerationStructureInfoKHR& copyInfo) const;

	// Bind callback to a function,
	// the callback will be called only once
	void BindCallback(CALLBACK_BINDING_POINT bindPoint, std::function<void(CommandSubmission*)>&& callback);

	friend class RenderPass;
	friend class GraphicsPipeline;
	friend class RayTracingAccelerationStructure;
};