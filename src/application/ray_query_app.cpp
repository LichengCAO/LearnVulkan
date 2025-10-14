#include "ray_query_app.h"
#include "device.h"
#include "shader.h"
#include "swapchain_pass.h"

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
	_InitRenderPass();
	_InitFramebuffers();
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
	_UninitFramebuffers();
	_UninitRenderPass();
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
	m_rtDSetLayout.AddBinding(1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT); // camera info
	m_rtDSetLayout.AddBinding(1, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, VK_SHADER_STAGE_FRAGMENT_BIT); // AS
	m_rtDSetLayout.AddBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT); // instance data
	m_rtDSetLayout.Init();

	m_modelDSetLayout.AddBinding(1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT); // object model matrix
	m_modelDSetLayout.Init();
}

void RayQueryApp::_UninitDescriptorSetLayouts()
{
	m_modelDSetLayout.Uninit();
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
	std::vector<StaticMesh> outMeshes;
	MeshUtility::Load("E:/GitStorage/LearnVulkan/res/models/wahoo/wahoo.obj", outMeshes);
	MeshUtility::Load("E:/GitStorage/LearnVulkan/res/models/ChessBoard/ChessBoard.obj", outMeshes);

	for (auto const& mesh : outMeshes)
	{
		Model model{};
		model.mesh = mesh;
		m_models.push_back(model);
	}
	
	m_models[1].transform.SetScale(0.1, 0.1, 0.1);

	m_models[0].transform.SetPosition(6, 0, 1);
	m_models[1].transform.SetPosition(0, 0, -2);

	m_models[0].transform.SetRotation(90, 0, 180);
	m_models[1].transform.SetRotation(0, 0, 90);
}

void RayQueryApp::_InitBuffers()
{
	// vertex, index, model buffers
	{
		Buffer::Information vertBufferInfo{};
		Buffer::Information indexBufferInfo{};
		Buffer::Information modelBufferInfo{};

		vertBufferInfo.memoryProperty = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		vertBufferInfo.usage =
			VK_BUFFER_USAGE_TRANSFER_DST_BIT
			| VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
			| VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
			| VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR
			| VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

		indexBufferInfo.memoryProperty = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		indexBufferInfo.usage = 
			VK_BUFFER_USAGE_TRANSFER_DST_BIT
			| VK_BUFFER_USAGE_INDEX_BUFFER_BIT
			| VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
			| VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR
			| VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

		modelBufferInfo.memoryProperty = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		modelBufferInfo.usage =
			VK_BUFFER_USAGE_TRANSFER_DST_BIT
			| VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

		for (auto const& model : m_models)
		{
			std::unique_ptr<Buffer> uptrVertexBuffer = std::make_unique<Buffer>();
			std::unique_ptr<Buffer> uptrIndexBuffer = std::make_unique<Buffer>();
			std::unique_ptr<Buffer> uptrModelBuffer = std::make_unique<Buffer>();
			std::vector<VBO> vertData{};

			vertData.reserve(model.mesh.verts.size());
			for (auto const& vert : model.mesh.verts)
			{
				VBO curData{};
				curData.position = glm::vec4(vert.position, 1.0f);
				CHECK_TRUE(vert.normal.has_value(), "We need a normal here!");
				curData.normal = glm::vec4(vert.normal.value(), 0.0f);
				vertData.push_back(curData);
			}

			vertBufferInfo.size = vertData.size() * sizeof(VBO);
			uptrVertexBuffer->Init(vertBufferInfo);

			indexBufferInfo.size = model.mesh.indices.size() * sizeof(uint32_t);
			uptrIndexBuffer->Init(indexBufferInfo);

			modelBufferInfo.size = sizeof(ObjectUBO);
			uptrModelBuffer->Init(modelBufferInfo);

			uptrVertexBuffer->CopyFromHost(vertData.data());
			uptrIndexBuffer->CopyFromHost(model.mesh.indices.data());
			{
				ObjectUBO ubo{};
				ubo.model = model.transform.GetModelMatrix();
				ubo.normalModel = model.transform.GetModelInverseTransposeMatrix();
				uptrModelBuffer->CopyFromHost(&ubo);
			}

			m_vertexBuffers.push_back(std::move(uptrVertexBuffer));
			m_indexBuffers.push_back(std::move(uptrIndexBuffer));
			m_modelBuffers.push_back(std::move(uptrModelBuffer));
		}
	}

	// Instance buffer
	{
		int n = m_models.size();
		Buffer::Information instBufferInfo{};
		std::vector<InstanceInformation> instInfos{};
		std::unique_ptr<Buffer> uptrInstanceBuffer = std::make_unique<Buffer>();

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
		uptrInstanceBuffer->Init(instBufferInfo);
		uptrInstanceBuffer->CopyFromHost(instInfos.data());

		m_instanceBuffer = std::move(uptrInstanceBuffer);
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
	for (auto& uptrBuffer : m_cameraBuffers)
	{
		uptrBuffer->Uninit();
	}
	m_cameraBuffers.clear();

	m_instanceBuffer->Uninit();
	m_instanceBuffer.reset();

	for (auto& uptrBuffer : m_modelBuffers)
	{
		uptrBuffer->Uninit();
	}
	m_modelBuffers.clear();

	for (auto& uptrBuffer : m_indexBuffers)
	{
		uptrBuffer->Uninit();
	}
	m_indexBuffers.clear();

	for (auto& uptrBuffer : m_vertexBuffers)
	{
		uptrBuffer->Uninit();
	}
	m_vertexBuffers.clear();
}

void RayQueryApp::_InitAS()
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
		trigData.vkDeviceAddressIndex = uptrIndexBuffer->GetDeviceAddress();

		trigData.uVertexCount = static_cast<uint32_t>(model.mesh.verts.size());
		trigData.uVertexStride = sizeof(VBO);
		trigData.vkDeviceAddressVertex = uptrVertexBuffer->GetDeviceAddress();

		instData.uBLASIndex = AS.AddBLAS({ trigData });
		instData.transformMatrix = model.transform.GetModelMatrix();

		instDatas.push_back(instData);
	}

	AS.SetUpTLAS(instDatas);
	AS.Init();
	m_rtAccelStruct = std::move(AS);
}

