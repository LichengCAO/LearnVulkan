#include "meshlet_app.h"
#define MAX_FRAME_COUNT 3

void MeshletApp::_InitPipelines()
{
	SimpleShader taskShader{};
	SimpleShader meshShader{};
	SimpleShader fragShader{};

	m_pipeline.AddDescriptorSetLayout(m_cameraDSetLayout.vkDescriptorSetLayout);
	m_pipeline.AddDescriptorSetLayout(m_modelTransformDSetLayout.vkDescriptorSetLayout);
	m_pipeline.AddDescriptorSetLayout(m_modelVertDSetLayout.vkDescriptorSetLayout);
	m_pipeline.BindToSubpass(&m_renderPass, 0);

	taskShader.SetSPVFile("E:/GitStorage/LearnVulkan/bin/shaders/mesh.task.spv");
	meshShader.SetSPVFile("E:/GitStorage/LearnVulkan/bin/shaders/mesh.mesh.spv");
	fragShader.SetSPVFile("E:/GitStorage/LearnVulkan/bin/shaders/mesh.frag.spv");
	taskShader.Init();
	meshShader.Init();
	fragShader.Init();

	m_pipeline.AddShader(taskShader.GetShaderStageInfo());
	m_pipeline.AddShader(meshShader.GetShaderStageInfo());
	m_pipeline.AddShader(fragShader.GetShaderStageInfo());
	m_pipeline.Init();

	fragShader.Uninit();
	meshShader.Uninit();
	taskShader.Uninit();
}

void MeshletApp::_UninitPipelines()
{
	m_pipeline.Uninit();
}

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

	CameraUBO cameraUBO{};
	cameraUBO.proj = m_camera.GetProjectionMatrix();
	cameraUBO.view = m_camera.GetViewMatrix();
	m_cameraBuffers[m_currentFrame]->CopyFromHost(&cameraUBO);
}

void MeshletApp::_DrawFrame()
{
	if (MyDevice::GetInstance().NeedRecreateSwapchain())
	{
		_ResizeWindow();
	}

	auto& cmd = m_commandSubmissions[m_currentFrame];
	cmd->WaitTillAvailable();
	auto imageIndex = pDevice->AquireAvailableSwapchainImageIndex(m_swapchainImageAvailabilities[m_currentFrame]);
	if (!imageIndex.has_value()) return;
	_UpdateUniformBuffer();
	WaitInformation waitInfo{};
	waitInfo.waitSamaphore = m_swapchainImageAvailabilities[m_currentFrame];
	waitInfo.waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	cmd->StartCommands({ waitInfo });

	cmd->StartRenderPass(&m_renderPass, m_framebuffers[m_currentFrame].get());
	for (int i = 0; i < m_models.size(); ++i)
	{
		GraphicsMeshPipelineInput meshInput{};
		meshInput.groupCountX = 126;
		meshInput.groupCountY = 1;
		meshInput.groupCountZ = 1;
		meshInput.imageSize = pDevice->GetSwapchainExtent();
		meshInput.pDescriptorSets = { m_cameraDSets[m_currentFrame].get(), m_modelTransformDSets[i][m_currentFrame].get(), m_modelVertDSets[i].get() };
		m_pipeline.Do(cmd->vkCommandBuffer, meshInput);
	}
	cmd->EndRenderPass();

	VkSemaphore renderpassFinish = cmd->SubmitCommands();
	MyDevice::GetInstance().PresentSwapchainImage({ renderpassFinish }, imageIndex.value());
	m_currentFrame = (m_currentFrame + 1) % MAX_FRAME_COUNT;
}

void MeshletApp::_ResizeWindow()
{
	vkDeviceWaitIdle(pDevice->vkDevice);
	//_UninitDescriptorSets();
	_UninitFramebuffers();
	_UninitImagesAndViews();
	//_UninitBuffers();
	pDevice->RecreateSwapchain();
	//_InitBuffers();
	_InitImagesAndViews();
	_InitFramebuffers();
	//_InitDescriptorSets();
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
	
	std::vector<Mesh> meshs;
	MeshLoader::Load("E:/GitStorage/LearnVulkan/res/models/bunny/bunny.obj", meshs);
	for (auto& mesh : meshs)
	{
		Model model{};
		model.mesh = mesh;
		m_models.push_back(model);
	}

	_InitRenderPass();
	_InitDescriptorSetLayouts();

	_InitBuffers();
	_InitImagesAndViews();
	_InitFramebuffers();
	_InitSampler();

	_InitDescriptorSets();
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
	//_UninitVertexInputs();
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
	m_modelTransformDSetLayout = DescriptorSetLayout{};
	m_modelTransformDSetLayout.AddBinding(1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_TASK_BIT_EXT | VK_SHADER_STAGE_MESH_BIT_EXT);
	m_modelTransformDSetLayout.Init();

	m_modelVertDSetLayout = DescriptorSetLayout{};
	m_modelVertDSetLayout.AddBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_MESH_BIT_EXT | VK_SHADER_STAGE_TASK_BIT_EXT);
	m_modelVertDSetLayout.Init();

	m_cameraDSetLayout = DescriptorSetLayout{};
	m_cameraDSetLayout.AddBinding(1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_TASK_BIT_EXT | VK_SHADER_STAGE_MESH_BIT_EXT);
	m_cameraDSetLayout.Init();
}

