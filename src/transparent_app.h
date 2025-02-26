#pragma once
#include "common.h"
#include "pipeline.h"
#include "pipeline_io.h"
#include "image.h"
#include "buffer.h"
#include "commandbuffer.h"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>
#include "camera.h"
#include "transform.h"
struct CameraBuffer
{
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 proj;
};
struct ModelTransform
{
	alignas(16) glm::mat4 model;
	alignas(16) glm::mat4 modelInvTranspose;
};
struct Vertex
{
	alignas(16) glm::vec3 pos{};
	alignas(16) glm::vec3 normal{};
	alignas(8)  glm::vec2 uv{};
	bool operator==(const Vertex& other) const { return pos == other.pos && uv == other.uv && normal == other.normal; }
};
namespace std 
{
	template<> struct hash<Vertex> 
	{
		size_t operator()(Vertex const& vertex) const 
		{
			return hash<glm::vec3>()(vertex.pos) ^
				  (hash<glm::vec2>()(vertex.uv) << 1)
				;
		}
	};
}
struct Model
{
	std::string objFilePath;
	std::string texturePath;
	Transform   transform{};
};

class TransparentApp
{
private:
	double lastTime = 0.0;
	float frameTime = 0.0f;
	uint32_t m_currentFrame = 0;
	std::vector<Model> m_models;

	// Descriptor sets
	DescriptorSetLayout m_oitDSetLayout;
	std::vector<DescriptorSet> m_oitDSets;
	std::vector<Buffer> m_oitSampleTexelBuffers;
	std::vector<BufferView> m_oitSampleTexelBufferViews;
	std::vector<Image> m_oitSampleCountImages;
	std::vector<ImageView> m_oitSampleCountImageViews;
	std::vector<Image> m_oitInUseImages;
	std::vector<ImageView> m_oitInUseImageViews;
	Buffer             m_oitViewportBuffer; //ok

	DescriptorSetLayout m_dSetLayout;
	std::vector<DescriptorSet> m_dSets;
	
	DescriptorSetLayout m_modelDSetLayout;
	std::vector<std::vector<DescriptorSet>> m_vecModelDSets;
	std::vector<std::vector<Buffer>> m_vecModelBuffers;
	std::vector<Texture> m_modelTextures;
	
	DescriptorSetLayout m_cameraDSetLayout;
	std::vector<DescriptorSet> m_cameraDSets;
	std::vector<Buffer> m_cameraBuffers;

	// Vertex inputs
	VertexInputLayout m_gbufferVertLayout;
	std::vector<Buffer> m_gbufferVertBuffers;
	std::vector<Buffer> m_gbufferIndexBuffers;
	
	VertexInputLayout m_vertLayout;
	Buffer m_vertBuffer;
	Buffer m_indexBuffer;
	// Samplers
	VkSampler m_vkSampler = VK_NULL_HANDLE;
	// Renderpass
	RenderPass m_gbufferRenderPass;
	std::vector<Image> m_depthImages;
	std::vector<ImageView> m_depthImageViews;
	std::vector<Image> m_gbufferPosImages;
	std::vector<ImageView> m_gbufferPosImageViews;
	std::vector<Image> m_gbufferNormalImages;
	std::vector<ImageView> m_gbufferNormalImageViews;
	std::vector<Image> m_gbufferAlbedoImages;
	std::vector<ImageView> m_gbufferAlbedoImageViews;
	std::vector<Framebuffer> m_gbufferFramebuffers;

	RenderPass m_oitRenderPass;

	RenderPass m_renderPass;
	std::vector<Image> m_swapchainImages;
	std::vector<ImageView> m_swapchainImageViews;
	std::vector<Framebuffer> m_framebuffers;
	//pipelines
	GraphicsPipeline m_gbufferPipeline;
	
	GraphicsPipeline m_oitPipeline;
	ComputePipeline  m_oitSortPipeline;

	GraphicsPipeline m_gPipeline;
	// semaphores
	std::vector<VkSemaphore>	   m_swapchainImageAvailabilities;
	std::vector<CommandSubmission> m_commandSubmissions;
	PersCamera m_camera{400, 300, glm::vec3(2,2,2), glm::vec3(0,0,0), glm::vec3(0,0,1)};
private:
	void _Init();
	void _Uninit();

	void _InitRenderPass();
	void _UninitRenderPass();

	void _InitDescriptorSetLayouts();
	void _UninitDescriptorSetLayouts();
	
	void _InitSampler();
	void _UninitSampler();

	void _InitBuffers();
	void _UninitBuffers();

	void _InitImagesAndViews();
	void _UninitImagesAndViews();
	
	void _InitFramebuffers();
	void _UninitFramebuffers();

	void _InitDescriptorSets();
	void _UninitDescriptorSets();
	
	void _InitVertexInputs();
	void _UninitVertexInputs();
	
	void _InitPipelines();
	void _UninitPipelines();
	
	void _FillDeviceMemoryWithZero();
	
	void _MainLoop();
	void _UpdateUniformBuffer();
	void _DrawFrame();

public:
	void Run();
};