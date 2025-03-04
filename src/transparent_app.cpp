#include "transparent_app.h"
#include "device.h"
#include "tiny_obj_loader.h"
#include <random>
#define MAX_FRAME_COUNT 3
void TransparentApp::_Init()
{
	MyDevice::GetInstance().Init();
	Model room{};
	Model wahoo{};
	Model sphere{};
	room.objFilePath = "E:/GitStorage/LearnVulkan/res/models/viking_room/viking_room.obj";
	room.texturePath = "E:/GitStorage/LearnVulkan/res/models/viking_room/viking_room.png";
	wahoo.objFilePath = "E:/GitStorage/LearnVulkan/res/models/wahoo/wahoo.obj";
	wahoo.texturePath = "E:/GitStorage/LearnVulkan/res/models/wahoo/wahoo.bmp";
	sphere.objFilePath = "E:/GitStorage/LearnVulkan/res/models/sphere/sphere.obj";
	sphere.texturePath = "E:/GitStorage/LearnVulkan/res/models/sphere/sphere.png";
	wahoo.transform.SetScale({ 0.05, 0.05, 0.05 });
	wahoo.transform.SetRotation({ 90, 0, 90 });
	wahoo.transform.SetPosition({ 0.5, 0.0, 0.2 });
	m_models = { wahoo, room};

	std::default_random_engine            rnd(3625);  // Fixed seed
	std::uniform_real_distribution<float> uniformDist;
	for (int i = 0; i < 50; ++i)
	{
		glm::vec3 center(uniformDist(rnd), uniformDist(rnd), uniformDist(rnd));
		center = (center - glm::vec3(0.5)) * 2.f;
		// Generate a random radius
		float radius = 2.f * 0.9f / 16;
		radius *= uniformDist(rnd) * 0.1f + 0.3f;
		sphere.transform.SetPosition(center);
		sphere.transform.SetScale(glm::vec3(radius));
		m_transModels.push_back(sphere);
	}

	_InitRenderPass();
	_InitDescriptorSetLayouts();

	_InitSampler();
	_InitBuffers();
	_InitImagesAndViews();
	_InitFramebuffers();
	
	_InitDescriptorSets();
	_InitVertexInputs();
	_InitPipelines();
	// init semaphores 
	VkSemaphoreCreateInfo semaphoreInfo{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
	m_swapchainImageAvailabilities.resize(MAX_FRAME_COUNT);
	for (int i = 0; i < MAX_FRAME_COUNT; ++i)
	{
		VK_CHECK(vkCreateSemaphore(MyDevice::GetInstance().vkDevice, &semaphoreInfo, nullptr, &m_swapchainImageAvailabilities[i]), "Failed to create semaphore!");
	}
	// init command buffers
	for (int i = 0; i < MAX_FRAME_COUNT; ++i)
	{
		m_commandSubmissions.push_back(CommandSubmission{});
		m_commandSubmissions.back().Init();
	}
}
void TransparentApp::_Uninit()
{
	for (auto& cmd : m_commandSubmissions)
	{
		cmd.Uninit();
	}
	for (auto& semaphore : m_swapchainImageAvailabilities)
	{
		vkDestroySemaphore(MyDevice::GetInstance().vkDevice, semaphore, nullptr);
	}
	_UninitPipelines();
	_UninitVertexInputs();
	_UninitDescriptorSets();

	_UninitFramebuffers();
	_UninitImagesAndViews();
	_UninitBuffers();
	_UninitSampler();

	_UninitDescriptorSetLayouts();
	_UninitRenderPass();
	
	MyDevice::GetInstance().Uninit();
}

void TransparentApp::_InitRenderPass()
{
	// oit
	{
		// TODO: make it attachment less some day
		AttachmentInformation readOnlyDepthInfo = AttachmentInformation::GetPresetInformation(AttachmentPreset::DEPTH);
		readOnlyDepthInfo.attachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
		readOnlyDepthInfo.attachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		readOnlyDepthInfo.attachmentDescription.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
		readOnlyDepthInfo.attachmentDescription.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
		m_oitRenderPass.AddAttachment(readOnlyDepthInfo);
		SubpassInformation oitSubpassInfo{};
		oitSubpassInfo.SetDepthStencilAttachment(0, true);
		m_oitRenderPass.AddSubpass(oitSubpassInfo);
		m_oitRenderPass.Init();
	}

	// distort
	{
		AttachmentInformation uvDistortInfo{};
		VkAttachmentDescription info{};
		VkClearValue clearColor{};
		info.format = VkFormat::VK_FORMAT_R32G32B32A32_SFLOAT;
		info.samples = VkSampleCountFlagBits::VK_SAMPLE_COUNT_1_BIT;
		info.loadOp = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_CLEAR;
		info.storeOp = VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_STORE;
		info.stencilLoadOp = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		info.stencilStoreOp = VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_DONT_CARE;
		info.initialLayout = VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED;
		info.finalLayout = VkImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		clearColor.color = { 0.0f, 0.0f, 0.0f, 0.0f };
		uvDistortInfo.attachmentDescription = info;
		uvDistortInfo.clearValue = clearColor;
		m_distortRenderPass.AddAttachment(uvDistortInfo);
		SubpassInformation distortSubpassInfo{};
		distortSubpassInfo.AddColorAttachment(0);
		m_distortRenderPass.AddSubpass(distortSubpassInfo);
		m_distortRenderPass.Init();
	}

	// gbuffer
	{
		AttachmentInformation albedoInfo = AttachmentInformation::GetPresetInformation(AttachmentPreset::GBUFFER_ALBEDO);
		AttachmentInformation posInfo = AttachmentInformation::GetPresetInformation(AttachmentPreset::GBUFFER_POSITION);
		AttachmentInformation normalInfo = AttachmentInformation::GetPresetInformation(AttachmentPreset::GBUFFER_NORMAL);
		AttachmentInformation gDepthInfo = AttachmentInformation::GetPresetInformation(AttachmentPreset::GBUFFER_DEPTH);
		AttachmentInformation depthInfo = AttachmentInformation::GetPresetInformation(AttachmentPreset::DEPTH);
		m_gbufferRenderPass.AddAttachment(albedoInfo);
		m_gbufferRenderPass.AddAttachment(posInfo);
		m_gbufferRenderPass.AddAttachment(normalInfo);
		m_gbufferRenderPass.AddAttachment(gDepthInfo);
		m_gbufferRenderPass.AddAttachment(depthInfo);
		SubpassInformation gSubpassInfo{};
		gSubpassInfo.AddColorAttachment(0);
		gSubpassInfo.AddColorAttachment(1);
		gSubpassInfo.AddColorAttachment(2);
		gSubpassInfo.AddColorAttachment(3);
		gSubpassInfo.SetDepthStencilAttachment(4);
		m_gbufferRenderPass.AddSubpass(gSubpassInfo);
		m_gbufferRenderPass.Init();
	}

	// final present
	{
		AttachmentInformation swapchainInfo = AttachmentInformation::GetPresetInformation(AttachmentPreset::SWAPCHAIN);
		m_renderPass.AddAttachment(swapchainInfo);
		SubpassInformation subpassInfo{};
		subpassInfo.AddColorAttachment(0);
		m_renderPass.AddSubpass(subpassInfo);
		m_renderPass.Init();
	}

}
void TransparentApp::_UninitRenderPass()
{
	m_renderPass.Uninit();
	m_gbufferRenderPass.Uninit();
	m_distortRenderPass.Uninit();
	m_oitRenderPass.Uninit();
}

void TransparentApp::_InitDescriptorSetLayouts()
{
	// OIT front device write read
	{
		DescriptorSetEntry binding0{}; // sample image
		binding0.descriptorCount = 1;
		binding0.descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
		binding0.stageFlags = VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT | VkShaderStageFlagBits::VK_SHADER_STAGE_COMPUTE_BIT;

		DescriptorSetEntry binding1{}; // sample count
		binding1.descriptorCount = 1;
		binding1.descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		binding1.stageFlags = VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT | VkShaderStageFlagBits::VK_SHADER_STAGE_COMPUTE_BIT;

		DescriptorSetEntry binding2{}; // in use
		binding2.descriptorCount = 1;
		binding2.descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		binding2.stageFlags = VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT;

		DescriptorSetEntry binding3{}; // viewport
		binding3.descriptorCount = 1;
		binding3.descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		binding3.stageFlags = VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT | VkShaderStageFlagBits::VK_SHADER_STAGE_COMPUTE_BIT;

		m_oitSampleDSetLayout.AddBinding(binding0);
		m_oitSampleDSetLayout.AddBinding(binding1);
		m_oitSampleDSetLayout.AddBinding(binding2);
		m_oitSampleDSetLayout.AddBinding(binding3);
		m_oitSampleDSetLayout.Init();
	}

	// OIT sorting device write
	{
		DescriptorSetEntry binding0{}; // output image
		binding0.descriptorCount = 1;
		binding0.descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		binding0.stageFlags = VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT | VkShaderStageFlagBits::VK_SHADER_STAGE_COMPUTE_BIT;

		m_oitColorDSetLayout.AddBinding(binding0);
		m_oitColorDSetLayout.Init();
	}

	// model related host write device read
	{
		DescriptorSetEntry binding0;
		binding0.descriptorCount = 1;
		binding0.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		binding0.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

		DescriptorSetEntry binding1;
		binding1.descriptorCount = 1;
		binding1.descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		binding1.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		m_modelDSetLayout.AddBinding(binding0);
		m_modelDSetLayout.AddBinding(binding1);
		m_modelDSetLayout.Init();
	}

	//camera host write device read
	{
		DescriptorSetEntry binding0;
		binding0.descriptorCount = 1;
		binding0.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		binding0.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

		m_cameraDSetLayout.AddBinding(binding0);
		m_cameraDSetLayout.Init();
	}

	// gbuffer device read
	{
		DescriptorSetEntry binding0; // albedo
		binding0.descriptorCount = 1;
		binding0.descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		binding0.stageFlags = VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT;

		DescriptorSetEntry binding1; // position
		binding1.descriptorCount = 1;
		binding1.descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		binding1.stageFlags = VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT;

		DescriptorSetEntry binding2; // normal
		binding2.descriptorCount = 1;
		binding2.descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		binding2.stageFlags = VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT;

		DescriptorSetEntry binding3; // depth
		binding3.descriptorCount = 1;
		binding3.descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		binding3.stageFlags = VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT;

		m_gbufferDSetLayout.AddBinding(binding0);
		m_gbufferDSetLayout.AddBinding(binding1);
		m_gbufferDSetLayout.AddBinding(binding2);
		m_gbufferDSetLayout.AddBinding(binding3);
		m_gbufferDSetLayout.Init();
	}

	// distort host write device read
	{
		DescriptorSetEntry binding0; // camera view information
		binding0.descriptorCount = 1;
		binding0.descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		binding0.stageFlags = VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT | VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT;

		m_distortDSetLayout.AddBinding(binding0);
		m_distortDSetLayout.Init();
	}

	// transparent device read
	{
		DescriptorSetEntry binding0; // oit
		binding0.descriptorCount = 1;
		binding0.descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		binding0.stageFlags = VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT;

		DescriptorSetEntry binding1; // distort uv
		binding1.descriptorCount = 1;
		binding1.descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		binding1.stageFlags = VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT;

		m_transOutputDSetLayout.AddBinding(binding0);
		m_transOutputDSetLayout.AddBinding(binding1);
		m_transOutputDSetLayout.Init();
	}
}
void TransparentApp::_UninitDescriptorSetLayouts()
{
	m_transOutputDSetLayout.Uninit();
	m_distortDSetLayout.Uninit();
	m_gbufferDSetLayout.Uninit();
	m_cameraDSetLayout.Uninit();
	m_modelDSetLayout.Uninit();
	m_oitColorDSetLayout.Uninit();
	m_oitSampleDSetLayout.Uninit();
}

void TransparentApp::_InitSampler()
{
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
	VK_CHECK(vkCreateSampler(MyDevice::GetInstance().vkDevice, &samplerInfo, nullptr, &m_vkSampler), "Failed to create app sampler!");
}
void TransparentApp::_UninitSampler()
{
	vkDestroySampler(MyDevice::GetInstance().vkDevice, m_vkSampler, nullptr);
	m_vkSampler = VK_NULL_HANDLE;
}

void TransparentApp::_InitBuffers()
{
	// init uniform buffers, storage buffers

	// oit
	{
		uint32_t width = MyDevice::GetInstance().GetSwapchainExtent().width;
		uint32_t height = MyDevice::GetInstance().GetSwapchainExtent().height;

		BufferInformation oitViewportBufferInfo{};
		oitViewportBufferInfo.size = sizeof(glm::ivec3);
		oitViewportBufferInfo.usage = VkBufferUsageFlagBits::VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
		oitViewportBufferInfo.memoryProperty = VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
		m_oitViewportBuffer.Init(oitViewportBufferInfo);

		BufferInformation oitSampleTexelBufferInfo{};
		oitSampleTexelBufferInfo.size = sizeof(glm::uvec4) * width * height * 5/* OIT_LAYER */;
		oitSampleTexelBufferInfo.usage = VkBufferUsageFlagBits::VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT | VkBufferUsageFlagBits::VK_BUFFER_USAGE_TRANSFER_DST_BIT; // we need to clear it first
		oitSampleTexelBufferInfo.memoryProperty = VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

		ViewportInformation info{};
		info.extent = glm::ivec4(width, height, width * height, 0);
		m_oitViewportBuffer.CopyFromHost(&info);

		m_oitSampleTexelBuffers.reserve(MAX_FRAME_COUNT);
		m_oitSampleTexelBufferViews.reserve(MAX_FRAME_COUNT);
		for (int i = 0; i < MAX_FRAME_COUNT; ++i)
		{
			m_oitSampleTexelBuffers.push_back(Buffer{});
			m_oitSampleTexelBuffers[i].Init(oitSampleTexelBufferInfo);
			m_oitSampleTexelBufferViews.push_back(m_oitSampleTexelBuffers[i].NewBufferView(VkFormat::VK_FORMAT_R32G32B32A32_UINT));
			m_oitSampleTexelBufferViews[i].Init();
		}
	}

	// model
	{
		BufferInformation modelBufferInfo;
		modelBufferInfo.size = sizeof(ModelTransform);
		modelBufferInfo.usage = VkBufferUsageFlagBits::VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
		modelBufferInfo.memoryProperty = VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
		m_vecModelBuffers.reserve(MAX_FRAME_COUNT);
		for (int i = 0; i < MAX_FRAME_COUNT; ++i)
		{
			int n = m_models.size();
			m_vecModelBuffers.push_back(std::vector<Buffer>{});
			m_vecModelBuffers[i].reserve(n);
			for (int j = 0; j < n; ++j)
			{
				m_vecModelBuffers[i].push_back(Buffer{});
				m_vecModelBuffers[i].back().Init(modelBufferInfo);
			}
		}
		m_vecTransModelBuffers.reserve(MAX_FRAME_COUNT);
		for (int i = 0; i < MAX_FRAME_COUNT; ++i)
		{
			int n = m_transModels.size();
			m_vecTransModelBuffers.push_back(std::vector<Buffer>{});
			m_vecTransModelBuffers[i].reserve(n);
			for (int j = 0; j < n; ++j)
			{
				m_vecTransModelBuffers[i].push_back(Buffer{});
				m_vecTransModelBuffers[i][j].Init(modelBufferInfo);
			}
		}
	}

	// camera
	{
		BufferInformation bufferInfo;
		bufferInfo.size = sizeof(CameraBuffer);
		bufferInfo.usage = VkBufferUsageFlagBits::VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;// | VkBufferUsageFlagBits::VK_BUFFER_USAGE_TRANSFER_DST_BIT; this is use for device to device transfer
		bufferInfo.memoryProperty = VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
		m_cameraBuffers.reserve(MAX_FRAME_COUNT);
		for (int i = 0; i < MAX_FRAME_COUNT; ++i)
		{
			m_cameraBuffers.push_back(Buffer{});
			m_cameraBuffers.back().Init(bufferInfo);
		}
	}

	// distort camera view info
	{
		BufferInformation bufferInfo;
		bufferInfo.size = sizeof(CameraViewInformation);
		bufferInfo.usage = VkBufferUsageFlagBits::VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
		bufferInfo.memoryProperty = VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
		m_distortBuffers.reserve(MAX_FRAME_COUNT);
		for (int i = 0; i < MAX_FRAME_COUNT; ++i)
		{
			m_distortBuffers.push_back(Buffer{});
			m_distortBuffers[i].Init(bufferInfo);
		}
	}
}
void TransparentApp::_UninitBuffers()
{
	// distort
	for (auto& buffer : m_distortBuffers)
	{
		buffer.Uninit();
	}
	m_distortBuffers.clear();

	// camera
	for (auto& buffer : m_cameraBuffers)
	{
		buffer.Uninit();
	}
	m_cameraBuffers.clear();

	// model
	for (auto& buffers : m_vecModelBuffers)
	{
		for (auto& buffer : buffers)
		{
			buffer.Uninit();
		}
		buffers.clear();
	}
	m_vecModelBuffers.clear();
	for (auto& buffers : m_vecTransModelBuffers)
	{
		for (auto& buffer : buffers)
		{
			buffer.Uninit();
		}
		buffers.clear();
	}
	m_vecTransModelBuffers.clear();

	// oit
	for (auto& view : m_oitSampleTexelBufferViews)
	{
		view.Uninit();
	}
	m_oitSampleTexelBufferViews.clear();
	for (auto& buffer : m_oitSampleTexelBuffers)
	{
		buffer.Uninit();
	}
	m_oitSampleTexelBuffers.clear();
	m_oitViewportBuffer.Uninit();
}

void TransparentApp::_InitImagesAndViews()
{
	// prevent implicit copy
	m_swapchainImages = MyDevice::GetInstance().GetSwapchainImages(); // no need to call Init() here, swapchain images are special
	uint32_t width = MyDevice::GetInstance().GetSwapchainExtent().width;
	uint32_t height = MyDevice::GetInstance().GetSwapchainExtent().height;
	
	auto n = MAX_FRAME_COUNT;
	m_swapchainImageViews.reserve(m_swapchainImages.size());
	m_depthImages.reserve(n);
	m_depthImageViews.reserve(n);
	m_gbufferPosImages.reserve(n);
	m_gbufferPosImageViews.reserve(n);
	m_gbufferNormalImages.reserve(n);
	m_gbufferNormalImageViews.reserve(n);
	m_gbufferAlbedoImages.reserve(n);
	m_gbufferAlbedoImageViews.reserve(n);
	m_gbufferDepthImages.reserve(n);
	m_gbufferDepthImageViews.reserve(n);
	m_oitSampleCountImages.reserve(n);
	m_oitSampleCountImageViews.reserve(n);
	m_oitInUseImages.reserve(n);
	m_oitInUseImageViews.reserve(n);
	m_oitColorImages.reserve(n);
	m_oitColorImageViews.reserve(n);
	m_distortImages.reserve(n);
	m_distortImageViews.reserve(n);
	
	// model textures
	{
		std::vector<std::string> textures;
		for (int i = 0; i < m_models.size(); ++i)
		{
			textures.push_back(m_models[i].texturePath);
		}
		m_modelTextures.reserve(textures.size());
		for (int i = 0; i < textures.size(); ++i)
		{
			m_modelTextures.push_back(Texture{});
			m_modelTextures[i].SetFilePath(textures[i]);
			m_modelTextures[i].Init();
		}
	}

	// OIT images
	{
		ImageInformation oitSampleCountImageInfo{};
		oitSampleCountImageInfo.format = VkFormat::VK_FORMAT_R32_UINT;
		oitSampleCountImageInfo.width = width;
		oitSampleCountImageInfo.height = height;
		oitSampleCountImageInfo.usage = VkImageUsageFlagBits::VK_IMAGE_USAGE_STORAGE_BIT | VkImageUsageFlagBits::VK_IMAGE_USAGE_TRANSFER_DST_BIT;// | VkImageUsageFlagBits::VK_IMAGE_USAGE_SAMPLED_BIT;
		oitSampleCountImageInfo.memoryProperty = VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		ImageViewInformation oitSampleCountImageViewInfo{};
		oitSampleCountImageViewInfo.aspectMask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT;

		ImageInformation oitInUseImageInfo{};
		oitInUseImageInfo.format = VkFormat::VK_FORMAT_R32_UINT;
		oitInUseImageInfo.width = width;
		oitInUseImageInfo.height = height;
		oitInUseImageInfo.usage = VkImageUsageFlagBits::VK_IMAGE_USAGE_STORAGE_BIT | VkImageUsageFlagBits::VK_IMAGE_USAGE_TRANSFER_DST_BIT;// | VkImageUsageFlagBits::VK_IMAGE_USAGE_SAMPLED_BIT;
		oitInUseImageInfo.memoryProperty = VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		ImageViewInformation oitInUseImageViewInfo{};
		oitInUseImageViewInfo.aspectMask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT;

		ImageInformation oitOutputImageInfo{};
		oitOutputImageInfo.format = VkFormat::VK_FORMAT_R32G32B32A32_SFLOAT;
		oitOutputImageInfo.width = width;
		oitOutputImageInfo.height = height;
		oitOutputImageInfo.usage = VkImageUsageFlagBits::VK_IMAGE_USAGE_STORAGE_BIT | VkImageUsageFlagBits::VK_IMAGE_USAGE_TRANSFER_DST_BIT | VkImageUsageFlagBits::VK_IMAGE_USAGE_SAMPLED_BIT;
		oitOutputImageInfo.memoryProperty = VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		ImageViewInformation oitOutputImageViewInfo{};
		oitOutputImageViewInfo.aspectMask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT;

		std::vector<std::vector<Image>*> vecOITImages = {
			&m_oitSampleCountImages,
			&m_oitInUseImages,
			&m_oitColorImages,
		};
		std::vector<std::vector<ImageView>*> vecOITImageViews = {
			&m_oitSampleCountImageViews,
			&m_oitInUseImageViews,
			&m_oitColorImageViews,
		};
		std::vector<ImageInformation*> oitImageInfos = {
			&oitSampleCountImageInfo,
			&oitInUseImageInfo,
			&oitOutputImageInfo
		};
		std::vector<ImageViewInformation*> oitViewInfos = {
			&oitSampleCountImageViewInfo,
			&oitInUseImageViewInfo,
			&oitOutputImageViewInfo
		};
		for (int i = 0; i < n; ++i)
		{
			for (int j = 0; j < vecOITImages.size(); ++j)
			{
				std::vector<Image>& oitImages = *vecOITImages[j];
				std::vector<ImageView>& oitViews = *vecOITImageViews[j];
				ImageInformation& imageInfo = *oitImageInfos[j];
				ImageViewInformation& viewInfo = *oitViewInfos[j];
				oitImages.push_back(Image{});
				Image& oitImage = oitImages.back();
				oitImage.SetImageInformation(imageInfo);
				oitImage.Init();
				oitImage.TransitionLayout(VK_IMAGE_LAYOUT_GENERAL);
				oitViews.push_back(oitImage.NewImageView(viewInfo));
				oitViews.back().Init();
			}
		}
	}

	// depth image and view
	{
		ImageInformation depthImageInfo;
		depthImageInfo.width = width;
		depthImageInfo.height = height;
		depthImageInfo.usage = VkImageUsageFlagBits::VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VkImageUsageFlagBits::VK_IMAGE_USAGE_SAMPLED_BIT;
		depthImageInfo.format = MyDevice::GetInstance().GetDepthFormat();
		depthImageInfo.memoryProperty = VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		ImageViewInformation depthImageViewInfo;
		depthImageViewInfo.aspectMask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_DEPTH_BIT;
		for (int i = 0; i < n; ++i)
		{
			m_depthImages.push_back(Image{});
			Image& depthImage = m_depthImages.back();
			depthImage.SetImageInformation(depthImageInfo);
			depthImage.Init();
			m_depthImageViews.push_back(depthImage.NewImageView(depthImageViewInfo));
			m_depthImageViews.back().Init();
		}
	}

	// gbuffer images and views
	{
		ImageInformation gbufferAlbedoImageInfo;
		gbufferAlbedoImageInfo.width = width;
		gbufferAlbedoImageInfo.height = height;
		gbufferAlbedoImageInfo.format = VkFormat::VK_FORMAT_R32G32B32A32_SFLOAT;
		gbufferAlbedoImageInfo.memoryProperty = VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		gbufferAlbedoImageInfo.usage = VkImageUsageFlagBits::VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VkImageUsageFlagBits::VK_IMAGE_USAGE_SAMPLED_BIT;
		ImageViewInformation gbufferAlbedoImageViewInfo;
		gbufferAlbedoImageViewInfo.aspectMask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT;

		ImageInformation gbufferPosImageInfo;
		gbufferPosImageInfo.width = width;
		gbufferPosImageInfo.height = height;
		gbufferPosImageInfo.format = VkFormat::VK_FORMAT_R32G32B32A32_SFLOAT;
		gbufferPosImageInfo.memoryProperty = VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		gbufferPosImageInfo.usage = VkImageUsageFlagBits::VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VkImageUsageFlagBits::VK_IMAGE_USAGE_SAMPLED_BIT;
		ImageViewInformation gbufferPosImageViewInfo;
		gbufferPosImageViewInfo.aspectMask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT;

		ImageInformation gbufferNormalImageInfo;
		gbufferNormalImageInfo.width = width;
		gbufferNormalImageInfo.height = height;
		gbufferNormalImageInfo.format = VkFormat::VK_FORMAT_R32G32B32A32_SFLOAT;
		gbufferNormalImageInfo.memoryProperty = VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		gbufferNormalImageInfo.usage = VkImageUsageFlagBits::VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VkImageUsageFlagBits::VK_IMAGE_USAGE_SAMPLED_BIT;
		ImageViewInformation gbufferNormalImageViewInfo;
		gbufferNormalImageViewInfo.aspectMask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT;

		ImageInformation gbufferDepthImageInfo;
		gbufferDepthImageInfo.width = width;
		gbufferDepthImageInfo.height = height;
		gbufferDepthImageInfo.format = VkFormat::VK_FORMAT_R32_SFLOAT;
		gbufferDepthImageInfo.memoryProperty = VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		gbufferDepthImageInfo.usage = VkImageUsageFlagBits::VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VkImageUsageFlagBits::VK_IMAGE_USAGE_SAMPLED_BIT;
		ImageViewInformation gbufferDepthImageViewInfo;
		gbufferDepthImageViewInfo.aspectMask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT;

		std::vector<std::vector<Image>*> vecGbufferImages = {
			&m_gbufferAlbedoImages,
			&m_gbufferPosImages,
			&m_gbufferNormalImages,
			&m_gbufferDepthImages
		};
		std::vector<std::vector<ImageView>*> vecGbufferImageViews = {
			&m_gbufferAlbedoImageViews,
			&m_gbufferPosImageViews,
			&m_gbufferNormalImageViews,
			&m_gbufferDepthImageViews
		};
		std::vector<ImageInformation*> gbufferImageInfos = {
			&gbufferAlbedoImageInfo,
			&gbufferPosImageInfo,
			&gbufferNormalImageInfo,
			&gbufferDepthImageInfo
		};
		std::vector<ImageViewInformation*> gbufferViewInfos = {
			&gbufferAlbedoImageViewInfo,
			&gbufferPosImageViewInfo,
			&gbufferNormalImageViewInfo,
			&gbufferDepthImageViewInfo
		};
		for (int i = 0; i < n; ++i)
		{
			for (int j = 0; j < vecGbufferImages.size(); ++j)
			{
				std::vector<Image>& gbufferImages = *vecGbufferImages[j];
				std::vector<ImageView>& gbufferViews = *vecGbufferImageViews[j];
				ImageInformation& imageInfo = *gbufferImageInfos[j];
				ImageViewInformation& viewInfo = *gbufferViewInfos[j];
				gbufferImages.push_back(Image{});
				Image& gbufferImage = gbufferImages.back();
				gbufferImage.SetImageInformation(imageInfo);
				gbufferImage.Init();
				gbufferViews.push_back(gbufferImage.NewImageView(viewInfo));
				gbufferViews.back().Init();
			}
		}
	}

	// swapchain view
	{
		ImageViewInformation swapchainImageViewInfo;
		swapchainImageViewInfo.aspectMask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT;
		for (int i = 0; i < m_swapchainImages.size(); ++i)
		{
			m_swapchainImageViews.push_back(m_swapchainImages[i].NewImageView(swapchainImageViewInfo));
			m_swapchainImageViews.back().Init();
		}
	}

	// distort image and view
	{
		ImageInformation distortUVImageInfo{};
		distortUVImageInfo.format = VkFormat::VK_FORMAT_R32G32B32A32_SFLOAT;
		distortUVImageInfo.width = width;
		distortUVImageInfo.height = height;
		distortUVImageInfo.usage = VkImageUsageFlagBits::VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VkImageUsageFlagBits::VK_IMAGE_USAGE_SAMPLED_BIT;
		distortUVImageInfo.memoryProperty = VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		ImageViewInformation distortUVImageViewInfo{};
		distortUVImageViewInfo.aspectMask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT;

		std::vector<std::vector<Image>*> vecDistortImages = 
		{
			&m_distortImages
		};
		std::vector<std::vector<ImageView>*> vecDistortImageViews = 
		{
			&m_distortImageViews
		};
		std::vector<ImageInformation*> distortImageInfos = 
		{
			&distortUVImageInfo,
		};
		std::vector<ImageViewInformation*> distortViewInfos = 
		{
			&distortUVImageViewInfo,
		};
		for (int i = 0; i < n; ++i)
		{
			for (int j = 0; j < vecDistortImages.size(); ++j)
			{
				std::vector<Image>& images = *vecDistortImages[j];
				std::vector<ImageView>& views = *vecDistortImageViews[j];
				ImageInformation& imageInfo = *distortImageInfos[j];
				ImageViewInformation& viewInfo = *distortViewInfos[j];
				images.push_back(Image{});
				Image& oitImage = images.back();
				oitImage.SetImageInformation(imageInfo);
				oitImage.Init();
				oitImage.TransitionLayout(VK_IMAGE_LAYOUT_GENERAL);
				views.push_back(oitImage.NewImageView(viewInfo));
				views.back().Init();
			}
		}
	}
}
void TransparentApp::_UninitImagesAndViews()
{
	std::vector<std::vector<ImageView>*> pViewVecsToUninit =
	{
		&m_swapchainImageViews,
		&m_depthImageViews,
		&m_gbufferAlbedoImageViews,
		&m_gbufferPosImageViews,
		&m_gbufferNormalImageViews,
		&m_gbufferDepthImageViews,
		&m_oitSampleCountImageViews,
		&m_oitInUseImageViews,
		&m_oitColorImageViews,
		&m_distortImageViews,
	};
	std::vector<std::vector<Image>*> pImageVecsToUninit =
	{
		&m_depthImages,
		&m_gbufferAlbedoImages,
		&m_gbufferPosImages,
		&m_gbufferNormalImages,
		&m_gbufferDepthImages,
		&m_oitSampleCountImages,
		&m_oitInUseImages,
		&m_oitColorImages,
		&m_distortImages
	};

	for (auto pViewVec : pViewVecsToUninit)
	{
		std::vector<ImageView>& viewVec = *pViewVec;
		for (auto& view : viewVec)
		{
			view.Uninit();
		}
		viewVec.clear();
	}
	for (auto pImageVec : pImageVecsToUninit)
	{
		std::vector<Image>& imageVec = *pImageVec;
		for (auto& image : imageVec)
		{
			image.Uninit();
		}
		imageVec.clear();
	}
	for (auto& texture : m_modelTextures)
	{
		texture.Uninit();
	}
	m_modelTextures.clear();
}

void TransparentApp::_InitFramebuffers()
{
	auto n = MAX_FRAME_COUNT;
	m_framebuffers.reserve(m_swapchainImages.size());
	m_gbufferFramebuffers.reserve(n);
	m_oitFramebuffers.reserve(n);
	m_distortFramebuffers.reserve(n);
	
	// Setup frame buffers
	for (int i = 0; i < n; ++i)
	{
		std::vector<const ImageView*> gbufferViews =
		{
			&m_gbufferAlbedoImageViews[i],
			&m_gbufferPosImageViews[i],
			&m_gbufferNormalImageViews[i],
			&m_gbufferDepthImageViews[i],
			&m_depthImageViews[i]
		};
		m_gbufferFramebuffers.push_back(m_gbufferRenderPass.NewFramebuffer(gbufferViews));
		m_gbufferFramebuffers.back().Init();

		std::vector<const ImageView*> oitDepthViews = 
		{
			&m_depthImageViews[i]
		};
		m_oitFramebuffers.push_back(m_oitRenderPass.NewFramebuffer(oitDepthViews));
		m_oitFramebuffers.back().Init();

		std::vector<const ImageView*> distortViews =
		{
			&m_distortImageViews[i],
		};
		m_distortFramebuffers.push_back(m_distortRenderPass.NewFramebuffer(distortViews));
		m_distortFramebuffers[i].Init();
	}
	for (int i = 0; i < m_swapchainImages.size(); ++i)
	{
		std::vector<const ImageView*> imageviews =
		{
			&m_swapchainImageViews[i],
		};
		m_framebuffers.push_back(m_renderPass.NewFramebuffer(imageviews));
		m_framebuffers.back().Init();
	}
}
void TransparentApp::_UninitFramebuffers()
{
	std::vector<std::vector<Framebuffer>*> pFramebufferVecsToUninit =
	{
		&m_framebuffers,
		&m_gbufferFramebuffers,
		&m_oitFramebuffers,
		&m_distortFramebuffers
	};
	for (auto pFramebufferVec : pFramebufferVecsToUninit)
	{
		std::vector<Framebuffer>& framebuffers = *pFramebufferVec;
		for (auto& framebuffer : framebuffers)
		{
			framebuffer.Uninit();
		}
		framebuffers.clear();
	}
}

void TransparentApp::_InitDescriptorSets()
{
	// OIT descriptor set
	{
		m_oitDSets.reserve(MAX_FRAME_COUNT);
		for (int i = 0; i < MAX_FRAME_COUNT; ++i)
		{
			VkDescriptorImageInfo sampleCountImageInfo{};
			sampleCountImageInfo.sampler = VK_NULL_HANDLE;						  // Typically not used for storage images
			sampleCountImageInfo.imageView = m_oitSampleCountImageViews[i].vkImageView; // VkImageView created from your image
			sampleCountImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;			  // Commonly used layout for storage images

			VkDescriptorImageInfo inUseImageInfo{};
			inUseImageInfo.sampler = VK_NULL_HANDLE;						  // Typically not used for storage images
			inUseImageInfo.imageView = m_oitInUseImageViews[i].vkImageView; // VkImageView created from your image
			inUseImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;			  // Commonly used layout for storage images

			m_oitDSets.push_back(DescriptorSet{});
			m_oitDSets[i].SetLayout(&m_oitSampleDSetLayout);
			m_oitDSets[i].Init();
			m_oitDSets[i].StartDescriptorSetUpdate();
			m_oitDSets[i].DescriptorSetUpdate_WriteBinding(0, &m_oitSampleTexelBufferViews[i]);
			m_oitDSets[i].DescriptorSetUpdate_WriteBinding(1, sampleCountImageInfo);
			m_oitDSets[i].DescriptorSetUpdate_WriteBinding(2, inUseImageInfo);
			m_oitDSets[i].DescriptorSetUpdate_WriteBinding(3, &m_oitViewportBuffer);
			m_oitDSets[i].FinishDescriptorSetUpdate();
		}

		m_oitColorDSets.reserve(MAX_FRAME_COUNT);
		for (int i = 0; i < MAX_FRAME_COUNT; ++i)
		{
			VkDescriptorImageInfo outputImageInfo{};
			outputImageInfo.sampler = VK_NULL_HANDLE;						  // Typically not used for storage images
			outputImageInfo.imageView = m_oitColorImageViews[i].vkImageView; // VkImageView created from your image
			outputImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;			  // Commonly used layout for storage images

			m_oitColorDSets.push_back(DescriptorSet{});
			m_oitColorDSets[i].SetLayout(&m_oitColorDSetLayout);
			m_oitColorDSets[i].Init();
			m_oitColorDSets[i].StartDescriptorSetUpdate();
			m_oitColorDSets[i].DescriptorSetUpdate_WriteBinding(0, outputImageInfo);
			m_oitColorDSets[i].FinishDescriptorSetUpdate();
		}
	}

	// distort descriptor set
	{
		m_distortDSets.reserve(MAX_FRAME_COUNT);
		for (int i = 0; i < MAX_FRAME_COUNT; ++i)
		{
			m_distortDSets.push_back(DescriptorSet{});
			m_distortDSets[i].SetLayout(&m_distortDSetLayout);
			m_distortDSets[i].Init();
			m_distortDSets[i].StartDescriptorSetUpdate();
			m_distortDSets[i].DescriptorSetUpdate_WriteBinding(0, &m_distortBuffers[i]);
			m_distortDSets[i].FinishDescriptorSetUpdate();
		}
	}

	// model descriptor set
	{
		m_vecModelDSets.reserve(MAX_FRAME_COUNT);
		for (int i = 0; i < MAX_FRAME_COUNT; ++i)
		{
			int n = m_models.size();
			m_vecModelDSets.push_back(std::vector<DescriptorSet>{});
			m_vecModelDSets[i].reserve(n);
			for (int j = 0; j < n; ++j)
			{
				m_vecModelDSets[i].push_back(DescriptorSet{});
				m_vecModelDSets[i][j].SetLayout(&m_modelDSetLayout);
				m_vecModelDSets[i][j].Init();
				m_vecModelDSets[i][j].StartDescriptorSetUpdate();
				m_vecModelDSets[i][j].DescriptorSetUpdate_WriteBinding(0, &m_vecModelBuffers[i][j]);
				m_vecModelDSets[i][j].DescriptorSetUpdate_WriteBinding(1, m_modelTextures[j].GetVkDescriptorImageInfo());
				m_vecModelDSets[i][j].FinishDescriptorSetUpdate();
			}
		}

		m_vecTransModelDSets.reserve(MAX_FRAME_COUNT);
		for (int i = 0; i < MAX_FRAME_COUNT; ++i)
		{
			int n = m_transModels.size();
			m_vecTransModelDSets.push_back(std::vector<DescriptorSet>{});
			m_vecTransModelDSets[i].reserve(n);
			for (int j = 0; j < n; ++j)
			{
				m_vecTransModelDSets[i].push_back(DescriptorSet{});
				m_vecTransModelDSets[i][j].SetLayout(&m_modelDSetLayout);
				m_vecTransModelDSets[i][j].Init();
				m_vecTransModelDSets[i][j].StartDescriptorSetUpdate();
				m_vecTransModelDSets[i][j].DescriptorSetUpdate_WriteBinding(0, &m_vecTransModelBuffers[i][j]);
				m_vecTransModelDSets[i][j].FinishDescriptorSetUpdate();
			}
		}
	}

	// camera descriptor set
	{
		// Setup descriptor sets 
		for (int i = 0; i < MAX_FRAME_COUNT; ++i)
		{
			m_cameraDSets.push_back(DescriptorSet{});
			m_cameraDSets.back().SetLayout(&m_cameraDSetLayout);
			m_cameraDSets.back().Init();
			m_cameraDSets.back().StartDescriptorSetUpdate();
			m_cameraDSets.back().DescriptorSetUpdate_WriteBinding(0, &m_cameraBuffers[i]);
			m_cameraDSets.back().FinishDescriptorSetUpdate();
		}
	}

	// gbuffer descriptor set
	{
		// Setup descriptor sets 
		for (int i = 0; i < MAX_FRAME_COUNT; ++i)
		{
			VkDescriptorImageInfo imageInfo0;
			imageInfo0.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageInfo0.imageView = m_gbufferAlbedoImageViews[i].vkImageView;
			imageInfo0.sampler = m_vkSampler;

			VkDescriptorImageInfo imageInfo1;
			imageInfo1.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageInfo1.imageView = m_gbufferPosImageViews[i].vkImageView;
			imageInfo1.sampler = m_vkSampler;

			VkDescriptorImageInfo imageInfo2;
			imageInfo2.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageInfo2.imageView = m_gbufferNormalImageViews[i].vkImageView;
			imageInfo2.sampler = m_vkSampler;

			VkDescriptorImageInfo imageInfo3;
			imageInfo3.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageInfo3.imageView = m_gbufferDepthImageViews[i].vkImageView;
			imageInfo3.sampler = m_vkSampler;

			VkDescriptorImageInfo imageInfo4;
			imageInfo4.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageInfo4.imageView = m_depthImageViews[i].vkImageView;
			imageInfo4.sampler = m_vkSampler;

			m_gbufferDSets.push_back(DescriptorSet{});
			m_gbufferDSets.back().SetLayout(&m_gbufferDSetLayout);
			m_gbufferDSets.back().Init();
			m_gbufferDSets.back().StartDescriptorSetUpdate();
			m_gbufferDSets.back().DescriptorSetUpdate_WriteBinding(0, imageInfo0);
			m_gbufferDSets.back().DescriptorSetUpdate_WriteBinding(1, imageInfo1);
			m_gbufferDSets.back().DescriptorSetUpdate_WriteBinding(2, imageInfo2);
			m_gbufferDSets.back().DescriptorSetUpdate_WriteBinding(3, imageInfo3);
			m_gbufferDSets.back().DescriptorSetUpdate_WriteBinding(4, imageInfo4);
			m_gbufferDSets.back().FinishDescriptorSetUpdate();
		}
	}

	// transparent
	{
		m_transOutputDSets.reserve(MAX_FRAME_COUNT);
		for (int i = 0; i < MAX_FRAME_COUNT; ++i)
		{
			VkDescriptorImageInfo oitOutputImageInfo{};
			oitOutputImageInfo.sampler = m_vkSampler;						    // Typically not used for storage images
			oitOutputImageInfo.imageView = m_oitColorImageViews[i].vkImageView; // VkImageView created from your image
			oitOutputImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			VkDescriptorImageInfo distortOutputImageInfo{};
			distortOutputImageInfo.sampler = m_vkSampler;						  // Typically not used for storage images
			distortOutputImageInfo.imageView = m_distortImageViews[i].vkImageView; // VkImageView created from your image
			distortOutputImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			m_transOutputDSets.push_back(DescriptorSet{});
			m_transOutputDSets[i].SetLayout(&m_transOutputDSetLayout);
			m_transOutputDSets[i].Init();
			m_transOutputDSets[i].StartDescriptorSetUpdate();
			m_transOutputDSets[i].DescriptorSetUpdate_WriteBinding(0, oitOutputImageInfo);
			m_transOutputDSets[i].DescriptorSetUpdate_WriteBinding(1, distortOutputImageInfo);
			m_transOutputDSets[i].FinishDescriptorSetUpdate();
		}
	}
}
void TransparentApp::_UninitDescriptorSets()
{
	m_transOutputDSets.clear();
	m_gbufferDSets.clear();
	m_cameraDSets.clear();
	for (auto& dsets : m_vecModelDSets)
	{
		dsets.clear();
	}
	m_vecModelDSets.clear();
	for (auto& dsets : m_vecTransModelDSets)
	{
		dsets.clear();
	}
	m_vecTransModelDSets.clear();
	m_distortDSets.clear();
	m_oitColorDSets.clear();
	m_oitDSets.clear();
}

void TransparentApp::_InitVertexInputs()
{
	// Setup model vertex input layout
	{
		VertexInputEntry location0;
		location0.format = VkFormat::VK_FORMAT_R32G32B32_SFLOAT;
		location0.offset = offsetof(Vertex, pos);
		VertexInputEntry location1;
		location1.format = VkFormat::VK_FORMAT_R32G32B32_SFLOAT;
		location1.offset = offsetof(Vertex, normal);
		VertexInputEntry location2;
		location2.format = VkFormat::VK_FORMAT_R32G32_SFLOAT;
		location2.offset = offsetof(Vertex, uv);

		m_gbufferVertLayout.AddLocation(location0);
		m_gbufferVertLayout.AddLocation(location1);
		m_gbufferVertLayout.AddLocation(location2);
		m_gbufferVertLayout.stride = sizeof(Vertex);
		m_gbufferVertLayout.inputRate = VkVertexInputRate::VK_VERTEX_INPUT_RATE_VERTEX;

		// Load models
		std::vector<std::string> models;
		for (int i = 0; i < m_models.size(); ++i)
		{
			models.push_back(m_models[i].objFilePath);
		}
		m_gbufferVertBuffers.reserve(models.size());
		m_gbufferIndexBuffers.reserve(models.size());

		for (int i = 0; i < models.size(); ++i)
		{
			std::vector<Vertex> vertices;
			std::vector<uint32_t> indices;
			_ReadObjFile(models[i], vertices, indices);
			// Upload to device
			m_gbufferVertBuffers.push_back(Buffer{});
			m_gbufferIndexBuffers.push_back(Buffer{});
			BufferInformation localBufferInfo;
			BufferInformation stagingBufferInfo;
			Buffer stagingBuffer;
			Buffer& vertBuffer = m_gbufferVertBuffers.back();
			Buffer& indexBuffer = m_gbufferIndexBuffers.back();
			stagingBufferInfo.size = sizeof(Vertex) * vertices.size();
			stagingBufferInfo.usage = VkBufferUsageFlagBits::VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
			stagingBufferInfo.memoryProperty = VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
			stagingBuffer.Init(stagingBufferInfo);
			stagingBuffer.CopyFromHost(vertices.data());

			localBufferInfo.size = sizeof(Vertex) * vertices.size();
			localBufferInfo.usage = VkBufferUsageFlagBits::VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VkBufferUsageFlagBits::VK_BUFFER_USAGE_TRANSFER_DST_BIT;
			localBufferInfo.memoryProperty = VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
			vertBuffer.Init(localBufferInfo);
			vertBuffer.CopyFromBuffer(stagingBuffer);

			stagingBuffer.Uninit();
			stagingBufferInfo.size = sizeof(uint32_t) * indices.size();
			stagingBuffer.Init(stagingBufferInfo);
			stagingBuffer.CopyFromHost(indices.data());

			localBufferInfo.size = sizeof(uint32_t) * indices.size();;
			localBufferInfo.usage = VkBufferUsageFlagBits::VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VkBufferUsageFlagBits::VK_BUFFER_USAGE_TRANSFER_DST_BIT;
			localBufferInfo.memoryProperty = VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
			indexBuffer.Init(localBufferInfo);
			indexBuffer.CopyFromBuffer(stagingBuffer);

			stagingBuffer.Uninit();
		}
	}
	
	struct TransparentVertex
	{
		alignas(16) glm::vec3 pos;
		alignas(16) glm::vec3 normal;
		alignas(16) glm::vec4 color;
	};
	{
		VertexInputEntry location0;
		location0.format = VkFormat::VK_FORMAT_R32G32B32_SFLOAT;
		location0.offset = offsetof(TransparentVertex, pos);
		VertexInputEntry location1;
		location1.format = VkFormat::VK_FORMAT_R32G32B32_SFLOAT;
		location1.offset = offsetof(TransparentVertex, normal);
		VertexInputEntry location2;
		location2.format = VkFormat::VK_FORMAT_R32G32B32A32_SFLOAT;
		location2.offset = offsetof(TransparentVertex, color);

		m_transModelVertLayout.AddLocation(location0);
		m_transModelVertLayout.AddLocation(location1);
		m_transModelVertLayout.AddLocation(location2);
		m_transModelVertLayout.stride = sizeof(TransparentVertex);
		m_transModelVertLayout.inputRate = VkVertexInputRate::VK_VERTEX_INPUT_RATE_VERTEX;

		std::vector<std::string> objFiles;
		for (auto& model : m_transModels)
		{
			objFiles.push_back(model.objFilePath);
		}
		m_transModelVertBuffers.reserve(objFiles.size());
		m_transModelIndexBuffers.reserve(objFiles.size());
		
		std::uniform_real_distribution<float> uniformDist;
		std::default_random_engine            rnd(3625);  // Fixed seed
		for (auto& objFile : objFiles)
		{
			std::vector<Vertex> verts{};
			std::vector<uint32_t> indices{};
			glm::vec4 color(uniformDist(rnd), uniformDist(rnd), uniformDist(rnd), uniformDist(rnd));
			color.x *= color.x;
			color.y *= color.y;
			color.z = 0.5f;
			_ReadObjFile(objFile, verts, indices);
			std::vector<TransparentVertex> vertices{};
			vertices.reserve(verts.size());
			for (int i = 0; i < verts.size(); ++i)
			{
				vertices.push_back({verts[i].pos, verts[i].normal, color});
			}
			m_transModelVertBuffers.push_back(Buffer{});
			m_transModelIndexBuffers.push_back(Buffer{});
			BufferInformation localBufferInfo;
			BufferInformation stagingBufferInfo;
			Buffer stagingBuffer;
			Buffer& vertBuffer = m_transModelVertBuffers.back();
			Buffer& indexBuffer = m_transModelIndexBuffers.back();
			stagingBufferInfo.size = sizeof(TransparentVertex) * vertices.size();
			stagingBufferInfo.usage = VkBufferUsageFlagBits::VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
			stagingBufferInfo.memoryProperty = VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
			stagingBuffer.Init(stagingBufferInfo);
			stagingBuffer.CopyFromHost(vertices.data());

			localBufferInfo.size = sizeof(TransparentVertex) * vertices.size();
			localBufferInfo.usage = VkBufferUsageFlagBits::VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VkBufferUsageFlagBits::VK_BUFFER_USAGE_TRANSFER_DST_BIT;
			localBufferInfo.memoryProperty = VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
			vertBuffer.Init(localBufferInfo);
			vertBuffer.CopyFromBuffer(stagingBuffer);

			stagingBuffer.Uninit();
			stagingBufferInfo.size = sizeof(uint32_t) * indices.size();
			stagingBuffer.Init(stagingBufferInfo);
			stagingBuffer.CopyFromHost(indices.data());

			localBufferInfo.size = sizeof(uint32_t) * indices.size();;
			localBufferInfo.usage = VkBufferUsageFlagBits::VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VkBufferUsageFlagBits::VK_BUFFER_USAGE_TRANSFER_DST_BIT;
			localBufferInfo.memoryProperty = VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
			indexBuffer.Init(localBufferInfo);
			indexBuffer.CopyFromBuffer(stagingBuffer);

			stagingBuffer.Uninit();
		}
	}

	// setup postprocess vertex input layout
	struct QuadVertex
	{
		alignas(8) glm::vec2 pos{};
		alignas(8) glm::vec2 uv{};
	};
	{
		{
			VertexInputEntry location0;
			location0.format = VkFormat::VK_FORMAT_R32G32_SFLOAT;
			location0.offset = offsetof(QuadVertex, pos);
			VertexInputEntry location1;
			location1.format = VkFormat::VK_FORMAT_R32G32_SFLOAT;
			location1.offset = offsetof(QuadVertex, uv);
			m_quadVertLayout.AddLocation(location0);
			m_quadVertLayout.AddLocation(location1);
			m_quadVertLayout.stride = sizeof(QuadVertex);
			m_quadVertLayout.inputRate = VkVertexInputRate::VK_VERTEX_INPUT_RATE_VERTEX;
		}

		std::vector<QuadVertex> vertices = 
		{
			{{-1.f, -1.f}, {0.0f, 0.0f}},
			{{ 1.f, -1.f}, {1.0f, 0.0f}},
			{{ 1.f,  1.f}, {1.0f, 1.0f}},
			{{-1.f,  1.f}, {0.0f, 1.0f}}
		};
		std::vector<uint32_t> indices = { 2, 1, 0, 0, 3, 2 };
		BufferInformation localBufferInfo;
		BufferInformation stagingBufferInfo;
		Buffer stagingBuffer;
		Buffer& vertBuffer = m_quadVertBuffer;
		Buffer& indexBuffer = m_quadIndexBuffer;
		stagingBufferInfo.size = sizeof(QuadVertex) * vertices.size();
		stagingBufferInfo.usage = VkBufferUsageFlagBits::VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		stagingBufferInfo.memoryProperty = VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
		stagingBuffer.Init(stagingBufferInfo);
		stagingBuffer.CopyFromHost(vertices.data());

		localBufferInfo.size = sizeof(QuadVertex) * vertices.size();
		localBufferInfo.usage = VkBufferUsageFlagBits::VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VkBufferUsageFlagBits::VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		localBufferInfo.memoryProperty = VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		vertBuffer.Init(localBufferInfo);
		vertBuffer.CopyFromBuffer(stagingBuffer);

		stagingBuffer.Uninit();
		stagingBufferInfo.size = sizeof(uint32_t) * indices.size();
		stagingBuffer.Init(stagingBufferInfo);
		stagingBuffer.CopyFromHost(indices.data());

		localBufferInfo.size = sizeof(uint32_t) * indices.size();;
		localBufferInfo.usage = VkBufferUsageFlagBits::VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VkBufferUsageFlagBits::VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		localBufferInfo.memoryProperty = VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		indexBuffer.Init(localBufferInfo);
		indexBuffer.CopyFromBuffer(stagingBuffer);

		stagingBuffer.Uninit();
	}
}
void TransparentApp::_UninitVertexInputs()
{
	for (auto& vertBuffer : m_gbufferVertBuffers)
	{
		vertBuffer.Uninit();
	}
	m_gbufferVertBuffers.clear();
	for (auto& indexBuffer : m_gbufferIndexBuffers)
	{
		indexBuffer.Uninit();
	}
	m_gbufferIndexBuffers.clear();

	for (auto& vertBuffer : m_transModelVertBuffers)
	{
		vertBuffer.Uninit();
	}
	m_transModelVertBuffers.clear();
	for (auto& indexBuffer : m_transModelIndexBuffers)
	{
		indexBuffer.Uninit();
	}
	m_transModelIndexBuffers.clear();

	m_quadVertBuffer.Uninit();
	m_quadIndexBuffer.Uninit();
}

void TransparentApp::_InitPipelines()
{
	SimpleShader oitFrontVertShader;
	SimpleShader oitFrontFragShader;
	oitFrontVertShader.SetSPVFile("E:/GitStorage/LearnVulkan/bin/shaders/oit.vert.spv");
	oitFrontFragShader.SetSPVFile("E:/GitStorage/LearnVulkan/bin/shaders/oit.frag.spv");
	oitFrontVertShader.Init();
	oitFrontFragShader.Init();

	m_oitPipeline.AddDescriptorSetLayout(&m_cameraDSetLayout);
	m_oitPipeline.AddDescriptorSetLayout(&m_modelDSetLayout);
	m_oitPipeline.AddDescriptorSetLayout(&m_oitSampleDSetLayout);
	m_oitPipeline.AddDescriptorSetLayout(&m_gbufferDSetLayout); // to read depth image
	m_oitPipeline.AddShader(&oitFrontFragShader);
	m_oitPipeline.AddShader(&oitFrontVertShader);
	m_oitPipeline.BindToSubpass(&m_oitRenderPass, 0);
	m_oitPipeline.AddVertexInputLayout(&m_transModelVertLayout);
	m_oitPipeline.Init();

	oitFrontVertShader.Uninit();
	oitFrontFragShader.Uninit();

	SimpleShader oitSortShader;
	oitSortShader.SetSPVFile("E:/GitStorage/LearnVulkan/bin/shaders/oit.comp.spv");
	oitSortShader.Init();

	m_oitSortPipeline.AddDescriptorSetLayout(&m_oitSampleDSetLayout);
	m_oitSortPipeline.AddDescriptorSetLayout(&m_oitColorDSetLayout);
	m_oitSortPipeline.AddShader(&oitSortShader);
	m_oitSortPipeline.Init();

	oitSortShader.Uninit();

	SimpleShader gbufferVertShader;
	SimpleShader gbufferFragShader;
	gbufferVertShader.SetSPVFile("E:/GitStorage/LearnVulkan/bin/shaders/gbuffer.vert.spv");
	gbufferFragShader.SetSPVFile("E:/GitStorage/LearnVulkan/bin/shaders/gbuffer.frag.spv");
	gbufferVertShader.Init();
	gbufferFragShader.Init();

	m_gbufferPipeline.AddDescriptorSetLayout(&m_cameraDSetLayout);
	m_gbufferPipeline.AddDescriptorSetLayout(&m_modelDSetLayout);
	m_gbufferPipeline.AddShader(&gbufferVertShader);
	m_gbufferPipeline.AddShader(&gbufferFragShader);
	m_gbufferPipeline.BindToSubpass(&m_gbufferRenderPass, 0);
	m_gbufferPipeline.AddVertexInputLayout(&m_gbufferVertLayout);
	m_gbufferPipeline.Init();

	gbufferVertShader.Uninit();
	gbufferFragShader.Uninit();

	SimpleShader vertShader;
	SimpleShader fragShader;
	vertShader.SetSPVFile("E:/GitStorage/LearnVulkan/bin/shaders/quad.vert.spv");
	fragShader.SetSPVFile("E:/GitStorage/LearnVulkan/bin/shaders/quad.frag.spv");
	vertShader.Init();
	fragShader.Init();

	m_gPipeline.AddDescriptorSetLayout(&m_gbufferDSetLayout);
	m_gPipeline.AddDescriptorSetLayout(&m_transOutputDSetLayout);
	m_gPipeline.AddShader(&vertShader);
	m_gPipeline.AddShader(&fragShader);
	m_gPipeline.BindToSubpass(&m_renderPass, 0);
	m_gPipeline.AddVertexInputLayout(&m_quadVertLayout);
	m_gPipeline.Init();

	vertShader.Uninit();
	fragShader.Uninit();
}
void TransparentApp::_UninitPipelines()
{
	m_gPipeline.Uninit();
	m_gbufferPipeline.Uninit();
	m_oitSortPipeline.Uninit();
	m_oitPipeline.Uninit();
}

void TransparentApp::_MainLoop()
{
	lastTime = glfwGetTime();
	while (!glfwWindowShouldClose(MyDevice::GetInstance().pWindow))
	{
		glfwPollEvents();
		_DrawFrame();
		double currentTime = glfwGetTime();
		frameTime = (currentTime - lastTime) * 1000.0;
		lastTime = currentTime;
	}

	vkDeviceWaitIdle(MyDevice::GetInstance().vkDevice);
}
void TransparentApp::_UpdateUniformBuffer()
{
	static std::optional<float> lastX;
	static std::optional<float> lastY;
	UserInput userInput = MyDevice::GetInstance().GetUserInput();
	if (!userInput.RMB)
	{
		lastX = userInput.xPos;
		lastY = userInput.yPos;
	}
	float sensitivity = 0.3f;
	float xoffset = lastX.has_value() ? static_cast<float>(userInput.xPos) - lastX.value() : 0.0f;
	float yoffset = lastY.has_value() ? lastY.value() - static_cast<float>(userInput.yPos) : 0.0f;
	lastX = userInput.xPos;
	lastY = userInput.yPos;
	xoffset *= sensitivity;
	yoffset *= sensitivity;
	m_camera.RotateAboutWorldUp(glm::radians(-xoffset));
	m_camera.RotateAboutRight(glm::radians(yoffset));
	if (userInput.RMB)
	{
		glm::vec3 fwd = glm::normalize(glm::cross(m_camera.world_up, m_camera.right));
		float speed = 0.005 * frameTime;
		glm::vec3 mov = m_camera.eye;
		if (userInput.W) mov += (speed * fwd);
		if (userInput.S) mov += (-speed * fwd);
		if (userInput.Q) mov += (speed * m_camera.world_up);
		if (userInput.E) mov += (-speed * m_camera.world_up);
		if (userInput.A) mov += (-speed * m_camera.right);
		if (userInput.D) mov += (speed * m_camera.right);
		m_camera.MoveTo(mov);
	}

	CameraBuffer ubo{};
	VkExtent2D swapchainExtent = MyDevice::GetInstance().GetSwapchainExtent();
	ubo.proj = m_camera.GetProjectionMatrix();
	ubo.proj[1][1] *= -1;
	ubo.view = m_camera.GetViewMatrix();
	m_cameraBuffers[m_currentFrame].CopyFromHost(&ubo);

	ModelTransform roomTransform{};
	ModelTransform wahooTransform{};
	Transform roomT{};
	Transform wahooT{};

	wahooT.SetScale({ 0.05, 0.05, 0.05 });
	wahooT.SetRotation({ 90, 0, 0 });
	wahooT.SetPosition({ 0, 0, 0.5 });

	roomTransform.model = roomT.GetModelMatrix();
	roomTransform.modelInvTranspose = roomT.GetModelInverseTransposeMatrix();
	wahooTransform.model = wahooT.GetModelMatrix();
	wahooTransform.modelInvTranspose = wahooT.GetModelInverseTransposeMatrix();

	for (int i = 0; i < m_models.size(); ++i)
	{
		ModelTransform modelTransform{};
		modelTransform.model = m_models[i].transform.GetModelMatrix();
		modelTransform.modelInvTranspose = m_models[i].transform.GetModelInverseTransposeMatrix();
		m_vecModelBuffers[m_currentFrame][i].CopyFromHost(&modelTransform);
	}

	for (int i = 0; i < m_transModels.size(); ++i)
	{
		ModelTransform modelTransform{};
		modelTransform.model = m_transModels[i].transform.GetModelMatrix();
		modelTransform.modelInvTranspose = m_transModels[i].transform.GetModelInverseTransposeMatrix();
		m_vecTransModelBuffers[m_currentFrame][i].CopyFromHost(&modelTransform);
	}
}
void TransparentApp::_DrawFrame()
{
	if (MyDevice::GetInstance().NeedRecreateSwapchain())
	{
		_ResizeWindow();
	}

	auto& cmd = m_commandSubmissions[m_currentFrame];
	auto& uniformBuffer = m_cameraBuffers[m_currentFrame];
	cmd.WaitTillAvailable();
	auto imageIndex = MyDevice::GetInstance().AquireAvailableSwapchainImageIndex(m_swapchainImageAvailabilities[m_currentFrame]);
	if (!imageIndex.has_value()) return;
	_UpdateUniformBuffer();
	WaitInformation waitInfo{};
	waitInfo.waitSamaphore = m_swapchainImageAvailabilities[m_currentFrame];
	waitInfo.waitStage = VkPipelineStageFlagBits::VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	cmd.StartCommands({ waitInfo });
	
	// draw opaque objects
	cmd.StartRenderPass(&m_gbufferRenderPass, &m_gbufferFramebuffers[m_currentFrame]);
	for (int i = 0; i < m_gbufferVertBuffers.size(); ++i)
	{
		PipelineInput input;
		VertexIndexInput indexInput;
		VertexInput vertInput;
		indexInput.pBuffer = &m_gbufferIndexBuffers[i];
		vertInput.pVertexInputLayout = &m_gbufferVertLayout;
		vertInput.pBuffer = &m_gbufferVertBuffers[i];
		input.pDescriptorSets = { &m_cameraDSets[m_currentFrame], &m_vecModelDSets[m_currentFrame][i] };
		input.imageSize = MyDevice::GetInstance().GetSwapchainExtent();
		input.pVertexIndexInput = &indexInput;
		input.pVertexInputs = { &vertInput };
		
		m_gbufferPipeline.Do(cmd.vkCommandBuffer, input); // the draw will be done unordered
	}
	cmd.EndRenderPass();

	// wait the oit image buffers transfer back to general first
	{
		ImageBarrierInformation info{};
		info.oldLayout = VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED;
		info.newLayout = VkImageLayout::VK_IMAGE_LAYOUT_GENERAL;
		info.pImageView = &m_oitSampleCountImageViews[m_currentFrame];
		info.srcAccessMask = VkAccessFlagBits::VK_ACCESS_NONE;
		info.dstAccessMask = VkAccessFlagBits::VK_ACCESS_TRANSFER_WRITE_BIT;
		VkImageMemoryBarrier sampleCountImageBarrier = _NewImageBarreir(info);

		info.pImageView = &m_oitInUseImageViews[m_currentFrame];
		VkImageMemoryBarrier inUseImageBarrier = _NewImageBarreir(info);
		
		std::vector<VkImageMemoryBarrier> imageBarriers = { sampleCountImageBarrier, inUseImageBarrier };
		vkCmdPipelineBarrier(cmd.vkCommandBuffer,
			VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
			VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TRANSFER_BIT,  
			0,
			0, nullptr,
			0, nullptr,
			static_cast<uint32_t>(imageBarriers.size()), imageBarriers.data()
		);
	}

	// clean storage images after layout transfer
	{
		VkClearColorValue clearColor32Ruint{ .uint32 = {0u} };
		VkImageSubresourceRange subresourceRange = {};
		subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;  // Specify the aspect, e.g., color
		subresourceRange.baseMipLevel = 0;                        // Start at the first mip level
		subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;    // Apply to all mip levels
		subresourceRange.baseArrayLayer = 0;                      // Start at the first array layer
		subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;  // Apply to all array layers
		vkCmdClearColorImage(
			cmd.vkCommandBuffer,
			m_oitSampleCountImages[m_currentFrame].vkImage,
			VK_IMAGE_LAYOUT_GENERAL,
			&clearColor32Ruint,
			1,
			&subresourceRange
		);
		vkCmdClearColorImage(
			cmd.vkCommandBuffer,
			m_oitInUseImages[m_currentFrame].vkImage,
			VK_IMAGE_LAYOUT_GENERAL,
			&clearColor32Ruint,
			1,
			&subresourceRange
		);
		vkCmdFillBuffer(
			cmd.vkCommandBuffer,
			m_oitSampleTexelBuffers[m_currentFrame].vkBuffer,
			0,
			m_oitSampleTexelBuffers[m_currentFrame].GetBufferInformation().size,
			0
		);
	}

	// wait for previous depth draw done, and previous OIT texel buffers and image buffers to clean
	{
		VkMemoryBarrier sampleDataBarrier{ VK_STRUCTURE_TYPE_MEMORY_BARRIER };
		sampleDataBarrier.srcAccessMask = VkAccessFlagBits::VK_ACCESS_TRANSFER_WRITE_BIT;
		sampleDataBarrier.dstAccessMask = VkAccessFlagBits::VK_ACCESS_SHADER_READ_BIT | VkAccessFlagBits::VK_ACCESS_SHADER_WRITE_BIT;
		
		ImageBarrierInformation info{};
		info.oldLayout = VkImageLayout::VK_IMAGE_LAYOUT_GENERAL;
		info.newLayout = VkImageLayout::VK_IMAGE_LAYOUT_GENERAL;
		info.srcAccessMask = VkAccessFlagBits::VK_ACCESS_TRANSFER_WRITE_BIT;
		info.dstAccessMask = VkAccessFlagBits::VK_ACCESS_SHADER_READ_BIT | VkAccessFlagBits::VK_ACCESS_SHADER_WRITE_BIT;
		
		info.pImageView = &m_oitSampleCountImageViews[m_currentFrame];
		VkImageMemoryBarrier sampleCountImageBarrier = _NewImageBarreir(info);
		
		info.pImageView = &m_oitInUseImageViews[m_currentFrame];
		VkImageMemoryBarrier inUseImageBarrier =_NewImageBarreir(info);

		//info.oldLayout = VkImageLayout::VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		//info.newLayout = VkImageLayout::VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
		//info.srcAccessMask = VkAccessFlagBits::VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		//info.dstAccessMask = VkAccessFlagBits::VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VkAccessFlagBits::VK_ACCESS_SHADER_READ_BIT;
		//info.pImageView = &m_depthImageViews[m_currentFrame];
		//VkImageMemoryBarrier depthImageBarrier = _NewImageBarreir(info);
		info.oldLayout = VkImageLayout::VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		info.newLayout = VkImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		info.srcAccessMask = VkAccessFlagBits::VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		info.dstAccessMask = VkAccessFlagBits::VK_ACCESS_SHADER_READ_BIT;
		info.pImageView = &m_gbufferDepthImageViews[m_currentFrame];
		VkImageMemoryBarrier depthImageBarrier = _NewImageBarreir(info);

		std::vector<VkImageMemoryBarrier> imageBarriers = { sampleCountImageBarrier, inUseImageBarrier, depthImageBarrier };
		std::vector<VkMemoryBarrier> memoryBarriers = { sampleDataBarrier };
		vkCmdPipelineBarrier(cmd.vkCommandBuffer,
			//VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT,
			//VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			VkPipelineStageFlagBits::VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			0,
			static_cast<uint32_t>(memoryBarriers.size()), memoryBarriers.data(),
			0, nullptr,
			static_cast<uint32_t>(imageBarriers.size()), imageBarriers.data()
		);
	}

	// draw transparent objects, write to sample data
	cmd.StartRenderPass(&m_oitRenderPass, &m_oitFramebuffers[m_currentFrame]);
	for (int i = 0; i < m_transModelVertBuffers.size(); ++i)
	{
		PipelineInput input;
		VertexIndexInput indexInput;
		VertexInput vertInput;
		indexInput.pBuffer = &m_transModelIndexBuffers[i];
		vertInput.pVertexInputLayout = &m_transModelVertLayout;
		vertInput.pBuffer = &m_transModelVertBuffers[i];
		input.pDescriptorSets = 
		{ 
			&m_cameraDSets[m_currentFrame],
			&m_vecTransModelDSets[m_currentFrame][i],
			&m_oitDSets[m_currentFrame], // we have synthcronization here, so i think it's ok
			&m_gbufferDSets[m_currentFrame]
		};
		input.imageSize = MyDevice::GetInstance().GetSwapchainExtent();
		input.pVertexIndexInput = &indexInput;
		input.pVertexInputs = { &vertInput };

		m_oitPipeline.Do(cmd.vkCommandBuffer, input); // the draw will be done unordered
	}
	cmd.EndRenderPass();

	// wait the oit output image transfer back to general first
	{
		ImageBarrierInformation info{};
		info.oldLayout = VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED;
		info.newLayout = VkImageLayout::VK_IMAGE_LAYOUT_GENERAL;
		info.pImageView = &m_oitColorImageViews[m_currentFrame];
		info.srcAccessMask = VkAccessFlagBits::VK_ACCESS_NONE;
		info.dstAccessMask = VkAccessFlagBits::VK_ACCESS_TRANSFER_WRITE_BIT;
		VkImageMemoryBarrier oitOutputImageBarrier = _NewImageBarreir(info);

		std::vector<VkImageMemoryBarrier> imageBarriers = { oitOutputImageBarrier };
		vkCmdPipelineBarrier(cmd.vkCommandBuffer,
			VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
			VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TRANSFER_BIT,
			0,
			0, nullptr,
			0, nullptr,
			static_cast<uint32_t>(imageBarriers.size()), imageBarriers.data()
		);
	}

	// clean oit output image
	{
		VkClearColorValue clearColor32Ruint{ .uint32 = {0u} };
		VkImageSubresourceRange subresourceRange = {};
		subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;  // Specify the aspect, e.g., color
		subresourceRange.baseMipLevel = 0;                        // Start at the first mip level
		subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;    // Apply to all mip levels
		subresourceRange.baseArrayLayer = 0;                      // Start at the first array layer
		subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;  // Apply to all array layers
		vkCmdClearColorImage(
			cmd.vkCommandBuffer,
			m_oitColorImages[m_currentFrame].vkImage,
			VK_IMAGE_LAYOUT_GENERAL,
			&clearColor32Ruint,
			1,
			&subresourceRange
		);
	}

	// wait for the oit sample buffer and sample count buffer to be done, oit output image to be cleared
	{
		ImageBarrierInformation info{};
		info.oldLayout = VkImageLayout::VK_IMAGE_LAYOUT_GENERAL;
		info.newLayout = VkImageLayout::VK_IMAGE_LAYOUT_GENERAL;
		info.srcAccessMask = VkAccessFlagBits::VK_ACCESS_SHADER_WRITE_BIT;
		info.dstAccessMask = VkAccessFlagBits::VK_ACCESS_SHADER_READ_BIT;

		info.pImageView = &m_oitSampleCountImageViews[m_currentFrame];
		VkImageMemoryBarrier sampleCountImageBarrier = _NewImageBarreir(info);

		info.srcAccessMask = VkAccessFlagBits::VK_ACCESS_TRANSFER_WRITE_BIT;
		info.dstAccessMask = VkAccessFlagBits::VK_ACCESS_SHADER_WRITE_BIT;
		info.pImageView = &m_oitColorImageViews[m_currentFrame];
		VkImageMemoryBarrier oitOutputImageBarrier = _NewImageBarreir(info);

		VkMemoryBarrier sampleDataBarrier{ VK_STRUCTURE_TYPE_MEMORY_BARRIER };
		sampleDataBarrier.srcAccessMask = VkAccessFlagBits::VK_ACCESS_SHADER_WRITE_BIT;
		sampleDataBarrier.dstAccessMask = VkAccessFlagBits::VK_ACCESS_SHADER_READ_BIT;
		
		std::vector<VkImageMemoryBarrier> imageBarriers = { sampleCountImageBarrier, oitOutputImageBarrier };
		std::vector<VkMemoryBarrier> memoryBarriers = { sampleDataBarrier }; 

		vkCmdPipelineBarrier(cmd.vkCommandBuffer,
			VkPipelineStageFlagBits::VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT,
			VkPipelineStageFlagBits::VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			0,
			static_cast<uint32_t>(memoryBarriers.size()), memoryBarriers.data(),
			0, nullptr,
			static_cast<uint32_t>(imageBarriers.size()), imageBarriers.data()
		);
	}

	// sort transparent and write to OIT output
	{
		VkExtent2D extent2d = MyDevice::GetInstance().GetSwapchainExtent();
		PipelineInput pipelineInput{};
		pipelineInput.groupCountX = (extent2d.width * extent2d.height + 255)/ 256;
		pipelineInput.groupCountY = 1;
		pipelineInput.groupCountZ = 1;
		pipelineInput.pDescriptorSets = { &m_oitDSets[m_currentFrame], &m_oitColorDSets[m_currentFrame] };
		m_oitSortPipeline.Do(cmd.vkCommandBuffer, pipelineInput);
	}
	
	// wait for sort to be done and opaque color draw done
	{
		std::vector<const ImageView*> pViewsToSync =
		{
			m_gbufferFramebuffers[m_currentFrame].attachments[0], //albedo
			m_gbufferFramebuffers[m_currentFrame].attachments[1], //pos
			m_gbufferFramebuffers[m_currentFrame].attachments[2]  //normal
		};
		std::vector<VkImageMemoryBarrier> imageBarriers;

		ImageBarrierInformation info{};
		info.oldLayout = VkImageLayout::VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		info.newLayout = VkImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		info.srcAccessMask = VkAccessFlagBits::VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		info.dstAccessMask = VkAccessFlagBits::VK_ACCESS_SHADER_READ_BIT;
		for (int i = 0; i < pViewsToSync.size(); ++i)
		{
			info.pImageView = pViewsToSync[i];
			VkImageMemoryBarrier barrier = _NewImageBarreir(info);
			imageBarriers.push_back(barrier);
		}

		info.oldLayout = VkImageLayout::VK_IMAGE_LAYOUT_GENERAL;
		info.newLayout = VkImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		info.srcAccessMask = VkAccessFlagBits::VK_ACCESS_SHADER_WRITE_BIT;
		info.dstAccessMask = VkAccessFlagBits::VK_ACCESS_SHADER_READ_BIT;
		info.pImageView = &m_oitColorImageViews[m_currentFrame];
		VkImageMemoryBarrier oitOutputImageBarrier = _NewImageBarreir(info);
		imageBarriers.push_back(oitOutputImageBarrier);

		vkCmdPipelineBarrier(cmd.vkCommandBuffer,
			VkPipelineStageFlagBits::VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VkPipelineStageFlagBits::VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			VkPipelineStageFlagBits::VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			0,
			0, nullptr,
			0, nullptr,
			static_cast<uint32_t>(imageBarriers.size()), imageBarriers.data()
		);
	}

	// draw final result to the swapchain
	cmd.StartRenderPass(&m_renderPass, &m_framebuffers[imageIndex.value()]);
	{
		PipelineInput input;
		VertexIndexInput indexInput;
		VertexInput vertInput;
		indexInput.pBuffer = &m_quadIndexBuffer;
		vertInput.pVertexInputLayout = &m_quadVertLayout;
		vertInput.pBuffer = &m_quadVertBuffer;
		input.pDescriptorSets = { &m_gbufferDSets[m_currentFrame], &m_transOutputDSets[m_currentFrame] };
		input.imageSize = MyDevice::GetInstance().GetSwapchainExtent();
		input.pVertexIndexInput = &indexInput;
		input.pVertexInputs = { &vertInput };

		m_gPipeline.Do(cmd.vkCommandBuffer, input);
	}
	cmd.EndRenderPass();

	VkSemaphore renderpassFinish = cmd.SubmitCommands();
	MyDevice::GetInstance().PresentSwapchainImage({ renderpassFinish }, imageIndex.value());
	m_currentFrame = (m_currentFrame + 1) % MAX_FRAME_COUNT;
}

void TransparentApp::_ResizeWindow()
{
	// TODO: recreate texel buffer as well
	vkDeviceWaitIdle(MyDevice::GetInstance().vkDevice);
	_UninitDescriptorSets();
	_UninitFramebuffers();
	_UninitImagesAndViews();
	_UninitBuffers();
	MyDevice::GetInstance().RecreateSwapchain();
	_InitBuffers();
	_InitImagesAndViews();
	_InitFramebuffers();
	_InitDescriptorSets();
}

VkImageMemoryBarrier TransparentApp::_NewImageBarreir(const ImageBarrierInformation& _info) const
{
	VkImageMemoryBarrier barrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
	auto viewInfo = _info.pImageView->GetImageViewInformation();
	barrier.oldLayout = _info.oldLayout;
	barrier.newLayout = _info.newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = _info.pImageView->pImage->vkImage;
	barrier.subresourceRange.aspectMask = viewInfo.aspectMask;
	barrier.subresourceRange.baseMipLevel = viewInfo.baseMipLevel;
	barrier.subresourceRange.levelCount = viewInfo.levelCount;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.srcAccessMask = _info.srcAccessMask;
	barrier.dstAccessMask = _info.dstAccessMask;
	return barrier;
}

void TransparentApp::_ReadObjFile(const std::string& objFile, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices) const
{
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warn, err;
	CHECK_TRUE(tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, objFile.c_str()), warn + err);
	std::unordered_map<Vertex, uint32_t> uniqueVertices{};
	for (const auto& shape : shapes)
	{
		for (const auto& index : shape.mesh.indices)
		{
			Vertex vertex{};
			vertex.pos = {
				attrib.vertices[3 * index.vertex_index + 0],
				attrib.vertices[3 * index.vertex_index + 1],
				attrib.vertices[3 * index.vertex_index + 2]
			};
			vertex.uv = {
				attrib.texcoords[2 * index.texcoord_index + 0],
				1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
			};
			vertex.normal = {
				attrib.normals[3 * index.normal_index + 0],
				attrib.normals[3 * index.normal_index + 1],
				attrib.normals[3 * index.normal_index + 2],
			};
			if (uniqueVertices.count(vertex) == 0)
			{
				uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
				vertices.push_back(vertex);
			}
			indices.push_back(uniqueVertices[vertex]);
		}
	}
}

void TransparentApp::Run()
{
	_Init();
	_MainLoop();
	_Uninit();
}