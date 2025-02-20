#pragma once
#include "common.h"

#include "pipeline.h"
#include "pipeline_io.h"
#include "image.h"
#include "buffer.h"
#include "commandbuffer.h"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>
#include "glm/glm.hpp"

struct UBO
{
	alignas(16) glm::mat4 model;
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 proj;
};

struct Vertex
{
	alignas(16) glm::vec3 pos;
	//alignas(16) glm::vec3 color;
	alignas(8)  glm::vec2 uv;
	bool operator==(const Vertex& other) const { return pos == other.pos && uv == other.uv; }
};
namespace std 
{
	template<> struct hash<Vertex> 
	{
		size_t operator()(Vertex const& vertex) const 
		{
			return hash<glm::vec3>()(vertex.pos) ^
				(hash<glm::vec2>()(vertex.uv) << 1);
		}
	};
}

class TransparentApp
{
private:
	uint32_t m_currentFrame = 0;
	// Descriptor sets
	DescriptorSetLayout m_dSetLayout;
	std::vector<Buffer> m_uniformBuffers;
	std::vector<DescriptorSet> m_dSets;
	// Vertex inputs
	VertexInputLayout m_vertLayout;
	std::vector<Buffer> m_vertBuffers;
	std::vector<Buffer> m_indexBuffers;
	// Renderpass
	RenderPass m_renderPass;
	std::vector<Image> m_swapchainImages;
	std::vector<ImageView> m_swapchainImageViews;
	std::vector<Image> m_depthImages;
	std::vector<ImageView> m_depthImageViews;
	std::vector<Framebuffer> m_framebuffers;
	//pipelines
	GraphicsPipeline m_gPipeline;
	// semaphores
	std::vector<VkSemaphore>	   m_swapchainImageAvailabilities;
	std::vector<CommandSubmission> m_commandSubmissions;
private:
	void _Init();
	void _InitVertexInputs();
	void _InitDescriptorSets();
	void _InitRenderPass();
	void _InitImageViewsAndFramebuffers();
	void _InitPipelines();
	void _MainLoop();
	void _DrawFrame();
	void _UninitVertexInputs();
	void _UninitDescriptorSets();
	void _UninitRenderPass();
	void _UninitImageViewsAndFramebuffers();
	void _UninitPipelines();
	void _Uninit();
public:
	void Run();
};