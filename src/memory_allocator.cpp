#include "memory_allocator.h"

void MemoryAllocator::AllocateForVkBuffer(VkBuffer _vkBuffer, VkMemoryPropertyFlags _flags, VkDeviceMemory& _outVkDeviceMem, VkDeviceSize& _outOffset)
{
	VmaAllocationCreateInfo info{};
	info.
}
