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
#include "transform.h"
#include "utils.h"

#define MYTYPE(x) std::vector<std::unique_ptr<x>>

class MeshletApp
{
private:
	struct Model
	{
		Mesh mesh;
		Transform transform;
	};
	struct VBO
	{
		alignas(16) glm::vec3 pos;
		alignas(16) glm::vec3 normal;
	};
	struct ModelTransformUBO 
	{
		glm::mat4 modelTransform;
		glm::mat4 normalTransform; // inverse transpose of model transform
	};
	struct CameraUBO
	{
		alignas(16) glm::mat4 view;
		alignas(16) glm::mat4 proj;
	};
private:
	double lastTime = 0.0;
	float frameTime = 0.0f;
	uint32_t m_currentFrame = 0;
	PersCamera m_camera{ 400, 300, glm::vec3(2,2,2), glm::vec3(0,0,0), glm::vec3(0,0,1) };
	MyDevice* pDevice = nullptr;

	std::vector<Model> m_models;

	// Descriptor sets
	DescriptorSetLayout m_cameraDSetLayout;
	MYTYPE(DescriptorSet) m_cameraDSets;
	MYTYPE(Buffer) m_cameraBuffers;

	DescriptorSetLayout m_modelTransformDSetLayout;
	std::vector<MYTYPE(DescriptorSet)> m_modelTransformDSets;
	std::vector<MYTYPE(Buffer)>        m_modelTransformBuffers;

	DescriptorSetLayout m_modelVertDSetLayout;
	std::vector<std::unique_ptr<DescriptorSet>> m_modelVertDSets;
	std::vector<std::unique_ptr<Buffer>>        m_modelVertBuffers;

	// Vertex inputs
	VertexInputLayout m_quadVertLayout;
	Buffer m_quadVertBuffer;
	Buffer m_quadIndexBuffer;

	// Samplers
	VkSampler m_vkSampler = VK_NULL_HANDLE;

	// Renderpass
	RenderPass m_renderPass;
	std::vector<Image> m_swapchainImages;
	MYTYPE(Image) m_depthImages;
	MYTYPE(ImageView) m_depthImageViews;
	MYTYPE(ImageView) m_swapchainImageViews;
	MYTYPE(Framebuffer) m_framebuffers;

	//pipelines
	GraphicsPipeline m_pipeline;

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

	void _InitPipelines();
	void _UninitPipelines();

	void _MainLoop();
	void _UpdateUniformBuffer();
	void _DrawFrame();

	void _ResizeWindow();
	
	VkImageLayout _GetImageLayout(ImageView* pImageView) const;
	VkImageLayout _GetImageLayout(VkImage vkImage, uint32_t baseArrayLayer, uint32_t layerCount, uint32_t baseMipLevel, uint32_t levelCount, VkImageAspectFlags aspect) const;
public:
	void Run();
};