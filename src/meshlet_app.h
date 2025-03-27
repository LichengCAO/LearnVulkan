#pragma once
#include "common.h"
#include "pipeline.h"
#include "pipeline_io.h"
#include "image.h"
#include "camera.h"
#include "commandbuffer.h"
#include "buffer.h"
#include "geometry.h"
#include "device.h"

#define MYTYPE(x) std::vector<std::unique_ptr<x>>

class MeshletApp
{
private:
	double lastTime = 0.0;
	float frameTime = 0.0f;
	uint32_t m_currentFrame = 0;
	PersCamera m_camera{ 400, 300, glm::vec3(2,2,2), glm::vec3(0,0,0), glm::vec3(0,0,1) };
	MyDevice* pDevice = nullptr;

	// Descriptor sets
	DescriptorSetLayout m_cameraDSetLayout;
	MYTYPE(DescriptorSet) m_uptrCameraDSets;
	MYTYPE(Buffer) m_uptrCameraBuffers;

	DescriptorSetLayout m_modelDSetLayout;
	std::vector<MYTYPE(DescriptorSet)> m_vecUptrModelDSets;
	std::vector<MYTYPE(Buffer)>        m_vecUptrModelBuffers;

	DescriptorSetLayout m_modelVertDSetLayout;
	std::vector<MYTYPE(DescriptorSet)> m_vecUptrModelVertDSets;
	std::vector<MYTYPE(Buffer)>        m_vecUptrModelVertBuffers;

	// Vertex inputs
	VertexInputLayout m_quadVertLayout;
	Buffer m_quadVertBuffer;
	Buffer m_quadIndexBuffer;

	// Samplers
	VkSampler m_vkSampler = VK_NULL_HANDLE;
	VkSampler m_vkLodSampler = VK_NULL_HANDLE;

	// Renderpass
	RenderPass m_renderPass;
	MYTYPE(Image) m_depthImages;
	MYTYPE(ImageView) m_depthImageViews;
	MYTYPE(Framebuffer) m_framebuffers;

	//pipelines
	GraphicsPipeline m_gbufferPipeline;

	// semaphores
	std::vector<VkSemaphore>  m_swapchainImageAvailabilities;
	MYTYPE(CommandSubmission) m_commandSubmissions;
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

	void _MainLoop();
	void _UpdateUniformBuffer();
	void _DrawFrame();

	void _ResizeWindow();

	void _ReadObjFile(const std::string& objFile, std::vector<Vertex>& verts, std::vector<uint32_t>& indices) const;
	
	VkImageLayout _GetImageLayout(ImageView* pImageView) const;
	VkImageLayout _GetImageLayout(VkImage vkImage, uint32_t baseArrayLayer, uint32_t layerCount, uint32_t baseMipLevel, uint32_t levelCount, VkImageAspectFlags aspect) const;
public:
	void Run();
};