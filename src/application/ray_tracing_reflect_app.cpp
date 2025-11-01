#include "ray_tracing_reflect_app.h"
#include "my_vulkan/commandbuffer.h"
#include "my_vulkan/device.h"
#include "pipeline_program.h"
#include "swapchain_pass.h"
#include "gui_pass.h"
#include "utility/glTF_loader.h"
#include "utility/vdb_loader.h"

#define UNINIT_UPTR(_x) if(_x)\
									{\
									_x->Uninit();\
									_x.reset();\
									}
void RayTracingReflectApp::_CreateCommandBuffers()
{
	for (int i = 0; i < 1; ++i)
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
		curInstData.uBLASIndex = m_uptrAccelStruct->PreAddBLAS({ m_rayTracingGeometryData[i] }); // single BLAS can hold multiple geometries, but here i bind single geometry with one BLAS

		instData.push_back(curInstData);
	}

	// add cloud
	{
		RayTracingAccelerationStructure::InstanceData curInstData{};
		RayTracingAccelerationStructure::AABBData aabbData{};

		aabbData.uAABBCount = 1;
		aabbData.uAABBStride = 24;
		aabbData.vkDeviceAddressAABB = m_uptrAABBBuffer->GetDeviceAddress();

		curInstData.uHitShaderGroupIndexOffset = 1;
		curInstData.transformMatrix = glm::mat4(1.0f);
		curInstData.uBLASIndex = m_uptrAccelStruct->PreAddBLAS({ aabbData });
	}

	m_uptrAccelStruct->PresetTLAS(instData);
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
	m_uptrPipeline->PresetMaxRecursion(2);
	m_uptrPipeline->PreAddMissShader("E:/GitStorage/LearnVulkan/bin/shaders/rt_pbr.rmiss.spv");
	m_uptrPipeline->PreAddRayGenerationShader("E:/GitStorage/LearnVulkan/bin/shaders/rt_pbr.rgen.spv");
	m_uptrPipeline->PreAddTriangleHitShaders("E:/GitStorage/LearnVulkan/bin/shaders/rt_pbr.rchit.spv", {});
	m_uptrPipeline->PreAddProceduralHitShaders("E:/GitStorage/LearnVulkan/bin/shaders/rt_vdb.rchit.spv", "E:/GitStorage/LearnVulkan/bin/shaders/rt_vdb.rint.spv", {});
	m_uptrPipeline->Init(1);

	m_uptrSwapchainPass = std::make_unique<SwapchainPass>();
	m_uptrSwapchainPass->Init(1);

	m_uptrGUIPass = std::make_unique<GUIPass>();
	m_uptrGUIPass->Init(1);
}

void RayTracingReflectApp::_UninitPipeline()
{
	if (m_uptrPipeline)
	{
		m_uptrPipeline->Uninit();
		m_uptrPipeline.reset();
	}
	if (m_uptrSwapchainPass)
	{
		m_uptrSwapchainPass->Uninit();
		m_uptrSwapchainPass.reset();
	}
	if (m_uptrGUIPass)
	{
		m_uptrGUIPass->Uninit();
		m_uptrGUIPass.reset();
	}
}

