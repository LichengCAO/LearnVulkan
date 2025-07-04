#include "meshlet_app.h"
#define MAX_FRAME_COUNT 3

void MeshletApp::_Init()
{
	pDevice = &MyDevice::GetInstance();
	pDevice->Init();
	
	_InitRenderPass();
	_InitDescriptorSetLayouts();

	_InitModels();
	_InitBuffers();
	_InitImagesAndViews();
	_InitFramebuffers();
	_InitSampler();

	_InitDescriptorSets();
	_InitPipelines();
	
	// init semaphores
	{
		VkSemaphoreCreateInfo semaphoreInfo{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
		m_swapchainImageAvailabilities.resize(MAX_FRAME_COUNT);
		for (int i = 0; i < MAX_FRAME_COUNT; ++i)
		{
			VK_CHECK(vkCreateSemaphore(pDevice->vkDevice, &semaphoreInfo, nullptr, &m_swapchainImageAvailabilities[i]), "Failed to create semaphore!");
		}
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

	Subpass subpassInfo{};
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
	m_cameraDSetLayout = DescriptorSetLayout{};
	m_cameraDSetLayout.AddBinding(1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_TASK_BIT_EXT | VK_SHADER_STAGE_MESH_BIT_EXT);
	m_cameraDSetLayout.AddBinding(1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_TASK_BIT_EXT);
	m_cameraDSetLayout.Init();

	m_meshletDSetLayout = DescriptorSetLayout{};
	m_meshletDSetLayout.AddBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_MESH_BIT_EXT); // meshlets
	m_meshletDSetLayout.AddBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_MESH_BIT_EXT); // meshlet vertices
	m_meshletDSetLayout.AddBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_MESH_BIT_EXT); // meshlet triangles
	m_meshletDSetLayout.AddBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_MESH_BIT_EXT); // VBOs
	m_meshletDSetLayout.AddBinding(1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_TASK_BIT_EXT | VK_SHADER_STAGE_MESH_BIT_EXT); // model UBO
	m_meshletDSetLayout.AddBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_TASK_BIT_EXT); // model bounding sbo
	m_meshletDSetLayout.Init();
}
void MeshletApp::_UninitDescriptorSetLayouts()
{
	m_meshletDSetLayout.Uninit();
	m_cameraDSetLayout.Uninit();
}

