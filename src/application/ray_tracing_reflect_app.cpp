#include "ray_tracing_reflect_app.h"
#include "my_vulkan/commandbuffer.h"
#include "my_vulkan/device.h"
#include "pipeline_program.h"
#include "swapchain_pass.h"
#include "utility/glTF_loader.h"

void RayTracingReflectApp::_CreateCommandBuffers()
{
	for (int i = 0; i < 3; ++i)
	{
		std::unique_ptr<CommandSubmission> uptrCmd = std::make_unique<CommandSubmission>();
		uptrCmd->Init();

		m_uptrCommands.push_back(std::move(uptrCmd));
	}
}

void RayTracingReflectApp::_DestroyCommandBuffers()
{
	for (auto& uptrCmd : m_uptrCommands)
	{
		uptrCmd->Uninit();
	}
	m_uptrCommands.clear();
}

void RayTracingReflectApp::_InitAccelerationStructures()
{
	std::vector<RayTracingAccelerationStructure::InstanceData> instData;

	m_uptrAccelStruct = std::make_unique<RayTracingAccelerationStructure>();

	for (size_t i = 0; i < m_rayTracingGeometryData.size(); ++i)
	{
		RayTracingAccelerationStructure::InstanceData curInstData{};

		curInstData.transformMatrix = m_rayTracingGeometryTransform[i];
		curInstData.uBLASIndex = m_uptrAccelStruct->AddBLAS({ m_rayTracingGeometryData[i] }); // single BLAS can hold multiple geometries, but here i bind single geometry with one BLAS

		instData.push_back(curInstData);
	}
	m_uptrAccelStruct->SetUpTLAS(instData);
	m_uptrAccelStruct->Init();
}

void RayTracingReflectApp::_UninitAccelerationStructures()
{
	if (m_uptrAccelStruct)
	{
		m_uptrAccelStruct->Uninit();
		m_uptrAccelStruct.reset();
	}
}

void RayTracingReflectApp::_InitPipeline()
{
	std::vector<VkDescriptorImageInfo> passThroughImages;

	m_uptrPipeline = std::make_unique<RayTracingProgram>();
	m_uptrPipeline->SetMaxRecursion(2);
	m_uptrPipeline->AddMissShader("E:/GitStorage/LearnVulkan/bin/shaders/rt_pbr.rmiss.spv");
	m_uptrPipeline->AddRayGenerationShader("E:/GitStorage/LearnVulkan/bin/shaders/rt_pbr.rgen.spv");
	m_uptrPipeline->AddTriangleHitShaders("E:/GitStorage/LearnVulkan/bin/shaders/rt_pbr.rchit.spv", {});
	m_uptrPipeline->Init(3);

	m_uptrSwapchainPass = std::make_unique<SwapchainPass>();
	m_uptrSwapchainPass->Init(3);
}

void RayTracingReflectApp::_UninitPipeline()
{
	if (m_uptrPipeline)
	{
		m_uptrPipeline->Uninit();
	}
	if (m_uptrSwapchainPass)
	{
		m_uptrSwapchainPass->Uninit();
	}
}

void RayTracingReflectApp::_CreateImagesAndViews()
{
	auto windowSize = MyDevice::GetInstance().GetSwapchainExtent();

	for (int i = 0; i < 3; ++i)
	{
		std::unique_ptr<Image> uptrImage = std::make_unique<Image>();
		std::unique_ptr<ImageView> uptrView;
		ImageView* pView = nullptr;
		Image::Information imageInfo{};

		imageInfo.format = VK_FORMAT_R32G32B32A32_SFLOAT;
		imageInfo.width = windowSize.width;
		imageInfo.height = windowSize.height;
		imageInfo.usage = VK_IMAGE_USAGE_STORAGE_BIT 
			| VK_IMAGE_USAGE_TRANSFER_DST_BIT 
			| VK_IMAGE_USAGE_SAMPLED_BIT;

		uptrImage->SetImageInformation(imageInfo);
		uptrImage->Init();
		uptrImage->ChangeLayoutAndFill(VK_IMAGE_LAYOUT_GENERAL, { 0, 0, 0, 1 });
		uptrImage->NewImageView(pView);
		uptrView.reset(pView);
		uptrView->Init();

		m_uptrOutputImages.push_back(std::move(uptrImage));
		m_uptrOutputViews.push_back(std::move(uptrView));
	}
}

