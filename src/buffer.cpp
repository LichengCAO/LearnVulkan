#include "buffer.h"
#include "device.h"
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
	assert(!vkBuffer.has_value());
	assert(!vkDeviceMemory.has_value());
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

	VkBuffer buffer;

	VK_CHECK(vkCreateBuffer(MyDevice::GetInstance().vkDevice, &bufferInfo, nullptr, &buffer), Failed to create buffer!);

	vkBuffer = buffer;
}

void Buffer::Uninit()
{
	if (vkBuffer.has_value())
	{
		vkDestroyBuffer(MyDevice::GetInstance().vkDevice, vkBuffer.value(), nullptr);
		vkBuffer.reset();
	}
	
	FreeMemory();
}

void Buffer::AllocateMemory()
{
	CHECK_TRUE(vkBuffer.has_value(), Try to allocate for a uninitialized buffer!);
	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(MyDevice::GetInstance().vkDevice, vkBuffer.value(), &memRequirements);

	VkMemoryAllocateInfo allocInfo = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.allocationSize = memRequirements.size, //could be different from size on Host!
		.memoryTypeIndex = _FindMemoryTypeIndex(memRequirements.memoryTypeBits , m_bufferInformation.memoryProperty),
	};

	VkDeviceMemory bufferMemory;
	VK_CHECK(vkAllocateMemory(MyDevice::GetInstance().vkDevice, &allocInfo, nullptr, &bufferMemory), Failed to allocate buffer memory!);

	vkDeviceMemory = bufferMemory;

	VkDeviceSize offset = 0; // should be divisible by memRequirements.alignment
	vkBindBufferMemory(MyDevice::GetInstance().vkDevice, vkBuffer.value(), bufferMemory, offset);
}

void Buffer::FreeMemory()
{
	if (vkDeviceMemory.has_value())
	{
		if (m_mappedMemory != nullptr)
		{
			vkUnmapMemory(MyDevice::GetInstance().vkDevice, vkDeviceMemory.value());
			m_mappedMemory = nullptr;
		}
		vkFreeMemory(MyDevice::GetInstance().vkDevice, vkDeviceMemory.value(), nullptr);
		vkDeviceMemory.reset();
	}
}

void Buffer::CopyFromHost(const void* src)
{
	CHECK_TRUE(vkDeviceMemory.has_value(), Not allocate memory yet!);
	if (m_mappedMemory == nullptr)
	{
		vkMapMemory(MyDevice::GetInstance().vkDevice, vkDeviceMemory.value(), 0, m_bufferInformation.size, 0, &m_mappedMemory);
	}
	memcpy(m_mappedMemory, src, (size_t)m_bufferInformation.size);
}

void Buffer::CopyFromBuffer(const Buffer& otherBuffer)
{
	CHECK_TRUE(otherBuffer.vkBuffer.has_value(), Source buffer is not initialized!);
	CHECK_TRUE(vkBuffer.has_value(), This buffer is not initialized!);
	MyDevice gDevice = MyDevice::GetInstance();
	VkCommandBuffer commandBuffer = gDevice.StartOneTimeCommands();

	VkBufferCopy cpyRegion = {
		.srcOffset = 0,
		.dstOffset = 0,
		.size = m_bufferInformation.size,
	};

	vkCmdCopyBuffer(commandBuffer, otherBuffer.vkBuffer.value(), vkBuffer.value(), 1, &cpyRegion);

	gDevice.FinishOneTimeCommands(commandBuffer);
}

BufferInformation Buffer::GetBufferInformation() const
{
	return m_bufferInformation;
}
