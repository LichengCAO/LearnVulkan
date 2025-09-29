#include "ray_tracing_app.h"
#include "swapchain_pass.h"
#include "shader.h"
#include <random>
#define MAX_FRAME_COUNT 3

void RayTracingApp::_Init()
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

void RayTracingApp::_Uninit()
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

void RayTracingApp::_InitDescriptorSetLayouts()
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

void RayTracingApp::_UninitDescriptorSetLayouts()
{
	m_compDSetLayout.Uninit();
	m_rtDSetLayout.Uninit();
}

void RayTracingApp::_InitSampler()
{
	m_vkSampler = MyDevice::GetInstance().samplerPool.GetSampler(VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
}

void RayTracingApp::_UninitSampler()
{
	MyDevice::GetInstance().samplerPool.ReturnSampler(&m_vkSampler);
}

void RayTracingApp::_InitModels()
{
	std::vector<StaticMesh> outMeshes;
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

	m_models[0 + 1].transform.SetRotation(90, 0, 0);
	m_models[1 + 1].transform.SetRotation(90, 0, 0);
	m_models[2 + 1].transform.SetRotation(0, 0, 90);
}

void RayTracingApp::_InitBuffers()
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

void RayTracingApp::_UninitBuffers()
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

void RayTracingApp::_InitAS()
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

void RayTracingApp::_UninitAS()
{
	for (int i = 0; i < MAX_FRAME_COUNT; ++i)
	{
		m_rtAccelStruct[i].Uninit();
	}
	m_rtAccelStruct.clear();
}

void RayTracingApp::_InitImagesAndViews()
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

void RayTracingApp::_UninitImagesAndViews()
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

void RayTracingApp::_InitDescriptorSets()
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
		uptrDSet->UpdateBinding(0, { m_cameraBuffers[i]->GetDescriptorInfo() });
		uptrDSet->UpdateBinding(1, { m_rtAccelStruct[i].vkAccelerationStructure });
		uptrDSet->UpdateBinding(2, { imageInfo });
		uptrDSet->UpdateBinding(3, { m_instanceBuffer[i]->GetDescriptorInfo() });
		uptrDSet->FinishUpdate();

		m_rtDSets.push_back(std::move(uptrDSet));
	}

	for (int i = 0; i < MAX_FRAME_COUNT; ++i)
	{
		std::unique_ptr<DescriptorSet> uptrDSet = std::make_unique<DescriptorSet>(m_compDSetLayout.NewDescriptorSet());

		uptrDSet->Init();
		uptrDSet->StartUpdate();
		//uptrDSet->UpdateBinding(0, m_cameraBuffers[i].get());
		uptrDSet->UpdateBinding(0, { m_vertexBuffers[0][i]->GetDescriptorInfo() }); // sphere
		uptrDSet->FinishUpdate();

		m_compDSets.push_back(std::move(uptrDSet));
	}
}

void RayTracingApp::_UninitDescriptorSets() 
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

void RayTracingApp::_InitPipelines()
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
	rgenId  = m_rtPipeline.AddShader(rgen.GetShaderStageInfo());
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

void RayTracingApp::_UninitPipelines()
{
	m_compPipeline.Uninit();
	m_rtPipeline.Uninit();
}

void RayTracingApp::_InitSwapchainPass()
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

void RayTracingApp::_UninitSwapchainPass()
{
	if (m_swapchainPass.get() != nullptr)
	{
		m_swapchainPass->Uninit();
	}
	m_swapchainPass.reset();
}

void RayTracingApp::_InitCommandBuffers()
{
	for (int i = 0; i < MAX_FRAME_COUNT; ++i)
	{
		std::unique_ptr<CommandSubmission> uptrCmd = std::make_unique<CommandSubmission>();
		uptrCmd->Init();
		m_commandSubmissions.push_back(std::move(uptrCmd));
	}
}

void RayTracingApp::_UninitCommandBuffers()
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

void RayTracingApp::_UpdateUniformBuffer()
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

	ubo.inverseViewProj = glm::inverse(m_camera.GetViewProjectionMatrix());
	ubo.eye = glm::vec4(m_camera.eye, 1.0f);
	m_cameraBuffers[m_currentFrame]->CopyFromHost(&ubo);
	m_models[1].transform.SetRotation(0, 0, lastTime * 10);
}

void RayTracingApp::_MainLoop()
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

