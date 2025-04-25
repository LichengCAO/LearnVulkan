#include "image.h"
#include "device.h"
#include "buffer.h"
#include "commandbuffer.h"
#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#endif
#include "stb_image.h"
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
	if (!m_initCalled) return;
	assert(vkImage == VK_NULL_HANDLE);
	assert(vkDeviceMemory == VK_NULL_HANDLE);
}

void Image::SetImageInformation(ImageInformation& imageInfo)
{
	CHECK_TRUE(vkImage == VK_NULL_HANDLE, "Image is already initialized!");
	m_imageInformation = imageInfo;
}

void Image::Init()
{
	m_initCalled = true;
	if (vkImage != VK_NULL_HANDLE) return;
	VkImageCreateInfo imgInfo{ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
	imgInfo.imageType = m_imageInformation.imageType;
	imgInfo.extent = {
		.width = m_imageInformation.width,
		.height = m_imageInformation.height,
		.depth = m_imageInformation.depth
	};
	imgInfo.mipLevels = m_imageInformation.mipLevels;
	imgInfo.arrayLayers = m_imageInformation.arrayLayers;
	imgInfo.format = m_imageInformation.format;
	imgInfo.tiling = m_imageInformation.tiling;
	CHECK_TRUE(m_imageInformation.initialLayout == VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED || m_imageInformation.initialLayout == VkImageLayout::VK_IMAGE_LAYOUT_PREINITIALIZED,
		"According to the Vulkan specification, VkImageCreateInfo::initialLayout must be set to VK_IMAGE_LAYOUT_UNDEFINED or VK_IMAGE_LAYOUT_PREINITIALIZED at image creation.");
	imgInfo.initialLayout = m_imageInformation.initialLayout;
	imgInfo.usage = m_imageInformation.usage;
	imgInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imgInfo.samples = m_imageInformation.samples;
	imgInfo.flags = 0;
	CHECK_TRUE(vkImage == VK_NULL_HANDLE, "VkImage is already created!");
	VK_CHECK(vkCreateImage(MyDevice::GetInstance().vkDevice, &imgInfo, nullptr, &vkImage), "Failed to crate image!");

	_AddImageLayout();
	_AllocateMemory();
}

void Image::Uninit()
{
	if (vkImage != VK_NULL_HANDLE)
	{
		_RemoveImageLayout();
		vkDestroyImage(MyDevice::GetInstance().vkDevice, vkImage, nullptr);
		vkImage = VK_NULL_HANDLE;
	}
	_FreeMemory();
}

void Image::_AllocateMemory()
{
	MyDevice gMyDevice = MyDevice::GetInstance();
	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(gMyDevice.vkDevice, vkImage, &memRequirements);
	VkMemoryAllocateInfo allocInfo = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.allocationSize = memRequirements.size,
		.memoryTypeIndex = _FindMemoryTypeIndex(memRequirements.memoryTypeBits, m_imageInformation.memoryProperty)
	};

	VK_CHECK(vkAllocateMemory(gMyDevice.vkDevice, &allocInfo, nullptr, &vkDeviceMemory), "Failed to allocate image memory!");

	vkBindImageMemory(gMyDevice.vkDevice, vkImage, vkDeviceMemory, 0);
}

void Image::_FreeMemory()
{
	if (vkDeviceMemory != VK_NULL_HANDLE)
	{
		MyDevice gMyDevice = MyDevice::GetInstance();
		vkFreeMemory(gMyDevice.vkDevice, vkDeviceMemory, nullptr);
		vkDeviceMemory = VK_NULL_HANDLE;
	}
}

void Image::_AddImageLayout() const
{
	MyDevice& device = MyDevice::GetInstance();
	ImageLayout layout{};
	layout.Reset(m_imageInformation.arrayLayers, m_imageInformation.mipLevels, m_imageInformation.initialLayout);
	device.imageLayouts.insert({ vkImage, layout });
}

void Image::_RemoveImageLayout() const
{
	MyDevice& device = MyDevice::GetInstance();
	device.imageLayouts.erase(vkImage);
}

VkImageLayout Image::_GetImageLayout() const
{
	MyDevice& device = MyDevice::GetInstance();
	auto itr = device.imageLayouts.find(vkImage);
	CHECK_TRUE(itr != device.imageLayouts.end(), "Layout is not recorded!");
	return itr->second.GetLayout(0, m_imageInformation.arrayLayers, 0, m_imageInformation.mipLevels);
}

