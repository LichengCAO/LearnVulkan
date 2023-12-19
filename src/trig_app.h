#pragma once

#define GLFW_INCLUDE_VULKAN //->auto load <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#define WIDTH 800
#define HEIGHT 600
#include <vector>


class HelloTriangleApplication {
public:
	void run();
private:
	GLFWwindow* m_window;
	VkInstance m_vkInstance;
	VkDebugUtilsMessengerEXT m_vkDebugMessenger;
	void printAvailableExtensions()const;
	VkDebugUtilsMessengerCreateInfoEXT getDefaultDebugMessengerCreateInfo()const;
	std::vector<VkExtensionProperties> getAvailableExtensions()const;
	std::vector<VkLayerProperties> getAvailableLayerProperties()const;
	std::vector<const char*> getRequiredExtensions()const;
	bool checkExtensionSupport(const std::vector<const char*>& _extensionToCheck)const;
	bool checkInstanceLayerSupport(const std::vector<const char*>& _layerToCheck)const;
	void initWindow();
	void initVulkan();
	void createInstance();
	void setupDebugMessenger();
	void mainLoop();
	void cleanUp();
};