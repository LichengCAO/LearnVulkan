#include "ray_query_app.h"

#define MAX_FRAME_COUNT 3

void RayQueryApp::_Init()
{
	MyDevice::GetInstance().Init();
	_InitDescriptorSetLayouts();
	_InitSampler();
	_InitModels();
	_InitBuffers();
	_InitAS();
	_InitImagesAndViews();
	_InitSwapchainPass();
	_InitDescriptorSets();
	_InitPipelines();
	_InitCommandBuffers();
}

void RayQueryApp::_Uninit()
{
	_UninitCommandBuffers();
	_UninitPipelines();
	_UninitDescriptorSets();
	_UninitSwapchainPass();
	_UninitImagesAndViews();
	_UninitAS();
	_UninitBuffers();
	m_models.clear();
	_UninitSampler();
	_UninitDescriptorSetLayouts();
	MyDevice::GetInstance().Uninit();
}

void RayQueryApp::_InitDescriptorSetLayouts()
{
	m_rtDSetLayout.AddBinding(1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_RAYGEN_BIT_KHR); // camera info
	m_rtDSetLayout.AddBinding(1, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_RAYGEN_BIT_KHR); // AS
	m_rtDSetLayout.AddBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_RAYGEN_BIT_KHR); // image output
	m_rtDSetLayout.AddBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR); // instance data
	m_rtDSetLayout.Init();

	//m_compDSetLayout.AddBinding(1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT); // time
	m_compDSetLayout.AddBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT); // vertex buffer
	m_compDSetLayout.Init();
}

void RayQueryApp::_UninitDescriptorSetLayouts()
{
	m_compDSetLayout.Uninit();
	m_rtDSetLayout.Uninit();
}

