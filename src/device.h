#pragma once
#include "trig_app.h"

#define VK_CHECK(vkcommand, message) \
do{\
if((vkcommand)!=VK_SUCCESS){\
   throw std::runtime_error(\
#message);\
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
private:
	MyDevice();

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
	void _CreateSwapChain();
	void _CreateCommandPool();// TODO
	void _CreateDescriptorPool(); // TODO

public:
	GLFWwindow*			pWindow = nullptr;
	VkInstance			vkInstance;
	VkSurfaceKHR		vkSurface;
	VkPhysicalDevice	vkPhysicalDevice;
	VkDevice			vkDevice;
	QueueFamilyIndices	queueFamilyIndices;
	VkSwapchainKHR		vkSwapchain;
	VkCommandPool		vkCommandPool;
	VkDescriptorPool	vkDescriptorPool;
	void Init();
	void Uninit();

	// TODO:
	VkCommandBuffer StartOneTimeCommands();
	void FinishOneTimeCommands(VkCommandBuffer commandBuffer);
public:
	static MyDevice& GetInstance();
};