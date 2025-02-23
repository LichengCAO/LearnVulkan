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
struct UBO
{
	alignas(16) glm::mat4 model;
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 proj;
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

class TransparentApp
{
private:
	uint32_t m_currentFrame = 0;
	// Descriptor sets
	DescriptorSetLayout m_dSetLayout;
	Texture m_texture{};
	std::vector<DescriptorSet> m_dSets;
	
	DescriptorSetLayout m_gbufferDSetLayout;
	std::vector<Buffer> m_uniformBuffers;
	std::vector<DescriptorSet> m_gbufferDSets;
	// Vertex inputs
	VertexInputLayout m_vertLayout;
	std::vector<Buffer> m_vertBuffers;
	std::vector<Buffer> m_indexBuffers;

	VertexInputLayout m_gbufferVertLayout;
	Buffer m_gbufferVertBuffer;
	Buffer m_gbufferIndexBuffer;
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

	RenderPass m_renderPass;
	std::vector<Image> m_swapchainImages;
	std::vector<ImageView> m_swapchainImageViews;
	std::vector<Framebuffer> m_framebuffers;
	//pipelines
	GraphicsPipeline m_gPipeline;
	GraphicsPipeline m_gbufferPipeline;
	// semaphores
	std::vector<VkSemaphore>	   m_swapchainImageAvailabilities;
	std::vector<CommandSubmission> m_commandSubmissions;
	PersCamera m_camera{400, 300, glm::vec3(2,2,2), glm::vec3(0,0,0), glm::vec3(0,0,1)};
private:
	void _Init();
	void _Uninit();
	void _InitVertexInputs();
	void _UninitVertexInputs();
	void _InitDescriptorSets();
	void _UninitDescriptorSets();
	void _InitRenderPass();
	void _UninitRenderPass();
	void _InitImageViewsAndFramebuffers();
	void _UninitImageViewsAndFramebuffers();
	void _InitPipelines();
	void _UninitPipelines();
	
	void _MainLoop();
	void _UpdateUniformBuffer();
	void _DrawFrame();

public:
	void Run();
};