#include "commandbuffer.h"
#include "device.h"
#include "image.h"
#include "buffer.h"
void CommandSubmission::_CreateSynchronizeObjects()
{
	VkSemaphoreCreateInfo semaphoreInfo{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
	VkFenceCreateInfo fenceInfo{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	VK_CHECK(vkCreateSemaphore(MyDevice::GetInstance().vkDevice, &semaphoreInfo, nullptr, &m_vkSemaphore), "Failed to create the signal semaphore!");
	VK_CHECK(vkCreateFence(MyDevice::GetInstance().vkDevice, &fenceInfo, nullptr, &vkFence), "Failed to create the availability fence!");
}

VkImageMemoryBarrier CommandSubmission::_NewImageBarrier(const ImageBarrierInformation& _info) const
{
	VkImageMemoryBarrier barrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
	auto viewInfo = _info.pImageView->GetImageViewInformation();
	barrier.oldLayout = _info.oldLayout;
	barrier.newLayout = _info.newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = _info.pImageView->pImage->vkImage;
	barrier.subresourceRange.aspectMask = viewInfo.aspectMask;
	barrier.subresourceRange.baseMipLevel = viewInfo.baseMipLevel;
	barrier.subresourceRange.levelCount = viewInfo.levelCount;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.srcAccessMask = _info.srcAccessMask;
	barrier.dstAccessMask = _info.dstAccessMask;
	return barrier;
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
	vkGetDeviceQueue(MyDevice::GetInstance().vkDevice, m_optQueueFamilyIndex.value(), 0, &m_vkQueue);
	VkCommandBufferAllocateInfo allocInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
	allocInfo.commandPool = MyDevice::GetInstance().vkCommandPools[m_optQueueFamilyIndex.value()];
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = 1;
	VK_CHECK(vkAllocateCommandBuffers(MyDevice::GetInstance().vkDevice, &allocInfo, &vkCommandBuffer), "Failed to allocate command buffer!");
}

void CommandSubmission::Uninit()
{
	if (vkCommandBuffer != VK_NULL_HANDLE)
	{
		// vkFreeCommandBuffers(MyDevice::GetInstance().vkDevice, MyDevice::GetInstance().vkCommandPools[m_optQueueFamilyIndex.value()], 1, &vkCommandBuffer);
		vkCommandBuffer = VK_NULL_HANDLE;
	}
	if (vkFence != VK_NULL_HANDLE)
	{
		vkDestroyFence(MyDevice::GetInstance().vkDevice, vkFence, nullptr);
		vkFence = VK_NULL_HANDLE;
	}
	if (m_vkSemaphore != VK_NULL_HANDLE)
	{
		vkDestroySemaphore(MyDevice::GetInstance().vkDevice, m_vkSemaphore, nullptr);
		m_vkSemaphore = VK_NULL_HANDLE;
	}
	m_vkWaitSemaphores.clear();
	CHECK_TRUE(!m_isRecording, "Still recording commands!");
}

void CommandSubmission::StartCommands(const std::vector<WaitInformation>& _waitInfos)
{
	CHECK_TRUE(vkCommandBuffer != VK_NULL_HANDLE, "Command buffer is not initialized!");
	CHECK_TRUE(!m_isRecording, "Already start commands!");
	if (vkFence == VK_NULL_HANDLE)
	{
		_CreateSynchronizeObjects();
	}
	vkWaitForFences(MyDevice::GetInstance().vkDevice, 1, &vkFence, VK_TRUE, UINT64_MAX);
	vkResetFences(MyDevice::GetInstance().vkDevice, 1, &vkFence);
	vkResetCommandBuffer(vkCommandBuffer, 0);
	m_isRecording = true;
	VkCommandBufferBeginInfo beginInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	VK_CHECK(vkBeginCommandBuffer(vkCommandBuffer, &beginInfo), "Failed to begin command!");
	m_vkWaitSemaphores.clear();
	m_vkWaitStages.clear();
	m_vkWaitSemaphores.reserve(_waitInfos.size());
	m_vkWaitStages.reserve(_waitInfos.size());
	for (const auto& waitInfo : _waitInfos)
	{
		m_vkWaitStages.push_back(waitInfo.waitStage);
		m_vkWaitSemaphores.push_back(waitInfo.waitSamaphore);
	}

	// clear all layout recorded when a new frame start
	m_mapImageLayout.clear();
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
		m_vkWaitStages.push_back(waitInfo.waitStage);
		m_vkWaitSemaphores.push_back(waitInfo.waitSamaphore);
	}
}

void CommandSubmission::StartRenderPass(const RenderPass* pRenderPass, const Framebuffer* pFramebuffer)
{
	CHECK_TRUE(!m_isInRenderpass, "Already in render pass!");
	m_isInRenderpass = true;
	m_pCurRenderpass = pRenderPass;
	m_pCurFramebuffer = pFramebuffer;
	std::vector<VkClearValue> clearValues;
	// CHECK_TRUE(pRenderPass->attachments.size() > 0, "No attachment in render pass!"); // the attachment-less renderpass is allowed in vulkan
	for (int i = 0; i < pRenderPass->attachments.size(); ++i)
	{
		// Note that the order of clearValues should be identical to the order of your attachments.
		clearValues.push_back(pRenderPass->attachments[i].clearValue);
	}
	VkRenderPassBeginInfo renderPassInfo{ VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
	renderPassInfo.renderPass = pRenderPass->vkRenderPass;
	renderPassInfo.renderArea.offset = { 0,0 };
	if (pFramebuffer != nullptr) renderPassInfo.framebuffer = pFramebuffer->vkFramebuffer;
	if (pFramebuffer != nullptr && pRenderPass->attachments.size() > 0)
	{
		ImageInformation imageInfo = pFramebuffer->attachments[0]->pImage->GetImageInformation();
		renderPassInfo.renderArea.extent = { imageInfo.width, imageInfo.height };
	}
	renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size()); 
	renderPassInfo.pClearValues = clearValues.data(); // should have same length as attachments, although index of those who don't clear on load will be ignored

	vkCmdBeginRenderPass(vkCommandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
}

VkQueue CommandSubmission::GetVkQueue() const
{
	return m_vkQueue;
}

void CommandSubmission::EndRenderPass()
{
	CHECK_TRUE(m_isInRenderpass, "Not in render pass!");
	
	for (int i = 0; i < m_pCurFramebuffer->attachments.size(); ++i)
	{
		const ImageView* key = m_pCurFramebuffer->attachments[i];
		VkImageLayout val = m_pCurRenderpass->attachments[i].attachmentDescription.finalLayout;
		m_mapImageLayout[key] = val;
	}

	m_isInRenderpass = false;
	m_pCurRenderpass = nullptr;
	m_pCurFramebuffer = nullptr;
	vkCmdEndRenderPass(vkCommandBuffer);
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
		VK_CHECK(vkEndCommandBuffer(vkCommandBuffer), "Failed to end command buffer!");
		m_isRecording = false;
		VK_CHECK(vkQueueSubmit(m_vkQueue, 1, &submitInfo, vkFence), "Failed to submit single time commands to queue!");
		vkQueueWaitIdle(m_vkQueue);
		Uninit(); // destroy the command buffer after one submission
	}
	else
	{
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &m_vkSemaphore;
		VK_CHECK(vkEndCommandBuffer(vkCommandBuffer), "Failed to end command buffer!");
		m_isRecording = false;
		VK_CHECK(vkQueueSubmit(m_vkQueue, 1, &submitInfo, vkFence), "Failed to submit commands to queue!");
	}
	return m_vkSemaphore;
}

void CommandSubmission::AddPipelineBarrier(
	VkPipelineStageFlags srcStageMask,
	VkPipelineStageFlags dstStageMask,
	const std::vector<VkMemoryBarrier>& memoryBarriers,  
	const std::vector<ImageBarrierInformation>& imageBarriers)
{
	std::vector<VkImageMemoryBarrier> vkImageBarriers;
	vkImageBarriers.reserve(imageBarriers.size());
	for (int i = 0; i < imageBarriers.size(); ++i)
	{
		const ImageView* key = imageBarriers[i].pImageView;
		VkImageLayout    val = imageBarriers[i].newLayout;
		m_mapImageLayout[key] = val;
		vkImageBarriers.push_back(_NewImageBarrier(imageBarriers[i]));
	}
	vkCmdPipelineBarrier(
		vkCommandBuffer,
		srcStageMask,
		dstStageMask,
		0,
		static_cast<uint32_t>(memoryBarriers.size()), memoryBarriers.data(),
		0, nullptr,
		static_cast<uint32_t>(vkImageBarriers.size()), vkImageBarriers.data()
	);
}

VkImageLayout CommandSubmission::GetImageLayout(const ImageView* pImageView) const
{
	VkImageLayout ret = VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED;
	if (m_mapImageLayout.find(pImageView) != m_mapImageLayout.end())
	{
		ret = m_mapImageLayout.at(pImageView);
	}
	return ret;
}

void CommandSubmission::FillImageView(const ImageView* pImageView, VkClearColorValue clearValue) const
{
	VkImageSubresourceRange subresourceRange = {};
	auto info = pImageView->GetImageViewInformation();
	subresourceRange.aspectMask = info.aspectMask;	   // Specify the aspect, e.g., color
	subresourceRange.baseMipLevel = info.baseMipLevel; // Start at the first mip level
	subresourceRange.levelCount = info.levelCount;	   // VK_REMAINING_MIP_LEVELS;    // Apply to all mip levels
	subresourceRange.baseArrayLayer = 0;               // Start at the first array layer
	subresourceRange.layerCount = 1;				   // VK_REMAINING_ARRAY_LAYERS;  // Apply to all array layers
	vkCmdClearColorImage(
		vkCommandBuffer,
		pImageView->pImage->vkImage,
		VK_IMAGE_LAYOUT_GENERAL,
		&clearValue,
		1,
		&subresourceRange
	);
}

void CommandSubmission::FillBuffer(const Buffer* pBuffer, uint32_t data) const
{
	vkCmdFillBuffer(
		vkCommandBuffer,
		pBuffer->vkBuffer,
		0,
		pBuffer->GetBufferInformation().size,
		data
	);
}

void CommandSubmission::WaitTillAvailable()const
{
	if (vkFence != VK_NULL_HANDLE && !m_isRecording)
	{
		vkWaitForFences(MyDevice::GetInstance().vkDevice, 1, &vkFence, VK_TRUE, UINT64_MAX);
	}
}