void RayTracingApp::_DrawFrame()
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
		input.vkDescriptorSets = { m_compDSets[m_currentFrame]->vkDescriptorSet };
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
	pipelineInput.vkDescriptorSets = { m_rtDSets[m_currentFrame]->vkDescriptorSet };
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

void RayTracingApp::_ResizeWindow()
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

RayTracingApp::RayTracingApp()
{
}

RayTracingApp::~RayTracingApp()
{
}

void RayTracingApp::Run()
{
	_Init();
	_MainLoop();
	_Uninit();
}

void RayTracingThousandsApp::_Init()
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

void RayTracingThousandsApp::_Uninit()
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

void RayTracingThousandsApp::_InitDescriptorSetLayouts()
{
	m_rtDSetLayout.AddBinding(1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_RAYGEN_BIT_KHR); // camera info
	m_rtDSetLayout.AddBinding(1, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_RAYGEN_BIT_KHR); // AS
	m_rtDSetLayout.AddBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_RAYGEN_BIT_KHR); // image output
	m_rtDSetLayout.AddBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR); // instance data
	m_rtDSetLayout.Init();
}

void RayTracingThousandsApp::_UninitDescriptorSetLayouts()
{
	m_rtDSetLayout.Uninit();
}

void RayTracingThousandsApp::_InitSampler()
{
	m_vkSampler = MyDevice::GetInstance().samplerPool.GetSampler(VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
}

void RayTracingThousandsApp::_UninitSampler()
{
	MyDevice::GetInstance().samplerPool.ReturnSampler(&m_vkSampler);
}

void RayTracingThousandsApp::_InitModels()
{
	std::vector<StaticMesh> outMeshes;
	std::random_device              rd;  // Will be used to obtain a seed for the random number engine
	std::mt19937                    gen(rd());  // Standard mersenne_twister_engine seeded with rd()
	std::normal_distribution<float> dis(1.0f, 1.0f);
	std::normal_distribution<float> disn(0.05f, 0.05f);
	
	MeshUtility::Load("E:/GitStorage/LearnVulkan/res/models/cube/cube.obj", outMeshes);

	for (uint32_t n = 0; n < 2000; ++n)
	{
		Model model{};
		float scale = fabsf(disn(gen));
		model.mesh = outMeshes[0];
		model.transform.SetPosition(dis(gen), 2.0f + dis(gen), dis(gen));
		model.transform.SetScale(scale, scale, scale);

		m_models.push_back(model);
	}
}

void RayTracingThousandsApp::_InitBuffers()
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

void RayTracingThousandsApp::_UninitBuffers()
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

void RayTracingThousandsApp::_InitAS()
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

			instDatas.push_back(instData);
		}

		AS.SetUpTLAS(instDatas);
		AS.Init();
		m_rtAccelStruct.push_back(std::move(AS));
	}
}

void RayTracingThousandsApp::_UninitAS()
{
	for (int i = 0; i < MAX_FRAME_COUNT; ++i)
	{
		m_rtAccelStruct[i].Uninit();
	}
	m_rtAccelStruct.clear();
}

void RayTracingThousandsApp::_InitImagesAndViews()
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

void RayTracingThousandsApp::_UninitImagesAndViews()
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

void RayTracingThousandsApp::_InitDescriptorSets()
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
		uptrDSet->UpdateBinding(0, { m_cameraBuffers[i]->GetDescriptorInfo() });
		uptrDSet->UpdateBinding(1, { m_rtAccelStruct[i].vkAccelerationStructure });
		uptrDSet->UpdateBinding(2, { imageInfo });
		uptrDSet->UpdateBinding(3, { m_instanceBuffer[i]->GetDescriptorInfo() });
		uptrDSet->FinishUpdate();

		m_rtDSets.push_back(std::move(uptrDSet));
	}
}

void RayTracingThousandsApp::_UninitDescriptorSets()
{
	for (auto& uptrDSet : m_rtDSets)
	{
		uptrDSet.reset();
	}
	m_rtDSets.clear();
}