void RayTracingReflectApp::_DestroyImagesAndViews()
{
	for (auto& uptrView : m_uptrOutputViews)
	{
		uptrView->Uninit();
	}

	for (auto& uptrImage : m_uptrOutputImages)
	{
		uptrImage->Uninit();
	}

	m_uptrOutputViews.clear();
	m_uptrOutputImages.clear();
}

void RayTracingReflectApp::_CreateBuffers()
{
	glTFLoader gltfLoader{};
	glTFLoader::SceneData glTFData{};
	std::vector<StaticMesh> meshes;
	std::vector<glm::mat4> modelMatrices;
	std::vector<glm::vec4> meshColors;
	std::vector<std::string> materialNames;
	std::vector<AddressData> addrData;
	
	glTFData.pMeshColors = &meshColors;
	glTFData.pModelMatrices = &modelMatrices;
	glTFData.pStaticMeshes = &meshes;
	glTFData.pMaterialNames = &materialNames;
	gltfLoader.Load("E:/GitStorage/LearnVulkan/res/models/cornell_box/scene.gltf");
	gltfLoader.GetSceneData(glTFData);

	// mesh data
	for (size_t i = 0; i < meshes.size(); ++i)
	{
		Buffer::Information bufferInfo{};
		RayTracingAccelerationStructure::TriangleData trigData{};
		AddressData instanceAddressData{};
		std::unique_ptr<Buffer> uptrVertexBuffer = std::make_unique<Buffer>();
		std::unique_ptr<Buffer> uptrIndexBuffer = std::make_unique<Buffer>();
		std::vector<Vertex> vertPositions;

		bufferInfo.memoryProperty = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		bufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
			| VK_BUFFER_USAGE_TRANSFER_DST_BIT
			| VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
			| VK_BUFFER_USAGE_2_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;
		bufferInfo.size = meshes[i].verts.size() * sizeof(Vertex);
		uptrVertexBuffer->Init(bufferInfo);

		bufferInfo.size = meshes[i].indices.size() * sizeof(uint32_t);
		uptrIndexBuffer->Init(bufferInfo);

		for (size_t j = 0; j < meshes[i].verts.size(); ++j)
		{
			vertPositions.push_back({ glm::vec4(meshes[i].verts[j].position, 1.0f) , glm::vec4(meshes[i].verts[j].normal.value(), 0.0f)});
		}
		uptrVertexBuffer->CopyFromHost(vertPositions.data());
		uptrIndexBuffer->CopyFromHost(meshes[i].indices.data());

		trigData.vkIndexType = VK_INDEX_TYPE_UINT32;
		trigData.uIndexCount = static_cast<uint32_t>(meshes[i].indices.size());
		trigData.uVertexCount = static_cast<uint32_t>(meshes[i].verts.size());
		trigData.uVertexStride = static_cast<uint32_t>(sizeof(Vertex));
		trigData.vkDeviceAddressIndex = uptrIndexBuffer->GetDeviceAddress();
		trigData.vkDeviceAddressVertex = uptrVertexBuffer->GetDeviceAddress();
		instanceAddressData.indexAddress = static_cast<uint64_t>(uptrIndexBuffer->GetDeviceAddress());
		instanceAddressData.vertexAddress = static_cast<uint64_t>(uptrVertexBuffer->GetDeviceAddress());

		addrData.push_back(instanceAddressData);

		m_rayTracingGeometryData.push_back(std::move(trigData));
		m_rayTracingGeometryTransform.push_back(modelMatrices[i]);
		m_uptrModelVertexBuffers.push_back(std::move(uptrVertexBuffer));
		m_uptrModelIndexBuffers.push_back(std::move(uptrIndexBuffer));
	}

	// camera data
	for (size_t i = 0; i < 3; ++i)
	{
		std::unique_ptr<Buffer> uptrBuffer = std::make_unique<Buffer>();
		Buffer::Information bufferInfo{};

		bufferInfo.memoryProperty = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
		bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
		bufferInfo.size = static_cast<uint32_t>(sizeof(CameraUBO));

		uptrBuffer->Init(bufferInfo);

		m_uptrCameraBuffers.push_back(std::move(uptrBuffer));
	}

	// address data
	{
		Buffer::Information bufferInfo{};
		m_uptrAddressBuffer = std::make_unique<Buffer>();
		bufferInfo.memoryProperty = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		bufferInfo.size = static_cast<uint32_t>(sizeof(AddressData) * addrData.size());
		bufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

		m_uptrAddressBuffer->Init(bufferInfo);
		m_uptrAddressBuffer->CopyFromHost(addrData.data());
	}

	// material data
	{
		Buffer::Information bufferInfo{};
		std::vector<Material> mtls;

		CHECK_TRUE(materialNames.size() == meshColors.size(), "Number of material names doesn't match number of material colors!");
		for (size_t i = 0; i < materialNames.size(); ++i)
		{
			Material curMtl{};

			curMtl.colorOrLight = meshColors[i];
			if (materialNames[i] == "light")
			{
				curMtl.colorOrLight.a = 1.0f;
				curMtl.materialType = glm::uvec4(1, 0, 0, 0);
			}
			else if (materialNames[i] == "backWall" || materialNames[i] == "leftWall")
			{
				curMtl.colorOrLight.a = 0.0f;
				curMtl.materialType = glm::uvec4(2, 0, 0, 0);
			}
			else
			{
				curMtl.colorOrLight.a = 0.0f;
			}

			mtls.push_back(curMtl);
		}

		m_uptrMaterialBuffer = std::make_unique<Buffer>();
		bufferInfo.memoryProperty = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		bufferInfo.size = static_cast<uint32_t>(sizeof(Material) * mtls.size());
		bufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

		m_uptrMaterialBuffer->Init(bufferInfo);
		m_uptrMaterialBuffer->CopyFromHost(mtls.data());
	}
	
}