void RayQueryApp::_UninitAS()
{
	m_rtAccelStruct.Uninit();
}

void RayQueryApp::_InitImagesAndViews()
{
	for (int i = 0; i < MAX_FRAME_COUNT; ++i)
	{
		std::unique_ptr<Image> uptrColorImage = std::make_unique<Image>();
		std::unique_ptr<Image> uptrDepthImage = std::make_unique<Image>();
		std::unique_ptr<ImageView> uptrColorView{};
		std::unique_ptr<ImageView> uptrDepthView{};
		Image::Information colorImageInfo{};
		Image::Information depthImageInfo{};

		colorImageInfo.arrayLayers = 1u;
		colorImageInfo.depth = 1u;
		colorImageInfo.format = VK_FORMAT_R32G32B32A32_SFLOAT;
		colorImageInfo.imageType = VK_IMAGE_TYPE_2D;
		colorImageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorImageInfo.memoryProperty = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		colorImageInfo.mipLevels = 1u;
		colorImageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		colorImageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		colorImageInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		colorImageInfo.width = MyDevice::GetInstance().GetSwapchainExtent().width;
		colorImageInfo.height = MyDevice::GetInstance().GetSwapchainExtent().height;

		depthImageInfo.arrayLayers = 1u;
		depthImageInfo.depth = 1u;
		depthImageInfo.format = MyDevice::GetInstance().GetDepthFormat();
		depthImageInfo.imageType = VK_IMAGE_TYPE_2D;
		depthImageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		depthImageInfo.memoryProperty = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		depthImageInfo.mipLevels = 1u;
		depthImageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		depthImageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		depthImageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		depthImageInfo.width = MyDevice::GetInstance().GetSwapchainExtent().width;
		depthImageInfo.height = MyDevice::GetInstance().GetSwapchainExtent().height;

		uptrColorImage->SetImageInformation(colorImageInfo);
		uptrColorImage->Init();
		uptrColorView = std::make_unique<ImageView>(uptrColorImage->NewImageView());
		uptrColorView->Init();

		uptrDepthImage->SetImageInformation(depthImageInfo);
		uptrDepthImage->Init();
		uptrDepthView = std::make_unique<ImageView>(uptrDepthImage->NewImageView(VK_IMAGE_ASPECT_DEPTH_BIT));
		uptrDepthView->Init();

		m_vecColorImageView.push_back(std::move(uptrColorView));
		m_vecColorImage.push_back(std::move(uptrColorImage));
		m_vecDepthImageView.push_back(std::move(uptrDepthView));
		m_vecDepthImage.push_back(std::move(uptrDepthImage));
	}
}

