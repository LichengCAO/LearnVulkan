#include "ray_tracing_app.h"
#include "swapchain_pass.h"
#define MAX_FRAME_COUNT 3

void RayTracingApp::_Init()
{
	_InitDescriptorSetLayouts();

	_InitSampler();

	_InitModels();
	_InitBuffers();

	_InitAS();

	_InitImagesAndViews();

	_InitSwapchainPass();

	_InitDescriptorSets();

	_InitPipelines();
}

void RayTracingApp::_Uninit()
{
	_UninitPipelines();

	_UninitDescriptorSets();

	_UninitSwapchainPass();

	_UninitImagesAndViews();

	_UninitAS();

	_UninitBuffers();
	m_models.clear();

	_UninitSampler();

	_UninitDescriptorSetLayouts();
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
	MeshUtility::Load("E:/GitStorage/LearnVulkan/res/models/bunny/bunny.obj", outMeshes);
	for (auto const& mesh : outMeshes)
	{
		Model model{};
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
		imgInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
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
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
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

void RayTracingApp::_MainLoop()
{

}

void RayTracingApp::_UpdateUniformBuffer()
{

}

void RayTracingApp::_DrawFrame()
{

}

void RayTracingApp::_ResizeWindow()
{
}
