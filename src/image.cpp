#include "image.h"
#include "device.h"
#include "buffer.h"
#include "commandbuffer.h"
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
	assert(vkImage == VK_NULL_HANDLE);
	assert(vkDeviceMemory == VK_NULL_HANDLE);
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

	VK_CHECK(vkCreateImage(MyDevice::GetInstance().vkDevice, &imgInfo, nullptr, &vkImage), Failed to crate image!);
}

void Image::Uninit()
{
	if (vkImage != VK_NULL_HANDLE)
	{
		vkDestroyImage(MyDevice::GetInstance().vkDevice, vkImage, nullptr);
		vkImage = VK_NULL_HANDLE;
	}
	FreeMemory();
}

void Image::AllocateMemory()
{
	CHECK_TRUE(vkImage != VK_NULL_HANDLE, Image is not initialized!);
	MyDevice gMyDevice = MyDevice::GetInstance();
	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(gMyDevice.vkDevice, vkImage, &memRequirements);
	VkMemoryAllocateInfo allocInfo = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.allocationSize = memRequirements.size,
		.memoryTypeIndex = _FindMemoryTypeIndex(memRequirements.memoryTypeBits, m_imageInformation.memoryProperty)
	};

	VK_CHECK(vkAllocateMemory(gMyDevice.vkDevice, &allocInfo, nullptr, &vkDeviceMemory), Failed to allocate image memory!);

	vkBindImageMemory(gMyDevice.vkDevice, vkImage, vkDeviceMemory, 0);
}

void Image::FreeMemory()
{
	if (vkDeviceMemory != VK_NULL_HANDLE)
	{
		MyDevice gMyDevice = MyDevice::GetInstance();
		vkFreeMemory(gMyDevice.vkDevice, vkDeviceMemory, nullptr);
		vkDeviceMemory = VK_NULL_HANDLE;
	}
}

void Image::CopyFromBuffer(const Buffer& stagingBuffer)
{
	CHECK_TRUE(vkImage != VK_NULL_HANDLE, Image is not initialized!);

	CommandSubmission cmdSubmit;
	
	cmdSubmit.Init();

	cmdSubmit.StartOneTimeCommands({});

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
		cmdSubmit.vkCommandBuffer,
		stagingBuffer.vkBuffer,
		vkImage,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		1,
		&region
	);

	cmdSubmit.SubmitCommands();
}

ImageView Image::NewImageView(const ImageViewInformation& imageViewInfo)
{
	CHECK_TRUE(vkImage != VK_NULL_HANDLE, Image is not initialized!);
	VkImageViewCreateInfo viewInfo = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.image = vkImage,
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
	assert(vkImageView == VK_NULL_HANDLE);
}

void ImageView::Init()
{
	CHECK_TRUE(pImage != nullptr, No image!);
	VkImageViewCreateInfo viewInfo = {
	.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
	.image = pImage->vkImage,
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
	VK_CHECK(vkCreateImageView(MyDevice::GetInstance().vkDevice, &viewInfo, nullptr, &vkImageView), Failed to create image view!);
}

void ImageView::Uninit()
{
	if (vkImageView != VK_NULL_HANDLE)
	{
		vkDestroyImageView(MyDevice::GetInstance().vkDevice, vkImageView, nullptr);
		vkImageView = VK_NULL_HANDLE;
	}
	pImage = nullptr;
}

ImageViewInformation ImageView::GetImageViewInformation() const
{
	return m_viewInformation;
}
