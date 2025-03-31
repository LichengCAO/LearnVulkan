#include "meshlet_app.h"

#define MAX_FRAME_COUNT 3

void MeshletApp::_MainLoop()
{
	lastTime = glfwGetTime();
	while (!glfwWindowShouldClose(pDevice->pWindow))
	{
		glfwPollEvents();
		_DrawFrame();
		double currentTime = glfwGetTime();
		frameTime = (currentTime - lastTime) * 1000.0;
		lastTime = currentTime;
	}

	vkDeviceWaitIdle(pDevice->vkDevice);
}

void MeshletApp::_UpdateUniformBuffer()
{
	static std::optional<float> lastX;
	static std::optional<float> lastY;
	UserInput userInput = pDevice->GetUserInput();
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
}

void MeshletApp::_DrawFrame()
{
	if (MyDevice::GetInstance().NeedRecreateSwapchain())
	{
		_ResizeWindow();
	}

	auto imageIndex = pDevice->AquireAvailableSwapchainImageIndex(m_swapchainImageAvailabilities[m_currentFrame]);
	if (!imageIndex.has_value()) return;

	auto& cmd = m_commandSubmissions[m_currentFrame];
	cmd.WaitTillAvailable();
	_UpdateUniformBuffer();
	WaitInformation waitInfo{};
	waitInfo.waitSamaphore = m_swapchainImageAvailabilities[m_currentFrame];
	waitInfo.waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	cmd.StartCommands({ waitInfo });


	VkSemaphore renderpassFinish = cmd.SubmitCommands();
	MyDevice::GetInstance().PresentSwapchainImage({ renderpassFinish }, imageIndex.value());
	m_currentFrame = (m_currentFrame + 1) % MAX_FRAME_COUNT;
}

VkImageLayout MeshletApp::_GetImageLayout(ImageView* pImageView) const
{
	auto info = pImageView->GetImageViewInformation();
	auto itr = pDevice->imageLayouts.find(pImageView->pImage->vkImage);
	CHECK_TRUE(itr != pDevice->imageLayouts.end(), "Layout is not recorded!");
	return itr->second.GetLayout(info.baseArrayLayer, info.layerCount, info.baseMipLevel, info.levelCount, info.aspectMask);
}

VkImageLayout MeshletApp::_GetImageLayout(VkImage vkImage, uint32_t baseArrayLayer, uint32_t layerCount, uint32_t baseMipLevel, uint32_t levelCount, VkImageAspectFlags aspect) const
{
	auto itr = pDevice->imageLayouts.find(vkImage);
	CHECK_TRUE(itr != pDevice->imageLayouts.end(), "Layout is not recorded!");
	return itr->second.GetLayout(baseArrayLayer, layerCount, baseMipLevel, levelCount, aspect);
}

void MeshletApp::Run()
{
	_Init();
	_MainLoop();
	_Uninit();
}

void MeshletApp::_Init()
{
	pDevice = &MyDevice::GetInstance();
	pDevice->Init();
	
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
		VK_CHECK(vkCreateSemaphore(pDevice->vkDevice, &semaphoreInfo, nullptr, &m_swapchainImageAvailabilities[i]), "Failed to create semaphore!");
	}
	
	// init command buffers
	for (int i = 0; i < MAX_FRAME_COUNT; ++i)
	{
		m_commandSubmissions.push_back(std::make_unique<CommandSubmission>());
		m_commandSubmissions.back()->Init();
	}
}

void MeshletApp::_Uninit()
{
	for (auto& cmd : m_commandSubmissions)
	{
		cmd->Uninit();
	}
	for (auto& semaphore : m_swapchainImageAvailabilities)
	{
		vkDestroySemaphore(pDevice->vkDevice, semaphore, nullptr);
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

	pDevice->Uninit();
}

void MeshletApp::_InitRenderPass()
{
	m_renderPass = RenderPass();

	AttachmentInformation swapchainInfo = AttachmentInformation::GetPresetInformation(AttachmentPreset::SWAPCHAIN);
	AttachmentInformation depthInfo = AttachmentInformation::GetPresetInformation(AttachmentPreset::DEPTH);
	m_renderPass.AddAttachment(swapchainInfo);
	m_renderPass.AddAttachment(depthInfo);

	SubpassInformation subpassInfo{};
	subpassInfo.AddColorAttachment(0);
	subpassInfo.SetDepthStencilAttachment(1);
	
	m_renderPass.AddSubpass(subpassInfo);
	m_renderPass.Init();
}

void MeshletApp::_UninitRenderPass()
{
	m_renderPass.Uninit();
}

void MeshletApp::_InitDescriptorSetLayouts()
{
	m_modelDSetLayout = DescriptorSetLayout{};
	m_modelDSetLayout.AddBinding(1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT);
	m_modelDSetLayout.Init();

	m_modelVertDSetLayout = DescriptorSetLayout{};
	m_modelVertDSetLayout.AddBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_MESH_BIT_EXT | VK_SHADER_STAGE_TASK_BIT_EXT);
	m_modelVertDSetLayout.Init();
}

