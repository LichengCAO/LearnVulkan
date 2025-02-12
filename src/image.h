#pragma once
#include "trig_app.h"
#include <map>

class Buffer;

struct ImageViewInformation
{
	uint32_t baseMipLevel = 0;
	uint32_t levelCount = 0;
	VkImageViewType viewType = VK_IMAGE_VIEW_TYPE_2D;
	VkImageAspectFlags aspectMask;
};
class ImageView
{
private:
	ImageViewInformation m_viewInformation;
public:
	const Image* pImage = nullptr;
	VkImageView vkImageView = VK_NULL_HANDLE;
	~ImageView();
	void Init();
	void Uninit();
	ImageViewInformation GetImageViewInformation() const;
	friend class Image;
};

struct ImageInformation
{
	uint32_t width = 0;
	uint32_t height = 0;
	uint32_t depth = 1;
	uint32_t mipLevels = 1;
	uint32_t arrayLayers = 1;
	VkFormat format = VK_FORMAT_R8G8B8A8_SRGB;
	VkImageType imageType = VK_IMAGE_TYPE_2D;
	VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;
	VkImageUsageFlags usage = 0;
	VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT;
	VkMemoryPropertyFlags memoryProperty = 0;
};
class Image
{
private:
	ImageInformation m_imageInformation;
	uint32_t _FindMemoryTypeIndex(uint32_t typeBits, VkMemoryPropertyFlags properties) const;
public:
	~Image();
	VkImage vkImage = VK_NULL_HANDLE;
	VkDeviceMemory vkDeviceMemory = VK_NULL_HANDLE;
	
	void Init(ImageInformation imageInfo);
	void Uninit();
	
	void AllocateMemory();
	void FreeMemory();
	
	void CopyFromBuffer(const Buffer& stagingBuffer);

	ImageView NewImageView(const ImageViewInformation& imageViewInfo);

	ImageInformation GetImageInformation() const;
};