void RayTracingThousandsApp::_InitPipelines()
{
	SimpleShader rgen{};
	SimpleShader rchit{};
	SimpleShader rchit2{};
	SimpleShader rmiss{};
	SimpleShader rmiss2{};
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

	rgen.Init();
	rchit.Init();
	rchit2.Init();
	rmiss.Init();
	rmiss2.Init();

	m_rtPipeline.AddDescriptorSetLayout(m_rtDSetLayout.vkDescriptorSetLayout);
	rgenId = m_rtPipeline.AddShader(rgen.GetShaderStageInfo());
	rchitId = m_rtPipeline.AddShader(rchit.GetShaderStageInfo());
	rmissId = m_rtPipeline.AddShader(rmiss.GetShaderStageInfo());
	rmiss2Id = m_rtPipeline.AddShader(rmiss2.GetShaderStageInfo());
	rchit2Id = m_rtPipeline.AddShader(rchit2.GetShaderStageInfo());

	m_rtPipeline.SetRayGenerationShaderRecord(rgenId);
	m_rtPipeline.AddTriangleHitShaderRecord(rchitId);
	m_rtPipeline.AddTriangleHitShaderRecord(rchit2Id);
	m_rtPipeline.AddMissShaderRecord(rmissId);
	m_rtPipeline.AddMissShaderRecord(rmiss2Id);
	m_rtPipeline.SetMaxRecursion(2u);

	m_rtPipeline.Init();

	rgen.Uninit();
	rchit.Uninit();
	rchit2.Uninit();
	rmiss.Uninit();
	rmiss2.Uninit();
}

void RayTracingThousandsApp::_UninitPipelines()
{
	m_rtPipeline.Uninit();
}

void RayTracingThousandsApp::_InitSwapchainPass()
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

void RayTracingThousandsApp::_UninitSwapchainPass()
{
	if (m_swapchainPass.get() != nullptr)
	{
		m_swapchainPass->Uninit();
	}
	m_swapchainPass.reset();
}

void RayTracingThousandsApp::_InitCommandBuffers()
{
	for (int i = 0; i < MAX_FRAME_COUNT; ++i)
	{
		std::unique_ptr<CommandSubmission> uptrCmd = std::make_unique<CommandSubmission>();
		uptrCmd->Init();
		m_commandSubmissions.push_back(std::move(uptrCmd));
	}
}

void RayTracingThousandsApp::_UninitCommandBuffers()
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

void RayTracingThousandsApp::_UpdateUniformBuffer()
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

	ubo.inverseViewProj = glm::inverse(m_camera.GetViewProjectionMatrix());
	ubo.eye = glm::vec4(m_camera.eye, 1.0f);
	m_cameraBuffers[m_currentFrame]->CopyFromHost(&ubo);
}

void RayTracingThousandsApp::_MainLoop()
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

void RayTracingThousandsApp::_DrawFrame()
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
	pipelineInput.vkDescriptorSets = { m_rtDSets[m_currentFrame]->vkDescriptorSet };
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

void RayTracingThousandsApp::_ResizeWindow()
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

RayTracingThousandsApp::RayTracingThousandsApp()
{
}

RayTracingThousandsApp::~RayTracingThousandsApp()
{
}

void RayTracingThousandsApp::Run()
{
	_Init();
	_MainLoop();
	_Uninit();
}

void RayTracingAABBsApp::_Init()
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

void RayTracingAABBsApp::_Uninit()
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

void RayTracingAABBsApp::_InitDescriptorSetLayouts()
{
	m_rtDSetLayout.AddBinding(1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_RAYGEN_BIT_KHR); // camera info
	m_rtDSetLayout.AddBinding(1, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_RAYGEN_BIT_KHR); // AS
	m_rtDSetLayout.AddBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_RAYGEN_BIT_KHR); // image output
	m_rtDSetLayout.AddBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR); // instance data
	m_rtDSetLayout.AddBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_INTERSECTION_BIT_KHR); // sphere data
	m_rtDSetLayout.Init();
}

void RayTracingAABBsApp::_UninitDescriptorSetLayouts()
{
	m_rtDSetLayout.Uninit();
}

