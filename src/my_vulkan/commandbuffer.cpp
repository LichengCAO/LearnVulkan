#include "commandbuffer.h"
#include "device.h"
#include "image.h"
#include "buffer.h"
void CommandSubmission::_CreateSynchronizeObjects()
{
	auto& device = MyDevice::GetInstance();

	m_vkSemaphore = device.CreateVkSemaphore();
	vkFence = device.CreateVkFence();
}

void CommandSubmission::_UpdateImageLayout(VkImage vkImage, VkImageSubresourceRange range, VkImageLayout layout) const
{
	MyDevice& device = MyDevice::GetInstance();
	if (device.imageLayouts.find(vkImage) != device.imageLayouts.end())
	{
		device.imageLayouts.at(vkImage).SetLayout(layout, range);
	}
}

void CommandSubmission::_BeginRenderPass(const VkRenderPassBeginInfo& info, VkSubpassContents content)
{
	CHECK_TRUE(!m_isInRenderpass, "Already in render pass!");
	m_isInRenderpass = true;
	vkCmdBeginRenderPass(vkCommandBuffer, &info, content);
}

void CommandSubmission::_DoCallbacks(CALLBACK_BINDING_POINT _bindingPoint)
{
	auto& queue = m_callbacks[_bindingPoint];
	while (!queue.empty())
	{
		auto& func = queue.front();
		func(this);
		queue.pop();
	}
}

void CommandSubmission::_AddPipelineBarrier2(const VkDependencyInfo& _dependency)
{
	vkCmdPipelineBarrier2KHR(vkCommandBuffer, &_dependency);
}

void CommandSubmission::SetQueueFamilyIndex(uint32_t _queueFamilyIndex)
{
	m_optQueueFamilyIndex = _queueFamilyIndex;
}

void CommandSubmission::Init()
{
	if (!m_optQueueFamilyIndex.has_value())
	{
		auto fallbackIndex = MyDevice::GetInstance().queueFamilyIndices.graphicsAndComputeFamily;
		CHECK_TRUE(fallbackIndex.has_value(), "Queue family index is not set!");
		m_optQueueFamilyIndex = fallbackIndex.value();
	}
	m_vkQueue = MyDevice::GetInstance().GetQueue(m_optQueueFamilyIndex.value(), 0);
	VkCommandBufferAllocateInfo allocInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
	allocInfo.commandPool = MyDevice::GetInstance().vkCommandPools[m_optQueueFamilyIndex.value()];
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = 1;
	VK_CHECK(vkAllocateCommandBuffers(MyDevice::GetInstance().vkDevice, &allocInfo, &vkCommandBuffer), "Failed to allocate command buffer!");
}

void CommandSubmission::Uninit()
{
	auto& device = MyDevice::GetInstance();
	WaitTillAvailable();
	if (vkCommandBuffer != VK_NULL_HANDLE)
	{
		// vkFreeCommandBuffers(MyDevice::GetInstance().vkDevice, MyDevice::GetInstance().vkCommandPools[m_optQueueFamilyIndex.value()], 1, &vkCommandBuffer);
		vkCommandBuffer = VK_NULL_HANDLE;
	}
	device.DestroyVkFence(vkFence);
	device.DestroyVkSemaphore(m_vkSemaphore);
	m_vkWaitSemaphores.clear();
	CHECK_TRUE(!m_isRecording, "Still recording commands!");
}