void MeshletApp::_UninitDescriptorSetLayouts()
{
	m_modelVertDSetLayout.Uninit();
	m_modelDSetLayout.Uninit();
}

void MeshletApp::_InitSampler()
{
	SamplerInfo info{};
	VkSamplerCreateInfo vkInfo{};

	vkInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE;
	vkInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE;
	vkInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE;
	vkInfo.anisotropyEnable = VK_FALSE;
	vkInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	vkInfo.compareEnable = VK_FALSE;
	vkInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	vkInfo.flags = 0;
	vkInfo.magFilter = VK_FILTER_LINEAR;
	vkInfo.minFilter = VK_FILTER_LINEAR;
	vkInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

	info.info = vkInfo;
	m_vkSampler = pDevice->samplerPool.GetSampler(info);
}

void MeshletApp::_UninitSampler()
{
	pDevice->samplerPool.ReturnSampler(&m_vkSampler);
}

void MeshletApp::_InitBuffers()
{
	m_vecUptrModelVertBuffers.reserve(m_models.size());
	for (const auto& model : m_models)
	{
		// model vertex buffers
		BufferInformation bufferInfo{};
		BufferInformation stageBufferInfo{};
		Buffer stageBuffer{};

		bufferInfo.size = static_cast<uint32_t>(model.verts.size() * sizeof(Vertex));
		bufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		bufferInfo.memoryProperty = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		
		stageBufferInfo.size = bufferInfo.size;
		stageBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		stageBufferInfo.memoryProperty = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;

		stageBuffer.Init(stageBufferInfo);
		stageBuffer.CopyFromHost(model.verts.data());
		std::unique_ptr<Buffer> uptrVertBuffer = std::make_unique<Buffer>();
		uptrVertBuffer->Init(bufferInfo);
		
		uptrVertBuffer->CopyFromBuffer(&stageBuffer);
		m_vecUptrModelVertBuffers.push_back(std::move(uptrVertBuffer));
		
		// model transform buffers
		m_vecUptrModelBuffers.push_back({});
		BufferInformation transformBufferInfo{};
		transformBufferInfo.memoryProperty = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
		transformBufferInfo.size = static_cast<uint32_t>(sizeof(ModelTransform));
		transformBufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
		for (int i = 0; i < MAX_FRAME_COUNT; ++i)
		{
			std::unique_ptr<Buffer> uptrTransform = std::make_unique<Buffer>();
			uptrTransform->Init(transformBufferInfo);
			m_vecUptrModelBuffers.back().push_back(std::move(uptrTransform));
		}
	}
}

void MeshletApp::_UninitBuffers()
{
	for (auto& uptr : m_vecUptrModelVertBuffers)
	{
		uptr->Uninit();
	}
	m_vecUptrModelVertBuffers.clear();

	for (auto& vec : m_vecUptrModelBuffers)
	{
		for (auto& uptr : vec)
		{
			uptr->Uninit();
		}
		vec.clear();
	}
	m_vecUptrModelBuffers.clear();
}

void MeshletApp::_InitImagesAndViews()
{
	for (int i = 0; i < MAX_FRAME_COUNT; ++i)
	{
		ImageInformation depthImageInfo{};
		depthImageInfo.arrayLayers = 1;
		depthImageInfo.depth = 1;
		depthImageInfo.format = pDevice->GetDepthFormat();
		depthImageInfo.height = pDevice->GetSwapchainExtent().height;
		depthImageInfo.width = pDevice->GetSwapchainExtent().width;
		depthImageInfo.imageType = VK_IMAGE_TYPE_2D;
		depthImageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		depthImageInfo.memoryProperty = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		depthImageInfo.mipLevels = 1;
		depthImageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		depthImageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		ImageViewInformation depthViewInfo{};
		depthViewInfo.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

		std::unique_ptr<Image> uptrDepthImage = std::make_unique<Image>();
		std::unique_ptr<ImageView> uptrDepthView = std::make_unique<ImageView>(uptrDepthImage->NewImageView(depthViewInfo));
		uptrDepthImage->SetImageInformation(depthImageInfo);
		uptrDepthImage->Init();
		m_depthImages.push_back(std::move(uptrDepthImage));
		uptrDepthView->Init();
		m_depthImageViews.push_back(std::move(uptrDepthView));
	}
}

void MeshletApp::_UninitImagesAndViews()
{
}
