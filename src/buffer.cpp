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
	return ~0;
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
	CHECK_TRUE(vkBuffer == VK_NULL_HANDLE, "VkBuffer is already created!");
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

bool Buffer::_IsHostCoherent() const
{
	bool bResult = false;

	bResult = ((m_bufferInformation.memoryProperty & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) != 0);

	return bResult;
}

void Buffer::_CopyFromHostWithMappedMemory(const void* src, size_t size)
{
	if (m_mappedMemory == nullptr)
	{
		vkMapMemory(MyDevice::GetInstance().vkDevice, vkDeviceMemory, 0, m_bufferInformation.size, 0, &m_mappedMemory);
	}
	memcpy(m_mappedMemory, src, size);
}

void Buffer::_CopyFromHostWithStaggingBuffer(const void* src, size_t size)
{
	BufferInformation stagBufInfo{};
	Buffer stagBuf{};

	stagBufInfo.memoryProperty = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
	stagBufInfo.size = static_cast<VkDeviceSize>(size);
	stagBufInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

	stagBuf.Init(stagBufInfo);
	stagBuf.CopyFromHost(src, size);

	CopyFromBuffer(stagBuf);

	stagBuf.Uninit();
}

void Buffer::CopyFromHost(const void* src)
{
	CopyFromHost(src, static_cast<size_t>(m_bufferInformation.size));
}

void Buffer::CopyFromHost(const void* src, size_t size)
{
	CHECK_TRUE(vkDeviceMemory != VK_NULL_HANDLE, "Not allocate memory yet!");
	CHECK_TRUE(size <= static_cast<size_t>(m_bufferInformation.size), "Try to copy too much data from host!");
	if (_IsHostCoherent())
	{
		_CopyFromHostWithMappedMemory(src, size);
	}
	else
	{
		_CopyFromHostWithStaggingBuffer(src, size);
	}
}

void Buffer::CopyFromBuffer(const Buffer& otherBuffer)
{
	CommandSubmission cmdSubmit{};
	VkBufferCopy copyInfo{};

	CHECK_TRUE(vkBuffer != VK_NULL_HANDLE, "This buffer is not initialized!");
	CHECK_TRUE(((m_bufferInformation.usage & VK_BUFFER_USAGE_TRANSFER_DST_BIT) != 0), "This buffer must have VK_BUFFER_USAGE_TRANSFER_DST_BIT to copy from other buffer!");

	copyInfo.size = m_bufferInformation.size;

	cmdSubmit.Init();
	cmdSubmit.StartOneTimeCommands({});
	cmdSubmit.CopyBuffer(otherBuffer.vkBuffer, vkBuffer, { copyInfo });
	cmdSubmit.SubmitCommands();
	cmdSubmit.Uninit();
}

void Buffer::CopyFromBuffer(const Buffer* pOtherBuffer)
{
	CopyFromBuffer(*pOtherBuffer);
}

BufferInformation Buffer::GetBufferInformation() const
{
	return m_bufferInformation;
}

VkDeviceAddress Buffer::GetDeviceAddress() const
{
	VkDeviceAddress vkDeviceAddr = 0;
	VkBufferDeviceAddressInfo info{ VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO };
	
	assert(vkBuffer != VK_NULL_HANDLE);
	info.pNext = nullptr;
	info.buffer = vkBuffer;

	vkDeviceAddr = vkGetBufferDeviceAddress(MyDevice::GetInstance().vkDevice, &info);

	return vkDeviceAddr;
}

BufferView Buffer::NewBufferView(VkFormat _format)
{
	CHECK_TRUE((m_bufferInformation.usage & VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT) || (m_bufferInformation.usage & VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT), "This buffer cannot have buffer views!");
	BufferView bufferview{};
	bufferview.m_format = _format;
	bufferview.pBuffer = this;
	return bufferview;
}

BufferView::~BufferView()
{
	assert(vkBufferView == VK_NULL_HANDLE);
}

void BufferView::Init()
{
	VkBufferViewCreateInfo createInfo{ VkStructureType::VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO };
	createInfo.buffer = pBuffer->vkBuffer;
	createInfo.format = m_format;
	createInfo.offset = 0;
	createInfo.range = pBuffer->GetBufferInformation().size;
	CHECK_TRUE(vkBufferView == VK_NULL_HANDLE, "VkBufferView is already created!");
	VK_CHECK(vkCreateBufferView(MyDevice::GetInstance().vkDevice, &createInfo, nullptr, &vkBufferView), "Failed to create buffer view!");
}

void BufferView::Uninit()
{
	if (vkBufferView != VK_NULL_HANDLE)
	{
		vkDestroyBufferView(MyDevice::GetInstance().vkDevice, vkBufferView, nullptr);
		vkBufferView = VK_NULL_HANDLE;
	}
	pBuffer = nullptr;
}
