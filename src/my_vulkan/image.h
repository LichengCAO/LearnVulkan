#pragma once
#include "common.h"

class Buffer;
class Image;
class Texture;
class CommandSubmission;
class MemoryAllocator;

class ImageView
{
public:
	struct Information
	{
		uint32_t baseMipLevel = 0;
		uint32_t levelCount = VK_REMAINING_MIP_LEVELS;
		uint32_t baseArrayLayer = 0;
		uint32_t layerCount = VK_REMAINING_ARRAY_LAYERS;
		VkFormat format = VK_FORMAT_UNDEFINED; // https://stackoverflow.com/questions/58600143/why-would-vkimageview-format-differ-from-the-underlying-vkimage-format
		VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		VkImageViewType type = VK_IMAGE_VIEW_TYPE_2D;
		VkImage vkImage = VK_NULL_HANDLE;
	};

private:
	Information m_viewInformation{};

public:
	const Image* pImage = nullptr;
	VkImageView vkImageView = VK_NULL_HANDLE;
	
public:
	~ImageView();
	
	void Init();
	void Uninit();
	
	const Information& GetImageViewInformation() const;
	VkImageSubresourceRange GetRange() const;

	friend class Image;
};

class ImageLayout
{
// commands that will change image layout:
//	vkCreateRenderPass vkMapMemory vkQueuePresentKHR vkQueueSubmit vkCmdCopyImage vkCmdCopyImageToBuffer vkCmdWaitEvents VkCmdPipelineBarrier
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

class Image
{
public:
	struct Information
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
		bool inSwapchain = false;
	};

private:
	bool m_initCalled = false;
	Information m_imageInformation{};

public:
	VkImage vkImage = VK_NULL_HANDLE;

private:
	void _AllocateMemory();
	
	void _FreeMemory();
	
	void _AddImageLayout() const;
	
	void _RemoveImageLayout() const;
	
	VkImageLayout _GetImageLayout() const;
	
	MemoryAllocator* _GetMemoryAllocator() const;

public:
	~Image();

	void SetImageInformation(const Information& imageInfo);

	void Init();

	void Uninit();
	
	void TransitionLayout(VkImageLayout newLayout);

	void CopyFromBuffer(const Buffer& stagingBuffer);

	// Fill image range with clear color,
	// if pCmd is nullptr, it will create a command buffer and wait till this action done,
	// else, it will record the command in the command buffer, and user need to manage the synchronization
	void Fill(
		const VkClearColorValue& clearColor,
		const VkImageSubresourceRange& range,
		CommandSubmission* pCmd = nullptr);
	
	// Fill image range with clear color,
	// if pCmd is nullptr, it will create a command buffer and wait till this action done,
	// else, it will record the command in the command buffer, and user need to manage the synchronization
	void Fill(
		const VkClearColorValue& clearColor,
		CommandSubmission* pCmd = nullptr);

	// Change image's layout and then fill it with clear color,
	// if pCmd is nullptr, it will create a command buffer and wait till this action done,
	// else, it will record the command in the command buffer, and user need to manage the synchronization
	void ChangeLayoutAndFill(
		VkImageLayout finalLayout,
		const VkClearColorValue& clearColor,
		CommandSubmission* pCmd = nullptr);

	// Return a image view of this image, 
	// the view returned is NOT initialized yet
	ImageView NewImageView(
		VkImageAspectFlags aspect = VK_IMAGE_ASPECT_COLOR_BIT,
		uint32_t baseMipLevel = 0,
		uint32_t levelCount = VK_REMAINING_MIP_LEVELS,
		uint32_t baseArrayLayer = 0,
		uint32_t layerCount = VK_REMAINING_ARRAY_LAYERS) const;

	const Information& GetImageInformation() const;

	VkExtent3D GetImageSize() const;

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
