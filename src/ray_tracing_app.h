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

class SwapchainPass;

class RayTracingApp
{
private:
	struct Model
	{
		Mesh mesh;
		Transform transform;
	};
	struct VBO
	{
		// no need to use alignas here, since we will use scalarEXT in shader, 
		// but it won't work somehow, do not know the reason for it yet
		alignas(16) glm::vec4 pos;
		alignas(16) glm::vec4 normal;
	};
	struct CameraUBO
	{
		glm::mat4 inverseViewProj;
		glm::vec4 eye;
	};
	
	// Describes information of a model on device side
	struct InstanceInformation
	{
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
	DescriptorSetLayout m_compDSetLayout;
	std::vector<std::unique_ptr<DescriptorSet>> m_compDSets;

	// cameraUBO changes across frames, i create buffers for each frame
	std::vector<std::unique_ptr<Buffer>> m_cameraBuffers;

	std::vector<std::unique_ptr<Buffer>> m_instanceBuffer;
	std::vector<std::vector<std::unique_ptr<Buffer>>> m_vertexBuffers;
	std::vector<std::vector<std::unique_ptr<Buffer>>> m_indexBuffers;

	std::vector<RayTracingAccelerationStructure> m_rtAccelStruct;

	// Samplers
	VkSampler m_vkSampler = VK_NULL_HANDLE;

	// images
	std::vector<std::unique_ptr<Image>> m_rtImages;
	std::vector<std::unique_ptr<ImageView>> m_rtImageViews;

	//pipelines
	RayTracingPipeline m_rtPipeline;
	ComputePipeline m_compPipeline;

	// command buffers
	std::vector<std::unique_ptr<CommandSubmission>> m_commandSubmissions;

	// swapchain pass
	std::unique_ptr<SwapchainPass> m_swapchainPass;

	// TLAS inputs to update TLAS
	std::vector<RayTracingAccelerationStructure::InstanceData> m_TLASInputs;
	std::vector<RayTracingAccelerationStructure::TriangleData> m_BLASInputs;

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

	glm::vec3 _GetNextPosition(float _offest);

public:
	// Need to implement these, otherwise default constructor/destructor will need full type of all attributes in header file
	RayTracingApp();
	~RayTracingApp();
	
	void Run();
};