void RayTracingReflectApp::_DestroyBuffers()
{
	if (m_uptrMaterialBuffer)
	{
		m_uptrMaterialBuffer->Uninit();
		m_uptrMaterialBuffer.reset();
	}

	for (auto& uptrCameraBuffer : m_uptrCameraBuffers)
	{
		uptrCameraBuffer->Uninit();
	}
	m_uptrCameraBuffers.clear();

	for (auto& uptrVertexBuffer : m_uptrModelVertexBuffers)
	{
		uptrVertexBuffer->Uninit();
	}
	m_uptrModelVertexBuffers.clear();

	for (auto& uptrIndexBuffer : m_uptrModelIndexBuffers)
	{
		uptrIndexBuffer->Uninit();
	}
	m_uptrModelIndexBuffers.clear();
	
	if (m_uptrAddressBuffer)
	{
		m_uptrAddressBuffer->Uninit();
		m_uptrAddressBuffer.reset();
	}
}

void RayTracingReflectApp::_UpdateUniformBuffer()
{
	static std::optional<float> lastX;
	static std::optional<float> lastY;
	static double lastTime = MyDevice::GetInstance().GetTime();
	static double currentTime = 0.0;
	static const float sensitivity = 0.3f;

	UserInput userInput = MyDevice::GetInstance().GetUserInput();
	CameraUBO ubo{};
	VkExtent2D swapchainExtent = MyDevice::GetInstance().GetSwapchainExtent();
	float xoffset;
	float yoffset;

	if (!userInput.RMB)
	{
		lastX = userInput.xPos;
		lastY = userInput.yPos;
	}
	xoffset = lastX.has_value() ? static_cast<float>(userInput.xPos) - lastX.value() : 0.0f;
	yoffset = lastY.has_value() ? lastY.value() - static_cast<float>(userInput.yPos) : 0.0f;
	currentTime = MyDevice::GetInstance().GetTime();
	lastX = userInput.xPos;
	lastY = userInput.yPos;
	xoffset *= sensitivity;
	yoffset *= sensitivity;
	m_camera.RotateAboutWorldUp(glm::radians(-xoffset));
	m_camera.RotateAboutRight(glm::radians(yoffset));
	if (userInput.RMB)
	{
		glm::vec3 fwd = glm::normalize(glm::cross(m_camera.world_up, m_camera.right));
		float speed = 0.5 * static_cast<float>(currentTime - lastTime);
		glm::vec3 mov = m_camera.eye;
		if (userInput.W) mov += (speed * fwd);
		if (userInput.S) mov += (-speed * fwd);
		if (userInput.Q) mov += (speed * m_camera.world_up);
		if (userInput.E) mov += (-speed * m_camera.world_up);
		if (userInput.A) mov += (-speed * m_camera.right);
		if (userInput.D) mov += (speed * m_camera.right);
		m_camera.MoveTo(mov);
	}
	if (userInput.RMB)
	{
		m_rayTraceFrame = 0u;
	}
	else
	{
		m_rayTraceFrame++;
	}

	ubo.inverseViewProj = glm::inverse(m_camera.GetViewProjectionMatrix());
	ubo.eye = glm::vec4(m_camera.eye, 1.0f);
	m_uptrCameraBuffers[m_currentFrame]->CopyFromHost(&ubo);

	lastTime = currentTime;
}

