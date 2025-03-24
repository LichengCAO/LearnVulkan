#pragma once
#include "common.h"
#include "vk_struct.h"
#include <VkBootstrap.h>
#include "pipeline_io.h"

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

class Image;

class MyDevice
{
private:
	static MyDevice* s_pInstance;

	vkb::Instance		m_instance;
	vkb::PhysicalDevice m_physicalDevice;
	vkb::Device			m_device;
	vkb::DispatchTable	m_dispatchTable;
	vkb::Swapchain		m_swapchain;
	VkQueue				m_vkPresentQueue = VK_NULL_HANDLE;
	bool				m_needRecreate = false;
	UserInput			m_userInput{};
private:
	MyDevice() {};

	std::vector<const char*> _GetInstanceRequiredExtensions() const;
	std::vector<const char*> _GetPhysicalDeviceRequiredExtensions() const;
	QueueFamilyIndices		 _GetQueueFamilyIndices(VkPhysicalDevice physicalDevice) const;
	SwapChainSupportDetails  _QuerySwapchainSupport(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface) const;
	VkSurfaceFormatKHR		 _ChooseSwapchainFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) const;
	VkPresentModeKHR		 _ChooseSwapchainPresentMode(const std::vector<VkPresentModeKHR>& availableModes) const;
	VkExtent2D				 _ChooseSwapchainExtent(const VkSurfaceCapabilitiesKHR& capabilities) const;

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

public:
	GLFWwindow*			pWindow = nullptr;
	VkInstance			vkInstance = VK_NULL_HANDLE;
	VkSurfaceKHR		vkSurface = VK_NULL_HANDLE;
	VkPhysicalDevice	vkPhysicalDevice = VK_NULL_HANDLE;
	VkDevice			vkDevice = VK_NULL_HANDLE;
	QueueFamilyIndices	queueFamilyIndices{};
	VkSwapchainKHR		vkSwapchain = VK_NULL_HANDLE;
	DescriptorAllocator descriptorAllocator{};
	std::unordered_map<uint32_t, VkCommandPool>		vkCommandPools;
	
	
	void Init();
	void Uninit();
	
	void RecreateSwapchain();
	std::vector<Image> GetSwapchainImages() const;
	std::optional<uint32_t> AquireAvailableSwapchainImageIndex(VkSemaphore finishSignal);
	void PresentSwapchainImage(const std::vector<VkSemaphore>& waitSemaphores, uint32_t imageIdx);
	bool NeedRecreateSwapchain() const;
	VkExtent2D GetSwapchainExtent() const;

	VkFormat FindSupportFormat(const std::vector<VkFormat>& candidates, VkImageTiling tilling, VkFormatFeatureFlags features) const;
	VkFormat GetDepthFormat() const;
	VkFormat GetSwapchainFormat() const;

	UserInput GetUserInput() const;
public:
	static MyDevice& GetInstance();
	static void OnFramebufferResized(GLFWwindow* _pWindow, int width, int height);
	static void CursorPosCallBack(GLFWwindow* _pWindow, double _xPos, double _yPos);
};