void MeshletApp::_UninitDescriptorSetLayouts()
{
	m_cameraDSetLayout.Uninit();
	m_modelVertDSetLayout.Uninit();
	m_modelTransformDSetLayout.Uninit();
}

void MeshletApp::_InitSampler()
{
	m_vkSampler = pDevice->samplerPool.GetSampler(VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
}

void MeshletApp::_UninitSampler()
{
	pDevice->samplerPool.ReturnSampler(&m_vkSampler);
}

void MeshletApp::_InitBuffers()
{
	m_modelVertBuffers.reserve(m_models.size());
	for (const auto& model : m_models)
	{
		// model vertex buffers
		std::vector<VBO> curVBOs;
		BufferInformation bufferInfo{};
		BufferInformation stageBufferInfo{};
		Buffer stageBuffer{};
		
		for (const auto& meshVert : model.mesh.verts)
		{
			VBO curVBO{};
			curVBO.pos = meshVert.position;
			CHECK_TRUE(meshVert.normal.has_value(), "We need a normal here!");
			curVBO.normal = meshVert.normal.value();

			curVBOs.push_back(curVBO);
		}

		bufferInfo.size = static_cast<uint32_t>(curVBOs.size() * sizeof(VBO));
		bufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		bufferInfo.memoryProperty = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		
		stageBufferInfo.size = bufferInfo.size;
		stageBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		stageBufferInfo.memoryProperty = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;

		stageBuffer.Init(stageBufferInfo);
		stageBuffer.CopyFromHost(curVBOs.data());
		std::unique_ptr<Buffer> uptrVertBuffer = std::make_unique<Buffer>();
		uptrVertBuffer->Init(bufferInfo);
		
		uptrVertBuffer->CopyFromBuffer(&stageBuffer);
		m_modelVertBuffers.push_back(std::move(uptrVertBuffer));
		
		// model transform buffers
		m_modelTransformBuffers.push_back({});
		BufferInformation transformBufferInfo{};
		transformBufferInfo.memoryProperty = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
		transformBufferInfo.size = static_cast<uint32_t>(sizeof(ModelTransformUBO));
		transformBufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
		for (int i = 0; i < MAX_FRAME_COUNT; ++i)
		{
			std::unique_ptr<Buffer> uptrTransform = std::make_unique<Buffer>();
			uptrTransform->Init(transformBufferInfo);
			m_modelTransformBuffers.back().push_back(std::move(uptrTransform));
		}
	}

	m_cameraBuffers.reserve(MAX_FRAME_COUNT);
	for (int i = 0; i < MAX_FRAME_COUNT; ++i)
	{
		std::unique_ptr<Buffer> cameraBuffer = std::make_unique<Buffer>(Buffer{});
		BufferInformation cameraBufferInfo{};
		cameraBufferInfo.memoryProperty = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
		cameraBufferInfo.size = static_cast<uint32_t>(sizeof(CameraUBO));
		cameraBufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
		
		cameraBuffer->Init(cameraBufferInfo);
		m_cameraBuffers.push_back(std::move(cameraBuffer));
	}
}

void MeshletApp::_UninitBuffers()
{
	for (auto& uptr : m_modelVertBuffers)
	{
		uptr->Uninit();
	}
	m_modelVertBuffers.clear();

	for (auto& vec : m_modelTransformBuffers)
	{
		for (auto& uptr : vec)
		{
			uptr->Uninit();
		}
		vec.clear();
	}
	m_modelTransformBuffers.clear();

	for (auto& uptr : m_cameraBuffers)
	{
		uptr->Uninit();
	}
	m_cameraBuffers.clear();
}

void MeshletApp::_InitImagesAndViews()
{
	m_swapchainImages = pDevice->GetSwapchainImages();
	int n = m_swapchainImages.size();
	for (int i = 0; i < n; ++i)
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
		uptrDepthImage->SetImageInformation(depthImageInfo);
		uptrDepthImage->Init();

		std::unique_ptr<ImageView> uptrDepthView = std::make_unique<ImageView>(uptrDepthImage->NewImageView(depthViewInfo));
		m_depthImages.push_back(std::move(uptrDepthImage));
		uptrDepthView->Init();
		m_depthImageViews.push_back(std::move(uptrDepthView));
		
		ImageViewInformation swapchainViewInfo{};
		swapchainViewInfo.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		swapchainViewInfo.levelCount = m_swapchainImages[i].GetImageInformation().mipLevels;
		std::unique_ptr<ImageView> uptrSwapchainView = std::make_unique<ImageView>(m_swapchainImages[i].NewImageView(swapchainViewInfo));
		uptrSwapchainView->Init();
		m_swapchainImageViews.push_back(std::move(uptrSwapchainView));
	}
}

