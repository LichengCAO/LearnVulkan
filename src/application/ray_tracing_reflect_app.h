#pragma once
#include "common.h"
#include "acceleration_structure.h"
#include "render_object/camera.h"

class RayTracingProgram;
class SwapchainPass;
class Buffer;
class Image;
class ImageView;
class CommandSubmission;

class RayTracingReflectApp
{
private:
	struct CameraUBO
	{
		glm::mat4 inverseViewProj;
		glm::vec4 eye;
	};
	struct AddressData
	{
		uint64_t vertexAddress;
		uint64_t indexAddress;
	};
	struct Material
	{
		glm::vec4 colorOrLight; // xyz: rgb, w: is_light
	};
	struct Vertex
	{
		glm::vec4 position;
		glm::vec4 normal;
	};

	std::unique_ptr<RayTracingProgram> m_uptrPipeline;
	std::unique_ptr<SwapchainPass> m_uptrSwapchainPass;
	std::unique_ptr<RayTracingAccelerationStructure> m_uptrAccelStruct;
	std::unique_ptr<Buffer> m_uptrAddressBuffer;
	std::unique_ptr<Buffer> m_uptrMaterialBuffer;

	std::vector<std::unique_ptr<Buffer>> m_uptrModelVertexBuffers;
	std::vector<std::unique_ptr<Buffer>> m_uptrModelIndexBuffers;
	std::vector<std::unique_ptr<Buffer>> m_uptrCameraBuffers;
	
	std::vector<std::unique_ptr<Image>> m_uptrOutputImages;
	std::vector<std::unique_ptr<ImageView>> m_uptrOutputViews;
	std::vector<std::unique_ptr<CommandSubmission>> m_uptrCommands;
	std::vector<RayTracingAccelerationStructure::TriangleData> m_rayTracingGeometryData;
	std::vector<glm::mat4> m_rayTracingGeometryTransform;
	VkSampler m_vkSampler = VK_NULL_HANDLE;
	uint32_t m_currentFrame = 0u;
	PersCamera m_camera{ 400, 300, glm::vec3(2,2,2), glm::vec3(0,0,0), glm::vec3(0,1,0) };

private:
	void _CreateCommandBuffers();
	void _DestroyCommandBuffers();

	void _InitAccelerationStructures();
	void _UninitAccelerationStructures();

	void _InitPipeline();
	void _UninitPipeline();

	void _CreateImagesAndViews();
	void _DestroyImagesAndViews();

	void _CreateBuffers();
	void _DestroyBuffers();

	void _UpdateUniformBuffer();

	void _Init();
	void _Uninit();

	void _ResizeWindow();
	void _DrawFrame();

public:
	RayTracingReflectApp();
	~RayTracingReflectApp();

	void Run();
};