#include "ray_tracing_app.h"

#define MAX_FRAME_COUNT 3

void RayTracingApp::_Init()
{
	_InitRenderPass();

	_InitDescriptorSetLayouts();

	_InitSampler();

	_InitModels();
	_InitBuffers();

	_InitAS();

	_InitImagesAndViews();

	_InitFramebuffers();

	_InitDescriptorSets();

	_InitPipelines();
}

void RayTracingApp::_Uninit()
{
	_UninitRenderPass();

	_UninitDescriptorSetLayouts();

	_UninitSampler();

	_UninitAS();
	
	m_models.clear();
	_UninitBuffers();

	_UninitImagesAndViews();

	_UninitFramebuffers();

	_UninitDescriptorSets();

	_UninitPipelines();
}

void RayTracingApp::_InitRenderPass()
{	
	RenderPass::Subpass subpassInfo{};
	RenderPass::Subpass attachmentLessSubpassInfo{};
	m_scRenderPass = RenderPass{};
	m_scRenderPass.AddAttachment(RenderPass::AttachmentPreset::GBUFFER_ALBEDO);

	subpassInfo.AddColorAttachment(0u);
	m_scRenderPass.AddSubpass(subpassInfo);

	m_scRenderPass.Init();

	m_rtRenderPass = RenderPass{};
	m_rtRenderPass.AddSubpass(attachmentLessSubpassInfo);
	m_rtRenderPass.Init();
}

void RayTracingApp::_UninitRenderPass()
{
	m_rtRenderPass.Uninit();
	m_scRenderPass.Uninit();
}

void RayTracingApp::_InitDescriptorSetLayouts()
{
	m_rtDSetLayout.AddBinding(1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_RAYGEN_BIT_KHR); // camera info
	m_rtDSetLayout.AddBinding(1, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_RAYGEN_BIT_KHR); // AS
	m_rtDSetLayout.AddBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_RAYGEN_BIT_KHR); // image output
	m_rtDSetLayout.AddBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR); // instance data
	m_rtDSetLayout.Init();

	m_scDSetLayout.AddBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
	m_scDSetLayout.Init();
}

void RayTracingApp::_UninitDescriptorSetLayouts()
{
	m_scDSetLayout.Uninit();
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
	Buffer::Information bufferInfo{};

	bufferInfo.memoryProperty = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	bufferInfo.usage = 
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

		bufferInfo.size = vertData.size() * sizeof(VBO);
		uptrVertexBuffer->Init(bufferInfo);

		bufferInfo.size = model.mesh.indices.size() * sizeof(uint32_t);
		uptrIndexBuffer->Init(bufferInfo);

		uptrVertexBuffer->CopyFromHost(vertData.data());
		uptrIndexBuffer->CopyFromHost(model.mesh.indices.data());

		m_vertexBuffers.push_back(std::move(uptrVertexBuffer));
		m_indexBuffers.push_back(std::move(uptrIndexBuffer));
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

		instData.uBLASIndex = m_rayTracingAS.AddBLAS({ trigData });
		instData.transformMatrix = model.transform.GetModelMatrix();

		instDatas.push_back(instData);
	}

	m_rayTracingAS.SetUpTLAS(instDatas);
	m_rayTracingAS.Init();
}

void RayTracingApp::_UninitAS()
{
	m_rayTracingAS.Uninit();
}

void RayTracingApp::_InitImagesAndViews()
{
	m_scImages = MyDevice::GetInstance().GetSwapchainImages();
	for (auto const& img : m_scImages)
	{
		m_scImageViews.push_back(std::make_unique<ImageView>(img.NewImageView()));
	}

	m_rtImages.clear();
	m_rtImages.reserve(MAX_FRAME_COUNT);
	for (int i = 0; i < MAX_FRAME_COUNT; ++i)
	{
		Image rtImage{};
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

		rtImage.SetImageInformation(imgInfo);
		rtImage.Init();

		m_rtImageViews.push_back(std::make_unique<ImageView>(rtImage.NewImageView()));
		m_rtImages.push_back(std::move(rtImage));
	}

	for (auto& imgView : m_scImageViews)
	{
		imgView->Init();
	}

	for (auto& imgView : m_rtImageViews)
	{
		imgView->Init();
	}
}

void RayTracingApp::_UninitImagesAndViews()
{
	for (auto& imgView : m_rtImageViews)
	{
		imgView->Uninit();
	}
	m_rtImageViews.clear();

	for (auto& imgView : m_scImageViews)
	{
		imgView->Uninit();
	}
	m_scImageViews.clear();

	for (auto& img : m_rtImages)
	{
		img.Uninit();
	}
	m_rtImages.clear();

	m_scImages.clear();
}

void RayTracingApp::_InitFramebuffers()
{
	for (auto const& imgView : m_scImageViews)
	{
		std::unique_ptr<Framebuffer> uptrFramebuffer = std::make_unique<Framebuffer>(m_scRenderPass.NewFramebuffer({ imgView.get() }));

		uptrFramebuffer->Init();

		m_scFramebuffers.push_back(std::move(uptrFramebuffer));
	}
}

void RayTracingApp::_UninitFramebuffers()
{
	for (auto& framebuffer : m_scFramebuffers)
	{
		framebuffer->Uninit();
	}
	m_scFramebuffers.clear();
}

void RayTracingApp::_InitDescriptorSets()
{
	for (int i = 0; i < MAX_FRAME_COUNT; ++i)
	{
		std::unique_ptr<DescriptorSet> uptrDSet = std::make_unique<DescriptorSet>(m_cameraDSetLayout.NewDescriptorSet());

		uptrDSet->Init();

		m_cameraDSets.push_back(std::move(uptrDSet));
	}

	{
		m_ASDSet = std::make_unique<DescriptorSet>(m_rtDSetLayout.NewDescriptorSet());

		m_ASDSet->Init();
	}

	for (int i = 0; i < MAX_FRAME_COUNT; ++i)
	{
		std::unique_ptr<DescriptorSet> uptrDSet = std::make_unique<DescriptorSet>(m_scDSetLayout.NewDescriptorSet());

		uptrDSet->Init();

		m_scDSets.push_back(std::move(uptrDSet));
	}
}

void RayTracingApp::_UninitDescriptorSets() 
{
	m_scDSets.clear();
	m_ASDSet.reset();
	m_cameraDSets.clear();
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

	m_rayTracingPipeline.AddDescriptorSetLayout(m_cameraDSetLayout.vkDescriptorSetLayout);
	m_rayTracingPipeline.AddDescriptorSetLayout(m_rtDSetLayout.vkDescriptorSetLayout);
	rgenId  = m_rayTracingPipeline.AddShader(rgen.GetShaderStageInfo());
	rchitId = m_rayTracingPipeline.AddShader(rchit.GetShaderStageInfo());
	rmissId = m_rayTracingPipeline.AddShader(rmiss.GetShaderStageInfo());

	m_rayTracingPipeline.SetRayGenerationShaderRecord(rgenId);
	m_rayTracingPipeline.AddHitShaderRecord(rchitId);
	m_rayTracingPipeline.AddMissShaderRecord(rmissId);
	m_rayTracingPipeline.SetMaxRecursion(1u);

	m_rayTracingPipeline.Init();

	rgen.Uninit();
	rchit.Uninit();
	rmiss.Uninit();
}

void RayTracingApp::_UninitPipelines()
{
	m_rayTracingPipeline.Uninit();
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