void CommandSubmission::StartCommands(const std::vector<WaitInformation>& _waitInfos)
{
	VkCommandBufferBeginInfo beginInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	size_t n = _waitInfos.size();
	CHECK_TRUE(vkCommandBuffer != VK_NULL_HANDLE, "Command buffer is not initialized!");
	CHECK_TRUE(!m_isRecording, "Already start commands!");
	
	WaitTillAvailable(); // To make sure that the first time we call start commands won't trigger COMMANDS_DONE
	if (vkFence == VK_NULL_HANDLE)
	{
		_CreateSynchronizeObjects();
	}
	vkResetFences(MyDevice::GetInstance().vkDevice, 1, &vkFence);
	vkResetCommandBuffer(vkCommandBuffer, 0);
	m_isRecording = true;
	VK_CHECK(vkBeginCommandBuffer(vkCommandBuffer, &beginInfo), "Failed to begin command!");
	m_vkWaitSemaphores.clear();
	m_vkWaitStages.clear();
	m_vkWaitSemaphores.reserve(n);
	m_vkWaitStages.reserve(n);
	for (const auto& waitInfo : _waitInfos)
	{
		m_vkWaitStages.push_back(waitInfo.waitPipelineStage);
		m_vkWaitSemaphores.push_back(waitInfo.waitSamaphore);
	}
}

std::optional<uint32_t> CommandSubmission::GetQueueFamilyIndex() const
{
	return m_optQueueFamilyIndex;
}

void CommandSubmission::StartOneTimeCommands(const std::vector<WaitInformation>& _waitInfos)
{
	CHECK_TRUE(vkCommandBuffer != VK_NULL_HANDLE, "Command buffer is not initialized!");
	CHECK_TRUE(!m_isRecording, "Already start commands!");
	
	m_isRecording = true;
	VkCommandBufferBeginInfo beginInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	
	VK_CHECK(vkBeginCommandBuffer(vkCommandBuffer, &beginInfo), "Failed to begin single time command!");
	m_vkWaitSemaphores.clear();
	m_vkWaitStages.clear();
	m_vkWaitSemaphores.reserve(_waitInfos.size());
	m_vkWaitStages.reserve(_waitInfos.size());
	for (const auto& waitInfo : _waitInfos)
	{
		m_vkWaitStages.push_back(waitInfo.waitPipelineStage);
		m_vkWaitSemaphores.push_back(waitInfo.waitSamaphore);
	}
}

void CommandSubmission::StartRenderPass(const RenderPass* pRenderPass, const Framebuffer* pFramebuffer)
{
	pRenderPass->StartRenderPass(this, pFramebuffer);
}

VkQueue CommandSubmission::GetVkQueue() const
{
	return m_vkQueue;
}

void CommandSubmission::EndRenderPass()
{
	CHECK_TRUE(m_isInRenderpass, "Not in render pass!");
	m_isInRenderpass = false;
	vkCmdEndRenderPass(vkCommandBuffer);
	_DoCallbacks(CALLBACK_BINDING_POINT::END_RENDER_PASS);
}

VkSemaphore CommandSubmission::SubmitCommands()
{
	CHECK_TRUE(m_isRecording, "Do not start commands yet!");
	VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &vkCommandBuffer;
	submitInfo.waitSemaphoreCount = static_cast<uint32_t>(m_vkWaitSemaphores.size());
	submitInfo.pWaitSemaphores = m_vkWaitSemaphores.data();
	submitInfo.pWaitDstStageMask = m_vkWaitStages.data();
	if (vkFence == VK_NULL_HANDLE)
	{
		SubmitCommandsAndWait();
		Uninit(); // destroy the command buffer after one submission
	}
	else
	{
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &m_vkSemaphore;
		m_isRecording = false;
		VK_CHECK(vkEndCommandBuffer(vkCommandBuffer), "Failed to end command buffer!");
		VK_CHECK(vkQueueSubmit(m_vkQueue, 1, &submitInfo, vkFence), "Failed to submit commands to queue!");
	}
	return m_vkSemaphore;
}

