#pragma once
#include "common.h"
#include "acceleration_structure.h"
#include "render_object/camera.h"

class RayTracingProgram;
class SwapchainPass;
class GUIPass;
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
		glm::uvec4 materialType;
	};
	struct Vertex
	{
		glm::vec4 position;
		glm::vec4 normal;
	};
	struct Coefficient
	{
		float sigma_a = 0.001;
		float sigma_s = 0.001;
		float g;
		bool operator==(const Coefficient& _other)
		{
			return _other.g == g && _other.sigma_a == sigma_a && _other.sigma_s == sigma_s;
		}
		bool operator!=(const Coefficient& _other)
		{
			return !(*this == _other);
		}
	};

	std::unique_ptr<RayTracingProgram> m_uptrPipeline;
	std::unique_ptr<SwapchainPass> m_uptrSwapchainPass;
	std::unique_ptr<GUIPass> m_uptrGUIPass;
	std::unique_ptr<RayTracingAccelerationStructure> m_uptrAccelStruct;
	std::unique_ptr<Buffer> m_uptrAddressBuffer;
	std::unique_ptr<Buffer> m_uptrMaterialBuffer;
	std::unique_ptr<Buffer> m_uptrNanoVDBBuffer;

	std::vector<std::unique_ptr<Buffer>> m_uptrModelVertexBuffers;
	std::vector<std::unique_ptr<Buffer>> m_uptrModelIndexBuffers;
	std::vector<std::unique_ptr<Buffer>> m_uptrCameraBuffers;
	std::unique_ptr<Buffer> m_uptrAABBBuffer;
	
	std::vector<std::unique_ptr<Image>> m_uptrOutputImages;
	std::vector<std::unique_ptr<ImageView>> m_uptrOutputViews;
	std::vector<std::unique_ptr<CommandSubmission>> m_uptrCommands;
	std::vector<RayTracingAccelerationStructure::TriangleData> m_rayTracingGeometryData;
	std::vector<glm::mat4> m_rayTracingGeometryTransform;
	std::vector<VkSemaphore> m_semaphores;
	VkSampler m_vkSampler = VK_NULL_HANDLE;
	uint32_t m_currentFrame = 0u;
	uint32_t m_rayTraceFrame = 0u;
	Coefficient m_coefficient{};
	Coefficient m_coefficientLast{};
	float m_fps = 0;
	PersCamera m_camera{ 400, 300, glm::vec3(2, 2, 2), glm::vec3(0,0,0), glm::vec3(0,1,0) };

private:
	void _CreateSemaphores();
	void _DestroySemaphores();

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

class RayTracingNanoVDBApp
{
private:
	std::unique_ptr<Buffer> m_uptrAABBBuffer;
	std::unique_ptr<Buffer> m_uptrCameraBuffer;
	std::unique_ptr<Buffer> m_uptrVDBBuffer;
	std::unique_ptr<RayTracingProgram>							m_uptrProgram;
	std::unique_ptr<RayTracingAccelerationStructure>	m_uptrAccelerationStructure;
	std::unique_ptr<GUIPass>				m_uptrGUIPass;
	std::unique_ptr<SwapchainPass>	m_uptrSwapchainPass;
	std::unique_ptr<Image>						m_uptrOutputImage;
	std::unique_ptr<ImageView>				m_uptrOutputView;
	std::unique_ptr<CommandSubmission> m_uptrCmd;


private:
	void _InitProgram();
	void _UnintProgram();

	void _InitBuffers();
	void _UninitBuffers();
	void _UpdateUniformBuffer();

	void _CreateImageAndViews();
	void _DestroyImageAndViews();

	void _InitAccelerationStructure();
	void _UninitAccelerationStructure();

	void _Init();
	void _Uninit();

	void _ResizeWindow();
	void _DrawFrame();

public:
	RayTracingNanoVDBApp();
	~RayTracingNanoVDBApp();

	void Run();
};