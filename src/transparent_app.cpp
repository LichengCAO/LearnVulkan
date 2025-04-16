#include "transparent_app.h"
#include "device.h"
#include "tiny_obj_loader.h"
#include <random>
#define MAX_FRAME_COUNT 3
#include "gaussian_blur.h"
void TransparentApp::_Init()
{
	MyDevice::GetInstance().Init();
	Model room{};
	Model wahoo{};
	Model sphere{};
	Model bunny{};
	Model chessBoard{};
	room.objFilePath = "E:/GitStorage/LearnVulkan/res/models/viking_room/viking_room.obj";
	room.texturePath = "E:/GitStorage/LearnVulkan/res/models/viking_room/viking_room.png";
	wahoo.objFilePath = "E:/GitStorage/LearnVulkan/res/models/wahoo/wahoo.obj";
	wahoo.texturePath = "E:/GitStorage/LearnVulkan/res/models/wahoo/wahoo.bmp";
	sphere.objFilePath = "E:/GitStorage/LearnVulkan/res/models/sphere/sphere.obj";
	sphere.texturePath = "E:/GitStorage/LearnVulkan/res/models/sphere/sphere.png";
	bunny.objFilePath = "E:/GitStorage/LearnVulkan/res/models/bunny/bunny.obj";
	bunny.texturePath = "E:/GitStorage/LearnVulkan/res/models/bunny/bunny.png";
	chessBoard.objFilePath = "E:/GitStorage/LearnVulkan/res/models/ChessBoard/ChessBoard.obj";
	chessBoard.texturePath = "E:/GitStorage/LearnVulkan/res/models/ChessBoard/ChessBoard.JPG";
	//chessBoard.texturePath = "E:/GitStorage/LearnVulkan/res/models/viking_room/viking_room.png";
	wahoo.transform.SetScale({ 0.05, 0.05, 0.05 });
	wahoo.transform.SetRotation({ 90, 0, 90 });
	wahoo.transform.SetPosition({ 0.5, 0.0, 0.2 });
	chessBoard.transform.SetRotation({ 0, 90, 90 });
	chessBoard.transform.SetScale({ 0.06, 0.06, 0.06 });
	room.transform.SetScale({ 1.5, 1.5, 1.5 });
	m_models = { room };
	m_transModels.clear(); 
	m_transMaterials.clear();

	std::default_random_engine            rnd(3625);  // Fixed seed
	std::uniform_real_distribution<float> uniformDist;
	SimpleMaterial material{};

	for (int i = 0; i < 6; ++i)
	{
		bunny.transform.SetRotation({ -90, 180, -90 });
		bunny.transform.SetScale({ 0.12, 0.12, 0.12 });
		bunny.transform.SetPosition({ 0.5, -0.4 + i * 0.2, 0.5 });
		material.roughness = (1.0f / 6.0f) * i;
		m_transModels.push_back(bunny);
		m_transMaterials.push_back(material);
	}
	for (int i = 0; i < 6; ++i)
	{
		bunny.transform.SetRotation({ -90, 180, -90 + 60 * i });
		bunny.transform.SetScale({ 0.12, 0.12, 0.12 });
		bunny.transform.SetPosition({ 0.5, -0.4 + i * 0.2, 0.25 });
		material.roughness = 0.25;
		m_transModels.push_back(bunny);
		m_transMaterials.push_back(material);
	}

	_InitRenderPass();
	_InitDescriptorSetLayouts();

	_InitBuffers();
	_InitImagesAndViews();
	_InitFramebuffers();
	_InitSampler();
	
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

		AttachmentInformation depthInfo = AttachmentInformation::GetPresetInformation(AttachmentPreset::DEPTH);
		depthInfo.attachmentDescription.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
		depthInfo.attachmentDescription.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
		depthInfo.attachmentDescription.loadOp = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_LOAD;
		depthInfo.attachmentDescription.storeOp = VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_DONT_CARE;

		m_distortRenderPass.AddAttachment(uvDistortInfo);
		m_distortRenderPass.AddAttachment(depthInfo);
		SubpassInformation distortSubpassInfo{};
		distortSubpassInfo.AddColorAttachment(0);
		distortSubpassInfo.SetDepthStencilAttachment(1, true);
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
		depthInfo.attachmentDescription.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
		depthInfo.attachmentDescription.storeOp = VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_STORE;
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

	// light pass
	{
		AttachmentInformation lightInfo = AttachmentInformation::GetPresetInformation(AttachmentPreset::GBUFFER_ALBEDO);
		m_lightRenderPass.AddAttachment(lightInfo);
		SubpassInformation subpassInfo{};
		subpassInfo.AddColorAttachment(0);
		m_lightRenderPass.AddSubpass(subpassInfo);
		m_lightRenderPass.Init();
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
	m_lightRenderPass.Uninit();
	m_gbufferRenderPass.Uninit();
	m_distortRenderPass.Uninit();
	m_oitRenderPass.Uninit();
}

void TransparentApp::_InitDescriptorSetLayouts()
{
	// OIT front device write read
	{
		m_oitSampleDSetLayout.AddBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT); // sample image
		m_oitSampleDSetLayout.AddBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT); // sample count
		m_oitSampleDSetLayout.AddBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_FRAGMENT_BIT); // in use
		m_oitSampleDSetLayout.AddBinding(1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT); // viewport
		m_oitSampleDSetLayout.Init();
	}

	// OIT sorting device write
	{
		m_oitColorDSetLayout.AddBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT);
		m_oitColorDSetLayout.Init();
	}

	// model related host write device read
	{
		m_modelDSetLayout.AddBinding(1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT);
		m_modelDSetLayout.AddBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
		m_modelDSetLayout.Init();
	}

	//camera host write device read
	{
		m_cameraDSetLayout.AddBinding(1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
		m_cameraDSetLayout.Init();
	}

	// gbuffer device read
	{
		m_gbufferDSetLayout.AddBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT); // albedo
		m_gbufferDSetLayout.AddBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT); // position
		m_gbufferDSetLayout.AddBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT); // normal
		m_gbufferDSetLayout.AddBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT); // depth
		m_gbufferDSetLayout.Init();
	}

	// distort host write device read
	{
		m_distortDSetLayout.AddBinding(1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT); // camera view information
		m_distortDSetLayout.Init();
	}

	// transparent device read
	{
		m_transOutputDSetLayout.AddBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT); // oit
		m_transOutputDSetLayout.AddBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT); // distort uv
		m_transOutputDSetLayout.Init();
	}

	// material host write device read
	{
		m_materialDSetLayout.AddBinding(1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT);
		m_materialDSetLayout.Init();
	}

	// blur device write and read
	{
		m_blurDSetLayout.AddBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT); // read only image
		m_blurDSetLayout.AddBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT); // write only image
		m_blurDSetLayout.AddBinding(1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT); // viewport information
		m_blurDSetLayout.AddBinding(1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT); // blur information
		m_blurDSetLayout.AddBinding(1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT); // kernel
		m_blurDSetLayout.Init();
	}

	// blur output device read
	{
		m_blurOutputDSetLayout.AddBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
		m_blurOutputDSetLayout.Init();
	}
}
void TransparentApp::_UninitDescriptorSetLayouts()
{
	m_blurOutputDSetLayout.Uninit();
	m_blurDSetLayout.Uninit();
	m_materialDSetLayout.Uninit();
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
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
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
	samplerInfo.maxLod = static_cast<float>(m_mipLevel);
	VK_CHECK(vkCreateSampler(MyDevice::GetInstance().vkDevice, &samplerInfo, nullptr, &m_vkLodSampler), "Failed to create app lod sampler!");
}
void TransparentApp::_UninitSampler()
{
	vkDestroySampler(MyDevice::GetInstance().vkDevice, m_vkSampler, nullptr);
	vkDestroySampler(MyDevice::GetInstance().vkDevice, m_vkLodSampler, nullptr);
	m_vkSampler = VK_NULL_HANDLE;
	m_vkLodSampler = VK_NULL_HANDLE;
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

	// material
	{
		BufferInformation materialBufferInfo;
		materialBufferInfo.size = sizeof(SimpleMaterial);
		materialBufferInfo.usage = VkBufferUsageFlagBits::VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
		materialBufferInfo.memoryProperty = VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
		m_vecMaterialBuffers.reserve(MAX_FRAME_COUNT);
		for (int i = 0; i < MAX_FRAME_COUNT; ++i)
		{
			int n = m_transModels.size();
			m_vecMaterialBuffers.push_back(std::vector<Buffer>{});
			m_vecMaterialBuffers[i].reserve(n);
			for (int j = 0; j < n; ++j)
			{
				m_vecMaterialBuffers[i].push_back(Buffer{});
				m_vecMaterialBuffers[i].back().Init(materialBufferInfo);
			}
		}
	}

	// camera
	{
		BufferInformation bufferInfo;
		bufferInfo.size = sizeof(CameraUBO);
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

	// blur
	{
		BufferInformation bufferInfo;
		bufferInfo.size = sizeof(GaussianBlurInformation);
		bufferInfo.usage = VkBufferUsageFlagBits::VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		bufferInfo.memoryProperty = VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;

		BufferInformation kernelBufferInfo;
		kernelBufferInfo.size = (m_blurRadius + 1 + 15) / 16 * sizeof(GaussianKernels);
		kernelBufferInfo.usage = VkBufferUsageFlagBits::VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
		kernelBufferInfo.memoryProperty = VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;

		m_blurBuffers.reserve(MAX_FRAME_COUNT);
		m_kernelBuffers.reserve(MAX_FRAME_COUNT);
		for (int i = 0; i < MAX_FRAME_COUNT; ++i)
		{
			m_blurBuffers.push_back(Buffer{});
			m_blurBuffers[i].Init(bufferInfo);
			m_kernelBuffers.push_back(Buffer{});
			m_kernelBuffers[i].Init(kernelBufferInfo);
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

	// material
	for (auto& buffers : m_vecMaterialBuffers)
	{
		for (auto& buffer : buffers)
		{
			buffer.Uninit();
		}
		buffers.clear();
	}
	m_vecMaterialBuffers.clear();

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

	// blur
	for (auto& buffer : m_blurBuffers)
	{
		buffer.Uninit();
	}
	m_blurBuffers.clear();
	for (auto& buffer : m_kernelBuffers)
	{
		buffer.Uninit();
	}
	m_kernelBuffers.clear();
}

void TransparentApp::_InitImagesAndViews()
{
	// prevent implicit copy
	m_swapchainImages = MyDevice::GetInstance().GetSwapchainImages(); // no need to call Init() here, swapchain images are special
	uint32_t width = MyDevice::GetInstance().GetSwapchainExtent().width;
	uint32_t height = MyDevice::GetInstance().GetSwapchainExtent().height;
	
	auto n = MAX_FRAME_COUNT;
	{
		m_swapchainImageViews.reserve(m_swapchainImages.size());
		m_depthImages.reserve(n);
		m_depthImageViews.reserve(n);
		m_gbufferPosImages.reserve(n);
		m_gbufferPosImageViews.reserve(n);
		m_gbufferNormalImages.reserve(n);
		m_gbufferNormalImageViews.reserve(n);
		m_gbufferAlbedoImages.reserve(n);
		m_gbufferAlbedoImageViews.reserve(n);
		m_gbufferReadAlbedoImageViews.reserve(n);
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
		m_lightImages.reserve(n);
		m_lightImageLayerViews.reserve(n);
		m_lightImageBlurViews.reserve(n);
	}
	
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
		m_mipLevel = static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;
		ImageInformation gbufferAlbedoImageInfo;
		gbufferAlbedoImageInfo.mipLevels = m_mipLevel;
		gbufferAlbedoImageInfo.width = width;
		gbufferAlbedoImageInfo.height = height;
		gbufferAlbedoImageInfo.format = VkFormat::VK_FORMAT_R32G32B32A32_SFLOAT;
		gbufferAlbedoImageInfo.memoryProperty = VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		gbufferAlbedoImageInfo.usage = VkImageUsageFlagBits::VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VkImageUsageFlagBits::VK_IMAGE_USAGE_SAMPLED_BIT
			| VkImageUsageFlagBits::VK_IMAGE_USAGE_TRANSFER_DST_BIT | VkImageUsageFlagBits::VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		ImageViewInformation gbufferAlbedoImageViewInfo;
		gbufferAlbedoImageViewInfo.aspectMask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT;
		gbufferAlbedoImageViewInfo.baseMipLevel = 0;// only view the 0 level 
		gbufferAlbedoImageViewInfo.levelCount = 1;
		ImageViewInformation gbufferReadAlbedoImageViewInfo;
		gbufferReadAlbedoImageViewInfo = gbufferAlbedoImageViewInfo;
		gbufferReadAlbedoImageViewInfo.levelCount = m_mipLevel;

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
		gbufferDepthImageInfo.format = VkFormat::VK_FORMAT_R32G32B32A32_SFLOAT;
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
			&m_gbufferDepthImageViews,
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
		for (int i = 0; i < n; ++i)
		{
			m_gbufferReadAlbedoImageViews.push_back(m_gbufferAlbedoImages[i].NewImageView(gbufferReadAlbedoImageViewInfo));
			m_gbufferReadAlbedoImageViews[i].Init();
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

	// light image and view for post rendering and blur
	{
		ImageInformation lightImageInfo{};
		lightImageInfo.format = VkFormat::VK_FORMAT_R32G32B32A32_SFLOAT;
		lightImageInfo.width = width;
		lightImageInfo.height = height;
		lightImageInfo.usage = 
			VkImageUsageFlagBits::VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | 
			VkImageUsageFlagBits::VK_IMAGE_USAGE_SAMPLED_BIT | 
			VkImageUsageFlagBits::VK_IMAGE_USAGE_STORAGE_BIT;
		lightImageInfo.memoryProperty = VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		lightImageInfo.arrayLayers = m_blurLayers + 1;
		
		ImageViewInformation layerViewInfo{};
		layerViewInfo.aspectMask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT;
		layerViewInfo.layerCount = 1;

		for (int i = 0; i < n; ++i)
		{
			m_lightImages.push_back(Image{});
			m_lightImages[i].SetImageInformation(lightImageInfo);
			m_lightImages[i].Init();
			
			m_lightImageLayerViews.push_back({});
			m_lightImageLayerViews[i].reserve(m_blurLayers + 1); // one extra layer to store xpass result
			for (int j = 0; j <= m_blurLayers; ++j)
			{
				layerViewInfo.baseArrayLayer = static_cast<uint32_t>(j);
				m_lightImageLayerViews[i].push_back(m_lightImages[i].NewImageView(layerViewInfo));
				m_lightImageLayerViews[i][j].Init();
			}
		}
	}

	// view for blurred image array
	{
		ImageViewInformation info{};
		info.baseArrayLayer = 0;
		info.layerCount = m_blurLayers;
		for (int i = 0; i < n; ++i)
		{
			m_lightImageBlurViews.push_back(m_lightImages[i].NewImageView(info));
			m_lightImageBlurViews[i].Init();
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
		&m_gbufferReadAlbedoImageViews,
		&m_lightImageBlurViews,
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
		&m_distortImages,
		&m_lightImages
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
	for (auto& imageViews : m_lightImageLayerViews)
	{
		for (auto& imageView : imageViews)
		{
			imageView.Uninit();
		}
		imageViews.clear();
	}
	m_lightImageLayerViews.clear();
}

void TransparentApp::_InitFramebuffers()
{
	auto n = MAX_FRAME_COUNT;
	m_framebuffers.reserve(m_swapchainImages.size());
	m_gbufferFramebuffers.reserve(n);
	m_oitFramebuffers.reserve(n);
	m_distortFramebuffers.reserve(n);
	m_lightFramebuffers.reserve(n);
	
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
			&m_depthImageViews[i]
		};
		m_distortFramebuffers.push_back(m_distortRenderPass.NewFramebuffer(distortViews));
		m_distortFramebuffers[i].Init();

		std::vector<const ImageView*> lightViews =
		{
			&m_lightImageLayerViews[i][0],
		};
		m_lightFramebuffers.push_back(m_lightRenderPass.NewFramebuffer(lightViews));
		m_lightFramebuffers[i].Init();
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
		&m_distortFramebuffers,
		&m_lightFramebuffers
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
			m_oitDSets[i].StartUpdate();
			m_oitDSets[i].UpdateBinding(0, &m_oitSampleTexelBufferViews[i]);
			m_oitDSets[i].UpdateBinding(1, sampleCountImageInfo);
			m_oitDSets[i].UpdateBinding(2, inUseImageInfo);
			m_oitDSets[i].UpdateBinding(3, &m_oitViewportBuffer);
			m_oitDSets[i].FinishUpdate();
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
			m_oitColorDSets[i].StartUpdate();
			m_oitColorDSets[i].UpdateBinding(0, outputImageInfo);
			m_oitColorDSets[i].FinishUpdate();
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
			m_distortDSets[i].StartUpdate();
			m_distortDSets[i].UpdateBinding(0, &m_distortBuffers[i]);
			m_distortDSets[i].FinishUpdate();
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
				m_vecModelDSets[i][j].StartUpdate();
				m_vecModelDSets[i][j].UpdateBinding(0, &m_vecModelBuffers[i][j]);
				m_vecModelDSets[i][j].UpdateBinding(1, m_modelTextures[j].GetVkDescriptorImageInfo());
				m_vecModelDSets[i][j].FinishUpdate();
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
				m_vecTransModelDSets[i][j].StartUpdate();
				m_vecTransModelDSets[i][j].UpdateBinding(0, &m_vecTransModelBuffers[i][j]);
				m_vecTransModelDSets[i][j].FinishUpdate();
			}
		}
	}

	// material descriptor set
	{
		m_vecMaterialDSets.reserve(MAX_FRAME_COUNT);
		for (int i = 0; i < MAX_FRAME_COUNT; ++i)
		{
			int n = m_transModels.size();
			m_vecMaterialDSets.push_back(std::vector<DescriptorSet>{});
			m_vecMaterialDSets[i].reserve(n);
			for (int j = 0; j < n; ++j)
			{
				m_vecMaterialDSets[i].push_back(DescriptorSet{});
				m_vecMaterialDSets[i][j].SetLayout(&m_materialDSetLayout);
				m_vecMaterialDSets[i][j].Init();
				m_vecMaterialDSets[i][j].StartUpdate();
				m_vecMaterialDSets[i][j].UpdateBinding(0, &m_vecMaterialBuffers[i][j]);
				m_vecMaterialDSets[i][j].FinishUpdate();
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
			m_cameraDSets.back().StartUpdate();
			m_cameraDSets.back().UpdateBinding(0, &m_cameraBuffers[i]);
			m_cameraDSets.back().FinishUpdate();
		}
	}

	// gbuffer descriptor set
	{
		// Setup descriptor sets 
		for (int i = 0; i < MAX_FRAME_COUNT; ++i)
		{
			VkDescriptorImageInfo imageInfo0;
			imageInfo0.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageInfo0.imageView = m_gbufferReadAlbedoImageViews[i].vkImageView;
			imageInfo0.sampler = m_vkLodSampler;

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

			//VkDescriptorImageInfo imageInfo4;
			//imageInfo4.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			//imageInfo4.imageView = m_depthImageViews[i].vkImageView;
			//imageInfo4.sampler = m_vkSampler;

			m_gbufferDSets.push_back(DescriptorSet{});
			m_gbufferDSets.back().SetLayout(&m_gbufferDSetLayout);
			m_gbufferDSets.back().Init();
			m_gbufferDSets.back().StartUpdate();
			m_gbufferDSets.back().UpdateBinding(0, imageInfo0);
			m_gbufferDSets.back().UpdateBinding(1, imageInfo1);
			m_gbufferDSets.back().UpdateBinding(2, imageInfo2);
			m_gbufferDSets.back().UpdateBinding(3, imageInfo3);
			m_gbufferDSets.back().FinishUpdate();
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
			m_transOutputDSets[i].StartUpdate();
			m_transOutputDSets[i].UpdateBinding(0, oitOutputImageInfo);
			m_transOutputDSets[i].UpdateBinding(1, distortOutputImageInfo);
			m_transOutputDSets[i].FinishUpdate();
		}
	}

	// blur descriptor set
	{
		m_blurLayeredDSetsX.reserve(MAX_FRAME_COUNT);
		m_blurLayeredDSetsY.reserve(MAX_FRAME_COUNT);
		for (int i = 0; i < MAX_FRAME_COUNT; ++i)
		{
			std::vector<VkDescriptorBufferInfo> kernelInfos{};
			int kernelElementCnt = m_blurRadius + 1;
			int maxJ = (kernelElementCnt + 15) / 16;
			kernelInfos.reserve(maxJ);
			for (int j = 0; j < maxJ; ++j)
			{
				VkDescriptorBufferInfo info{};
				info.buffer = m_kernelBuffers[i].vkBuffer;
				info.offset = j * sizeof(float) * 16; // min offset is size of mat4
				info.range = (j == (maxJ - 1)) ? (kernelElementCnt % 16) * sizeof(float) : 16 * sizeof(float);
				kernelInfos.push_back(info);
			}

			m_blurLayeredDSetsX.push_back({});
			m_blurLayeredDSetsY.push_back({});
			m_blurLayeredDSetsX[i].reserve(m_blurLayers - 1);
			m_blurLayeredDSetsY[i].reserve(m_blurLayers - 1);
			for (int j = 0; j < m_blurLayers - 1; ++j)
			{
				VkDescriptorImageInfo blurInputImageInfoX{};
				blurInputImageInfoX.sampler = m_vkSampler;
				blurInputImageInfoX.imageView = m_lightImageLayerViews[i][j].vkImageView;
				blurInputImageInfoX.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

				VkDescriptorImageInfo blurOutputImageInfoX{};
				blurOutputImageInfoX.sampler = m_vkSampler;
				blurOutputImageInfoX.imageView = m_lightImageLayerViews[i][m_blurLayers].vkImageView;
				blurOutputImageInfoX.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

				m_blurLayeredDSetsX[i].push_back(DescriptorSet{});
				m_blurLayeredDSetsX[i][j].SetLayout(&m_blurDSetLayout);
				m_blurLayeredDSetsX[i][j].Init();
				m_blurLayeredDSetsX[i][j].StartUpdate();
				m_blurLayeredDSetsX[i][j].UpdateBinding(0, blurInputImageInfoX);
				m_blurLayeredDSetsX[i][j].UpdateBinding(1, blurOutputImageInfoX);
				m_blurLayeredDSetsX[i][j].UpdateBinding(2, &m_oitViewportBuffer);
				m_blurLayeredDSetsX[i][j].UpdateBinding(3, &m_blurBuffers[i]);
				m_blurLayeredDSetsX[i][j].UpdateBinding(4, kernelInfos);
				m_blurLayeredDSetsX[i][j].FinishUpdate();

				VkDescriptorImageInfo blurInputImageInfoY{};
				blurInputImageInfoY.sampler = m_vkSampler;
				blurInputImageInfoY.imageView = m_lightImageLayerViews[i][m_blurLayers].vkImageView;
				blurInputImageInfoY.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

				VkDescriptorImageInfo blurOutputImageInfoY{};
				blurOutputImageInfoY.sampler = m_vkSampler;
				blurOutputImageInfoY.imageView = m_lightImageLayerViews[i][j + 1].vkImageView;
				blurOutputImageInfoY.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

				m_blurLayeredDSetsY[i].push_back(DescriptorSet{});
				m_blurLayeredDSetsY[i][j].SetLayout(&m_blurDSetLayout);
				m_blurLayeredDSetsY[i][j].Init();
				m_blurLayeredDSetsY[i][j].StartUpdate();
				m_blurLayeredDSetsY[i][j].UpdateBinding(0, blurInputImageInfoY);
				m_blurLayeredDSetsY[i][j].UpdateBinding(1, blurOutputImageInfoY);
				m_blurLayeredDSetsY[i][j].UpdateBinding(2, &m_oitViewportBuffer);
				m_blurLayeredDSetsY[i][j].UpdateBinding(3, &m_blurBuffers[i]);
				m_blurLayeredDSetsY[i][j].UpdateBinding(4, kernelInfos);
				m_blurLayeredDSetsY[i][j].FinishUpdate();
			}

		}
	}

	// blur output descriptor set
	{
		m_blurOutputDSets.reserve(MAX_FRAME_COUNT);
		for (int i = 0; i < MAX_FRAME_COUNT; ++i)
		{
			VkDescriptorImageInfo blurOutputImageInfo{};
			blurOutputImageInfo.sampler = m_vkSampler;
			blurOutputImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			blurOutputImageInfo.imageView = m_lightImageBlurViews[i].vkImageView;
			m_blurOutputDSets.push_back(DescriptorSet{});
			m_blurOutputDSets[i].SetLayout(&m_blurOutputDSetLayout);
			m_blurOutputDSets[i].Init();
			m_blurOutputDSets[i].StartUpdate();
			m_blurOutputDSets[i].UpdateBinding(0, blurOutputImageInfo);
			m_blurOutputDSets[i].FinishUpdate();
		}
	}
}
void TransparentApp::_UninitDescriptorSets()
{
	m_blurOutputDSets.clear();
	for (auto& dsets : m_blurLayeredDSetsX)
	{
		dsets.clear();
	}
	m_blurLayeredDSetsX.clear();
	for (auto& dsets : m_blurLayeredDSetsY)
	{
		dsets.clear();
	}
	m_blurLayeredDSetsY.clear();
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
	for (auto& dsets : m_vecMaterialDSets)
	{
		dsets.clear();
	}
	m_vecMaterialDSets.clear();
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
			color.z *= color.z;
			color.a = 0.05f;
			color = glm::vec4(glm::vec3(0.0f, 0.0f, 1.0f), color.a);
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
	m_oitPipeline.AddShader(oitFrontFragShader.GetShaderStageInfo());
	m_oitPipeline.AddShader(oitFrontVertShader.GetShaderStageInfo());
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
	m_oitSortPipeline.AddShader(oitSortShader.GetShaderStageInfo());
	m_oitSortPipeline.Init();

	oitSortShader.Uninit();

	SimpleShader distortVertShader;
	SimpleShader distortFragShader;
	distortVertShader.SetSPVFile("E:/GitStorage/LearnVulkan/bin/shaders/distort.vert.spv");
	distortFragShader.SetSPVFile("E:/GitStorage/LearnVulkan/bin/shaders/distort.frag.spv");
	distortVertShader.Init();
	distortFragShader.Init();

	m_distortPipeline.AddDescriptorSetLayout(&m_cameraDSetLayout);
	m_distortPipeline.AddDescriptorSetLayout(&m_modelDSetLayout);
	m_distortPipeline.AddDescriptorSetLayout(&m_distortDSetLayout);
	m_distortPipeline.AddDescriptorSetLayout(&m_gbufferDSetLayout);
	m_distortPipeline.AddDescriptorSetLayout(&m_materialDSetLayout); // to read from material
	m_distortPipeline.AddShader(distortVertShader.GetShaderStageInfo());
	m_distortPipeline.AddShader(distortFragShader.GetShaderStageInfo());
	m_distortPipeline.BindToSubpass(&m_distortRenderPass, 0);
	m_distortPipeline.SetColorAttachmentAsAdd(0);
	m_distortPipeline.AddVertexInputLayout(&m_transModelVertLayout);
	m_distortPipeline.Init();

	distortVertShader.Uninit();
	distortFragShader.Uninit();

	SimpleShader gbufferVertShader;
	SimpleShader gbufferFragShader;
	gbufferVertShader.SetSPVFile("E:/GitStorage/LearnVulkan/bin/shaders/gbuffer.vert.spv");
	gbufferFragShader.SetSPVFile("E:/GitStorage/LearnVulkan/bin/shaders/gbuffer.frag.spv");
	gbufferVertShader.Init();
	gbufferFragShader.Init();

	m_gbufferPipeline.AddDescriptorSetLayout(&m_cameraDSetLayout);
	m_gbufferPipeline.AddDescriptorSetLayout(&m_modelDSetLayout);
	m_gbufferPipeline.AddShader(gbufferVertShader.GetShaderStageInfo());
	m_gbufferPipeline.AddShader(gbufferFragShader.GetShaderStageInfo());
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

	//m_gPipeline.AddDescriptorSetLayout(&m_gbufferDSetLayout);
	m_gPipeline.AddDescriptorSetLayout(&m_blurOutputDSetLayout);
	m_gPipeline.AddDescriptorSetLayout(&m_transOutputDSetLayout);
	m_gPipeline.AddShader(vertShader.GetShaderStageInfo());
	m_gPipeline.AddShader(fragShader.GetShaderStageInfo());
	m_gPipeline.BindToSubpass(&m_renderPass, 0);
	m_gPipeline.AddVertexInputLayout(&m_quadVertLayout);
	m_gPipeline.Init();

	vertShader.Uninit();
	fragShader.Uninit();

	SimpleShader lightVertShader;
	SimpleShader lightFragShader;
	lightVertShader.SetSPVFile("E:/GitStorage/LearnVulkan/bin/shaders/light.vert.spv");
	lightFragShader.SetSPVFile("E:/GitStorage/LearnVulkan/bin/shaders/light.frag.spv");
	lightVertShader.Init();
	lightFragShader.Init();

	m_lightPipeline.AddDescriptorSetLayout(&m_gbufferDSetLayout);
	m_lightPipeline.AddShader(lightVertShader.GetShaderStageInfo());
	m_lightPipeline.AddShader(lightFragShader.GetShaderStageInfo());
	m_lightPipeline.BindToSubpass(&m_lightRenderPass, 0);
	m_lightPipeline.AddVertexInputLayout(&m_quadVertLayout);
	m_lightPipeline.Init();

	lightVertShader.Uninit();
	lightFragShader.Uninit();

	SimpleShader blurCompShader;
	blurCompShader.SetSPVFile("E:/GitStorage/LearnVulkan/bin/shaders/gaussian.comp.spv");
	blurCompShader.Init();
	m_blurPipelineX.AddDescriptorSetLayout(&m_blurDSetLayout);
	m_blurPipelineX.AddShader(blurCompShader.GetShaderStageInfo());
	m_blurPipelineX.Init();
	blurCompShader.Uninit();

	SimpleShader blurCompShader2;
	blurCompShader2.SetSPVFile("E:/GitStorage/LearnVulkan/bin/shaders/gaussian2.comp.spv");
	blurCompShader2.Init();
	m_blurPipelineY.AddDescriptorSetLayout(&m_blurDSetLayout);
	m_blurPipelineY.AddShader(blurCompShader2.GetShaderStageInfo());
	m_blurPipelineY.Init();
	blurCompShader2.Uninit();
}
void TransparentApp::_UninitPipelines()
{
	m_blurPipelineY.Uninit();
	m_blurPipelineX.Uninit();
	m_lightPipeline.Uninit();
	m_gPipeline.Uninit();
	m_gbufferPipeline.Uninit();
	m_distortPipeline.Uninit();
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

	CameraUBO ubo{};
	VkExtent2D swapchainExtent = MyDevice::GetInstance().GetSwapchainExtent();
	ubo.proj = m_camera.GetProjectionMatrix();
	ubo.proj[1][1] *= -1;
	ubo.view = m_camera.GetViewMatrix();
	ubo.eye = glm::vec4(m_camera.eye, 1.0f);
	m_cameraBuffers[m_currentFrame].CopyFromHost(&ubo);

	//struct ModelTransform
	//{
	//	alignas(16) glm::mat4 model;
	//	alignas(16) glm::mat4 modelInvTranspose;
	//};
	for (int i = 0; i < m_models.size(); ++i)
	{
		ModelTransform modelTransform{};
		modelTransform.model = m_models[i].transform.GetModelMatrix();
		modelTransform.modelInvTranspose = m_models[i].transform.GetModelInverseTransposeMatrix();
		m_vecModelBuffers[m_currentFrame][i].CopyFromHost(&modelTransform);
	}
	int len = m_transModels.size();
	for (int i = 0; i < m_transModels.size(); ++i)
	{
		ModelTransform modelTransform{};
		modelTransform.model = m_transModels[i].transform.GetModelMatrix();
		modelTransform.modelInvTranspose = m_transModels[i].transform.GetModelInverseTransposeMatrix();
		m_vecTransModelBuffers[m_currentFrame][i].CopyFromHost(&modelTransform);
		m_vecMaterialBuffers[m_currentFrame][i].CopyFromHost(&m_transMaterials[i]);
	}

	//struct CameraViewInformation
	//{
	//	alignas(16) glm::mat4 normalView; // inverse transpose of view matrix
	//	alignas(4)  float invTanHalfFOV;
	//	alignas(4)  float screenRatioXY;
	//};
	CameraViewInformation cameraViewInfo{};
	cameraViewInfo.normalView = m_camera.GetInverseTransposeViewMatrix();
	cameraViewInfo.invTanHalfFOV = m_camera.GetInverseTangentHalfFOVy();
	cameraViewInfo.screenRatioXY = m_camera.aspect;
	m_distortBuffers[m_currentFrame].CopyFromHost(&cameraViewInfo);

	std::vector<float> kernels = Get1DGaussian(m_blurRadius);
	m_kernelBuffers[m_currentFrame].CopyFromHost(kernels.data(), kernels.size() * sizeof(float));

	GaussianBlurInformation blurInfo{};
	blurInfo.blurRad = m_blurRadius;
	m_blurBuffers[m_currentFrame].CopyFromHost(&blurInfo);
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
	waitInfo.waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	cmd.StartCommands({ waitInfo });
	
	// draw opaque objects
	cmd.StartRenderPass(&m_gbufferRenderPass, &m_gbufferFramebuffers[m_currentFrame]);
	for (int i = 0; i < m_gbufferVertBuffers.size(); ++i)
	{
		GraphicsVertexPipelineInput input;
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

	// clean oit storage images and texel buffers -> DONE
	{
		// wait the oit image buffers transfer back to general first
		ImageBarrierBuilder barrierBuilder{};
		VkImageMemoryBarrier sampleCountBarrier = barrierBuilder.NewBarrier(
			m_oitSampleCountImages[m_currentFrame].vkImage,
			//VK_IMAGE_LAYOUT_UNDEFINED,
			_GetImageLayout(&m_oitSampleCountImageViews[m_currentFrame]),
			VK_IMAGE_LAYOUT_GENERAL,
			VK_ACCESS_NONE, VK_ACCESS_TRANSFER_WRITE_BIT
		);
		VkImageMemoryBarrier inUseBarrier = barrierBuilder.NewBarrier(
			m_oitInUseImages[m_currentFrame].vkImage,
			//VK_IMAGE_LAYOUT_UNDEFINED,
			_GetImageLayout(&m_oitInUseImageViews[m_currentFrame]),
			VK_IMAGE_LAYOUT_GENERAL,
			VK_ACCESS_NONE, VK_ACCESS_TRANSFER_WRITE_BIT
		);

		cmd.AddPipelineBarrier(
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			{ sampleCountBarrier, inUseBarrier }
		);

		// clean
		VkClearColorValue clearColor32Ruint{ .uint32 = {0u} };
		cmd.FillImageView(&m_oitSampleCountImageViews[m_currentFrame], clearColor32Ruint);
		cmd.FillImageView(&m_oitInUseImageViews[m_currentFrame], clearColor32Ruint);
		cmd.FillBuffer(&m_oitSampleTexelBuffers[m_currentFrame], 0);

		// wait clean
		VkMemoryBarrier sampleDataBarrier{ VK_STRUCTURE_TYPE_MEMORY_BARRIER };
		sampleDataBarrier.srcAccessMask = VkAccessFlagBits::VK_ACCESS_TRANSFER_WRITE_BIT;
		sampleDataBarrier.dstAccessMask = VkAccessFlagBits::VK_ACCESS_SHADER_READ_BIT | VkAccessFlagBits::VK_ACCESS_SHADER_WRITE_BIT;
		VkImageMemoryBarrier sampleCountBarrier2 = barrierBuilder.NewBarrier(
			m_oitSampleCountImages[m_currentFrame].vkImage,
			_GetImageLayout(&m_oitSampleCountImageViews[m_currentFrame]),
			VK_IMAGE_LAYOUT_GENERAL,
			VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT
		);
		VkImageMemoryBarrier inUseBarrier2 = barrierBuilder.NewBarrier(
			m_oitInUseImages[m_currentFrame].vkImage,
			_GetImageLayout(&m_oitInUseImageViews[m_currentFrame]),
			VK_IMAGE_LAYOUT_GENERAL,
			VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT
		);

		cmd.AddPipelineBarrier(
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			{ sampleDataBarrier },
			{ sampleCountBarrier2, inUseBarrier2 }
		);
	}

	// wait for previous depth draw, gbuffer draw done
	{	
		ImageBarrierBuilder barrierBuilder{};
		VkImageMemoryBarrier gbufferPosBarrier = barrierBuilder.NewBarrier(
			m_gbufferPosImages[m_currentFrame].vkImage,
			_GetImageLayout(&m_gbufferPosImageViews[m_currentFrame]), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT
		);
		VkImageMemoryBarrier gbufferNormalBarrier = barrierBuilder.NewBarrier(
			m_gbufferNormalImages[m_currentFrame].vkImage,
			_GetImageLayout(&m_gbufferNormalImageViews[m_currentFrame]), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT
		);
		VkImageMemoryBarrier gbufferAlbedoBarrier = barrierBuilder.NewBarrier(
			m_gbufferAlbedoImages[m_currentFrame].vkImage,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT
		);
		VkImageMemoryBarrier gbufferDepthBarrier = barrierBuilder.NewBarrier(
			m_gbufferDepthImages[m_currentFrame].vkImage,
			_GetImageLayout(&m_gbufferDepthImageViews[m_currentFrame]), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT
		);
		barrierBuilder.SetAspect(VK_IMAGE_ASPECT_DEPTH_BIT);
		VkImageMemoryBarrier depthBarrier = barrierBuilder.NewBarrier(
			m_depthImages[m_currentFrame].vkImage,
			_GetImageLayout(&m_depthImageViews[m_currentFrame]), VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
			VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT
		);

		cmd.AddPipelineBarrier(
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			{ depthBarrier, gbufferDepthBarrier }
		);
	}

	// draw transparent objects, write to uv distort
	cmd.StartRenderPass(&m_distortRenderPass, &m_distortFramebuffers[m_currentFrame]);
	for (int i = 0; i < m_transModelVertBuffers.size(); ++i)
	{
		GraphicsVertexPipelineInput input;
		VertexIndexInput indexInput;
		VertexInput vertInput;
		indexInput.pBuffer = &m_transModelIndexBuffers[i];
		vertInput.pVertexInputLayout = &m_transModelVertLayout;
		vertInput.pBuffer = &m_transModelVertBuffers[i];
		input.pDescriptorSets =
		{
			&m_cameraDSets[m_currentFrame],
			&m_vecTransModelDSets[m_currentFrame][i],
			&m_distortDSets[m_currentFrame],
			&m_gbufferDSets[m_currentFrame],
			&m_vecMaterialDSets[m_currentFrame][i]
		};
		input.imageSize = MyDevice::GetInstance().GetSwapchainExtent();
		input.pVertexIndexInput = &indexInput;
		input.pVertexInputs = { &vertInput };

		m_distortPipeline.Do(cmd.vkCommandBuffer, input); // the draw will be done unordered
	}
	cmd.EndRenderPass();

	// draw transparent objects, write to sample data
	cmd.StartRenderPass(&m_oitRenderPass, &m_oitFramebuffers[m_currentFrame]);
	for (int i = 0; i < m_transModelVertBuffers.size(); ++i)
	{
		GraphicsVertexPipelineInput input;
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

	// do light pass
	if (m_mipLevel > 1)
	{
		ImageBarrierBuilder imageBarrierBuilder{};
		imageBarrierBuilder.SetMipLevelRange(1, ~0);
		VkImageMemoryBarrier albedoMipBarrier = imageBarrierBuilder.NewBarrier(m_gbufferAlbedoImages[m_currentFrame].vkImage,
			VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_ACCESS_NONE, VK_ACCESS_SHADER_READ_BIT);
		cmd.AddPipelineBarrier(
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			{ albedoMipBarrier }
		);
	}
	cmd.StartRenderPass(&m_lightRenderPass, &m_lightFramebuffers[m_currentFrame]);
	{
		GraphicsVertexPipelineInput input{};
		VertexIndexInput indexInput{};
		VertexInput vertInput{};
		indexInput.indexType = VK_INDEX_TYPE_UINT32;
		indexInput.pBuffer = &m_quadIndexBuffer;
		vertInput.pBuffer = &m_quadVertBuffer;
		vertInput.pVertexInputLayout = &m_quadVertLayout;
		input.imageSize = MyDevice::GetInstance().GetSwapchainExtent();
		input.pDescriptorSets = { &m_gbufferDSets[m_currentFrame] };
		input.pVertexIndexInput = &indexInput;
		input.pVertexInputs = { &vertInput };
		m_lightPipeline.Do(cmd.vkCommandBuffer, input);
	}
	cmd.EndRenderPass();

	// gaussian blur light image -> DONE
	{
		VkExtent2D imageSize = MyDevice::GetInstance().GetSwapchainExtent();
		// transfer light output image layer 0 to SHADER_READONLY
		ImageBarrierBuilder barrierBuilder{};
		barrierBuilder.SetArrayLayerRange(0, 1);
		barrierBuilder.SetMipLevelRange(0, 1);
		barrierBuilder.SetAspect(VK_IMAGE_ASPECT_COLOR_BIT);
		VkImageMemoryBarrier imageBarrier = barrierBuilder.NewBarrier(m_lightImages[m_currentFrame].vkImage, 
			//VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			_GetImageLayout(m_lightImages[m_currentFrame].vkImage, 0, 1, 0, 1, VK_IMAGE_ASPECT_COLOR_BIT),
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT
		);
		cmd.AddPipelineBarrier(
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			{ imageBarrier }
		);

		for (int i = 1; i < m_blurLayers; ++i)
		{
			// transfer tmp to GENERAL to write to
			barrierBuilder.Reset();
			barrierBuilder.SetArrayLayerRange(m_blurLayers);
			VkImageMemoryBarrier imageBarrierTmp = barrierBuilder.NewBarrier(m_lightImages[m_currentFrame].vkImage,
				// VK_IMAGE_LAYOUT_UNDEFINED,
				_GetImageLayout(m_lightImages[m_currentFrame].vkImage, m_blurLayers, 1, 0, 1, VK_IMAGE_ASPECT_COLOR_BIT),
				VK_IMAGE_LAYOUT_GENERAL,
				VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_SHADER_WRITE_BIT
				// read by last layer
			);
			cmd.AddPipelineBarrier(
				VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
				{ imageBarrierTmp }
			);

			// do image blur X
			ComputePipelineInput inputX{};
			inputX.groupCountX = (imageSize.width * imageSize.height + 255) / 256;
			inputX.groupCountY = 1;
			inputX.groupCountZ = 1;
			inputX.pDescriptorSets = { &m_blurLayeredDSetsX[m_currentFrame][i - 1] };
			m_blurPipelineX.Do(cmd.vkCommandBuffer, inputX);

			// transfer blurred tmp to SHDAER_READONLY, next layer to GENERAL
			imageBarrierTmp = barrierBuilder.NewBarrier(m_lightImages[m_currentFrame].vkImage,
				// VK_IMAGE_LAYOUT_GENERAL,
				_GetImageLayout(m_lightImages[m_currentFrame].vkImage, m_blurLayers, 1, 0, 1, VK_IMAGE_ASPECT_COLOR_BIT),
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT
			);
			barrierBuilder.Reset();
			barrierBuilder.SetArrayLayerRange(i);
			VkImageMemoryBarrier imageBarrierI = barrierBuilder.NewBarrier(m_lightImages[m_currentFrame].vkImage,
				// VK_IMAGE_LAYOUT_UNDEFINED,
				_GetImageLayout(m_lightImages[m_currentFrame].vkImage, i, 1, 0, 1, VK_IMAGE_ASPECT_COLOR_BIT),
				VK_IMAGE_LAYOUT_GENERAL,
				VK_ACCESS_NONE, VK_ACCESS_SHADER_WRITE_BIT
			);
			cmd.AddPipelineBarrier(
				VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
				{ imageBarrierI, imageBarrierTmp }
			);

			// do image blur Y
			ComputePipelineInput inputY{};
			inputY.groupCountX = (imageSize.width * imageSize.height + 255) / 256;
			inputY.groupCountY = 1;
			inputY.groupCountZ = 1;
			inputY.pDescriptorSets = { &m_blurLayeredDSetsY[m_currentFrame][i - 1] };
			m_blurPipelineY.Do(cmd.vkCommandBuffer, inputY);

			// transfer the final result to SHADER_READONLY
			barrierBuilder.Reset();
			barrierBuilder.SetArrayLayerRange(i);
			imageBarrierI = barrierBuilder.NewBarrier(m_lightImages[m_currentFrame].vkImage,
				// VK_IMAGE_LAYOUT_GENERAL,
				_GetImageLayout(m_lightImages[m_currentFrame].vkImage, i, 1, 0, 1, VK_IMAGE_ASPECT_COLOR_BIT),
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT
			);
			cmd.AddPipelineBarrier(
				VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
				{ imageBarrierI }
			);
		}
	}

	// clean oit output image -> DONE
	{
		// wait the oit output image transfer back to general first
		ImageBarrierBuilder barrierBuilder{};
		VkImageMemoryBarrier oitOutputImageBarrier = barrierBuilder.NewBarrier(
			m_oitColorImages[m_currentFrame].vkImage,
			// VK_IMAGE_LAYOUT_UNDEFINED,
			_GetImageLayout(m_oitColorImages[m_currentFrame].vkImage, 0, 1, 0, 1, VK_IMAGE_ASPECT_COLOR_BIT),
			VK_IMAGE_LAYOUT_GENERAL,
			VK_ACCESS_NONE, VK_ACCESS_TRANSFER_WRITE_BIT
		);
		cmd.AddPipelineBarrier(
			VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
			VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TRANSFER_BIT,
			{ oitOutputImageBarrier }
		);

		VkClearColorValue clearColor32Ruint{ .uint32 = {0u} };
		cmd.FillImageView(&m_oitColorImageViews[m_currentFrame], clearColor32Ruint);

		// wait clear done
		VkImageMemoryBarrier oitOutputImageBarrier2 = barrierBuilder.NewBarrier(
			m_oitColorImages[m_currentFrame].vkImage,
			// VK_IMAGE_LAYOUT_GENERAL,
			_GetImageLayout(m_oitColorImages[m_currentFrame].vkImage, 0, 1, 0, 1, VK_IMAGE_ASPECT_COLOR_BIT),
			VK_IMAGE_LAYOUT_GENERAL,
			VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_WRITE_BIT
		);
	}

	// wait for the oit sample buffer and sample count buffer to be done
	{
		ImageBarrierBuilder barrierBuilder{};
		VkMemoryBarrier sampleDataBarrier{ VK_STRUCTURE_TYPE_MEMORY_BARRIER };
		sampleDataBarrier.srcAccessMask = VkAccessFlagBits::VK_ACCESS_SHADER_WRITE_BIT;
		sampleDataBarrier.dstAccessMask = VkAccessFlagBits::VK_ACCESS_SHADER_READ_BIT;
		VkImageMemoryBarrier sampleCountBarrier = barrierBuilder.NewBarrier(
			m_oitSampleCountImages[m_currentFrame].vkImage,
			_GetImageLayout(&m_oitSampleCountImageViews[m_currentFrame]),
			VK_IMAGE_LAYOUT_GENERAL,
			VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT
		);

		cmd.AddPipelineBarrier(
			VkPipelineStageFlagBits::VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT,
			VkPipelineStageFlagBits::VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			{ sampleDataBarrier },
			{ sampleCountBarrier }
		);
	}

	// sort transparent and write to OIT output
	{
		VkExtent2D extent2d = MyDevice::GetInstance().GetSwapchainExtent();
		ComputePipelineInput pipelineInput{};
		pipelineInput.groupCountX = (extent2d.width * extent2d.height + 255)/ 256;
		pipelineInput.groupCountY = 1;
		pipelineInput.groupCountZ = 1;
		pipelineInput.pDescriptorSets = { &m_oitDSets[m_currentFrame], &m_oitColorDSets[m_currentFrame] };
		m_oitSortPipeline.Do(cmd.vkCommandBuffer, pipelineInput);
	}
	
	// wait for oit sort, distort uv done
	{
		ImageBarrierBuilder barrierBuilder{};
		VkImageMemoryBarrier oitSortBarrier = barrierBuilder.NewBarrier(
			m_oitColorImages[m_currentFrame].vkImage,
			_GetImageLayout(&m_oitColorImageViews[m_currentFrame]),
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT
		);
		VkImageMemoryBarrier oitDistortBarrier = barrierBuilder.NewBarrier(
			m_distortImages[m_currentFrame].vkImage,
			_GetImageLayout(&m_distortImageViews[m_currentFrame]),
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT
		);

		cmd.AddPipelineBarrier(
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			{ oitDistortBarrier, oitSortBarrier }
		);
	}

	// transfer the rest mipmap level of gbuffer albedo to transfer dst first
	{
		// transfer the albedo gbuffer image to transfer_dst first to make mipmaps
		ImageBarrierBuilder barrierBuilder{};
		VkImageMemoryBarrier gbufferAlbedoBarrier = barrierBuilder.NewBarrier(
			m_gbufferAlbedoImages[m_currentFrame].vkImage,
			// VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			_GetImageLayout(m_gbufferAlbedoImages[m_currentFrame].vkImage, 0, 1, 0, 1, VK_IMAGE_ASPECT_COLOR_BIT),
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_TRANSFER_WRITE_BIT
			);		
		if (m_mipLevel > 1)
		{
			barrierBuilder.SetMipLevelRange(1, VK_REMAINING_MIP_LEVELS);
			VkImageMemoryBarrier barrier = barrierBuilder.NewBarrier(
				m_gbufferAlbedoImages[m_currentFrame].vkImage,
				// VK_IMAGE_LAYOUT_UNDEFINED,
				_GetImageLayout(m_gbufferAlbedoImages[m_currentFrame].vkImage, 0, 1, 1, VK_REMAINING_MIP_LEVELS,  VK_IMAGE_ASPECT_COLOR_BIT),
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				VK_ACCESS_NONE, VK_ACCESS_TRANSFER_WRITE_BIT);
			cmd.AddPipelineBarrier(
				VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
				VK_PIPELINE_STAGE_TRANSFER_BIT,
				{ barrier }
			);
		}
		cmd.AddPipelineBarrier(
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			{ gbufferAlbedoBarrier }
		);
	}

	// generate gbuffer mipmaps
	{		
		ImageBlitBuilder blitBuilder{};
		ImageBarrierBuilder barrierBuilder{};

		int32_t mipWidth = m_gbufferAlbedoImages[m_currentFrame].GetImageInformation().width;
		int32_t mipHeight = m_gbufferAlbedoImages[m_currentFrame].GetImageInformation().height;
		for (uint32_t i = 1; i < m_mipLevel; i++) {
			barrierBuilder.Reset();
			barrierBuilder.SetMipLevelRange(i - 1);
			VkImageMemoryBarrier transferBarrier1 = barrierBuilder.NewBarrier(
				m_gbufferAlbedoImages[m_currentFrame].vkImage,
				// VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				_GetImageLayout(m_gbufferAlbedoImages[m_currentFrame].vkImage, 0, 1, i - 1, 1, VK_IMAGE_ASPECT_COLOR_BIT),
				VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT
			);

			// transfer to src first
			cmd.AddPipelineBarrier(
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
				{ transferBarrier1 }
			);

			VkImageBlit blit = blitBuilder.NewBlit(
				{ mipWidth, mipHeight }, static_cast<uint32_t>(i - 1),
				{ mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1 }, static_cast<uint32_t>(i)
			);

			cmd.BlitImage(
				m_gbufferAlbedoImages[m_currentFrame].vkImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				m_gbufferAlbedoImages[m_currentFrame].vkImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				{ blit }
			);

			VkImageMemoryBarrier transferBarrer2 = barrierBuilder.NewBarrier(
				m_gbufferAlbedoImages[m_currentFrame].vkImage,
				//VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				_GetImageLayout(m_gbufferAlbedoImages[m_currentFrame].vkImage, 0, 1, i - 1, 1, VK_IMAGE_ASPECT_COLOR_BIT),
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_SHADER_READ_BIT
			);

			// transfer self to read only
			cmd.AddPipelineBarrier(
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT,
				{ transferBarrer2 }
			);

			if (mipWidth > 1) mipWidth /= 2;
			if (mipHeight > 1) mipHeight /= 2;
		}

		// transfer last to read only
		barrierBuilder.Reset();
		barrierBuilder.SetMipLevelRange(m_mipLevel - 1);
		VkImageMemoryBarrier transferBarrier = barrierBuilder.NewBarrier(
			m_gbufferAlbedoImages[m_currentFrame].vkImage,
			//VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			_GetImageLayout(m_gbufferAlbedoImages[m_currentFrame].vkImage, 0, 1, m_mipLevel - 1, 1, VK_IMAGE_ASPECT_COLOR_BIT),
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT
		);
		cmd.AddPipelineBarrier(
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			{ transferBarrier }
		);
	}

	// draw final result to the swapchain
	cmd.StartRenderPass(&m_renderPass, &m_framebuffers[imageIndex.value()]);
	{
		GraphicsVertexPipelineInput input;
		VertexIndexInput indexInput;
		VertexInput vertInput;
		indexInput.pBuffer = &m_quadIndexBuffer;
		vertInput.pVertexInputLayout = &m_quadVertLayout;
		vertInput.pBuffer = &m_quadVertBuffer;
		input.pDescriptorSets = { &m_blurOutputDSets[m_currentFrame], &m_transOutputDSets[m_currentFrame] };
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
			if (index.texcoord_index != -1)
			{
				vertex.uv = {
					attrib.texcoords[2 * index.texcoord_index + 0],
					1.0f - attrib.texcoords[2 * index.texcoord_index + 1] 
				};
			}
			if (index.normal_index != -1)
			{
				vertex.normal = {
					attrib.normals[3 * index.normal_index + 0],
					attrib.normals[3 * index.normal_index + 1],
					attrib.normals[3 * index.normal_index + 2],
				};
			}

			if (uniqueVertices.count(vertex) == 0)
			{
				uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
				vertices.push_back(vertex);
			}
			indices.push_back(uniqueVertices[vertex]);
		}
	}
}

VkImageLayout TransparentApp::_GetImageLayout(ImageView* pImageView) const
{
	MyDevice& device = MyDevice::GetInstance();
	auto info = pImageView->GetImageViewInformation();
	auto itr = device.imageLayouts.find(pImageView->pImage->vkImage);
	CHECK_TRUE(itr != device.imageLayouts.end(), "Layout is not recorded!");
	return itr->second.GetLayout(info.baseArrayLayer, info.layerCount, info.baseMipLevel, info.levelCount, info.aspectMask);
}

VkImageLayout TransparentApp::_GetImageLayout(VkImage vkImage, uint32_t baseArrayLayer, uint32_t layerCount, uint32_t baseMipLevel, uint32_t levelCount, VkImageAspectFlags aspect) const
{
	MyDevice& device = MyDevice::GetInstance();
	auto itr = device.imageLayouts.find(vkImage);
	CHECK_TRUE(itr != device.imageLayouts.end(), "Layout is not recorded!");
	return itr->second.GetLayout(baseArrayLayer, layerCount, baseMipLevel, levelCount, aspect);
}

void TransparentApp::Run()
{
	_Init();
	_MainLoop();
	_Uninit();
}