void Image::TransitionLayout(VkImageLayout newLayout)
{
	VkImageLayout oldLayout = _GetImageLayout();
	if (oldLayout == newLayout) return;
	CommandSubmission cmdSubmit;
	cmdSubmit.Init();
	cmdSubmit.StartOneTimeCommands({});
	ImageBarrierBuilder barrierBuilder{};
	VkImageMemoryBarrier barrier{};
	VkAccessFlags aspect = VK_IMAGE_ASPECT_NONE;

	if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) 
	{
		bool hasStencilComponent = (m_imageInformation.format == VK_FORMAT_D32_SFLOAT_S8_UINT || m_imageInformation.format == VK_FORMAT_D24_UNORM_S8_UINT);
		if (hasStencilComponent) 
		{
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
		}
		else
		{
			aspect = VK_IMAGE_ASPECT_DEPTH_BIT;
		}
	}
	else
	{
		aspect = VK_IMAGE_ASPECT_COLOR_BIT;
	}

	barrierBuilder.SetAspect(aspect);
	barrier = barrierBuilder.NewBarrier(vkImage, oldLayout, newLayout, VK_ACCESS_NONE, VK_ACCESS_NONE);

	// one time submit command will wait to be done anyway
	cmdSubmit.AddPipelineBarrier(
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
		VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
		{ barrier }
	);

	cmdSubmit.SubmitCommands();
	//m_imageInformation.initialLayout = newLayout;
}

void Image::CopyFromBuffer(const Buffer& stagingBuffer)
{
	CHECK_TRUE(vkImage != VK_NULL_HANDLE, "Image is not initialized!");

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

	cmdSubmit.CopyBufferToImage(
		stagingBuffer.vkBuffer,
		vkImage,
		_GetImageLayout(),
		{ region }
	);

	cmdSubmit.SubmitCommands();
}

