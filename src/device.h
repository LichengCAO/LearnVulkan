#pragma once
#include "trig_app.h"

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

	void _InitGLFW();
	void _CreateInstance();
	void _CreateSurface();
	void _SelectPhysicalDevice();
	void _CreateLogicalDevice();
	void _CreateSwapChain();

public:
	GLFWwindow*			pWindow;
	VkInstance			vkInstance;
	VkSurfaceKHR		vkSurface;
	VkPhysicalDevice	vkPhysicalDevice;
	VkDevice			vkDevice;
	QueueFamilyIndices	queueFamilyIndices;
	void Init();
	void Uninit();
public:
	static MyDevice& GetInstance();
};