void RayQueryApp::_InitSampler()
{
	m_vkSampler = MyDevice::GetInstance().samplerPool.GetSampler(VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
}

void RayQueryApp::_UninitSampler()
{
	MyDevice::GetInstance().samplerPool.ReturnSampler(&m_vkSampler);
}

void RayQueryApp::_InitModels()
{
	std::vector<Mesh> outMeshes;
	MeshUtility::Load("E:/GitStorage/LearnVulkan/res/models/sphere/sphere.obj", outMeshes);
	MeshUtility::Load("E:/GitStorage/LearnVulkan/res/models/bunny/bunny.obj", outMeshes);
	MeshUtility::Load("E:/GitStorage/LearnVulkan/res/models/wahoo/wahoo.obj", outMeshes);
	MeshUtility::Load("E:/GitStorage/LearnVulkan/res/models/ChessBoard/ChessBoard.obj", outMeshes);

	for (auto const& mesh : outMeshes)
	{
		Model model{};
		model.mesh = mesh;
		m_models.push_back(model);
	}
	//m_models[0].transform.SetScale(0.5, 0.5, 0.5);
	m_models[2].transform.SetScale(0.5, 0.5, 0.5);
	//m_models[2].transform.SetScale(0.1, 0.1, 0.1);

	m_models[0 + 1].transform.SetPosition(0, 0, 0);
	m_models[1 + 1].transform.SetPosition(6, 0, 1);
	m_models[2 + 1].transform.SetPosition(0, 0, -2);

	m_models[0 + 1].transform.SetRotation(90, 0, 180);
	m_models[1 + 1].transform.SetRotation(90, 0, 180);
	m_models[2 + 1].transform.SetRotation(0, 0, 90);
}

void RayQueryApp::_InitBuffers()
{
	// vertex, index buffers
	{
		Buffer::Information vertBufferInfo{};

		vertBufferInfo.memoryProperty = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		vertBufferInfo.usage =
			VK_BUFFER_USAGE_TRANSFER_DST_BIT
			| VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
			| VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR
			| VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

		for (auto const& model : m_models)
		{
			std::vector<std::unique_ptr<Buffer>> vecVertexBuffers{};
			std::vector<std::unique_ptr<Buffer>> vecIndexBuffers{};

			vecVertexBuffers.reserve(MAX_FRAME_COUNT);
			vecIndexBuffers.reserve(MAX_FRAME_COUNT);
			for (int i = 0; i < MAX_FRAME_COUNT; ++i)
			{
				std::unique_ptr<Buffer> uptrVertexBuffer = std::make_unique<Buffer>();
				std::unique_ptr<Buffer> uptrIndexBuffer = std::make_unique<Buffer>();
				std::vector<VBO> vertData{};

				vertData.reserve(model.mesh.verts.size());
				for (auto const& vert : model.mesh.verts)
				{
					VBO curData{};
					curData.pos = glm::vec4(vert.position, 1.0f);
					CHECK_TRUE(vert.normal.has_value(), "We need a normal here!");
					curData.normal = glm::vec4(vert.normal.value(), 0.0f);
					vertData.push_back(curData);
				}

				vertBufferInfo.size = vertData.size() * sizeof(VBO);
				uptrVertexBuffer->Init(vertBufferInfo);

				vertBufferInfo.size = model.mesh.indices.size() * sizeof(uint32_t);
				uptrIndexBuffer->Init(vertBufferInfo);

				uptrVertexBuffer->CopyFromHost(vertData.data());
				uptrIndexBuffer->CopyFromHost(model.mesh.indices.data());

				vecVertexBuffers.push_back(std::move(uptrVertexBuffer));
				vecIndexBuffers.push_back(std::move(uptrIndexBuffer));
			}

			m_vertexBuffers.push_back(std::move(vecVertexBuffers));
			m_indexBuffers.push_back(std::move(vecIndexBuffers));
		}
	}

	// Instance buffer
	{
		for (int j = 0; j < MAX_FRAME_COUNT; ++j)
		{
			int n = m_models.size();
			Buffer::Information instBufferInfo{};
			std::vector<InstanceInformation> instInfos{};
			std::unique_ptr<Buffer> uptrInstanceBuffer = std::make_unique<Buffer>();

			instInfos.reserve(n);
			for (int i = 0; i < n; ++i)
			{
				InstanceInformation instInfo{};

				instInfo.vertexBuffer = m_vertexBuffers[i][j]->GetDeviceAddress();
				instInfo.indexBuffer = m_indexBuffers[i][j]->GetDeviceAddress();

				instInfos.push_back(instInfo);
			}

			instBufferInfo.memoryProperty = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
			instBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
			instBufferInfo.size = instInfos.size() * sizeof(InstanceInformation);
			uptrInstanceBuffer->Init(instBufferInfo);
			uptrInstanceBuffer->CopyFromHost(instInfos.data());

			m_instanceBuffer.push_back(std::move(uptrInstanceBuffer));
		}
	}

	// Camera buffer
	{
		Buffer::Information cameraBufferInfo{};

		cameraBufferInfo.memoryProperty = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
		cameraBufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
		cameraBufferInfo.size = sizeof(CameraUBO);

		for (int i = 0; i < MAX_FRAME_COUNT; ++i)
		{
			std::unique_ptr<Buffer> uptrCamera = std::make_unique<Buffer>();

			uptrCamera->Init(cameraBufferInfo);

			// the buffer is update every frame, so I don't write to it here

			m_cameraBuffers.push_back(std::move(uptrCamera));
		}
	}
}

void RayQueryApp::_UninitBuffers()
{
	for (auto& vertexBuffers : m_vertexBuffers)
	{
		for (auto& uptrBuffer : vertexBuffers)
		{
			uptrBuffer->Uninit();
		}
		vertexBuffers.clear();
	}
	m_vertexBuffers.clear();

	for (auto& indexBuffers : m_indexBuffers)
	{
		for (auto& uptrBuffer : indexBuffers)
		{
			uptrBuffer->Uninit();
		}
		indexBuffers.clear();
	}
	m_indexBuffers.clear();

	for (auto& uptrBuffer : m_instanceBuffer)
	{
		uptrBuffer->Uninit();
	}
	m_instanceBuffer.clear();

	for (auto& uptrBuffer : m_cameraBuffers)
	{
		uptrBuffer->Uninit();
	}
	m_cameraBuffers.clear();
}

void RayQueryApp::_InitAS()
{
	m_rtAccelStruct.reserve(MAX_FRAME_COUNT);
	for (int j = 0; j < MAX_FRAME_COUNT; ++j)
	{
		std::vector<RayTracingAccelerationStructure::InstanceData> instDatas;
		RayTracingAccelerationStructure AS{};

		instDatas.reserve(m_models.size());
		for (int i = 0; i < m_models.size(); ++i)
		{
			RayTracingAccelerationStructure::TriangleData trigData{};
			RayTracingAccelerationStructure::InstanceData instData{};
			auto const& model = m_models[i];
			auto const& uptrIndexBuffer = m_indexBuffers[i];
			auto const& uptrVertexBuffer = m_vertexBuffers[i];

			trigData.uIndexCount = static_cast<uint32_t>(model.mesh.indices.size());
			trigData.vkIndexType = VK_INDEX_TYPE_UINT32;
			trigData.vkDeviceAddressIndex = uptrIndexBuffer[j]->GetDeviceAddress();

			trigData.uVertexCount = static_cast<uint32_t>(model.mesh.verts.size());
			trigData.uVertexStride = sizeof(VBO);
			trigData.vkDeviceAddressVertex = uptrVertexBuffer[j]->GetDeviceAddress();

			instData.uBLASIndex = AS.AddBLAS({ trigData });
			instData.transformMatrix = model.transform.GetModelMatrix();
			if (i == 0)
			{
				m_BLASInputs = { trigData };
			}

			instDatas.push_back(instData);
		}

		AS.SetUpTLAS(instDatas);
		AS.Init();
		m_rtAccelStruct.push_back(std::move(AS));
		m_TLASInputs = instDatas;
	}
}

void RayQueryApp::_UninitAS()
{
	for (int i = 0; i < MAX_FRAME_COUNT; ++i)
	{
		m_rtAccelStruct[i].Uninit();
	}
	m_rtAccelStruct.clear();
}

void RayQueryApp::_InitImagesAndViews()
{
	m_rtImages.clear();
	m_rtImages.reserve(MAX_FRAME_COUNT);
	for (int i = 0; i < MAX_FRAME_COUNT; ++i)
	{
		std::unique_ptr<Image> rtImage = std::make_unique<Image>();
		std::unique_ptr<ImageView> rtView{};
		Image::Information imgInfo{};

		imgInfo.arrayLayers = 1u;
		imgInfo.depth = 1u;
		imgInfo.format = VK_FORMAT_R32G32B32A32_SFLOAT;
		imgInfo.imageType = VK_IMAGE_TYPE_2D;
		imgInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imgInfo.memoryProperty = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		imgInfo.mipLevels = 1u;
		imgInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imgInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imgInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		imgInfo.width = MyDevice::GetInstance().GetSwapchainExtent().width;
		imgInfo.height = MyDevice::GetInstance().GetSwapchainExtent().height;

		rtImage->SetImageInformation(imgInfo);
		rtImage->Init();
		rtView = std::make_unique<ImageView>(rtImage->NewImageView());
		rtView->Init();

		m_rtImageViews.push_back(std::move(rtView));
		m_rtImages.push_back(std::move(rtImage));
	}
}

void RayQueryApp::_UninitImagesAndViews()
{
	for (auto& imgView : m_rtImageViews)
	{
		imgView->Uninit();
	}
	m_rtImageViews.clear();

	for (auto& img : m_rtImages)
	{
		img->Uninit();
	}
	m_rtImages.clear();
}

void RayQueryApp::_InitDescriptorSets()
{
	for (int i = 0; i < MAX_FRAME_COUNT; ++i)
	{
		std::unique_ptr<DescriptorSet> uptrDSet = std::make_unique<DescriptorSet>(m_rtDSetLayout.NewDescriptorSet());
		VkDescriptorImageInfo imageInfo{};

		imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
		imageInfo.imageView = m_rtImageViews[i]->vkImageView;
		imageInfo.sampler = m_vkSampler;

		uptrDSet->Init();
		uptrDSet->StartUpdate();
		uptrDSet->UpdateBinding(0, m_cameraBuffers[i].get());
		uptrDSet->UpdateBinding(1, { m_rtAccelStruct[i].vkAccelerationStructure });
		uptrDSet->UpdateBinding(2, imageInfo);
		uptrDSet->UpdateBinding(3, m_instanceBuffer[i].get());
		uptrDSet->FinishUpdate();

		m_rtDSets.push_back(std::move(uptrDSet));
	}

	for (int i = 0; i < MAX_FRAME_COUNT; ++i)
	{
		std::unique_ptr<DescriptorSet> uptrDSet = std::make_unique<DescriptorSet>(m_compDSetLayout.NewDescriptorSet());

		uptrDSet->Init();
		uptrDSet->StartUpdate();
		//uptrDSet->UpdateBinding(0, m_cameraBuffers[i].get());
		uptrDSet->UpdateBinding(0, m_vertexBuffers[0][i].get()); // sphere
		uptrDSet->FinishUpdate();

		m_compDSets.push_back(std::move(uptrDSet));
	}
}

void RayQueryApp::_UninitDescriptorSets()
{
	for (auto& uptrDSet : m_compDSets)
	{
		uptrDSet.reset();
	}
	m_compDSets.clear();

	for (auto& uptrDSet : m_rtDSets)
	{
		uptrDSet.reset();
	}
	m_rtDSets.clear();
}

void RayQueryApp::_InitPipelines()
{
	SimpleShader rgen{};
	SimpleShader rchit{};
	SimpleShader rchit2{};
	SimpleShader rmiss{};
	SimpleShader rmiss2{};
	SimpleShader anim{};
	uint32_t rgenId = 0u;
	uint32_t rchitId = 0u;
	uint32_t rchit2Id = 0u;
	uint32_t rmissId = 0u;
	uint32_t rmiss2Id = 0u;

	rgen.SetSPVFile("E:/GitStorage/LearnVulkan/bin/shaders/rt.rgen.spv");
	rchit.SetSPVFile("E:/GitStorage/LearnVulkan/bin/shaders/rt_shadow.rchit.spv");
	rchit2.SetSPVFile("E:/GitStorage/LearnVulkan/bin/shaders/rt_simple.rchit.spv");
	rmiss.SetSPVFile("E:/GitStorage/LearnVulkan/bin/shaders/rt.rmiss.spv");
	rmiss2.SetSPVFile("E:/GitStorage/LearnVulkan/bin/shaders/rt_shadow.rmiss.spv");
	anim.SetSPVFile("E:/GitStorage/LearnVulkan/bin/shaders/rt_anim.comp.spv");

	rgen.Init();
	rchit.Init();
	rchit2.Init();
	rmiss.Init();
	rmiss2.Init();
	anim.Init();

	m_rtPipeline.AddDescriptorSetLayout(m_rtDSetLayout.vkDescriptorSetLayout);
	rgenId = m_rtPipeline.AddShader(rgen.GetShaderStageInfo());
	rchitId = m_rtPipeline.AddShader(rchit.GetShaderStageInfo());
	rmissId = m_rtPipeline.AddShader(rmiss.GetShaderStageInfo());
	rmiss2Id = m_rtPipeline.AddShader(rmiss2.GetShaderStageInfo());
	rchit2Id = m_rtPipeline.AddShader(rchit2.GetShaderStageInfo());

	m_compPipeline.AddDescriptorSetLayout(&m_compDSetLayout);
	m_compPipeline.AddShader(anim.GetShaderStageInfo());
	m_compPipeline.AddPushConstant(VK_SHADER_STAGE_COMPUTE_BIT, sizeof(float));

	m_rtPipeline.SetRayGenerationShaderRecord(rgenId);
	m_rtPipeline.AddTriangleHitShaderRecord(rchitId);
	m_rtPipeline.AddTriangleHitShaderRecord(rchit2Id);
	m_rtPipeline.AddMissShaderRecord(rmissId);
	m_rtPipeline.AddMissShaderRecord(rmiss2Id);
	m_rtPipeline.SetMaxRecursion(2u);

	m_rtPipeline.Init();
	m_compPipeline.Init();

	rgen.Uninit();
	rchit.Uninit();
	rchit2.Uninit();
	rmiss.Uninit();
	rmiss2.Uninit();
	anim.Uninit();
}

void RayQueryApp::_UninitPipelines()
{
	m_compPipeline.Uninit();
	m_rtPipeline.Uninit();
}

void RayQueryApp::_InitSwapchainPass()
{
	_UninitSwapchainPass();
	std::vector<VkDescriptorImageInfo> imageInfos;
	imageInfos.reserve(MAX_FRAME_COUNT);
	for (int i = 0; i < MAX_FRAME_COUNT; ++i)
	{
		VkDescriptorImageInfo imageInfo{};
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.imageView = m_rtImageViews[i]->vkImageView;
		imageInfo.sampler = m_vkSampler;
		imageInfos.push_back(imageInfo);
	}
	m_swapchainPass = std::make_unique<SwapchainPass>();
	m_swapchainPass->PreSetPassThroughImages(imageInfos);
	m_swapchainPass->Init();
}

void RayQueryApp::_UninitSwapchainPass()
{
	if (m_swapchainPass.get() != nullptr)
	{
		m_swapchainPass->Uninit();
	}
	m_swapchainPass.reset();
}

void RayQueryApp::_InitCommandBuffers()
{
	for (int i = 0; i < MAX_FRAME_COUNT; ++i)
	{
		std::unique_ptr<CommandSubmission> uptrCmd = std::make_unique<CommandSubmission>();
		uptrCmd->Init();
		m_commandSubmissions.push_back(std::move(uptrCmd));
	}
}

void RayQueryApp::_UninitCommandBuffers()
{
	for (auto& uptrCmd : m_commandSubmissions)
	{
		if (uptrCmd.get() != nullptr)
		{
			uptrCmd->Uninit();
			uptrCmd.reset();
		}
	}
	m_commandSubmissions.clear();
}

void RayQueryApp::_UpdateUniformBuffer()
{
	static std::optional<float> lastX;
	static std::optional<float> lastY;
	UserInput userInput = MyDevice::GetInstance().GetUserInput();
	CameraUBO ubo{};
	VkExtent2D swapchainExtent = MyDevice::GetInstance().GetSwapchainExtent();
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

	//ubo.proj[1][1] *= -1;
	//ubo.F = glm::vec4(glm::normalize(m_camera.ref - m_camera.eye), 1.0f);
	//ubo.V = glm::vec4(m_camera.V, 1.0f);
	//ubo.H = glm::vec4(m_camera.H, 1.0f);
	ubo.inverseViewProj = glm::inverse(m_camera.GetViewProjectionMatrix());
	ubo.eye = glm::vec4(m_camera.eye, 1.0f);
	m_cameraBuffers[m_currentFrame]->CopyFromHost(&ubo);
	m_models[1].transform.SetRotation(0, 0, lastTime * 10);
}

void RayQueryApp::_MainLoop()
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

void RayQueryApp::_DrawFrame()
{
	std::unique_ptr<CommandSubmission>& cmd = m_commandSubmissions[m_currentFrame];
	auto& device = MyDevice::GetInstance();
	RayTracingPipeline::PipelineInput pipelineInput{};
	CommandSubmission::WaitInformation scPassWait{};
	ImageBarrierBuilder barrierBuilder{};
	VkClearColorValue clearColor{};
	clearColor.uint32[3] = 1.0f;
	if (device.NeedRecreateSwapchain())
	{
		_ResizeWindow();
	}

	cmd->WaitTillAvailable();
	_UpdateUniformBuffer();
	cmd->StartCommands({});

	for (int i = 0; i < m_TLASInputs.size(); ++i)
	{
		m_TLASInputs[i].transformMatrix = m_models[i].transform.GetModelMatrix();
	}

	// update vertex buffer with compute shader
	{
		ComputePipeline::PipelineInput input{};
		VkMemoryBarrier barrier{ VK_STRUCTURE_TYPE_MEMORY_BARRIER };
		float time = static_cast<float>(lastTime);
		input.groupCountX = m_models[0].mesh.verts.size();
		input.groupCountY = 1;
		input.groupCountZ = 1;
		input.pDescriptorSets = { m_compDSets[m_currentFrame].get() };
		input.pushConstants = { {VK_SHADER_STAGE_COMPUTE_BIT, &time} };
		m_compPipeline.Do(cmd->vkCommandBuffer, input);

		barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;

		cmd->AddPipelineBarrier(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, { barrier });
	}

	m_rtAccelStruct[m_currentFrame].UpdateBLAS(m_BLASInputs, 0, cmd.get());
	m_rtAccelStruct[m_currentFrame].UpdateTLAS(m_TLASInputs, cmd.get());
	RayTracingAccelerationStructure::RecordPipelineBarrier(cmd.get());

	// transfer image to general and fill it with zero
	{
		m_rtImages[m_currentFrame]->ChangeLayoutAndFill(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, clearColor, cmd.get());
		auto barrier2 = barrierBuilder.NewBarrier(
			m_rtImages[m_currentFrame]->vkImage,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			VK_IMAGE_LAYOUT_GENERAL,
			VK_ACCESS_TRANSFER_WRITE_BIT,
			VK_ACCESS_SHADER_WRITE_BIT);
		cmd->AddPipelineBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR, { barrier2 });
	}

	// run ray tracing pipeline
	pipelineInput.uDepth = 1u;
	pipelineInput.uWidth = device.GetSwapchainExtent().width;
	pipelineInput.uHeight = device.GetSwapchainExtent().height;
	pipelineInput.pDescriptorSets = { m_rtDSets[m_currentFrame].get() };
	m_rtPipeline.Do(cmd->vkCommandBuffer, pipelineInput);

	// transfer image to shader read only
	{
		auto barrier = barrierBuilder.NewBarrier(
			m_rtImages[m_currentFrame]->vkImage,
			VK_IMAGE_LAYOUT_GENERAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_ACCESS_SHADER_WRITE_BIT,
			VK_ACCESS_SHADER_READ_BIT);
		cmd->AddPipelineBarrier(VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, { barrier });
	}

	scPassWait.waitSamaphore = cmd->SubmitCommands();
	scPassWait.waitStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	m_swapchainPass->Do({ scPassWait });

	m_currentFrame = (m_currentFrame + 1) % MAX_FRAME_COUNT;
}

void RayQueryApp::_ResizeWindow()
{
	MyDevice::GetInstance().RecreateSwapchain();
	std::vector<VkDescriptorImageInfo> imageInfos;

	_UninitDescriptorSets();
	_UninitImagesAndViews();
	_InitImagesAndViews();
	_InitDescriptorSets();

	imageInfos.reserve(MAX_FRAME_COUNT);
	for (int i = 0; i < MAX_FRAME_COUNT; ++i)
	{
		VkDescriptorImageInfo imageInfo{};
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
		imageInfo.imageView = m_rtImageViews[i]->vkImageView;
		imageInfo.sampler = m_vkSampler;
		imageInfos.push_back(imageInfo);
	}
	m_swapchainPass->RecreateSwapchain(imageInfos);
}

RayQueryApp::RayQueryApp()
{
}

RayQueryApp::~RayQueryApp()
{
}

void RayQueryApp::Run()
{
	_Init();
	_MainLoop();
	_Uninit();
}