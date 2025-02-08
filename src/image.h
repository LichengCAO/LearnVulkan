#pragma once
#include "trig_app.h"
#include <map>
struct ImageInformation
{
	uint32_t width = 0;
	uint32_t height = 0;
	uint32_t depth = 1;
	uint32_t mipLevels = 1;
	uint32_t arrayLayers = 1;
	VkFormat format			= VK_FORMAT_R8G8B8A8_SRGB;
	VkImageType imageType	= VK_IMAGE_TYPE_2D;
	VkImageTiling tiling	= VK_IMAGE_TILING_OPTIMAL;
	VkImageUsageFlags usage = 0;
	VkSampleCountFlagBits samples			= VK_SAMPLE_COUNT_1_BIT;
	VkMemoryPropertyFlags memoryProperty	= 0;
};

struct ImageViewInformation
{
	uint32_t baseMipLevel = 0;
	uint32_t levelCount = 0;
	VkImageViewType viewType = VK_IMAGE_VIEW_TYPE_2D;
	VkImageAspectFlags aspectMask;
};

class Buffer;

class Image
{
private:
	struct ImageView
	{
		VkImageView vkImageView;
		ImageViewInformation viewInformation;
	};
private:
	ImageInformation m_imageInformation;
	std::map<std::string, ImageView> m_mapImageViews;

	uint32_t _FindMemoryTypeIndex(uint32_t typeBits, VkMemoryPropertyFlags properties) const;
	void _DestroyImageView(VkImageView imageView);

public:
	VkImage vkImage;
	std::optional<VkDeviceMemory> vkDeviceMemory;
	
	void Init(ImageInformation imageInfo);
	void Uninit();
	
	void AllocateMemory(VkMemoryPropertyFlags properties);
	void FreeMemory();
	VkImageView NewImageView(std::string name, const ImageViewInformation& imageViewInfo);
	std::optional<VkImageView> GetImageView(std::string name) const;
	void DestroyImageView(std::string name);
	
	void CopyFromBuffer(const Buffer& stagingBuffer);
};