ImageView Image::NewImageView(const ImageViewInformation& imageViewInfo) const
{
	CHECK_TRUE(vkImage != VK_NULL_HANDLE, "Image is not initialized!");
	ImageView val{};
	val.m_viewInformation = imageViewInfo;
	
	if (imageViewInfo.levelCount == VK_REMAINING_MIP_LEVELS)
	{
		CHECK_TRUE(m_imageInformation.mipLevels > imageViewInfo.baseMipLevel, "Wrong base mip level!");
		val.m_viewInformation.levelCount = m_imageInformation.mipLevels - imageViewInfo.baseMipLevel;
	}
	if (imageViewInfo.layerCount == VK_REMAINING_ARRAY_LAYERS)
	{
		CHECK_TRUE(m_imageInformation.arrayLayers > imageViewInfo.baseArrayLayer, "Wrong base array layer!");
		val.m_viewInformation.layerCount = m_imageInformation.arrayLayers - imageViewInfo.baseArrayLayer;
	}
	CHECK_TRUE((val.m_viewInformation.baseArrayLayer + val.m_viewInformation.layerCount) <= m_imageInformation.arrayLayers, "Image doesn't have these layers!");
	CHECK_TRUE((val.m_viewInformation.baseMipLevel + val.m_viewInformation.levelCount) <= m_imageInformation.mipLevels, "Image doesn't have these mipmap levels!");

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
	CHECK_TRUE(pImage != nullptr, "No image!");
	CHECK_TRUE(m_viewInformation.layerCount != 0, "No layer count!");
	VkImageViewType viewType = VK_IMAGE_VIEW_TYPE_MAX_ENUM;
	if (m_viewInformation.layerCount == 1)
	{
		switch (m_viewInformation.type)
		{
		case ImageType::IMAGE_TYPE_1D:
			viewType = VK_IMAGE_VIEW_TYPE_1D;
			break;
		case ImageType::IMAGE_TYPE_2D:
			viewType = VK_IMAGE_VIEW_TYPE_2D;
			break;
		case ImageType::IMAGE_TYPE_3D:
			viewType = VK_IMAGE_VIEW_TYPE_3D;
			break;
		case ImageType::IMAGE_TYPE_CUBE:
			viewType = VK_IMAGE_VIEW_TYPE_CUBE;
			break;
		default:
			CHECK_TRUE(false, "Unhandled image type!");
			break;
		}
	}
	else
	{
		switch (m_viewInformation.type)
		{
		case ImageType::IMAGE_TYPE_1D:
			viewType = VK_IMAGE_VIEW_TYPE_1D_ARRAY;
			break;
		case ImageType::IMAGE_TYPE_2D:
			viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
			break;
		case ImageType::IMAGE_TYPE_3D:
			CHECK_TRUE(false, "No 3D image array!");
			break;
		case ImageType::IMAGE_TYPE_CUBE:
			viewType = VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
			break;
		default:
			CHECK_TRUE(false, "Unhandled image type!");
			break;
		}
	}
	VkImageViewCreateInfo viewInfo = {
	.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
	.image = pImage->vkImage,
	.viewType = viewType,
	.format = pImage->GetImageInformation().format,
	.subresourceRange = {
		.aspectMask = m_viewInformation.aspectMask,
		.baseMipLevel = m_viewInformation.baseMipLevel,
		.levelCount = m_viewInformation.levelCount,
		.baseArrayLayer = m_viewInformation.baseArrayLayer,
		.layerCount = m_viewInformation.layerCount
	}
	};
	CHECK_TRUE(vkImageView == VK_NULL_HANDLE, "VkImageView is already created!");
	VK_CHECK(vkCreateImageView(MyDevice::GetInstance().vkDevice, &viewInfo, nullptr, &vkImageView), "Failed to create image view!");
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

void Texture::SetFilePath(std::string path)
{
	m_filePath = path;
}

Texture::~Texture()
{
	assert(vkSampler == VK_NULL_HANDLE);
}

void Texture::Init()
{
	CHECK_TRUE(!m_filePath.empty(), "No image file!");
	// load image
	int texWidth, texHeight, texChannels;
	stbi_uc* pixels = stbi_load(m_filePath.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
	CHECK_TRUE(pixels, "Failed to load texture image!");
	VkDeviceSize imageSize = texWidth * texHeight * 4;

	// copy host image to device
	Buffer stagingBuffer;
	BufferInformation bufferInfo;
	bufferInfo.memoryProperty = VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
	bufferInfo.size = imageSize;
	bufferInfo.usage = VkBufferUsageFlagBits::VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	stagingBuffer.Init(bufferInfo);
	stagingBuffer.CopyFromHost(pixels);

	stbi_image_free(pixels);

	ImageInformation imageInfo;
	imageInfo.width = static_cast<uint32_t>(texWidth);
	imageInfo.height = static_cast<uint32_t>(texHeight);
	imageInfo.memoryProperty = VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	imageInfo.usage = VkImageUsageFlagBits::VK_IMAGE_USAGE_TRANSFER_DST_BIT | VkImageUsageFlagBits::VK_IMAGE_USAGE_SAMPLED_BIT;
	image.SetImageInformation(imageInfo);
	image.Init();
	image.TransitionLayout(VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	image.CopyFromBuffer(stagingBuffer);
	image.TransitionLayout(VkImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	stagingBuffer.Uninit();

	// create image view
	ImageViewInformation imageviewInfo;
	imageviewInfo.aspectMask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT;
	imageView = image.NewImageView(imageviewInfo);
	imageView.Init();

	// create sampler
	VkSamplerCreateInfo samplerInfo{ VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = 0.0f;
	VkPhysicalDeviceProperties properties{};
	vkGetPhysicalDeviceProperties(MyDevice::GetInstance().vkPhysicalDevice, &properties);
	// samplerInfo.anisotropyEnable = VK_TRUE;
	// samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
	samplerInfo.anisotropyEnable = VK_FALSE;
	samplerInfo.maxAnisotropy = 1.0f;
	CHECK_TRUE(vkSampler == VK_NULL_HANDLE, "VkSampler is already created!");
	VK_CHECK(vkCreateSampler(MyDevice::GetInstance().vkDevice, &samplerInfo, nullptr, &vkSampler), "Failed to create texture sampler!");
}

void Texture::Uninit()
{
	vkDestroySampler(MyDevice::GetInstance().vkDevice, vkSampler, nullptr);
	vkSampler = VK_NULL_HANDLE;
	m_filePath = "";
	imageView.Uninit();
	image.Uninit();
}

VkDescriptorImageInfo Texture::GetVkDescriptorImageInfo() const
{
	VkDescriptorImageInfo info;
	info.imageLayout = image._GetImageLayout();
	info.sampler = vkSampler;
	info.imageView = imageView.vkImageView;
	return info;
}

bool ImageLayout::_IsInRange(uint32_t baseLayer, uint32_t& layerCount, uint32_t baseLevel, uint32_t& levelCount) const
{
	CHECK_TRUE(m_entries.size() > 0 && m_entries[0].size() > 0, "Image Layout doesn't have range!");
	uint32_t maxLayer = static_cast<uint32_t>(m_entries.size());
	uint32_t maxLevel = static_cast<uint32_t>(m_entries[0].size());
	if (layerCount == VK_REMAINING_ARRAY_LAYERS)
	{
		CHECK_TRUE(baseLayer < maxLayer, "Wrong base layer!");
		layerCount = maxLayer - baseLayer;
	}
	if (levelCount == VK_REMAINING_MIP_LEVELS)
	{
		CHECK_TRUE(baseLevel < maxLevel, "Wrong base level!");
		levelCount = maxLevel - baseLevel;
	}
	bool layerOk = (baseLayer + layerCount) <= m_entries.size();
	bool levelOK = (baseLevel + levelCount) <= m_entries[0].size();
	return layerOk && levelOK;
}
void ImageLayout::Reset(uint32_t layerCount, uint32_t levelCount, VkImageLayout layout)
{
	m_entries.resize(layerCount);
	for (auto& entry : m_entries)
	{
		entry.resize(levelCount);
	}
	for (int i = 0; i < layerCount; ++i)
	{
		for (int j = 0; j < levelCount; ++j)
		{
			m_entries[i][j].SetLayout(layout, VK_IMAGE_ASPECT_FLAG_BITS_MAX_ENUM);
		}
	}
}
VkImageLayout ImageLayout::GetLayout(VkImageSubresourceRange range) const
{
	return GetLayout(range.baseArrayLayer, range.layerCount, range.baseMipLevel, range.levelCount, range.aspectMask);
}
VkImageLayout ImageLayout::GetLayout(uint32_t baseLayer, uint32_t layerCount, uint32_t baseLevel, uint32_t levelCount, VkImageAspectFlags aspect) const
{
	VkImageLayout ret = VK_IMAGE_LAYOUT_UNDEFINED;
	bool found = false;
	CHECK_TRUE(_IsInRange(baseLayer, layerCount, baseLevel, levelCount), "Layout out of range");
	for (int i = baseLayer; i < baseLayer + layerCount; ++i)
	{
		for (int j = baseLevel; j < baseLevel + levelCount; ++j)
		{
			if (!found)
			{
				ret = m_entries[i][j].GetLayout(aspect);
				found = true;
			}
			else
			{
				CHECK_TRUE(ret == m_entries[i][j].GetLayout(aspect), "Subresource doesn't share the same layout!");
			}
		}
	}
	return ret;
}
void ImageLayout::SetLayout(VkImageLayout layout, VkImageSubresourceRange range)
{
	SetLayout(layout, range.baseArrayLayer, range.layerCount, range.baseMipLevel, range.levelCount, range.aspectMask);
}
void ImageLayout::SetLayout(VkImageLayout layout, uint32_t baseLayer, uint32_t layerCount, uint32_t baseLevel, uint32_t levelCount, VkImageAspectFlags aspect)
{
	CHECK_TRUE(_IsInRange(baseLayer, layerCount, baseLevel, levelCount), "Layout out of range");
	for (int i = baseLayer; i < baseLayer + layerCount; ++i)
	{
		for (int j = baseLevel; j < baseLevel + levelCount; ++j)
		{
			m_entries[i][j].SetLayout(layout, aspect);
		}
	}
}

VkImageLayout ImageLayout::ImageLayoutEntry::GetLayout(VkImageAspectFlags aspect) const
{
	VkImageLayout ret = VK_IMAGE_LAYOUT_UNDEFINED;
	bool found = false;
	for (const auto& p : m_layouts)
	{
		if (aspect & p.first)
		{
			if (!found)
			{
				ret = p.second;
				found = true;
			}
			else
			{
				CHECK_TRUE(ret == p.second, "Not the same layout");
			}
		}
	}
	return ret;
}
void ImageLayout::ImageLayoutEntry::SetLayout(VkImageLayout layout, VkImageAspectFlags aspect)
{
	for (auto& p : m_layouts)
	{
		if (p.first & aspect)
		{
			p.second = layout;
		}
	}
}
