#include "image.h"
#include "device.h"
#include "buffer.h"
#include "commandbuffer.h"
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
	CHECK_TRUE(m_imageInformation.layout == VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED || m_imageInformation.layout == VkImageLayout::VK_IMAGE_LAYOUT_PREINITIALIZED,
		"According to the Vulkan specification, VkImageCreateInfo::initialLayout must be set to VK_IMAGE_LAYOUT_UNDEFINED or VK_IMAGE_LAYOUT_PREINITIALIZED at image creation.");
	imgInfo.initialLayout = m_imageInformation.layout;
	imgInfo.usage = m_imageInformation.usage;
	imgInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imgInfo.samples = m_imageInformation.samples;
	imgInfo.flags = 0;

	VK_CHECK(vkCreateImage(MyDevice::GetInstance().vkDevice, &imgInfo, nullptr, &vkImage), "Failed to crate image!");

	_AllocateMemory();
}

void Image::Uninit()
{
	if (vkImage != VK_NULL_HANDLE)
	{
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

void Image::TransitionLayout(VkImageLayout newLayout)
{
	if (m_imageInformation.layout == newLayout) return;
	CommandSubmission cmdSubmit;
	cmdSubmit.Init();
	cmdSubmit.StartOneTimeCommands({});
	VkImageMemoryBarrier barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
	barrier.oldLayout = m_imageInformation.layout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = vkImage;

	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) 
	{
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		bool hasStencilComponent = (m_imageInformation.format == VK_FORMAT_D32_SFLOAT_S8_UINT || m_imageInformation.format == VK_FORMAT_D24_UNORM_S8_UINT);
		if (hasStencilComponent) 
		{
			barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
		}
	}
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;

	VkPipelineStageFlags srcStage = VK_PIPELINE_STAGE_NONE;
	VkPipelineStageFlags dstStage = VK_PIPELINE_STAGE_NONE;
	if (m_imageInformation.layout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) 
	{
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		bool hasStencilComponent = (m_imageInformation.format == VK_FORMAT_D32_SFLOAT_S8_UINT || m_imageInformation.format == VK_FORMAT_D24_UNORM_S8_UINT);
		if (hasStencilComponent)
		{
			barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
		}
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		//The reading happens in the VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT stage 
		//and the writing in the VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT
		dstStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;	// the earlist pipeline stage that matches the specified operations
		srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

	}
	else if (m_imageInformation.layout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) 
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (m_imageInformation.layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) 
	{
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else if (m_imageInformation.layout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_GENERAL)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VkAccessFlagBits::VK_ACCESS_TRANSFER_WRITE_BIT;
		srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else 
	{
		throw std::invalid_argument("Unsupported layout transition!");
	}

	vkCmdPipelineBarrier(
		cmdSubmit.vkCommandBuffer,
		srcStage, 
		dstStage,
		0,
		0, nullptr,
		0, nullptr,
		1, &barrier
	);

	cmdSubmit.SubmitCommands();
	m_imageInformation.layout = newLayout;
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

	CHECK_TRUE(m_imageInformation.layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, "The layout is not transform to dst optimal yet!");

	vkCmdCopyBufferToImage(
		cmdSubmit.vkCommandBuffer,
		stagingBuffer.vkBuffer,
		vkImage,
		m_imageInformation.layout,
		1,
		&region
	);

	cmdSubmit.SubmitCommands();
}

ImageView Image::NewImageView(const ImageViewInformation& imageViewInfo)
{
	CHECK_TRUE(vkImage != VK_NULL_HANDLE, "Image is not initialized!");
	ImageView val{};
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
	CHECK_TRUE(pImage != nullptr, "No image!");
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
	info.imageLayout = image.GetImageInformation().layout;
	info.sampler = vkSampler;
	info.imageView = imageView.vkImageView;
	return info;
}
