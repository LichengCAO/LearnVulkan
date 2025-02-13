#include "commandbuffer.h"
#include "device.h"
void CommandSubmission::_CreateSynchronizeObjects()
{
	VkSemaphoreCreateInfo semaphoreInfo{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
	VkFenceCreateInfo fenceInfo{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	VK_CHECK(vkCreateSemaphore(MyDevice::GetInstance().vkDevice, &semaphoreInfo, nullptr, &m_vkSemaphore), Failed to create the signal semaphore!);
	VK_CHECK(vkCreateFence(MyDevice::GetInstance().vkDevice, &fenceInfo, nullptr, &vkFence), Failed to create the availability fence!);
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
		CHECK_TRUE(fallbackIndex.has_value(), Queue family index is not set!);
		m_optQueueFamilyIndex = fallbackIndex.value();
	}
	vkGetDeviceQueue(MyDevice::GetInstance().vkDevice, m_optQueueFamilyIndex.value(), 0, &m_vkQueue);
	VkCommandBufferAllocateInfo allocInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
	allocInfo.commandPool = MyDevice::GetInstance().vkCommandPools[m_optQueueFamilyIndex.value()];
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = 1;
	VK_CHECK(vkAllocateCommandBuffers(MyDevice::GetInstance().vkDevice, &allocInfo, &vkCommandBuffer), Failed to allocate command buffer!);
}

void CommandSubmission::Uninit()
{
	if (vkCommandBuffer != VK_NULL_HANDLE)
	{
		vkFreeCommandBuffers(MyDevice::GetInstance().vkDevice, MyDevice::GetInstance().vkCommandPools[m_optQueueFamilyIndex.value()], 1, &vkCommandBuffer);
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
	CHECK_TRUE(!m_isRecording, Still recording commands!);
}

void CommandSubmission::StartCommands(const std::vector<VkSemaphore>& _waitSemaphores)
{
	CHECK_TRUE(vkCommandBuffer != VK_NULL_HANDLE, Command buffer is not initialized!);
	CHECK_TRUE(!m_isRecording, Already start commands!);
	if (vkFence == VK_NULL_HANDLE)
	{
		_CreateSynchronizeObjects();
	}
	vkWaitForFences(MyDevice::GetInstance().vkDevice, 1, &vkFence, VK_TRUE, UINT64_MAX);
	vkResetFences(MyDevice::GetInstance().vkDevice, 1, &vkFence);
	m_isRecording = true;
	VkCommandBufferBeginInfo beginInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	VK_CHECK(vkBeginCommandBuffer(vkCommandBuffer, &beginInfo), Failed to begin command!);
	m_vkWaitSemaphores = _waitSemaphores;
}

void CommandSubmission::StartOneTimeCommands(const std::vector<VkSemaphore>& _waitSemaphores)
{
	CHECK_TRUE(vkCommandBuffer != VK_NULL_HANDLE, Command buffer is not initialized!);
	CHECK_TRUE(!m_isRecording, Already start commands!);
	
	m_isRecording = true;
	VkCommandBufferBeginInfo beginInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	
	VK_CHECK(vkBeginCommandBuffer(vkCommandBuffer, &beginInfo), Failed to begin single time command!);
	m_vkWaitSemaphores = _waitSemaphores;
}

VkSemaphore CommandSubmission::SubmitCommands()
{
	CHECK_TRUE(m_isRecording, Do not start commands yet!);
	VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &vkCommandBuffer;
	submitInfo.waitSemaphoreCount = static_cast<uint32_t>(m_vkWaitSemaphores.size());
	submitInfo.pWaitSemaphores = m_vkWaitSemaphores.data();
	if (vkFence == VK_NULL_HANDLE)
	{
		VK_CHECK(vkQueueSubmit(m_vkQueue, 1, &submitInfo, vkFence), Failed to submit single time commands to queue!);
		vkQueueWaitIdle(m_vkQueue);
		Uninit(); // destroy the command buffer after one submission
	}
	else
	{
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &m_vkSemaphore;
		VK_CHECK(vkQueueSubmit(m_vkQueue, 1, &submitInfo, vkFence), Failed to submit commands to queue!);
	}
	m_isRecording = false;
	return m_vkSemaphore;
}

void CommandSubmission::WaitTillAvailable()const
{
	if (vkFence != VK_NULL_HANDLE && !m_isRecording)
	{
		vkWaitForFences(MyDevice::GetInstance().vkDevice, 1, &vkFence, VK_TRUE, UINT64_MAX);
	}
}
