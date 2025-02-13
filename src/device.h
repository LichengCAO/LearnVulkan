#pragma once
#include "trig_app.h"

#define VK_CHECK(vkcommand, message) \
do{\
if((vkcommand)!=VK_SUCCESS){\
   throw std::runtime_error(\
#message);\
}\
}while(0)

#define CHECK_TRUE(condition, failedMessage) \
do{\
if(!(condition)){\
   throw std::runtime_error(\
#failedMessage);\
}\
}while(0)

class MyDevice
{
private:
	static MyDevice s_instance;

	vkb::Instance		m_instance;
	vkb::PhysicalDevice m_physicalDevice;
	vkb::Device			m_device;
	vkb::DispatchTable	m_dispatchTable;
	vkb::Swapchain		m_swapchain;

	std::vector<std::vector<VkSemaphore>> m_frameSemaphores;
	std::vector<std::vector<VkFence>> m_frameFences;
	std::vector<std::vector<VkCommandBuffer>> m_frameCommandBuffers;
	uint32_t m_currentFrame = 0;
	uint32_t m_currentCommandBuffer = 0;
	uint32_t m_currentSemophore = 0;
private:
	MyDevice();

	std::vector<const char*> _GetInstanceRequiredExtensions() const;
	std::vector<const char*> _GetPhysicalDeviceRequiredExtensions() const;
	QueueFamilyIndices		 _GetQueueFamilyIndices(VkPhysicalDevice physicalDevice) const;
	SwapChainSupportDetails  _QuerySwapchainSupport(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface) const;
	VkSurfaceFormatKHR		 _ChooseSwapchainFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) const;
	VkPresentModeKHR		 _ChooseSwapchainPresentMode(const std::vector<VkPresentModeKHR>& availableModes) const;
	VkExtent2D				 _ChooseSwapchainExtent(const VkSurfaceCapabilitiesKHR& capabilities) const;
	VkSemaphore				 _GetCurrentSemaphore();
	VkFence					 _GetCurrentFence();

	void _InitGLFW();
	void _CreateInstance();
	void _CreateSurface();
	void _SelectPhysicalDevice();
	void _CreateLogicalDevice();
	void _CreateSwapChain();
	void _CreateCommandPool();// TODO
	void _CreateDescriptorPool(); // TODO

public:
	GLFWwindow*			pWindow = nullptr;
	VkInstance			vkInstance = VK_NULL_HANDLE;
	VkSurfaceKHR		vkSurface = VK_NULL_HANDLE;
	VkPhysicalDevice	vkPhysicalDevice = VK_NULL_HANDLE;
	VkDevice			vkDevice = VK_NULL_HANDLE;
	QueueFamilyIndices	queueFamilyIndices;
	VkSwapchainKHR		vkSwapchain = VK_NULL_HANDLE;
	std::vector<VkCommandPool>		vkCommandPools;
	VkDescriptorPool	vkDescriptorPool = VK_NULL_HANDLE;
	void Init();
	void Uninit();

	// TODO:
	void StartOneTimeCommands();
	void FinishOneTimeCommands();

	void StartGraphicsCommandBuffer();
	void FinishGraphicsCommandBuffer();

	void StartComputeCommandBuffer();
	void FinishComputeCommandBuffer();

	void StartFrame();
	void EndFrame();
public:
	static MyDevice& GetInstance();
};