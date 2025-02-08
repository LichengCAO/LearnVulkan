#pragma once
#include "trig_app.h"

struct BufferInformation
{
	VkDeviceSize size = 0;
	VkBufferUsageFlags usage = 0;
	VkSharingMode sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	VkMemoryPropertyFlags memoryProperty = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
};

class Buffer
{
private:
	BufferInformation m_bufferInformation;

	uint32_t _FindMemoryTypeIndex(uint32_t typeBits, VkMemoryPropertyFlags properties) const;
public:
	VkBuffer vkBuffer;
	std::optional<VkDeviceMemory> vkDeviceMemory;

	void Init(BufferInformation bufferInfo);
	void Uninit();

	void AllocateMemory();
	void FreeMemory();

	void CopyFromHost(void* src);
	void CopyFromBuffer(const Buffer& otherBuffer);
};