void RayTracingAABBsApp::_InitSampler()
{
	m_vkSampler = MyDevice::GetInstance().samplerPool.GetSampler(VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
}

void RayTracingAABBsApp::_UninitSampler()
{
	MyDevice::GetInstance().samplerPool.ReturnSampler(&m_vkSampler);
}

void RayTracingAABBsApp::_InitModels()
{
	std::vector<StaticMesh> outMeshes;
	Model modelChessBoard{};
	MeshUtility::Load("E:/GitStorage/LearnVulkan/res/models/ChessBoard/ChessBoard.obj", outMeshes);

	modelChessBoard.mesh = outMeshes[0];
	modelChessBoard.transform.SetRotation(0, 0, 90);
	modelChessBoard.transform.SetPosition(0, 0, -2);
	modelChessBoard.transform.SetScale(2, 2, 1);

	m_models.push_back(std::move(modelChessBoard));
}

void RayTracingAABBsApp::_InitBuffers()
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
			std::vector<VBO> vertData{};
			std::unique_ptr<Buffer> uptrVertexBuffer = std::make_unique<Buffer>();
			std::unique_ptr<Buffer> uptrIndexBuffer = std::make_unique<Buffer>();
			vertData.reserve(model.mesh.verts.size());
			for (auto const& vert : model.mesh.verts)
			{
				VBO vbo{};
				vbo.position = glm::vec4(vert.position, 1.0f);
				vbo.normal = vert.normal.has_value() ? glm::vec4(vert.normal.value(), 0) : glm::vec4(0, 0, 1, 0);
				vertData.push_back(std::move(vbo));
			}
			vertBufferInfo.size = vertData.size() * sizeof(VBO);
			uptrVertexBuffer->Init(vertBufferInfo);

			vertBufferInfo.size = model.mesh.indices.size() * sizeof(uint32_t);
			uptrIndexBuffer->Init(vertBufferInfo);

			uptrVertexBuffer->CopyFromHost(vertData.data());
			uptrIndexBuffer->CopyFromHost(model.mesh.indices.data());

			m_vertBuffers.push_back(std::move(uptrVertexBuffer));
			m_indexBuffers.push_back(std::move(uptrIndexBuffer));
		}
	}

	// AABB
	{
		Buffer::Information bufferInfo;
		size_t sphereCount = s_SphereCount;
		std::random_device                    rd{};
		std::mt19937                          gen{ rd() };
		std::normal_distribution<float>       xzd{ 0.f, 5.f };
		std::normal_distribution<float>       yd{ 6.f, 3.f };
		std::uniform_real_distribution<float> radd{ .05f, .2f };
		std::vector<glm::vec4> spheres;
		std::vector<AABB> AABBs;

		m_AABBsBuffer = std::make_unique<Buffer>();
		m_spheresBuffer = std::make_unique<Buffer>();

		bufferInfo.memoryProperty = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		bufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT 
			| VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR
			| VK_BUFFER_USAGE_TRANSFER_DST_BIT;

		spheres.reserve(sphereCount);
		AABBs.reserve(sphereCount);

		for (size_t i = 0; i < sphereCount; ++i)
		{
			glm::vec4 sphere = glm::vec4(xzd(gen), yd(gen), xzd(gen), radd(gen));
			AABB aabb{};
			
			aabb.minX = sphere.x - sphere.w;
			aabb.minY = sphere.y - sphere.w;
			aabb.minZ = sphere.z - sphere.w;
			aabb.maxX = sphere.x + sphere.w;
			aabb.maxY = sphere.y + sphere.w;
			aabb.maxZ = sphere.z + sphere.w;
			
			spheres.push_back(std::move(sphere));
			AABBs.push_back(std::move(aabb));
		}

		bufferInfo.size = static_cast<uint32_t>(spheres.size()) * sizeof(glm::vec4);
		m_spheresBuffer->Init(bufferInfo);
		m_spheresBuffer->CopyFromHost(spheres.data());

		bufferInfo.usage |= VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
		bufferInfo.size = static_cast<uint32_t>(AABBs.size()) * sizeof(AABB);
		m_AABBsBuffer->Init(bufferInfo);
		m_AABBsBuffer->CopyFromHost(AABBs.data());
	}

	// Instance buffer
	{
		Buffer::Information instBufferInfo{};
		std::vector<InstanceInformation> instInfos{};
		m_instanceBuffer = std::make_unique<Buffer>();

		instInfos.reserve(m_models.size());
		for (size_t i = 0; i < m_models.size(); ++i)
		{
			InstanceInformation instInfo{};
			instInfo.vertexBuffer = m_vertBuffers[i]->GetDeviceAddress();
			instInfo.indexBuffer = m_indexBuffers[i]->GetDeviceAddress();
			instInfos.push_back(std::move(instInfo));
		}

		instBufferInfo.memoryProperty = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		instBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
		instBufferInfo.size = instInfos.size() * sizeof(InstanceInformation);

		m_instanceBuffer->Init(instBufferInfo);
		m_instanceBuffer->CopyFromHost(instInfos.data());
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

void RayTracingAABBsApp::_UninitBuffers()
{
	for (auto& uptrBuffer : m_cameraBuffers)
	{
		uptrBuffer->Uninit();
	}
	m_cameraBuffers.clear();

	m_instanceBuffer->Uninit();
	m_instanceBuffer.reset();

	m_AABBsBuffer->Uninit();
	m_AABBsBuffer.reset();

	m_spheresBuffer->Uninit();
	m_spheresBuffer.reset();

	for (auto& uptrBuffer : m_indexBuffers)
	{
		uptrBuffer->Uninit();
	}
	m_indexBuffers.clear();

	for (auto& uptrBuffer : m_vertBuffers)
	{
		uptrBuffer->Uninit();
	}
	m_vertBuffers.clear();
}

void RayTracingAABBsApp::_InitAS()
{
	std::vector<RayTracingAccelerationStructure::InstanceData> instDatas;
	RayTracingAccelerationStructure AS{};

	instDatas.reserve(m_models.size() + 1); // one for the BLAS that stores all AABBs
	CHECK_TRUE(m_vertBuffers.size() == m_indexBuffers.size(), "Length of index and vertex buffers should be same!");
	CHECK_TRUE(m_models.size() == m_indexBuffers.size(), "Length of index and model buffers should be same!");
	for (size_t i = 0; i < m_models.size(); ++i)
	{
		RayTracingAccelerationStructure::TriangleData trigData{};
		RayTracingAccelerationStructure::InstanceData instData{};
		auto const& model = m_models[i];
		auto const& uptrIndexBuffer = m_indexBuffers[i];
		auto const& uptrVertexBuffer = m_vertBuffers[i];

		trigData.uIndexCount = static_cast<uint32_t>(model.mesh.indices.size());
		trigData.vkIndexType = VK_INDEX_TYPE_UINT32;
		trigData.vkDeviceAddressIndex = uptrIndexBuffer->GetDeviceAddress();

		trigData.uVertexCount = static_cast<uint32_t>(model.mesh.verts.size());
		trigData.uVertexStride = sizeof(VBO);
		trigData.vkDeviceAddressVertex = uptrVertexBuffer->GetDeviceAddress();

		instData.uBLASIndex = AS.AddBLAS({ trigData });
		instData.transformMatrix = model.transform.GetModelMatrix();

		instDatas.push_back(instData);
	}

	{
		RayTracingAccelerationStructure::InstanceData instData{};
		RayTracingAccelerationStructure::AABBData aabbData{};

		aabbData.uAABBCount = static_cast<uint32_t>(s_SphereCount);
		aabbData.uAABBStride = sizeof(AABB);
		aabbData.vkDeviceAddressAABB = m_AABBsBuffer->GetDeviceAddress();

		instData.transformMatrix = glm::identity<glm::mat4>();
		instData.uBLASIndex = AS.AddBLAS({ aabbData });
		instData.uHitShaderGroupIndex = 1;

		instDatas.push_back(std::move(instData));
	}

	AS.SetUpTLAS(instDatas);
	AS.Init();
	m_rtAccelStruct = std::move(AS);
}

void RayTracingAABBsApp::_UninitAS()
{
	m_rtAccelStruct.Uninit();
}

void RayTracingAABBsApp::_InitImagesAndViews()
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

void RayTracingAABBsApp::_UninitImagesAndViews()
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

void RayTracingAABBsApp::_InitDescriptorSets()
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
		uptrDSet->UpdateBinding(0, { m_cameraBuffers[i]->GetDescriptorInfo() });
		uptrDSet->UpdateBinding(1, { m_rtAccelStruct.vkAccelerationStructure });
		uptrDSet->UpdateBinding(2, { imageInfo });
		uptrDSet->UpdateBinding(3, { m_instanceBuffer->GetDescriptorInfo() });
		uptrDSet->UpdateBinding(4, { m_spheresBuffer->GetDescriptorInfo() });
		uptrDSet->FinishUpdate();

		m_rtDSets.push_back(std::move(uptrDSet));
	}
}