void RayTracingReflectApp::_Init()
{
	MyDevice::GetInstance().Init();
	m_vkSampler = MyDevice::GetInstance().samplerPool.GetSampler(VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
	_CreateCommandBuffers();
	_CreateImagesAndViews();
	_CreateBuffers();
	_InitPipeline();
	_InitAccelerationStructures();
}

void RayTracingReflectApp::_Uninit()
{
	MyDevice::GetInstance().WaitIdle();
	_UninitAccelerationStructures();
	_UninitPipeline();
	_DestroyBuffers();
	_DestroyImagesAndViews();
	_DestroyCommandBuffers();
	MyDevice::GetInstance().samplerPool.ReturnSampler(&m_vkSampler);
	MyDevice::GetInstance().Uninit();
}

void RayTracingReflectApp::_ResizeWindow()
{
	std::vector<VkDescriptorImageInfo> newViews;

	MyDevice::GetInstance().WaitIdle();
	MyDevice::GetInstance().RecreateSwapchain();
	_DestroyImagesAndViews();
	_DestroyBuffers();
	_CreateBuffers();
	_CreateImagesAndViews();
	m_uptrSwapchainPass->OnSwapchainRecreated();
}

void RayTracingReflectApp::_DrawFrame()
{
	auto cmd = m_uptrCommands[m_currentFrame].get();

	if (MyDevice::GetInstance().NeedRecreateSwapchain())
	{
		_ResizeWindow();
	}

	cmd->WaitTillAvailable();
	_UpdateUniformBuffer();
	cmd->StartCommands({});

	{
		auto& binder = m_uptrPipeline->GetDescriptorSetManager();
		binder.StartBind();
		binder.BindDescriptor(0, 0, { m_uptrCameraBuffers[m_currentFrame]->GetDescriptorInfo() }, DescriptorSetManager::DESCRIPTOR_BIND_SETTING::CONSTANT_DESCRIPTOR_SET_PER_FRAME);
		binder.BindDescriptor(0, 1, { m_uptrAccelStruct->vkAccelerationStructure }, DescriptorSetManager::DESCRIPTOR_BIND_SETTING::CONSTANT_DESCRIPTOR_SET_ACROSS_FRAMES);
		binder.BindDescriptor(0, 2, { m_uptrOutputViews[m_currentFrame]->GetDescriptorInfo(m_vkSampler, VK_IMAGE_LAYOUT_GENERAL) }, DescriptorSetManager::DESCRIPTOR_BIND_SETTING::CONSTANT_DESCRIPTOR_SET_PER_FRAME);
		binder.BindDescriptor(0, 3, { m_uptrAddressBuffer->GetDescriptorInfo() }, DescriptorSetManager::DESCRIPTOR_BIND_SETTING::CONSTANT_DESCRIPTOR_SET_ACROSS_FRAMES);
		binder.BindDescriptor(1, 0, { m_uptrMaterialBuffer->GetDescriptorInfo() }, DescriptorSetManager::DESCRIPTOR_BIND_SETTING::CONSTANT_DESCRIPTOR_SET_ACROSS_FRAMES);
		binder.EndBind();
		binder.EndFrame();
	}
	
	// update frame count in ray tracing
	{
		uint32_t frameCount = m_rayTraceFrame / 3 + 1;
		m_uptrPipeline->PushConstant(VK_SHADER_STAGE_RAYGEN_BIT_KHR, &frameCount);
	}

	m_uptrPipeline->TraceRay(
		cmd,
		MyDevice::GetInstance().GetSwapchainExtent().width,
		MyDevice::GetInstance().GetSwapchainExtent().height);
	auto syncSignal = cmd->SubmitCommands();

	m_uptrSwapchainPass->Execute({ m_uptrOutputViews[m_currentFrame]->GetDescriptorInfo(m_vkSampler, VK_IMAGE_LAYOUT_GENERAL) }, { syncSignal });

	m_uptrPipeline->EndFrame();
	m_currentFrame = (m_currentFrame + 1) % 3;
}

RayTracingReflectApp::RayTracingReflectApp()
{
}

RayTracingReflectApp::~RayTracingReflectApp()
{
}

void RayTracingReflectApp::Run()
{
	_Init();
	while (!MyDevice::GetInstance().NeedCloseWindow())
	{
		MyDevice::GetInstance().StartFrame();
		_DrawFrame();
	}
	_Uninit();
}
