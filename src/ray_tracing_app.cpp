#include "ray_tracing_app.h"

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
	SubpassInformation subpassInfo{};
	m_renderPass = RenderPass{};
	m_renderPass.AddAttachment(AttachmentInformation::GetPresetInformation(AttachmentPreset::GBUFFER_ALBEDO));

	subpassInfo.AddColorAttachment(0u);
	m_renderPass.AddSubpass(subpassInfo);

	m_renderPass.Init();
}

void RayTracingApp::_UninitRenderPass()
{
	m_renderPass.Uninit();
}

void RayTracingApp::_InitDescriptorSetLayouts()
{
	m_modelDSetLayout.AddBinding(1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_RAYGEN_BIT_KHR); // camera info
	m_modelDSetLayout.AddBinding(1, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_RAYGEN_BIT_KHR); // AS
	m_modelDSetLayout.AddBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_RAYGEN_BIT_KHR); // image output
	m_modelDSetLayout.AddBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR); // instance data

	m_modelDSetLayout.Init();
}

void RayTracingApp::_UninitDescriptorSetLayouts()
{
	m_modelDSetLayout.Uninit();
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
	BufferInformation bufferInfo{};

	bufferInfo.memoryProperty = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT
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
