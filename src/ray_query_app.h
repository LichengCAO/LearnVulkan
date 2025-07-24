#pragma once
#include "common.h"
#include "geometry.h"
#include "transform.h"
#include "utils.h"
#include "camera.h"
#include "pipeline_io.h"
#include "pipeline.h"
#include "acceleration_structure.h"

class MyDevice;
class Buffer;
class Image;
class SwapchainPass;

class RayQueryApp
{
private:
	struct Model
	{
		Mesh mesh;
		Transform transform;
	};
	struct VBO
	{
		alignas(16) glm::vec4 position;
		alignas(16) glm::vec4 normal;
	};
	struct ObjectUBO
	{
		glm::mat4 model;
	};
	struct CameraUBO
	{
		glm::mat4 viewProj;
	};
	struct InstanceInformation
	{
		// Describes information of a model on device side
		VkDeviceAddress vertexBuffer;
		VkDeviceAddress indexBuffer;
	};

private:
	double lastTime = 0.0;
	float frameTime = 0.0f;
	uint32_t m_currentFrame = 0;
	PersCamera m_camera{ 400, 300, glm::vec3(2,2,2), glm::vec3(0,0,0), glm::vec3(0,0,1) };
	MyDevice* pDevice = nullptr;

	std::vector<Model> m_models;

	// Descriptor set count: MAX_FRAME_COUNT
	DescriptorSetLayout m_rtDSetLayout;
	std::vector<std::unique_ptr<DescriptorSet>> m_rtDSets;

	// cameraUBO changes across frames, i create buffers for each frame
	std::vector<std::unique_ptr<Buffer>> m_cameraBuffers;

	std::unique_ptr<Buffer> m_instanceBuffer;
	std::vector<std::unique_ptr<Buffer>> m_vertexBuffers;
	std::vector<std::unique_ptr<Buffer>> m_indexBuffers;
	std::vector<std::unique_ptr<Buffer>> m_modelBuffers;

	RayTracingAccelerationStructure m_rtAccelStruct;

	// Samplers
	VkSampler m_vkSampler = VK_NULL_HANDLE;

	// images
	std::vector<std::unique_ptr<Image>> m_vecColorImage;
	std::vector<std::unique_ptr<Image>> m_vecDepthImage;
	std::vector<std::unique_ptr<ImageView>> m_vecColorImageView;
	std::vector<std::unique_ptr<ImageView>> m_vecDepthImageView;

	// Render pass and framebuffers
	std::unique_ptr<RenderPass> m_uptrRenderPass;
	std::vector<std::unique_ptr<Framebuffer>> m_vecFramebuffer;

	//pipelines
	GraphicsPipeline m_graphicPipeline;

	// command buffers
	std::vector<std::unique_ptr<CommandSubmission>> m_commandSubmissions;

	// swapchain pass
	std::unique_ptr<SwapchainPass> m_swapchainPass;

private:
	void _Init();
	void _Uninit();

	void _InitDescriptorSetLayouts();
	void _UninitDescriptorSetLayouts();

	void _InitSampler();
	void _UninitSampler();

	void _InitModels();
	void _InitBuffers();
	void _UninitBuffers();

	void _InitAS();
	void _UninitAS();

	void _InitImagesAndViews();
	void _UninitImagesAndViews();

	void _InitRenderPass();
	void _UninitRenderPass();

	void _InitFramebuffers();
	void _UninitFramebuffers();

	void _InitDescriptorSets();
	void _UninitDescriptorSets();

	void _InitPipelines();
	void _UninitPipelines();

	void _InitSwapchainPass();
	void _UninitSwapchainPass();

	void _InitCommandBuffers();
	void _UninitCommandBuffers();

	void _UpdateUniformBuffer();

	void _MainLoop();

	void _DrawFrame();

	void _ResizeWindow();

public:
	// Need to implement these, otherwise default constructor/destructor will need full type of all attributes in header file
	RayQueryApp();
	~RayQueryApp();

	void Run();
};