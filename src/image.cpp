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

void Image::_DestroyImageView(VkImageView imageView)
{
	vkDestroyImageView(MyDevice::GetInstance().vkDevice, imageView, nullptr);
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
	for (auto& p : m_mapImageViews)
	{
		_DestroyImageView(p.second.vkImageView);
	}
	m_mapImageViews.clear();
	vkDestroyImage(MyDevice::GetInstance().vkDevice, vkImage, nullptr);
	FreeMemory();
}

void Image::AllocateMemory()
{
	MyDevice gMyDevice = MyDevice::GetInstance();
	VkDeviceMemory deviceMemory;
	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(gMyDevice.vkDevice, vkImage, &memRequirements);
	VkMemoryAllocateInfo allocInfo = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.allocationSize = memRequirements.size,
		.memoryTypeIndex = _FindMemoryTypeIndex(memRequirements.memoryTypeBits, m_imageInformation.memoryProperty)
	};

	VK_CHECK(vkAllocateMemory(gMyDevice.vkDevice, &allocInfo, nullptr, &deviceMemory), Failed to allocate image memory!);

	vkBindImageMemory(gMyDevice.vkDevice, vkImage, deviceMemory, 0);
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

VkImageView Image::NewImageView(std::string name, const ImageViewInformation& imageViewInfo)
{
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
	VkImageView imageView;
	VK_CHECK(vkCreateImageView(MyDevice::GetInstance().vkDevice, &viewInfo, nullptr, &imageView), Failed to create image view!);
	ImageView val;
	val.viewInformation = imageViewInfo;
	val.vkImageView = imageView;

	m_mapImageViews.insert({ name, val });

	return imageView;
}

std::optional<VkImageView> Image::GetImageView(std::string name) const
{
	std::optional<VkImageView> ret;
	if (m_mapImageViews.find(name) != m_mapImageViews.end())
	{
		ret = m_mapImageViews.at(name).vkImageView;
	}
	return ret;
}

void Image::DestroyImageView(std::string name)
{
	auto imageView = GetImageView(name);
	if (imageView.has_value())
	{
		_DestroyImageView(imageView.value());
		m_mapImageViews.erase(name);
	}
}

void Image::CopyFromBuffer(const Buffer& stagingBuffer)
{
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
		stagingBuffer.vkBuffer,
		vkImage,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		1,
		&region
	);

	gDevice.FinishOneTimeCommands(commandBuffer);
}