void RayTracingAABBsApp::_UninitDescriptorSets()
{
	for (auto& uptrDSet : m_rtDSets)
	{
		uptrDSet.reset();
	}
	m_rtDSets.clear();
}

void RayTracingAABBsApp::_InitPipelines()
{
	SimpleShader rgen{};
	SimpleShader rchit{};
	SimpleShader rchit2{};
	SimpleShader rmiss{};
	SimpleShader rmiss2{};
	SimpleShader rint{};
	uint32_t rgenId = 0u;
	uint32_t rchitId = 0u;
	uint32_t rchit2Id = 0u;
	uint32_t rmissId = 0u;
	uint32_t rmiss2Id = 0u;
	uint32_t rintId = 0u;

	rgen.SetSPVFile("E:/GitStorage/LearnVulkan/bin/shaders/rt.rgen.spv");
	rchit.SetSPVFile("E:/GitStorage/LearnVulkan/bin/shaders/rt_shadow.rchit.spv");
	rchit2.SetSPVFile("E:/GitStorage/LearnVulkan/bin/shaders/rt_implicit.rchit.spv");
	rmiss.SetSPVFile("E:/GitStorage/LearnVulkan/bin/shaders/rt.rmiss.spv");
	rmiss2.SetSPVFile("E:/GitStorage/LearnVulkan/bin/shaders/rt_shadow.rmiss.spv");
	rint.SetSPVFile("E:/GitStorage/LearnVulkan/bin/shaders/rt_implicit.rint.spv");

	rgen.Init();
	rchit.Init();
	rchit2.Init();
	rmiss.Init();
	rmiss2.Init();
	rint.Init();

	m_rtPipeline.AddDescriptorSetLayout(m_rtDSetLayout.vkDescriptorSetLayout);
	rgenId = m_rtPipeline.AddShader(rgen.GetShaderStageInfo());
	rchitId = m_rtPipeline.AddShader(rchit.GetShaderStageInfo());
	rmissId = m_rtPipeline.AddShader(rmiss.GetShaderStageInfo());
	rmiss2Id = m_rtPipeline.AddShader(rmiss2.GetShaderStageInfo());
	rchit2Id = m_rtPipeline.AddShader(rchit2.GetShaderStageInfo());
	rintId = m_rtPipeline.AddShader(rint.GetShaderStageInfo());

	m_rtPipeline.SetRayGenerationShaderRecord(rgenId);
	m_rtPipeline.AddTriangleHitShaderRecord(rchitId);
	m_rtPipeline.AddProceduralHitShaderRecord(rchit2Id, rintId);
	m_rtPipeline.AddMissShaderRecord(rmissId);
	m_rtPipeline.AddMissShaderRecord(rmiss2Id);
	m_rtPipeline.SetMaxRecursion(2u);

	m_rtPipeline.Init();

	rint.Uninit();
	rgen.Uninit();
	rchit.Uninit();
	rchit2.Uninit();
	rmiss.Uninit();
	rmiss2.Uninit();
}

