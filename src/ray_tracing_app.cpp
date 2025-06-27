#include "ray_tracing_app.h"

void RayTracingApp::_Init()
{
	_InitRenderPass();

	_InitDescriptorSetLayouts();

	_InitSampler();

	_InitModels();
	_InitBuffers();

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
