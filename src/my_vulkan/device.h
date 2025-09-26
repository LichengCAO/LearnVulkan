#pragma once
#include "common.h"
#include "vk_struct.h"
#include <VkBootstrap.h>
#include "pipeline_io.h"
#include "image.h"
#include "sampler.h"

class MemoryAllocator;

struct UserInput
{
	bool W = false;
	bool S = false;
	bool A = false;
	bool D = false;
	bool Q = false;
	bool E = false;
	bool LMB = false;
	bool RMB = false;
	double xPos = 0.0;
	double yPos = 0.0;
};

class MyDevice final
{
private:
	static std::unique_ptr<MyDevice> s_uptrInstance;

	vkb::Instance		m_instance;
	vkb::PhysicalDevice m_physicalDevice;
	vkb::Device			m_device;
	vkb::DispatchTable	m_dispatchTable;
	vkb::Swapchain		m_swapchain;
	VkQueue				m_vkPresentQueue = VK_NULL_HANDLE;
	bool				m_needRecreate = false;
	bool				m_initialized = false;
	UserInput			m_userInput{};
	std::vector<std::unique_ptr<Image>> m_uptrSwapchainImages;
	std::unique_ptr<MemoryAllocator> m_uptrMemoryAllocator;

private:
	MyDevice();
	MyDevice(const MyDevice& _other) = delete;

	std::vector<const char*> _GetInstanceRequiredExtensions() const;
	QueueFamilyIndices		 _GetQueueFamilyIndices(VkPhysicalDevice physicalDevice) const;
	SwapChainSupportDetails  _QuerySwapchainSupport(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface) const;
	VkSurfaceFormatKHR		 _ChooseSwapchainFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) const;
	VkPresentModeKHR		 _ChooseSwapchainPresentMode(const std::vector<VkPresentModeKHR>& availableModes) const;
	VkExtent2D				 _ChooseSwapchainExtent(const VkSurfaceCapabilitiesKHR& capabilities) const;

	void _InitVolk();
	void _InitGLFW();
	void _CreateInstance();
	void _CreateSurface();
	void _SelectPhysicalDevice();
	void _CreateLogicalDevice();
	void _CreateCommandPools();
	void _DestroyCommandPools();
	void _InitDescriptorAllocator();
	void _CreateSwapchain();
	void _DestroySwapchain();
	void _CreateMemoryAllocator();
	void _DestroyMemoryAllocator();

	// Add required extensions to the device, before select physical device
	void _AddBaseExtensionsAndFeatures(vkb::PhysicalDeviceSelector& _selector) const;
	void _AddRayTracingExtensionsAndFeatures(vkb::PhysicalDeviceSelector& _selector) const;
	void _AddMeshShaderExtensionsAndFeatures(vkb::PhysicalDeviceSelector& _selector) const;
	void _AddRayQueryExtensionsAndFeatures(vkb::PhysicalDeviceSelector& _selector) const;

	// get VkImages in current swapchain
	void _GetVkSwapchainImages(std::vector<VkImage>& _vkImages) const;

	// update m_uptrSwapchainImages
	void _UpdateSwapchainImages();

public:
	GLFWwindow*			pWindow = nullptr;
	VkInstance			vkInstance = VK_NULL_HANDLE;
	VkSurfaceKHR		vkSurface = VK_NULL_HANDLE;
	VkPhysicalDevice	vkPhysicalDevice = VK_NULL_HANDLE;
	VkDevice			vkDevice = VK_NULL_HANDLE;
	QueueFamilyIndices	queueFamilyIndices{};
	VkSwapchainKHR		vkSwapchain = VK_NULL_HANDLE;
	SamplerPool         samplerPool{};
	DescriptorSetAllocator descriptorAllocator{};
	std::unordered_map<uint32_t, VkCommandPool>		vkCommandPools;
	std::unordered_map<VkImage, ImageLayout>        imageLayouts;
	
	~MyDevice();

	void Init();
	void Uninit();
	
	void RecreateSwapchain();
	std::vector<Image> GetSwapchainImages() const; //deprecated
	void GetSwapchainImagePointers(std::vector<Image*>& _output) const;
	std::optional<uint32_t> AquireAvailableSwapchainImageIndex(VkSemaphore finishSignal);
	void PresentSwapchainImage(const std::vector<VkSemaphore>& waitSemaphores, uint32_t imageIdx);
	bool NeedRecreateSwapchain() const;
	VkExtent2D GetSwapchainExtent() const;
	VkFormat FindSupportFormat(const std::vector<VkFormat>& candidates, VkImageTiling tilling, VkFormatFeatureFlags features) const;
	VkFormat GetDepthFormat() const;
	VkFormat GetSwapchainFormat() const;
	UserInput GetUserInput() const;
	void WaitIdle() const;

	MemoryAllocator* GetMemoryAllocator();

public:
	static MyDevice& GetInstance();
	static void OnFramebufferResized(GLFWwindow* _pWindow, int width, int height);
	static void CursorPosCallBack(GLFWwindow* _pWindow, double _xPos, double _yPos);

	// do this or use static std::unique_ptr<MyDevice> instance{ new MyDevice() };
	friend std::unique_ptr<MyDevice> std::make_unique<MyDevice>();
};