void RayTracingAABBsApp::_UninitPipelines()
{
	m_rtPipeline.Uninit();
}

void RayTracingAABBsApp::_InitSwapchainPass()
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

void RayTracingAABBsApp::_UninitSwapchainPass()
{
	if (m_swapchainPass.get() != nullptr)
	{
		m_swapchainPass->Uninit();
	}
	m_swapchainPass.reset();
}

void RayTracingAABBsApp::_InitCommandBuffers()
{
	for (int i = 0; i < MAX_FRAME_COUNT; ++i)
	{
		std::unique_ptr<CommandSubmission> uptrCmd = std::make_unique<CommandSubmission>();
		uptrCmd->Init();
		m_commandSubmissions.push_back(std::move(uptrCmd));
	}
}

void RayTracingAABBsApp::_UninitCommandBuffers()
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

void RayTracingAABBsApp::_UpdateUniformBuffer()
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

	ubo.inverseViewProj = glm::inverse(m_camera.GetViewProjectionMatrix());
	ubo.eye = glm::vec4(m_camera.eye, 1.0f);
	m_cameraBuffers[m_currentFrame]->CopyFromHost(&ubo);
}

void RayTracingAABBsApp::_MainLoop()
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

void RayTracingAABBsApp::_DrawFrame()
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
	pipelineInput.vkDescriptorSets = { m_rtDSets[m_currentFrame]->vkDescriptorSet };
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

void RayTracingAABBsApp::_ResizeWindow()
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

RayTracingAABBsApp::RayTracingAABBsApp()
{
}

RayTracingAABBsApp::~RayTracingAABBsApp()
{
}

void RayTracingAABBsApp::Run()
{
	_Init();
	_MainLoop();
	_Uninit();
}