void CommandSubmission::SubmitCommands(const std::vector<VkSemaphore>& _semaphoresToSignal)
{
	CHECK_TRUE(m_isRecording, "Do not start commands yet!");
	VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &vkCommandBuffer;
	submitInfo.waitSemaphoreCount = static_cast<uint32_t>(m_vkWaitSemaphores.size());
	submitInfo.pWaitSemaphores = m_vkWaitSemaphores.data();
	submitInfo.pWaitDstStageMask = m_vkWaitStages.data();
	if (vkFence == VK_NULL_HANDLE)
	{
		SubmitCommandsAndWait();
		Uninit(); // destroy the command buffer after one submission
	}
	else
	{
		submitInfo.signalSemaphoreCount = static_cast<uint32_t>(_semaphoresToSignal.size());
		submitInfo.pSignalSemaphores = _semaphoresToSignal.data();
		m_isRecording = false;
		VK_CHECK(vkEndCommandBuffer(vkCommandBuffer), "Failed to end command buffer!");
		VK_CHECK(vkQueueSubmit(m_vkQueue, 1, &submitInfo, vkFence), "Failed to submit commands to queue!");
	}
}

void CommandSubmission::SubmitCommandsAndWait()
{
	CHECK_TRUE(m_isRecording, "Do not start commands yet!");
	VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &vkCommandBuffer;
	submitInfo.waitSemaphoreCount = static_cast<uint32_t>(m_vkWaitSemaphores.size());
	submitInfo.pWaitSemaphores = m_vkWaitSemaphores.data();
	submitInfo.pWaitDstStageMask = m_vkWaitStages.data();

	VK_CHECK(vkEndCommandBuffer(vkCommandBuffer), "Failed to end command buffer!");
	m_isRecording = false;
	VK_CHECK(vkQueueSubmit(m_vkQueue, 1, &submitInfo, vkFence), "Failed to submit single time commands to queue!");
	vkQueueWaitIdle(m_vkQueue);
	_DoCallbacks(CALLBACK_BINDING_POINT::COMMANDS_DONE);
}

void CommandSubmission::AddPipelineBarrier(
	VkPipelineStageFlags srcStageMask, 
	VkPipelineStageFlags dstStageMask, 
	const std::vector<VkImageMemoryBarrier>& imageBarriers)
{
	AddPipelineBarrier(srcStageMask, dstStageMask, {}, imageBarriers);
}
void CommandSubmission::AddPipelineBarrier(
	VkPipelineStageFlags srcStageMask,
	VkPipelineStageFlags dstStageMask,
	const std::vector<VkMemoryBarrier>& memoryBarriers)
{
	std::vector<VkImageMemoryBarrier> imageBarriers{};
	AddPipelineBarrier(srcStageMask, dstStageMask, memoryBarriers, imageBarriers);
}
void CommandSubmission::AddPipelineBarrier(
	VkPipelineStageFlags srcStageMask,
	VkPipelineStageFlags dstStageMask,
	const std::vector<VkMemoryBarrier>& memoryBarriers,
	const std::vector<VkImageMemoryBarrier>& imageBarriers)
{
	uint32_t memoryBarriersCount = static_cast<uint32_t>(memoryBarriers.size());
	uint32_t imageBarriersCount = static_cast<uint32_t>(imageBarriers.size());
	const VkMemoryBarrier* pMemoryBarriers = memoryBarriersCount == 0 ? nullptr : memoryBarriers.data();
	const VkImageMemoryBarrier* pImageBarriers = imageBarriersCount == 0 ? nullptr : imageBarriers.data();
	vkCmdPipelineBarrier(vkCommandBuffer,
		srcStageMask,
		dstStageMask,
		0,
		memoryBarriersCount, pMemoryBarriers,
		0, nullptr,
		imageBarriersCount, pImageBarriers
	);
	// update image layout
	for (const auto& imageBarrier : imageBarriers)
	{
		_UpdateImageLayout(imageBarrier.image, imageBarrier.subresourceRange, imageBarrier.newLayout);
	}
}

void CommandSubmission::ClearColorImage(VkImage vkImage, VkImageLayout vkImageLayout, const VkClearColorValue& clearColor, const std::vector<VkImageSubresourceRange>& ranges)
{
	vkCmdClearColorImage(
		vkCommandBuffer,
		vkImage,
		vkImageLayout,
		&clearColor,
		static_cast<uint32_t>(ranges.size()),
		ranges.data()
	);
}

