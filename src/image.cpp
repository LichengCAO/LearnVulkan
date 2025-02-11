#include "image.h"
#include "device.h"
#include "buffer.h"

uint32_t Image::_FindMemoryTypeIndex(uint32_t typeBits, VkMemoryPropertyFlags properties) const
{
	VkPhysicalDeviceMemoryProperties memProperties;
	MyDevice gMyDevice = MyDevice::GetInstance();
	vkGetPhysicalDeviceMemoryProperties(gMyDevice.vkPhysicalDevice, &memProperties);
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

Image::~Image()
{
	assert(!vkImage.has_value());
	assert(!vkDeviceMemory.has_value());
}

void Image::Init(ImageInformation imageInfo)
{
	VkImageCreateInfo imgInfo = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.imageType = imgInfo.imageType,
		.extent = {
			.width = imageInfo.width,
			.height = imageInfo.height,
			.depth = imageInfo.depth
		},
		.mipLevels = imageInfo.mipLevels,
		.arrayLayers = imageInfo.arrayLayers,
	};

	imgInfo.format = imageInfo.format;
	imgInfo.tiling = imageInfo.tiling;
	imgInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imgInfo.usage = imageInfo.usage;
	imgInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imgInfo.samples = imageInfo.samples;
	imgInfo.flags = 0;

	m_imageInformation = imageInfo;

	VkImage image;

	VK_CHECK(vkCreateImage(MyDevice::GetInstance().vkDevice, &imgInfo, nullptr, &image), Failed to crate image!);

	vkImage = image;
}

void Image::Uninit()
{
	for (auto& p : m_mapImageViews)
	{
		_DestroyImageView(p.second.vkImageView);
	}
	m_mapImageViews.clear();
	if (vkImage.has_value())
	{
		vkDestroyImage(MyDevice::GetInstance().vkDevice, vkImage.value(), nullptr);
		vkImage.reset();
	}
	FreeMemory();
}

void Image::AllocateMemory()
{
	CHECK_TRUE(vkImage.has_value(), Image is not initialized!);
	MyDevice gMyDevice = MyDevice::GetInstance();
	VkDeviceMemory deviceMemory;
	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(gMyDevice.vkDevice, vkImage.value(), &memRequirements);
	VkMemoryAllocateInfo allocInfo = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.allocationSize = memRequirements.size,
		.memoryTypeIndex = _FindMemoryTypeIndex(memRequirements.memoryTypeBits, m_imageInformation.memoryProperty)
	};

	VK_CHECK(vkAllocateMemory(gMyDevice.vkDevice, &allocInfo, nullptr, &deviceMemory), Failed to allocate image memory!);

	vkBindImageMemory(gMyDevice.vkDevice, vkImage.value(), deviceMemory, 0);
	vkDeviceMemory = deviceMemory;
}

void Image::FreeMemory()
{
	if (vkDeviceMemory.has_value())
	{
		MyDevice gMyDevice = MyDevice::GetInstance();
		vkFreeMemory(gMyDevice.vkDevice, vkDeviceMemory.value(), nullptr);
		vkDeviceMemory.reset();
	}
}

void Image::CopyFromBuffer(const Buffer& stagingBuffer)
{
	CHECK_TRUE(vkImage.has_value(), Image is not initialized!);

	MyDevice gDevice = MyDevice::GetInstance();
	VkCommandBuffer commandBuffer = gDevice.StartOneTimeCommands();

	VkBufferImageCopy region = {
	.bufferOffset = 0,
	.bufferRowLength = 0,
	.bufferImageHeight = 0,
	.imageSubresource = {
		.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
		.mipLevel = 0,
		.baseArrayLayer = 0,
		.layerCount = 1
	},
	.imageOffset = {0,0,0},
	.imageExtent = {m_imageInformation.width, m_imageInformation.height, 1},
	};

	vkCmdCopyBufferToImage(
		commandBuffer,
		stagingBuffer.vkBuffer.value(),
		vkImage.value(),
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		1,
		&region
	);

	gDevice.FinishOneTimeCommands(commandBuffer);
}

ImageView Image::NewImageView(const ImageViewInformation& imageViewInfo)
{
	CHECK_TRUE(vkImage.has_value(), Image is not initialized!);
	VkImageViewCreateInfo viewInfo = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.image = vkImage.value(),
		.viewType = imageViewInfo.viewType,
		.format = m_imageInformation.format,
		.subresourceRange = {
			.aspectMask = imageViewInfo.aspectMask,
			.baseMipLevel = imageViewInfo.baseMipLevel,
			.levelCount = imageViewInfo.levelCount,
			.baseArrayLayer = 0,
			.layerCount = 1
		}
	};
	ImageView val;
	val.m_viewInformation = imageViewInfo;
	val.pImage = this;

	return val;
}

ImageInformation Image::GetImageInformation() const
{
	return m_imageInformation;
}

ImageView::~ImageView()
{
	assert(!vkImageView.has_value());
}

void ImageView::Init()
{
	CHECK_TRUE(pImage != nullptr, No image!);
	VkImageViewCreateInfo viewInfo = {
	.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
	.image = pImage->vkImage.value(),
	.viewType = m_viewInformation.viewType,
	.format = pImage->GetImageInformation().format,
	.subresourceRange = {
		.aspectMask = m_viewInformation.aspectMask,
		.baseMipLevel = m_viewInformation.baseMipLevel,
		.levelCount = m_viewInformation.levelCount,
		.baseArrayLayer = 0,
		.layerCount = 1
	}
	};
	VkImageView imageView;
	VK_CHECK(vkCreateImageView(MyDevice::GetInstance().vkDevice, &viewInfo, nullptr, &imageView), Failed to create image view!);
	vkImageView = imageView;
}

void ImageView::Uninit()
{
	if (vkImageView.has_value())
	{
		vkDestroyImageView(MyDevice::GetInstance().vkDevice, vkImageView.value(), nullptr);
		vkImageView.reset();
	}
	pImage = nullptr;
}

ImageViewInformation ImageView::GetImageViewInformation() const
{
	return m_viewInformation;
}