void RayQueryApp::_UninitImagesAndViews()
{
	for (auto& uptrView : m_vecDepthImageView)
	{
		uptrView->Uninit();
	}
	m_vecDepthImageView.clear();

	for (auto& uptrImage : m_vecDepthImage)
	{
		uptrImage->Uninit();
	}
	m_vecDepthImage.clear();

	for (auto& imgView : m_vecColorImageView)
	{
		imgView->Uninit();
	}
	m_vecColorImageView.clear();

	for (auto& img : m_vecColorImage)
	{
		img->Uninit();
	}
	m_vecColorImage.clear();
}

void RayQueryApp::_InitRenderPass()
{
	RenderPass::Subpass subpass{};

	m_uptrRenderPass = std::make_unique<RenderPass>();

	m_uptrRenderPass->AddAttachment(RenderPass::AttachmentPreset::COLOR_OUTPUT);
	m_uptrRenderPass->AddAttachment(RenderPass::AttachmentPreset::DEPTH);

	subpass.AddColorAttachment(0);
	subpass.SetDepthStencilAttachment(1);

	m_uptrRenderPass->AddSubpass(subpass);

	m_uptrRenderPass->Init();
}

void RayQueryApp::_UninitRenderPass()
{
	m_uptrRenderPass->Uninit();
	m_uptrRenderPass.reset();
}

void RayQueryApp::_InitFramebuffers()
{
	for (size_t i = 0; i < MAX_FRAME_COUNT; ++i)
	{
		std::unique_ptr<Framebuffer> uptrFramebuffer;
		std::vector<const ImageView*> vecRenderView = {
			m_vecColorImageView[i].get(),
			m_vecDepthImageView[i].get(),
		};
		uptrFramebuffer = std::make_unique<Framebuffer>(m_uptrRenderPass->NewFramebuffer(vecRenderView));
		uptrFramebuffer->Init();
		m_vecFramebuffer.push_back(std::move(uptrFramebuffer));
	}
}

void RayQueryApp::_UninitFramebuffers()
{
	for (auto& uptrBuffer : m_vecFramebuffer)
	{
		uptrBuffer->Uninit();
	}
	m_vecFramebuffer.clear();
}

void RayQueryApp::_InitDescriptorSets()
{
	for (size_t i = 0; i < MAX_FRAME_COUNT; ++i)
	{
		std::unique_ptr<DescriptorSet> uptrDSet = std::make_unique<DescriptorSet>(m_rtDSetLayout.NewDescriptorSet());
		VkDescriptorImageInfo imageInfo{};

		imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
		imageInfo.imageView = m_vecColorImageView[i]->vkImageView;
		imageInfo.sampler = m_vkSampler;

		uptrDSet->Init();
		uptrDSet->StartUpdate();
		uptrDSet->UpdateBinding(0, { m_cameraBuffers[i]->GetDescriptorInfo() });
		uptrDSet->UpdateBinding(1, { m_rtAccelStruct.vkAccelerationStructure });
		uptrDSet->UpdateBinding(2, { m_instanceBuffer->GetDescriptorInfo() });
		uptrDSet->FinishUpdate();

		m_rtDSets.push_back(std::move(uptrDSet));
	}

	for (size_t i = 0; i < m_models.size(); ++i)
	{
		std::unique_ptr<DescriptorSet> uptrDSet = std::make_unique<DescriptorSet>(m_modelDSetLayout.NewDescriptorSet());

		uptrDSet->Init();
		uptrDSet->StartUpdate();
		uptrDSet->UpdateBinding(0, { m_modelBuffers[i]->GetDescriptorInfo() });
		uptrDSet->FinishUpdate();

		m_modelDSets.push_back(std::move(uptrDSet));
	}
}