void CommandSubmission::FillBuffer(VkBuffer vkBuffer, VkDeviceSize offset, VkDeviceSize size, uint32_t data)
{
	vkCmdFillBuffer(vkCommandBuffer, vkBuffer, offset, size, data);
}

void CommandSubmission::BlitImage(VkImage srcImage, VkImageLayout srcLayout, VkImage dstImage, VkImageLayout dstLayout, const std::vector<VkImageBlit>& regions, VkFilter filter)
{
	vkCmdBlitImage(
		vkCommandBuffer,
		srcImage,
		srcLayout,
		dstImage,
		dstLayout,
		static_cast<uint32_t>(regions.size()),
		regions.data(),
		filter
	);

	for (const auto& region : regions)
	{
		VkImageSubresourceRange range{};
		range.aspectMask = region.dstSubresource.aspectMask;
		range.baseArrayLayer = region.dstSubresource.baseArrayLayer;
		range.layerCount = region.dstSubresource.layerCount;
		range.baseMipLevel = region.dstSubresource.mipLevel;
		range.levelCount = 1;
		_UpdateImageLayout(dstImage, range, dstLayout);
	}
}

void CommandSubmission::CopyBuffer(VkBuffer vkBufferFrom, VkBuffer vkBufferTo, std::vector<VkBufferCopy> const& copies) const
{
	vkCmdCopyBuffer(vkCommandBuffer, vkBufferFrom, vkBufferTo, static_cast<uint32_t>(copies.size()), copies.data());
}

void CommandSubmission::CopyBufferToImage(VkBuffer vkBuffer, VkImage vkImage, VkImageLayout layout, const std::vector<VkBufferImageCopy>& regions) const
{
	vkCmdCopyBufferToImage(
		vkCommandBuffer,
		vkBuffer,
		vkImage,
		layout,
		static_cast<uint32_t>(regions.size()),
		regions.data()
	);
}

void CommandSubmission::BuildAccelerationStructures(const std::vector<VkAccelerationStructureBuildGeometryInfoKHR>& buildGeomInfos, const std::vector<const VkAccelerationStructureBuildRangeInfoKHR*>& buildRangeInfoPtrs) const
{
	vkCmdBuildAccelerationStructuresKHR(vkCommandBuffer, static_cast<uint32_t>(buildGeomInfos.size()), buildGeomInfos.data(), buildRangeInfoPtrs.data());
}

void CommandSubmission::WriteAccelerationStructuresProperties(const std::vector<VkAccelerationStructureKHR>& vkAccelerationStructs, VkQueryType queryType, VkQueryPool queryPool, uint32_t firstQuery) const
{
	vkCmdWriteAccelerationStructuresPropertiesKHR(vkCommandBuffer, static_cast<uint32_t>(vkAccelerationStructs.size()), vkAccelerationStructs.data(), queryType, queryPool, firstQuery);
}

void CommandSubmission::CopyAccelerationStructure(const VkCopyAccelerationStructureInfoKHR& copyInfo) const
{
	vkCmdCopyAccelerationStructureKHR(vkCommandBuffer, &copyInfo);
}

void CommandSubmission::BindCallback(CALLBACK_BINDING_POINT bindPoint, std::function<void(CommandSubmission*)>&& callback)
{
	m_callbacks[bindPoint].push(std::move(callback));
}

void CommandSubmission::WaitTillAvailable()
{
	if (vkFence != VK_NULL_HANDLE && !m_isRecording)
	{
		vkWaitForFences(MyDevice::GetInstance().vkDevice, 1, &vkFence, VK_TRUE, UINT64_MAX);
		_DoCallbacks(CALLBACK_BINDING_POINT::COMMANDS_DONE);
	}
}

