#pragma once
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
#include "ray_tracing.h"

class RayTracingApp
{
private:
	struct Model
	{
		Mesh mesh;
		Transform transform;
	};
	// no need to use alignas here, since we will use scalarEXT in shader
	struct VBO
	{
		/*alignas(16)*/ glm::vec3 pos;
		/*alignas(16)*/ glm::vec3 normal;
	};
	struct CameraUBO
	{
		glm::mat4 view;
		glm::mat4 proj;
		glm::vec4 eye;
	};

private:
	double lastTime = 0.0;
	float frameTime = 0.0f;
	uint32_t m_currentFrame = 0;
	PersCamera m_camera{ 400, 300, glm::vec3(2,2,2), glm::vec3(0,0,0), glm::vec3(0,0,1) };
	MyDevice* pDevice = nullptr;

	std::vector<Model> m_models;

	// cameraUBO changes across frames, i create buffers for each frame
	DescriptorSetLayout m_cameraDSetLayout;
	std::vector<std::unique_ptr<DescriptorSet>> m_cameraDSets;
	std::vector<std::unique_ptr<Buffer>>        m_cameraBuffers;

	// following buffers can be used cross frames, so i just create one buffer for one mesh instead of multiple buffers for each frame
	DescriptorSetLayout m_modelDSetLayout;
	std::unique_ptr<DescriptorSet> m_ASDSet;
	std::vector<std::unique_ptr<Buffer>> m_vertexBuffers;
	std::vector<std::unique_ptr<Buffer>> m_indexBuffers;
	RayTracingAccelerationStructure m_rayTracingAS;

	// Samplers
	VkSampler m_vkSampler = VK_NULL_HANDLE;

	// Renderpass
	RenderPass m_renderPass;
	std::vector<Image> m_swapchainImages;
	std::vector<std::unique_ptr<ImageView>> m_swapchainImageViews;
	std::vector<std::unique_ptr<Framebuffer>> m_framebuffers;

	//pipelines
	RayTracingPipeline m_pipeline;

	// semaphores
	std::vector<VkSemaphore>  m_swapchainImageAvailabilities;
	std::vector<std::unique_ptr<CommandSubmission>> m_commandSubmissions;

private:
	void _Init();
	void _Uninit();

	void _InitRenderPass();
	void _UninitRenderPass();

	void _InitDescriptorSetLayouts();
	void _UninitDescriptorSetLayouts();

	void _InitSampler();
	void _UninitSampler();

	void _InitModels();
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