void MeshletApp::_InitSampler()
{
	m_vkSampler = pDevice->samplerPool.GetSampler(VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
}
void MeshletApp::_UninitSampler()
{
	pDevice->samplerPool.ReturnSampler(&m_vkSampler);
}

void MeshletApp::_InitModels()
{
	std::vector<Mesh> meshs;
	MeshUtility::Load("E:/GitStorage/LearnVulkan/res/models/bunny/bunny.obj", meshs);
	for (auto& mesh : meshs)
	{
		Model model{};
		model.transform.SetScale({ 2.0, 2.0, 2.0 });
		model.transform.SetRotation({ -90, 0, 0 });
		model.mesh = mesh;
		MeshUtility::BuildMeshlets(mesh, model.vecMeshlet, model.vecMeshletBounds, model.vecVertexRemap, model.vecTriangleIndex);
		m_models.push_back(model);
	}
}
void MeshletApp::_InitBuffers()
{
	int n = m_models.size();

	m_cameraBuffers.reserve(MAX_FRAME_COUNT);
	for (int i = 0; i < MAX_FRAME_COUNT; ++i)
	{
		std::unique_ptr<Buffer> cameraBuffer = std::make_unique<Buffer>(Buffer{});
		std::unique_ptr<Buffer> frustumBuffer = std::make_unique<Buffer>(Buffer{});
		Buffer::Information bufferInfo{};
		bufferInfo.memoryProperty = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
		bufferInfo.size = static_cast<uint32_t>(sizeof(CameraUBO));
		bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
		
		cameraBuffer->Init(bufferInfo);
		m_cameraBuffers.push_back(std::move(cameraBuffer));

		bufferInfo.size = static_cast<uint32_t>(sizeof(FrustumUBO));
		frustumBuffer->Init(bufferInfo);
		m_frustumBuffers.push_back(std::move(frustumBuffer));
	}

	m_meshletDSets.reserve(n);
	for (int i = 0; i < n; ++i)
	{
		Buffer::Information bufferInfo;
		Model& curModel = m_models[i];
		std::unique_ptr<Buffer> meshletBuffer = std::make_unique<Buffer>(Buffer{});
		std::unique_ptr<Buffer> meshletBoundsBuffer = std::make_unique<Buffer>(Buffer{});
		std::unique_ptr<Buffer> meshletVertexBuffer = std::make_unique<Buffer>(Buffer{});
		std::unique_ptr<Buffer> meshletTriangleBuffer = std::make_unique<Buffer>(Buffer{});
		std::unique_ptr<Buffer> meshletVBOBuffer = std::make_unique<Buffer>(Buffer{});
		std::unique_ptr<Buffer> meshUBOBuffer = std::make_unique<Buffer>(Buffer{});
		
		bufferInfo.memoryProperty = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		bufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		
		{
			std::vector<MeshletSBO> sbos;
			sbos.reserve(curModel.vecMeshlet.size());
			for (const auto& meshlet : curModel.vecMeshlet)
			{
				MeshletSBO sbo{};
				sbo.triangleCount = meshlet.triangleCount;
				sbo.triangleOffset = meshlet.triangleOffset;
				sbo.vertexCount = meshlet.vertexCount;
				sbo.vertexOffset = meshlet.vertexOffset;
				sbos.push_back(sbo);
			}
			bufferInfo.size = static_cast<uint32_t>(sizeof(MeshletSBO)) * static_cast<uint32_t>(sbos.size());
			meshletBuffer->Init(bufferInfo);
			meshletBuffer->CopyFromHost(sbos.data());
		}

		{
			std::vector<MeshletBoundsSBO> sbos;
			sbos.reserve(curModel.vecMeshletBounds.size());
			for (const auto& meshletBounds : curModel.vecMeshletBounds)
			{
				MeshletBoundsSBO sbo{};
				sbo.boundSphere = glm::vec4(meshletBounds.center, meshletBounds.radius);
				sbo.coneApex = meshletBounds.coneApex;
				sbo.coneNormalCutoff = glm::vec4(meshletBounds.coneAxis, meshletBounds.coneCutoff);
				sbos.push_back(sbo);
			}
			bufferInfo.size = static_cast<uint32_t>(sizeof(MeshletBoundsSBO)) * static_cast<uint32_t>(sbos.size());
			m_tBound = sbos;
			meshletBoundsBuffer->Init(bufferInfo);
			meshletBoundsBuffer->CopyFromHost(sbos.data());
		}
		
		bufferInfo.size = static_cast<uint32_t>(sizeof(uint32_t)) * static_cast<uint32_t>(curModel.vecVertexRemap.size());
		meshletVertexBuffer->Init(bufferInfo);
		meshletVertexBuffer->CopyFromHost(curModel.vecVertexRemap.data());

		bufferInfo.size = static_cast<uint32_t>(sizeof(uint8_t)) * static_cast<uint32_t>(curModel.vecTriangleIndex.size());
		meshletTriangleBuffer->Init(bufferInfo);
		meshletTriangleBuffer->CopyFromHost(curModel.vecTriangleIndex.data());

		{
			std::vector<VBO> vbos;
			vbos.reserve(curModel.mesh.verts.size());
			for (const auto& vertex : curModel.mesh.verts)
			{
				VBO vbo{};
				vbo.normal = vertex.normal.has_value() ? vertex.normal.value() : glm::vec3(0, 0, 1);
				vbo.pos = vertex.position;
				vbos.push_back(vbo);
			}
			bufferInfo.size = static_cast<uint32_t>(sizeof(VBO)) * static_cast<uint32_t>(curModel.mesh.verts.size());
			meshletVBOBuffer->Init(bufferInfo);
			meshletVBOBuffer->CopyFromHost(vbos.data());
		}

		{
			ModelUBO ubo{};
			ubo.meshletCount = static_cast<uint32_t>(curModel.vecMeshlet.size());
			ubo.model = curModel.transform.GetModelMatrix();
			ubo.inverseTranposeModel = curModel.transform.GetModelInverseTransposeMatrix();
			ubo.maxScale = 1.0f;
			{
				float scale2 = 0.0f;
				for (int col = 0; col < 3; ++col)
				{
					float curScale2 = glm::dot(glm::vec3(ubo.model[col]), glm::vec3(ubo.model[col]));
					scale2 = std::max(scale2, curScale2);
				}
				ubo.maxScale = std::sqrt(scale2);
			}

			bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

			bufferInfo.size = static_cast<uint32_t>(sizeof(ModelUBO));
			meshUBOBuffer->Init(bufferInfo);
			meshUBOBuffer->CopyFromHost(&ubo);
		}

		m_meshletBuffers.push_back(std::move(meshletBuffer));
		m_meshletVertexBuffers.push_back(std::move(meshletVertexBuffer));
		m_meshletTriangleBuffers.push_back(std::move(meshletTriangleBuffer));
		m_meshletVBOBuffers.push_back(std::move(meshletVBOBuffer));
		m_meshUBOBuffers.push_back(std::move(meshUBOBuffer));
		m_meshletBoundsBuffers.push_back(std::move(meshletBoundsBuffer));
	}
}
void MeshletApp::_UninitBuffers()
{
	auto meshBuffers = {
		&m_meshUBOBuffers,
		&m_meshletVBOBuffers,
		&m_meshletTriangleBuffers,
		&m_meshletVertexBuffers,
		&m_meshletBuffers,
		&m_meshletBoundsBuffers,
	};
	for (auto pVec : meshBuffers)
	{
		for (auto& uptr : *pVec)
		{
			uptr->Uninit();
		}
		pVec->clear();
	}

	for (auto& uptr : m_cameraBuffers)
	{
		uptr->Uninit();
	}
	m_cameraBuffers.clear();
	
	for (auto& uptr : m_frustumBuffers)
	{
		uptr->Uninit();
	}
	m_frustumBuffers.clear();
}

void MeshletApp::_InitImagesAndViews()
{
	m_swapchainImages = pDevice->GetSwapchainImages();
	int n = m_swapchainImages.size();
	for (int i = 0; i < n; ++i)
	{
		Image::Information depthImageInfo{};
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

		std::unique_ptr<Image> uptrDepthImage = std::make_unique<Image>();
		uptrDepthImage->SetImageInformation(depthImageInfo);
		uptrDepthImage->Init();

		std::unique_ptr<ImageView> uptrDepthView = std::make_unique<ImageView>(uptrDepthImage->NewImageView(VK_IMAGE_ASPECT_DEPTH_BIT));
		m_depthImages.push_back(std::move(uptrDepthImage));
		uptrDepthView->Init();
		m_depthImageViews.push_back(std::move(uptrDepthView));
		
		std::unique_ptr<ImageView> uptrSwapchainView = std::make_unique<ImageView>(m_swapchainImages[i].NewImageView());
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
	// camera
	for (int i = 0; i < MAX_FRAME_COUNT; ++i)
	{
		std::unique_ptr<DescriptorSet> cameraDSet = std::make_unique<DescriptorSet>(m_cameraDSetLayout.NewDescriptorSet());
		Buffer const* pCameraBuffer = m_cameraBuffers[i].get();
		Buffer const* pFrustumBuffer = m_frustumBuffers[i].get();
		VkDescriptorBufferInfo cameraBufferInfo{};
		cameraBufferInfo.buffer = pCameraBuffer->vkBuffer;
		cameraBufferInfo.offset = 0;
		cameraBufferInfo.range = pCameraBuffer->GetBufferInformation().size;
		cameraDSet->Init();
		cameraDSet->StartUpdate();
		cameraDSet->UpdateBinding(0, { cameraBufferInfo });
		cameraDSet->UpdateBinding(1, pFrustumBuffer);
		cameraDSet->FinishUpdate();
		m_cameraDSets.push_back(std::move(cameraDSet));
	}

	// meshlet
	for (int i = 0; i < m_models.size(); ++i)
	{
		std::unique_ptr<DescriptorSet> meshletDSet = std::make_unique<DescriptorSet>(m_meshletDSetLayout.NewDescriptorSet());
		VkDescriptorBufferInfo bufferInfo{};
		meshletDSet->Init();
		meshletDSet->StartUpdate();
		meshletDSet->UpdateBinding(0, m_meshletBuffers[i].get());
		meshletDSet->UpdateBinding(1, m_meshletVertexBuffers[i].get());
		meshletDSet->UpdateBinding(2, m_meshletTriangleBuffers[i].get());
		meshletDSet->UpdateBinding(3, m_meshletVBOBuffers[i].get());
		meshletDSet->UpdateBinding(4, m_meshUBOBuffers[i].get());
		meshletDSet->UpdateBinding(5, m_meshletBoundsBuffers[i].get());
		meshletDSet->FinishUpdate();
		m_meshletDSets.push_back(std::move(meshletDSet));
	}
}
void MeshletApp::_UninitDescriptorSets()
{
	m_cameraDSets.clear();
	m_meshletDSets.clear();
}

void MeshletApp::_InitPipelines()
{
	SimpleShader taskShader{};
	SimpleShader meshShader{};
	SimpleShader fragShader{};

	m_pipeline.AddDescriptorSetLayout(m_cameraDSetLayout.vkDescriptorSetLayout);
	m_pipeline.AddDescriptorSetLayout(m_meshletDSetLayout.vkDescriptorSetLayout);
	m_pipeline.BindToSubpass(&m_renderPass, 0);

	taskShader.SetSPVFile("E:/GitStorage/LearnVulkan/bin/shaders/flat_task.task.spv");
	meshShader.SetSPVFile("E:/GitStorage/LearnVulkan/bin/shaders/flat_task.mesh.spv");
	fragShader.SetSPVFile("E:/GitStorage/LearnVulkan/bin/shaders/flat_task.frag.spv");
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
	cameraUBO.eye = glm::vec4(m_camera.eye, 1.0f);
	m_cameraBuffers[m_currentFrame]->CopyFromHost(&cameraUBO);

	FrustumUBO frustumUBO{};
	Frustum cameraFrustum = m_camera.GetFrustum();
	frustumUBO.leftFace = cameraFrustum.leftPlane;
	frustumUBO.rightFace = cameraFrustum.rightPlane;
	frustumUBO.topFace = cameraFrustum.topPlane;
	frustumUBO.bottomFace = cameraFrustum.bottomPlane;
	frustumUBO.farFace = cameraFrustum.farPlane;
	frustumUBO.nearFace = cameraFrustum.nearPlane;
	m_frustumBuffers[m_currentFrame]->CopyFromHost(&frustumUBO);

	//int culled = 0;
	//for (int i = 0; i < m_tBound.size(); ++i)
	//{
	//	glm::vec4 b = m_tBound[i].boundSphere;
	//	glm::vec4 bcenter = glm::vec4(glm::vec3(b), 1.0f);
	//	float r = b.w;
	//	if (
	//		(glm::dot(bcenter, frustumUBO.leftFace)      < r)
	//		&& (glm::dot(bcenter, frustumUBO.rightFace)  < r)
	//		&& (glm::dot(bcenter, frustumUBO.topFace)    < r)
	//		&& (glm::dot(bcenter, frustumUBO.bottomFace) < r)
	//		&& (glm::dot(bcenter, frustumUBO.farFace)    < r)
	//		&& (glm::dot(bcenter, frustumUBO.nearFace)   < r)
	//		)
	//	{
	//		glm::vec4 v = cameraUBO.proj * cameraUBO.view * bcenter;
	//		v /= v.w;
	//		std::cout << v[0] << ", " << v[1] << ", " << v[2] << ", " << v[3] << std::endl;
	//	}
	//	else
	//	{
	//		culled++;
	//	}
	//}
	//std::cout << "culled: " << culled << std::endl;

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
	CommandSubmission::WaitInformation waitInfo{};
	waitInfo.waitSamaphore = m_swapchainImageAvailabilities[m_currentFrame];
	waitInfo.waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	cmd->StartCommands({ waitInfo });

	cmd->StartRenderPass(&m_renderPass, m_framebuffers[imageIndex.value()].get());
	for (int i = 0; i < m_models.size(); ++i)
	{
		GraphicsPipeline::PipelineInput_Mesh meshInput{};
		const uint32_t meshletCountPerWorkgroup = 32;
		// don't use task shader here
		//meshInput.groupCountX = static_cast<uint32_t>(m_models[i].vecMeshlet.size()); // meshlet count
		//meshInput.groupCountY = 1;
		//meshInput.groupCountZ = 1;
		// use task shader here
		meshInput.groupCountX = (static_cast<uint32_t>(m_models[i].vecMeshlet.size()) + meshletCountPerWorkgroup - 1) / meshletCountPerWorkgroup; // task workgroup count
		meshInput.groupCountY = 1;
		meshInput.groupCountZ = 1;
		meshInput.imageSize = pDevice->GetSwapchainExtent();
		meshInput.pDescriptorSets = { m_cameraDSets[m_currentFrame].get(), m_meshletDSets[i].get() };
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