void RayQueryApp::_UninitDescriptorSets()
{
	for (auto& uptrDSet : m_modelDSets)
	{
		uptrDSet.reset();
	}
	m_modelDSets.clear();

	for (auto& uptrDSet : m_rtDSets)
	{
		uptrDSet.reset();
	}
	m_rtDSets.clear();
}

void RayQueryApp::_InitPipelines()
{
	SimpleShader vertShader{};
	SimpleShader fragShader{};
	VertexInputLayout vertInputLayout{};

	vertShader.SetSPVFile("E:/GitStorage/LearnVulkan/bin/shaders/rq.vert.spv");
	fragShader.SetSPVFile("E:/GitStorage/LearnVulkan/bin/shaders/rq.frag.spv");

	vertShader.Init();
	fragShader.Init();

	vertInputLayout.AddLocation(VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(VBO, position));
	vertInputLayout.AddLocation(VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(VBO, normal));
	vertInputLayout.SetUpVertex(sizeof(VBO));

	m_graphicPipeline.AddDescriptorSetLayout(m_rtDSetLayout.vkDescriptorSetLayout);
	m_graphicPipeline.AddDescriptorSetLayout(m_modelDSetLayout.vkDescriptorSetLayout);
	m_graphicPipeline.AddVertexInputLayout(&vertInputLayout);

	m_graphicPipeline.AddShader(vertShader.GetShaderStageInfo());
	m_graphicPipeline.AddShader(fragShader.GetShaderStageInfo());
	
	m_graphicPipeline.BindToSubpass(m_uptrRenderPass.get(), 0);

	m_graphicPipeline.Init();

	fragShader.Uninit();
	vertShader.Uninit();
}

void RayQueryApp::_UninitPipelines()
{
	m_graphicPipeline.Uninit();
}

void RayQueryApp::_InitSwapchainPass()
{
	_UninitSwapchainPass();
	m_swapchainPass = std::make_unique<SwapchainPass>();
	m_swapchainPass->Init(MAX_FRAME_COUNT);
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

	ubo.viewProj = m_camera.GetViewProjectionMatrix();
	m_cameraBuffers[m_currentFrame]->CopyFromHost(&ubo);
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
	GraphicsPipeline::PipelineInput_DrawIndexed pipelineInput{};
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
	cmd->StartRenderPass(m_uptrRenderPass.get(), m_vecFramebuffer[m_currentFrame].get());
	
	{
		GraphicsPipeline::PipelineInput_DrawIndexed input{};

		input.imageSize = m_vecFramebuffer[m_currentFrame]->GetImageSize();
		input.vkIndexType = VK_INDEX_TYPE_UINT32;
		for (size_t i = 0; i < m_models.size(); ++i)
		{
			input.indexBuffer = m_indexBuffers[i]->vkBuffer;
			input.indexCount = static_cast<uint32_t>(m_models[i].mesh.indices.size());
			input.vertexBuffers = { m_vertexBuffers[i]->vkBuffer };
			input.vkDescriptorSets = { m_rtDSets[m_currentFrame]->vkDescriptorSet, m_modelDSets[i]->vkDescriptorSet };
			m_graphicPipeline.Do(cmd->vkCommandBuffer, input);
		}
	}

	cmd->EndRenderPass();
	m_swapchainPass->Execute(m_vecColorImageView[m_currentFrame]->GetDescriptorInfo(m_vkSampler, VK_IMAGE_LAYOUT_GENERAL), { cmd->SubmitCommands() });

	m_currentFrame = (m_currentFrame + 1) % MAX_FRAME_COUNT;
}

void RayQueryApp::_ResizeWindow()
{
	MyDevice::GetInstance().RecreateSwapchain();

	_UninitDescriptorSets();
	_UninitFramebuffers();
	_UninitImagesAndViews();
	_InitImagesAndViews();
	_InitFramebuffers();
	_InitDescriptorSets();
	m_swapchainPass->OnSwapchainRecreated();
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