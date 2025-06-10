#pragma once
#include "common.h"

class BufferView;
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
	void CopyFromHost(const void* src, size_t size);
	void CopyFromBuffer(const Buffer& otherBuffer);
	void CopyFromBuffer(const Buffer* pOtherBuffer);

	BufferInformation GetBufferInformation() const;
	VkDeviceAddress GetDeviceAddress() const;

	BufferView NewBufferView(VkFormat _format);
};
class BufferView // use for texel buffer
{
	VkFormat m_format = VkFormat::VK_FORMAT_UNDEFINED;
public:
	const Buffer* pBuffer = nullptr;
	VkBufferView vkBufferView = VK_NULL_HANDLE;
	~BufferView();
	void Init();
	void Uninit();

	friend class Buffer;
};

//Resource Type :
//
//Storage Texel Buffer : Operates on buffer resources; ideal for 1D data that can benefit from format interpretation(e.g., integer or floating - point conversion).
//Storage Image : Operates on image resources; enables multi - dimensional access and supports a broader set of operations specific to images.
//Shader Access :
//
//Storage Texel Buffer : Accessed in shaders using imageLoadand imageStore functions with buffer access semantics.
//Storage Image : Fully supports imageLoad, imageStore, and atomic operations, providing more flexibility for image - oriented tasks.
//Setup Complexity :
//
//Storage Texel Buffer : Generally simpler to set up and use due to buffer - based nature.
//Storage Image : Involves more setup(memory layout transitions, view configurations) but offers richer functionality for image processing.