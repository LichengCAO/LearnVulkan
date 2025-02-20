#include "buffer.h"
#include "device.h"
#include "commandbuffer.h"
uint32_t Buffer::_FindMemoryTypeIndex(uint32_t typeBits, VkMemoryPropertyFlags properties) const
{
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(MyDevice::GetInstance().vkPhysicalDevice, &memProperties);
	for (uint32_t res = 0; res < memProperties.memoryTypeCount; ++res) {
		bool suitableType = typeBits & (1 << res);
		bool suitableMemoryProperty = (memProperties.memoryTypes[res].propertyFlags & properties) == properties;
		if (suitableType && suitableMemoryProperty) {
			return res;
		}
	}
	throw std::runtime_error("Failed to find suitable memory type!");
	return uint32_t();
}

Buffer::~Buffer()
{
	assert(vkBuffer == VK_NULL_HANDLE);
	assert(vkDeviceMemory == VK_NULL_HANDLE);
	assert(m_mappedMemory == nullptr);
}

void Buffer::Init(BufferInformation info)
{
	VkBufferCreateInfo bufferInfo = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.size = info.size,
		.usage = info.usage,
		.sharingMode = info.sharingMode,
	};
	m_bufferInformation = info;

	VK_CHECK(vkCreateBuffer(MyDevice::GetInstance().vkDevice, &bufferInfo, nullptr, &vkBuffer), "Failed to create buffer!");

	_AllocateMemory();
}

void Buffer::Uninit()
{
	if (vkBuffer != VK_NULL_HANDLE)
	{
		vkDestroyBuffer(MyDevice::GetInstance().vkDevice, vkBuffer, nullptr);
		vkBuffer = VK_NULL_HANDLE;
	}
	
	_FreeMemory();
}

void Buffer::_AllocateMemory()
{
	CHECK_TRUE(vkBuffer != VK_NULL_HANDLE, "Try to allocate for a uninitialized buffer!");
	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(MyDevice::GetInstance().vkDevice, vkBuffer, &memRequirements);

	VkMemoryAllocateInfo allocInfo = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.allocationSize = memRequirements.size, //could be different from size on Host!
		.memoryTypeIndex = _FindMemoryTypeIndex(memRequirements.memoryTypeBits , m_bufferInformation.memoryProperty),
	};

	VK_CHECK(vkAllocateMemory(MyDevice::GetInstance().vkDevice, &allocInfo, nullptr, &vkDeviceMemory), "Failed to allocate buffer memory!");

	VkDeviceSize offset = 0; // should be divisible by memRequirements.alignment
	vkBindBufferMemory(MyDevice::GetInstance().vkDevice, vkBuffer, vkDeviceMemory, offset);
}

void Buffer::_FreeMemory()
{
	if (vkDeviceMemory != VK_NULL_HANDLE)
	{
		if (m_mappedMemory != nullptr)
		{
			vkUnmapMemory(MyDevice::GetInstance().vkDevice, vkDeviceMemory);
			m_mappedMemory = nullptr;
		}
		vkFreeMemory(MyDevice::GetInstance().vkDevice, vkDeviceMemory, nullptr);
		vkDeviceMemory = VK_NULL_HANDLE;
	}
}

void Buffer::CopyFromHost(const void* src)
{
	CHECK_TRUE(vkDeviceMemory != VK_NULL_HANDLE, "Not allocate memory yet!");
	if (m_mappedMemory == nullptr)
	{
		vkMapMemory(MyDevice::GetInstance().vkDevice, vkDeviceMemory, 0, m_bufferInformation.size, 0, &m_mappedMemory);
	}
	memcpy(m_mappedMemory, src, (size_t)m_bufferInformation.size);
}

void Buffer::CopyFromBuffer(const Buffer& otherBuffer)
{
	CHECK_TRUE(vkBuffer != VK_NULL_HANDLE, "This buffer is not initialized!");

	CommandSubmission cmdSubmit;
	cmdSubmit.Init();
	cmdSubmit.StartOneTimeCommands({});

	VkBufferCopy cpyRegion = {
		.srcOffset = 0,
		.dstOffset = 0,
		.size = m_bufferInformation.size,
	};

	vkCmdCopyBuffer(cmdSubmit.vkCommandBuffer, otherBuffer.vkBuffer, vkBuffer, 1, &cpyRegion);

	cmdSubmit.SubmitCommands();
}

void Buffer::CopyFromBuffer(const Buffer* pOtherBuffer)
{
	CopyFromBuffer(*pOtherBuffer);
}

BufferInformation Buffer::GetBufferInformation() const
{
	return m_bufferInformation;
}
