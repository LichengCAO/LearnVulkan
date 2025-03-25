#pragma once
#include "common.h"

class Buffer;
class Image;
class Texture;

enum class ImageType
{
	IMAGE_TYPE_1D,
	IMAGE_TYPE_2D,
	IMAGE_TYPE_3D,
	IMAGE_TYPE_CUBE
};
struct ImageViewInformation
{
	uint32_t baseMipLevel = 0;
	uint32_t levelCount = VK_REMAINING_MIP_LEVELS;
	uint32_t baseArrayLayer = 0;
	uint32_t layerCount = VK_REMAINING_ARRAY_LAYERS;
	VkImageAspectFlags aspectMask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT;
	ImageType type = ImageType::IMAGE_TYPE_2D;
};
class ImageView
{
private:
	ImageViewInformation m_viewInformation{};
public:
	const Image* pImage = nullptr;
	VkImageView vkImageView = VK_NULL_HANDLE;
	~ImageView();
	void Init();
	void Uninit();
	ImageViewInformation GetImageViewInformation() const;
	friend class Image;
};

// vkCreateRenderPass vkMapMemory vkQueuePresentKHR vkQueueSubmit vkCmdCopyImage vkCmdCopyImageToBuffer vkCmdWaitEvents VkCmdPipelineBarrier
class ImageLayout
{
private:
	class ImageLayoutEntry
	{
	private:
		std::vector<std::pair<VkImageAspectFlagBits, VkImageLayout>> m_layouts{
			{VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED},
			{VK_IMAGE_ASPECT_DEPTH_BIT, VK_IMAGE_LAYOUT_UNDEFINED},
			{VK_IMAGE_ASPECT_STENCIL_BIT, VK_IMAGE_LAYOUT_UNDEFINED}
		};
	public:
		VkImageLayout GetLayout(VkImageAspectFlags aspect) const;
		void          SetLayout(VkImageLayout layout, VkImageAspectFlags aspect);
	};
	std::vector<std::vector<ImageLayoutEntry>> m_entries;
	bool _IsInRange(uint32_t baseLayer, uint32_t& layerCount, uint32_t baseLevel, uint32_t& levelCount) const;
public:
	void Reset(uint32_t layerCount, uint32_t levelCount, VkImageLayout layout);

	VkImageLayout GetLayout(VkImageSubresourceRange range) const;
	VkImageLayout GetLayout(uint32_t baseLayer, uint32_t layerCount, uint32_t baseLevel, uint32_t levelCount, VkImageAspectFlags aspect = VK_IMAGE_ASPECT_COLOR_BIT) const;
	void SetLayout(VkImageLayout layout, VkImageSubresourceRange range);
	void SetLayout(VkImageLayout layout,
		uint32_t baseLayer, uint32_t layerCount,
		uint32_t baseLevel, uint32_t levelCount,
		VkImageAspectFlags aspect = VK_IMAGE_ASPECT_COLOR_BIT);
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
	VkMemoryPropertyFlags memoryProperty = 0; // image memory
	VkImageLayout initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; // transfer layout
};
class Image
{
private:
	bool m_initCalled = false;
	ImageInformation m_imageInformation{};
	uint32_t _FindMemoryTypeIndex(uint32_t typeBits, VkMemoryPropertyFlags properties) const;
	void _AllocateMemory();
	void _FreeMemory();
	void _AddImageLayout() const;
	void _RemoveImageLayout() const;
	VkImageLayout _GetImageLayout() const;
public:
	~Image();
	VkImage vkImage = VK_NULL_HANDLE;
	VkDeviceMemory vkDeviceMemory = VK_NULL_HANDLE;
	
	void SetImageInformation(ImageInformation& imageInfo);

	void Init();
	void Uninit();
	
	void TransitionLayout(VkImageLayout newLayout);
	void CopyFromBuffer(const Buffer& stagingBuffer);

	ImageView NewImageView(const ImageViewInformation& imageViewInfo);

	ImageInformation GetImageInformation() const;
	friend class Texture;
};

class Texture
{
private:
	std::string m_filePath;
public:
	Image     image{};
	ImageView imageView{};
	VkSampler vkSampler = VK_NULL_HANDLE;
	~Texture();
	void SetFilePath(std::string path);
	void Init();
	void Uninit();
	VkDescriptorImageInfo GetVkDescriptorImageInfo() const;
};
