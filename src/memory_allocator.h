#pragma once
#include "common.h"
#include "vk_mem_alloc.h"

class MemoryAllocator
{
private:
	static std::unique_ptr<VmaAllocator> uptrVmaAllocator;

public:
	// Allocate memory and then bind it with vkImage
	void AllocateForVkImage(VkImage _vkImage, VkMemoryPropertyFlags _flags, VkDeviceMemory& _outVkDeviceMem);

	// Allocate memory and then bind it with vkBuffer
	void AllocateForVkBuffer(VkBuffer _vkBuffer, VkMemoryPropertyFlags _flags, VkDeviceMemory& _outVkDeviceMem, VkDeviceSize& _outOffset);

	void FreeMemory(VkImage _vkImage);

	void FreeMemory(VkBuffer _vkBuffer);
};