ImageBarrierBuilder::ImageBarrierBuilder()
{
	Reset();
}
void ImageBarrierBuilder::Reset()
{
	m_subresourceRange.aspectMask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT;
	m_subresourceRange.baseArrayLayer = 0;
	m_subresourceRange.baseMipLevel = 0;
	m_subresourceRange.layerCount = 1;
	m_subresourceRange.levelCount = 1;
}
void ImageBarrierBuilder::SetMipLevelRange(uint32_t baseMipLevel, uint32_t levelCount)
{
	m_subresourceRange.baseMipLevel = baseMipLevel;
	m_subresourceRange.levelCount = levelCount;
}
void ImageBarrierBuilder::SetArrayLayerRange(uint32_t baseArrayLayer, uint32_t layerCount)
{
	m_subresourceRange.baseArrayLayer = baseArrayLayer;
	m_subresourceRange.layerCount = layerCount;
}
void ImageBarrierBuilder::SetAspect(VkImageAspectFlags aspectMask)
{
	m_subresourceRange.aspectMask = aspectMask;
}
VkImageMemoryBarrier ImageBarrierBuilder::NewBarrier(VkImage _image, VkImageLayout _oldLayout, VkImageLayout _newLayout, VkAccessFlags _srcAccessMask, VkAccessFlags _dstAccessMask) const
{
	VkImageMemoryBarrier barrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
	barrier.oldLayout = _oldLayout;
	barrier.newLayout = _newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = _image;
	barrier.subresourceRange = m_subresourceRange;
	barrier.srcAccessMask = _srcAccessMask;
	barrier.dstAccessMask = _dstAccessMask;
	return barrier;
}

ImageBlitBuilder::ImageBlitBuilder()
{
	Reset();
}

void ImageBlitBuilder::Reset()
{
	m_srcSubresourceLayers.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	m_srcSubresourceLayers.baseArrayLayer = 0;
	m_srcSubresourceLayers.layerCount = 1;

	m_dstSubresourceLayers.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	m_dstSubresourceLayers.baseArrayLayer = 0;
	m_dstSubresourceLayers.layerCount = 1;
}

void ImageBlitBuilder::SetSrcAspect(VkImageAspectFlags aspectMask)
{
	m_srcSubresourceLayers.aspectMask = aspectMask;
}

void ImageBlitBuilder::SetDstAspect(VkImageAspectFlags aspectMask)
{
	m_dstSubresourceLayers.aspectMask = aspectMask;
}

void ImageBlitBuilder::SetSrcArrayLayerRange(uint32_t baseArrayLayer, uint32_t layerCount)
{
	m_srcSubresourceLayers.baseArrayLayer = baseArrayLayer;
	m_srcSubresourceLayers.layerCount = layerCount;
}

void ImageBlitBuilder::SetDstArrayLayerRange(uint32_t baseArrayLayer, uint32_t layerCount)
{
	m_dstSubresourceLayers.baseArrayLayer = baseArrayLayer;
	m_dstSubresourceLayers.layerCount = layerCount;
}

VkImageBlit ImageBlitBuilder::NewBlit(VkOffset3D srcOffsetUL, VkOffset3D srcOffsetLR, uint32_t srcMipLevel, VkOffset3D dstOffsetUL, VkOffset3D dstOffsetLR, uint32_t dstMipLevel) const
{
	VkImageBlit blit{};
	blit.srcOffsets[0] = srcOffsetUL;
	blit.srcOffsets[1] = srcOffsetLR;
	blit.dstOffsets[0] = dstOffsetUL;
	blit.dstOffsets[1] = dstOffsetLR;
	blit.srcSubresource = m_srcSubresourceLayers;
	blit.srcSubresource.mipLevel = srcMipLevel;
	blit.dstSubresource = m_dstSubresourceLayers;
	blit.dstSubresource.mipLevel = dstMipLevel;
	return blit;
}

VkImageBlit ImageBlitBuilder::NewBlit(VkOffset2D srcOffsetLR, uint32_t srcMipLevel, VkOffset2D dstOffsetLR, uint32_t dstMipLevel) const
{
	return NewBlit({ 0, 0, 0 }, { srcOffsetLR.x, srcOffsetLR.y, 1 }, srcMipLevel, { 0, 0, 0 }, { dstOffsetLR.x, dstOffsetLR.y, 1 }, dstMipLevel);
}