void RayTracingReflectApp::_CreateImagesAndViews()
{
	auto windowSize = MyDevice::GetInstance().GetSwapchainExtent();

	for (int i = 0; i < 1; ++i)
	{
		std::unique_ptr<Image> uptrImage = std::make_unique<Image>();
		std::unique_ptr<ImageView> uptrView;
		ImageView* pView = nullptr;
		Image::CreateInformation imageInfo{};

		imageInfo.optFormat = VK_FORMAT_R32G32B32A32_SFLOAT;
		imageInfo.optWidth = windowSize.width;
		imageInfo.optHeight = windowSize.height;
		imageInfo.usage = VK_IMAGE_USAGE_STORAGE_BIT 
			| VK_IMAGE_USAGE_TRANSFER_DST_BIT 
			| VK_IMAGE_USAGE_SAMPLED_BIT;

		uptrImage->PresetCreateInformation(imageInfo);
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

void RayTracingReflectApp::_CreateSemaphores()
{
	auto& device = MyDevice::GetInstance();

	for (int i = 0; i < 1; ++i)
	{
		m_semaphores.push_back(device.CreateVkSemaphore());
	}
}

void RayTracingReflectApp::_DestroySemaphores()
{
	auto& device = MyDevice::GetInstance();

	for (auto& semophore : m_semaphores)
	{
		device.DestroyVkSemaphore(semophore);
	}

	m_semaphores.clear();
}

void RayTracingReflectApp::_CreateBuffers()
{
	glTFLoader gltfLoader{};
	glTFLoader::SceneData glTFData{};
	MyVDBLoader vdbLoader{};
	MyVDBLoader::CompactData vdbData{};
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
	//MeshUtility::Load("E:/GitStorage/LearnVulkan/res/models/bunny/bunny.obj", meshes);
	//meshColors.push_back(glm::vec4(1.0f));
	//modelMatrices.push_back(glm::mat4(1.0));
	//materialNames.push_back("bunny");
	vdbLoader.Load("E:\\GitStorage\\LearnVulkan\\res\\models\\cloud\\Stratocumulus 1.vdb", vdbData);

	// mesh data
	for (size_t i = 0; i < meshes.size(); ++i)
	{
		Buffer::CreateInformation bufferInfo{};
		RayTracingAccelerationStructure::TriangleData trigData{};
		AddressData instanceAddressData{};
		std::unique_ptr<Buffer> uptrVertexBuffer = std::make_unique<Buffer>();
		std::unique_ptr<Buffer> uptrIndexBuffer = std::make_unique<Buffer>();
		std::vector<Vertex> vertPositions;

		if (materialNames[i] == "shortBox" || materialNames[i] == "tallBox") continue;

		bufferInfo.optMemoryProperty = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		bufferInfo.optSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		bufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
			| VK_BUFFER_USAGE_TRANSFER_DST_BIT
			| VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
			| VK_BUFFER_USAGE_2_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;
		bufferInfo.size = meshes[i].verts.size() * sizeof(Vertex);
		uptrVertexBuffer->PresetCreateInformation(bufferInfo);
		uptrVertexBuffer->Init();

		bufferInfo.size = meshes[i].indices.size() * sizeof(uint32_t);
		uptrIndexBuffer->PresetCreateInformation(bufferInfo);
		uptrIndexBuffer->Init();

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

	// material data
	{
		Buffer::CreateInformation bufferInfo{};
		std::vector<Material> mtls;
		static const std::vector<std::string> mtlNames = {
			"unknown",     // 0
			"light",       // 1
			"backWall",    // 2
			"ceiling",     // 3
			"floor",       // 4
			"leftWall",    // 5
			"rightWall",   // 6
			"shortBox",    // 7
			"tallBox",     // 8
			"bunny"
		};
		CHECK_TRUE(materialNames.size() == meshColors.size(), "Number of material names doesn't match number of material colors!");
		for (size_t i = 0; i < materialNames.size(); ++i)
		{
			Material curMtl{};

			if (materialNames[i] == "shortBox" || materialNames[i] == "tallBox") continue;

			curMtl.colorOrLight = meshColors[i];
			for (uint32_t j = 0; j < mtlNames.size(); ++j)
			{
				if (materialNames[i] == mtlNames[j])
				{
					curMtl.materialType = glm::uvec4(j, 0, 0, 0);
				}
			}

			mtls.push_back(curMtl);
		}

		m_uptrMaterialBuffer = std::make_unique<Buffer>();
		bufferInfo.optMemoryProperty = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		bufferInfo.size = static_cast<uint32_t>(sizeof(Material) * mtls.size());
		bufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		m_uptrMaterialBuffer->PresetCreateInformation(bufferInfo);
		m_uptrMaterialBuffer->Init();
		m_uptrMaterialBuffer->CopyFromHost(mtls.data());
	}

	// camera data
	for (size_t i = 0; i < 1; ++i)
	{
		std::unique_ptr<Buffer> uptrBuffer = std::make_unique<Buffer>();
		Buffer::CreateInformation bufferInfo{};

		bufferInfo.optMemoryProperty = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
		bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
		bufferInfo.size = static_cast<uint32_t>(sizeof(CameraUBO));
		uptrBuffer->PresetCreateInformation(bufferInfo);
		uptrBuffer->Init();

		m_uptrCameraBuffers.push_back(std::move(uptrBuffer));
	}

	// address data
	{
		Buffer::CreateInformation bufferInfo{};
		m_uptrAddressBuffer = std::make_unique<Buffer>();
		bufferInfo.optMemoryProperty = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		bufferInfo.size = static_cast<uint32_t>(sizeof(AddressData) * addrData.size());
		bufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		m_uptrAddressBuffer->PresetCreateInformation(bufferInfo);
		m_uptrAddressBuffer->Init();
		m_uptrAddressBuffer->CopyFromHost(addrData.data());
	}

	// cloud data
	{
		Buffer::CreateInformation bufferInfo{};
		Buffer::CreateInformation aabbBufferInfo{};

		m_uptrNanoVDBBuffer = std::make_unique<Buffer>();
		bufferInfo.optMemoryProperty = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		bufferInfo.size = sizeof(uint32_t) + vdbData.data.size();
		bufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		m_uptrNanoVDBBuffer->PresetCreateInformation(bufferInfo);
		m_uptrNanoVDBBuffer->Init();
		m_uptrNanoVDBBuffer->CopyFromHost(&vdbData.offsets[3], 0, sizeof(vdbData.offsets[3]));
		m_uptrNanoVDBBuffer->CopyFromHost(vdbData.data.data(), sizeof(vdbData.offsets[3]), vdbData.data.size());

		m_uptrAABBBuffer = std::make_unique<Buffer>();
		aabbBufferInfo.optMemoryProperty = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		aabbBufferInfo.size = 24;
		aabbBufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT  | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_2_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;
		m_uptrAABBBuffer->PresetCreateInformation(aabbBufferInfo);
		m_uptrAABBBuffer->Init();
		m_uptrAABBBuffer->CopyFromHost(&vdbData.minBound, 0, 12);
		m_uptrAABBBuffer->CopyFromHost(&vdbData.maxBound, 12, 12);
	}
	
}

void RayTracingReflectApp::_DestroyBuffers()
{
	if (m_uptrAABBBuffer)
	{
		m_uptrAABBBuffer->Uninit();
		m_uptrAABBBuffer.reset();
	}

	if (m_uptrNanoVDBBuffer)
	{
		m_uptrNanoVDBBuffer->Uninit();
		m_uptrNanoVDBBuffer.reset();
	}
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
	if (userInput.RMB || (m_coefficient != m_coefficientLast))
	{
		m_rayTraceFrame = 0u;
		m_coefficientLast = m_coefficient;
	}
	else
	{
		m_rayTraceFrame++;
	}

	ubo.inverseViewProj = glm::inverse(m_camera.GetViewProjectionMatrix());
	ubo.eye = glm::vec4(m_camera.eye, 1.0f);
	m_uptrCameraBuffers[m_currentFrame]->CopyFromHost(&ubo);

	m_fps = 1.0f / static_cast<float>(currentTime - lastTime);
	lastTime = currentTime;
}

void RayTracingReflectApp::_Init()
{
	MyDevice::GetInstance().Init();
	m_vkSampler = MyDevice::GetInstance().samplerPool.GetSampler(VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
	_CreateSemaphores();
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
	_DestroySemaphores();
	MyDevice::GetInstance().samplerPool.ReturnSampler(&m_vkSampler);
	MyDevice::GetInstance().Uninit();
}

void RayTracingReflectApp::_ResizeWindow()
{
	std::vector<VkDescriptorImageInfo> newViews;

	MyDevice::GetInstance().WaitIdle();
	MyDevice::GetInstance().RecreateSwapchain();
	_DestroyImagesAndViews();
	_CreateImagesAndViews();
	m_uptrSwapchainPass->OnSwapchainRecreated();
	m_uptrGUIPass->OnSwapchainRecreated();
}

void RayTracingReflectApp::_DrawFrame()
{
	auto cmd = m_uptrCommands[m_currentFrame].get();
	VkDescriptorImageInfo guiOutput{};

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
		binder.BindDescriptor(1, 1, { m_uptrNanoVDBBuffer->GetDescriptorInfo() }, DescriptorSetManager::DESCRIPTOR_BIND_SETTING::CONSTANT_DESCRIPTOR_SET_ACROSS_FRAMES);
		binder.EndBind();
		binder.EndFrame();
	}
	
	// update frame count in ray tracing
	{
		uint32_t frameCount = m_rayTraceFrame + 1;
		m_uptrPipeline->PushConstant(VK_SHADER_STAGE_RAYGEN_BIT_KHR, &frameCount);
		m_uptrPipeline->PushConstant(VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, &m_coefficient);
	}

	m_uptrPipeline->TraceRay(
		cmd,
		MyDevice::GetInstance().GetSwapchainExtent().width,
		MyDevice::GetInstance().GetSwapchainExtent().height);

	auto& gui = m_uptrGUIPass->StartPass({ m_uptrOutputViews[m_currentFrame]->GetDescriptorInfo(m_vkSampler, VK_IMAGE_LAYOUT_GENERAL) }, { cmd->SubmitCommands() });
	gui.StartWindow("this is ui");
	gui.SliderFloat("sigma_a", m_coefficient.sigma_a, 0.001, 10);
	gui.SliderFloat("sigma_s", m_coefficient.sigma_s, 0.001, 10);
	gui.SliderFloat("g", m_coefficient.g, -0.999f, 0.999f);
	gui.FrameRateText();
	std::stringstream ss;
	ss << "camera position: " << m_camera.eye.x << ", " << m_camera.eye.y << ", " << m_camera.eye.z;
	gui.Text(ss.str());
	gui.EndWindow();
	m_uptrGUIPass->EndPass({ m_semaphores[m_currentFrame] }, guiOutput);

	m_uptrSwapchainPass->Execute({ guiOutput }, { m_semaphores[m_currentFrame] });

	m_uptrPipeline->EndFrame();
	m_currentFrame = 0;// (m_currentFrame + 1) % 1;
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

void RayTracingNanoVDBApp::_InitProgram()
{
	m_uptrProgram = std::make_unique<RayTracingProgram>();

	m_uptrProgram->PreAddMissShader("E:/GitStorage/LearnVulkan/bin/shaders/rt_pbr.rmiss.spv");
	m_uptrProgram->PreAddProceduralHitShaders("E:/GitStorage/LearnVulkan/bin/shaders/rt_vdb.rchit.spv", "E:/GitStorage/LearnVulkan/bin/shaders/rt_pbr.rint.spv", {});
	m_uptrProgram->PreAddRayGenerationShader("E:/GitStorage/LearnVulkan/bin/shaders/rt_vdb.rgen.spv");
	m_uptrProgram->PresetMaxRecursion(2);
	m_uptrProgram->Init(1);

	m_uptrGUIPass = std::make_unique<GUIPass>();
	m_uptrGUIPass->Init(1);

	m_uptrSwapchainPass = std::make_unique<SwapchainPass>();
	m_uptrSwapchainPass->Init(1);
}

void RayTracingNanoVDBApp::_UnintProgram()
{
	if (m_uptrSwapchainPass)
	{
		m_uptrSwapchainPass->Uninit();
		m_uptrSwapchainPass.reset();
	}

	if (m_uptrGUIPass)
	{
		m_uptrGUIPass->Uninit();
		m_uptrGUIPass.reset();
	}

	if (m_uptrProgram)
	{
		m_uptrProgram->Uninit();
		m_uptrProgram.reset();
	}
}

void RayTracingNanoVDBApp::_InitBuffersAndSceneObjects()
{
	Buffer::CreateInformation createInfo{};
	Buffer::CreateInformation createInfo2{};
	Buffer::CreateInformation createInfo3{};
	MyVDBLoader loader{};
	MyVDBLoader::CompactData sceneData{};
	RayTracingAccelerationStructure::AABBData aabbData{};
	RayTracingAccelerationStructure::InstanceData instData{};
	VkAabbPositionsKHR bbox{};

	m_uptrAABBBuffer = std::make_unique<Buffer>();
	m_uptrCameraBuffer = std::make_unique<Buffer>();
	m_uptrVDBBuffer = std::make_unique<Buffer>();
	m_uptrAccelerationStructure = std::make_unique<RayTracingAccelerationStructure>();

	loader.Load("E:/GitStorage/LearnVulkan/res/models/cloud/Stratocumulus 1.vdb", sceneData);
	bbox.minX = sceneData.minBound.r;
	bbox.minY = sceneData.minBound.g;
	bbox.minZ = sceneData.minBound.b;
	bbox.maxX = sceneData.maxBound.r;
	bbox.maxY = sceneData.maxBound.g;
	bbox.maxZ = sceneData.maxBound.b;

	createInfo.usage =
		VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR
		| VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
		| VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	createInfo.size = sizeof(bbox);
	m_uptrAABBBuffer->PresetCreateInformation(createInfo);
	m_uptrAABBBuffer->Init();
	m_uptrAABBBuffer->CopyFromHost(&bbox);
	aabbData.uAABBCount = 1;
	aabbData.uAABBStride = sizeof(bbox);
	aabbData.vkDeviceAddressAABB = m_uptrAABBBuffer->GetDeviceAddress();
	instData.uBLASIndex = m_uptrAccelerationStructure->PreAddBLAS({ aabbData });
	instData.uHitShaderGroupIndexOffset = 0;
	m_uptrAccelerationStructure->PresetTLAS({ instData });
	m_uptrAccelerationStructure->Init();

	createInfo2.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
	createInfo2.size = sizeof(CameraUBO);
	createInfo2.optMemoryProperty = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
	m_uptrCameraBuffer->PresetCreateInformation(createInfo2);
	m_uptrCameraBuffer->Init();

	createInfo3.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	createInfo3.size = 4 + sceneData.data.size();
	m_uptrVDBBuffer->PresetCreateInformation(createInfo);
	m_uptrVDBBuffer->Init();
	m_uptrVDBBuffer->CopyFromHost(&sceneData.offsets[3], 0, 4);
	m_uptrVDBBuffer->CopyFromHost(sceneData.data.data(), 4, sceneData.data.size());
}

void RayTracingNanoVDBApp::_UninitBuffersAndSceneObjects()
{
	UNINIT_UPTR(m_uptrVDBBuffer);
	UNINIT_UPTR(m_uptrCameraBuffer);
	UNINIT_UPTR(m_uptrAccelerationStructure);
	UNINIT_UPTR(m_uptrAABBBuffer);
}

void RayTracingNanoVDBApp::_UpdateUniformBuffer()
{
	UserInput userInput = MyDevice::GetInstance().GetUserInput();
	CameraUBO ubo{};
	m_camera->UpdateCamera();
	m_frameCount = userInput.RMB ? 0 : m_frameCount + 1;
	ubo.inverseViewProj = glm::inverse(m_camera->camera.GetViewProjectionMatrix());
	ubo.eye = glm::vec4(m_camera->camera.eye, 1.0f);
	m_uptrCameraBuffer->CopyFromHost(&ubo);
}

void RayTracingNanoVDBApp::_CreateImageAndViews()
{
	Image::CreateInformation imageInfo{};
	ImageView* pImageView = nullptr;

	m_uptrOutputImage = std::make_unique<Image>();
	imageInfo.usage = VK_IMAGE_USAGE_STORAGE_BIT
		| VK_IMAGE_USAGE_TRANSFER_DST_BIT
		| VK_IMAGE_USAGE_SAMPLED_BIT;
	m_uptrOutputImage->PresetCreateInformation(imageInfo);
	m_uptrOutputImage->Init();
	m_uptrOutputImage->ChangeLayoutAndFill(VK_IMAGE_LAYOUT_GENERAL, { 0u, 0u, 0u, 0u });
	m_uptrOutputImage->NewImageView(pImageView);
	m_uptrOutputView.reset(pImageView);
	m_uptrOutputView->Init();
}

void RayTracingNanoVDBApp::_DestroyImageAndViews()
{
	UNINIT_UPTR(m_uptrOutputView);
	UNINIT_UPTR(m_uptrOutputImage);
}

void RayTracingNanoVDBApp::_Init()
{
	MyDevice::GetInstance().Init();
	m_uptrCmd = std::make_unique<CommandSubmission>();
	m_uptrCmd->Init();
	_CreateImageAndViews();
	_InitBuffersAndSceneObjects();
	_InitProgram();

	m_camera = std::make_unique<CameraComponent>(400, 300, glm::vec3(2, 2, 2), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
	m_sampler = MyDevice::GetInstance().samplerPool.GetSampler(VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER);
	m_semaphore = MyDevice::GetInstance().CreateVkSemaphore();
}

void RayTracingNanoVDBApp::_Uninit()
{
	MyDevice::GetInstance().DestroyVkSemaphore(m_semaphore);
	MyDevice::GetInstance().samplerPool.ReturnSampler(m_sampler);
	_UnintProgram();
	_UninitBuffersAndSceneObjects();
	_DestroyImageAndViews();
	UNINIT_UPTR(m_uptrCmd);
	MyDevice::GetInstance().Uninit();
}

void RayTracingNanoVDBApp::_ResizeWindow()
{
	MyDevice::GetInstance().WaitIdle();
	MyDevice::GetInstance().RecreateSwapchain();
	m_uptrGUIPass->OnSwapchainRecreated();
	m_uptrSwapchainPass->OnSwapchainRecreated();
	_DestroyImageAndViews();
	_CreateImageAndViews();
}

void RayTracingNanoVDBApp::_DrawFrame()
{
	if (MyDevice::GetInstance().NeedRecreateSwapchain())
	{
		_ResizeWindow();
		return;
	}
	m_uptrCmd->WaitTillAvailable();
	_UpdateUniformBuffer();
	m_uptrCmd->StartCommands({});
	auto& binder = m_uptrProgram->GetDescriptorSetManager();
	binder.StartBind();
	binder.BindDescriptor(0, 0, { m_uptrCameraBuffer->GetDescriptorInfo() });
	binder.BindDescriptor(0, 1, { m_uptrAccelerationStructure->vkAccelerationStructure });
	binder.BindDescriptor(0, 2, { m_uptrOutputView->GetDescriptorInfo(m_sampler, VK_IMAGE_LAYOUT_GENERAL) });
	binder.BindDescriptor(1, 1, { m_uptrVDBBuffer->GetDescriptorInfo() });
	binder.EndBind();
	m_uptrProgram->PushConstant(VK_SHADER_STAGE_RAYGEN_BIT_KHR, &m_frameCount);
	m_uptrProgram->TraceRay(m_uptrCmd.get(), m_uptrOutputImage->GetImageSize().width, m_uptrOutputImage->GetImageSize().height);
	auto semaphore = m_uptrCmd->SubmitCommands();
	auto& gui = m_uptrGUIPass->StartPass(m_uptrOutputView->GetDescriptorInfo(m_sampler, VK_IMAGE_LAYOUT_GENERAL), { semaphore });
	gui.StartWindow("VDB test");
	gui.FrameRateText();
	gui.EndWindow();
	VkDescriptorImageInfo imageInfo{};
	m_uptrGUIPass->EndPass({ m_semaphore }, imageInfo);
	m_uptrSwapchainPass->Execute(imageInfo, { m_semaphore });
	m_uptrProgram->EndFrame();
}

RayTracingNanoVDBApp::RayTracingNanoVDBApp()
{
}

RayTracingNanoVDBApp::~RayTracingNanoVDBApp()
{
}

void RayTracingNanoVDBApp::Run()
{
	_Init();
	while (!MyDevice::GetInstance().NeedCloseWindow())
	{
		MyDevice::GetInstance().StartFrame();
		_DrawFrame();
	}
	_Uninit();
}
