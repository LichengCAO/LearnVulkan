#include "ray_tracing_app.h"
#include "swapchain_pass.h"
#include "shader.h"
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
}

void RayTracingApp::_UninitDescriptorSetLayouts()
{
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
	std::vector<Mesh> outMeshes;
	MeshUtility::Load("E:/GitStorage/LearnVulkan/res/models/sphere/sphere.obj", outMeshes);
	for (auto const& mesh : outMeshes)
	{
		Model model{};
		model.transform.SetScale({ 0.5, 0.5, 0.5 });
		//model.transform.SetRotation({ -90, 0, 0 });
		model.mesh = mesh;
		m_models.push_back(model);
	}
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
			std::unique_ptr<Buffer> uptrVertexBuffer = std::make_unique<Buffer>();
			std::unique_ptr<Buffer> uptrIndexBuffer = std::make_unique<Buffer>();
			std::vector<VBO> vertData{};

			vertData.reserve(model.mesh.verts.size());
			for (auto const& vert : model.mesh.verts)
			{
				VBO curData{};
				curData.pos = vert.position;
				CHECK_TRUE(vert.normal.has_value(), "We need a normal here!");
				curData.normal = vert.normal.value();
				vertData.push_back(curData);
			}

			vertBufferInfo.size = vertData.size() * sizeof(VBO);
			uptrVertexBuffer->Init(vertBufferInfo);

			vertBufferInfo.size = model.mesh.indices.size() * sizeof(uint32_t);
			uptrIndexBuffer->Init(vertBufferInfo);

			uptrVertexBuffer->CopyFromHost(vertData.data());
			uptrIndexBuffer->CopyFromHost(model.mesh.indices.data());

			m_vertexBuffers.push_back(std::move(uptrVertexBuffer));
			m_indexBuffers.push_back(std::move(uptrIndexBuffer));
		}
	}

	// Instance buffer
	{
		Buffer::Information instBufferInfo{};
		std::vector<InstanceInformation> instInfos{};
		int n = m_models.size();

		instInfos.reserve(n);
		for (int i = 0; i < n; ++i)
		{
			InstanceInformation instInfo{};
			
			instInfo.vertexBuffer = m_vertexBuffers[i]->GetDeviceAddress();
			instInfo.indexBuffer = m_indexBuffers[i]->GetDeviceAddress();
			
			instInfos.push_back(instInfo);
		}

		instBufferInfo.memoryProperty = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		instBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
		instBufferInfo.size = instInfos.size() * sizeof(InstanceInformation);

		m_instanceBuffer = std::make_unique<Buffer>();
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

void RayTracingApp::_UninitBuffers()
{
	for (auto& uptrBuffer : m_vertexBuffers)
	{
		uptrBuffer->Uninit();
	}
	m_vertexBuffers.clear();
	for (auto& uptrBuffer : m_indexBuffers)
	{
		uptrBuffer->Uninit();
	}
	m_indexBuffers.clear();

	m_instanceBuffer->Uninit();

	for (auto& uptrBuffer : m_cameraBuffers)
	{
		uptrBuffer->Uninit();
	}
	m_cameraBuffers.clear();
}

void RayTracingApp::_InitAS()
{
	std::vector<RayTracingAccelerationStructure::InstanceData> instDatas;

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
		trigData.vkDeviceAddressIndex = uptrIndexBuffer->GetDeviceAddress();

		trigData.uVertexCount = static_cast<uint32_t>(model.mesh.verts.size());
		trigData.uVertexStride = sizeof(VBO);
		trigData.vkDeviceAddressVertex = uptrVertexBuffer->GetDeviceAddress();

		instData.uBLASIndex = m_rtAccelStruct.AddBLAS({ trigData });
		instData.transformMatrix = model.transform.GetModelMatrix();

		instDatas.push_back(instData);
	}

	m_rtAccelStruct.SetUpTLAS(instDatas);
	m_rtAccelStruct.Init();
}

void RayTracingApp::_UninitAS()
{
	m_rtAccelStruct.Uninit();
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
		uptrDSet->UpdateBinding(0, m_cameraBuffers[i].get());
		uptrDSet->UpdateBinding(1, { m_rtAccelStruct.vkAccelerationStructure });
		uptrDSet->UpdateBinding(2, imageInfo);
		uptrDSet->UpdateBinding(3, m_instanceBuffer.get());
		uptrDSet->FinishUpdate();

		m_rtDSets.push_back(std::move(uptrDSet));
	}
}

void RayTracingApp::_UninitDescriptorSets() 
{
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
	SimpleShader rmiss{};
	uint32_t rgenId = 0u;
	uint32_t rchitId = 0u;
	uint32_t rmissId = 0u;

	rgen.SetSPVFile("E:/GitStorage/LearnVulkan/bin/shaders/rt.rgen.spv");
	rchit.SetSPVFile("E:/GitStorage/LearnVulkan/bin/shaders/rt.rchit.spv");
	rmiss.SetSPVFile("E:/GitStorage/LearnVulkan/bin/shaders/rt.rmiss.spv");

	rgen.Init();
	rchit.Init();
	rmiss.Init();

	m_rtPipeline.AddDescriptorSetLayout(m_rtDSetLayout.vkDescriptorSetLayout);
	rgenId  = m_rtPipeline.AddShader(rgen.GetShaderStageInfo());
	rchitId = m_rtPipeline.AddShader(rchit.GetShaderStageInfo());
	rmissId = m_rtPipeline.AddShader(rmiss.GetShaderStageInfo());

	m_rtPipeline.SetRayGenerationShaderRecord(rgenId);
	m_rtPipeline.AddHitShaderRecord(rchitId);
	m_rtPipeline.AddMissShaderRecord(rmissId);
	m_rtPipeline.SetMaxRecursion(1u);

	m_rtPipeline.Init();

	rgen.Uninit();
	rchit.Uninit();
	rmiss.Uninit();
}

void RayTracingApp::_UninitPipelines()
{
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

	//ubo.proj[1][1] *= -1;
	//ubo.F = glm::vec4(glm::normalize(m_camera.ref - m_camera.eye), 1.0f);
	//ubo.V = glm::vec4(m_camera.V, 1.0f);
	//ubo.H = glm::vec4(m_camera.H, 1.0f);
	ubo.inverseViewProj = glm::inverse(m_camera.GetViewProjectionMatrix());
	ubo.eye = glm::vec4(m_camera.eye, 1.0f);
	m_cameraBuffers[m_currentFrame]->CopyFromHost(&ubo);
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
