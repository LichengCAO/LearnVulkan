#pragma once
#include "common.h"

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
	BufferInformation m_bufferInformation{};
	void* m_mappedMemory = nullptr; // we store this value, since mapping is not free

	uint32_t _FindMemoryTypeIndex(uint32_t typeBits, VkMemoryPropertyFlags properties) const;
	void _AllocateMemory();
	void _FreeMemory();
public:
	~Buffer();
	VkBuffer	   vkBuffer = VK_NULL_HANDLE;
	VkDeviceMemory vkDeviceMemory = VK_NULL_HANDLE;

	void Init(BufferInformation bufferInfo);
	void Uninit();

	void CopyFromHost(const void* src);
	void CopyFromBuffer(const Buffer& otherBuffer);
	void CopyFromBuffer(const Buffer* pOtherBuffer);

	BufferInformation GetBufferInformation() const;
};