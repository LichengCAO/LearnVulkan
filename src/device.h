#pragma once
#include "common.h"
#include "vk_struct.h"
#include <VkBootstrap.h>

class DescriptorAllocator;
class Image;
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
	void _CreateCommandPools();
	void _InitDescriptorAllocator();

public:
	GLFWwindow*			pWindow = nullptr;
	VkInstance			vkInstance = VK_NULL_HANDLE;
	VkSurfaceKHR		vkSurface = VK_NULL_HANDLE;
	VkPhysicalDevice	vkPhysicalDevice = VK_NULL_HANDLE;
	VkDevice			vkDevice = VK_NULL_HANDLE;
	QueueFamilyIndices	queueFamilyIndices;
	VkSwapchainKHR		vkSwapchain = VK_NULL_HANDLE;
	DescriptorAllocator descriptorAllocator;
	std::unordered_map<uint32_t, VkCommandPool>		vkCommandPools;
	void Init();
	void Uninit();
	
	void CreateSwapchain();
	std::vector<Image> GetSwapchainImages() const;
	void DestroySwapchain();
	VkExtent2D GetCurrentSwapchainExtent() const;

	// TODO:
	void StartFrame();
	void EndFrame();
public:
	static MyDevice& GetInstance();
};