void MeshletApp::_UninitImagesAndViews()
{
	for (auto& uptrView : m_depthImageViews)
	{
		uptrView->Uninit();
	}
	m_depthImageViews.clear();
	for (auto& uptrImage : m_depthImages)
	{
		uptrImage->Uninit();
	}
	m_depthImages.clear();
	for (auto& uptrView : m_swapchainImageViews)
	{
		uptrView->Uninit();
	}
	m_swapchainImageViews.clear();
}

void MeshletApp::_InitFramebuffers()
{
	int n = m_swapchainImageViews.size();

	for (int i = 0; i < n; ++i)
	{
		std::vector<const ImageView*> renderViews{ 
			m_swapchainImageViews[i].get(),
			m_depthImageViews[i].get()
		};
		std::unique_ptr<Framebuffer> uptrFramebuffer = std::make_unique<Framebuffer>(m_renderPass.NewFramebuffer(renderViews));
		uptrFramebuffer->Init();
		m_framebuffers.push_back(std::move(uptrFramebuffer));
	}
}

void MeshletApp::_UninitFramebuffers()
{
	for (auto& uptrFramebuffer : m_framebuffers)
	{
		uptrFramebuffer->Uninit();
	}
	m_framebuffers.clear();
}

void MeshletApp::_InitDescriptorSets()
{
	int n = MAX_FRAME_COUNT;
	// model related
	for (int i = 0; i < m_models.size(); ++i)
	{
		const auto& model = m_models[i];
		MYTYPE(DescriptorSet) tmpTransformDSet;
		for (int j = 0; j < n; ++j)
		{
			std::unique_ptr<DescriptorSet> modelTransformDSet = std::make_unique<DescriptorSet>(m_modelTransformDSetLayout.NewDescriptorSet());
			const Buffer* pTransformBuffer = m_modelTransformBuffers[i][j].get();
			VkDescriptorBufferInfo transformBufferInfo{};
			
			transformBufferInfo.buffer = pTransformBuffer->vkBuffer;
			transformBufferInfo.offset = 0;
			transformBufferInfo.range = pTransformBuffer->GetBufferInformation().size;
			
			modelTransformDSet->Init();
			modelTransformDSet->StartUpdate();
			modelTransformDSet->UpdateBinding(0, { transformBufferInfo });
			modelTransformDSet->FinishUpdate();

			tmpTransformDSet.push_back(std::move(modelTransformDSet));
		}
		m_modelTransformDSets.push_back(std::move(tmpTransformDSet));
		
		VkDescriptorBufferInfo vertBufferInfo{};
		const Buffer* pVertBuffer = m_modelVertBuffers[i].get();
		std::unique_ptr<DescriptorSet> modelVertDSet = std::make_unique<DescriptorSet>(m_modelVertDSetLayout.NewDescriptorSet());
		vertBufferInfo.buffer = pVertBuffer->vkBuffer;
		vertBufferInfo.offset = 0;
		vertBufferInfo.range = pVertBuffer->GetBufferInformation().size;
		modelVertDSet->Init();
		modelVertDSet->StartUpdate();
		modelVertDSet->UpdateBinding(0, { vertBufferInfo });
		modelVertDSet->FinishUpdate();
		m_modelVertDSets.push_back(std::move(modelVertDSet));
	}

	// camera
	for (int i = 0; i < n; ++i)
	{
		std::unique_ptr<DescriptorSet> cameraDSet = std::make_unique<DescriptorSet>(m_cameraDSetLayout.NewDescriptorSet());
		const Buffer* pCameraBuffer = m_cameraBuffers[i].get();
		VkDescriptorBufferInfo cameraBufferInfo{};
		cameraBufferInfo.buffer = pCameraBuffer->vkBuffer;
		cameraBufferInfo.offset = 0;
		cameraBufferInfo.range = pCameraBuffer->GetBufferInformation().size;
		cameraDSet->Init();
		cameraDSet->StartUpdate();
		cameraDSet->UpdateBinding(0, { cameraBufferInfo });
		cameraDSet->FinishUpdate();
		m_cameraDSets.push_back(std::move(cameraDSet));
	}
}

void MeshletApp::_UninitDescriptorSets()
{
	m_cameraDSets.clear();
	m_modelVertDSets.clear();
	for (auto& vec : m_modelTransformDSets)
	{
		vec.clear();
	}
	m_modelTransformDSets.clear();
}
