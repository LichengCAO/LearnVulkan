#pragma once

#define GLFW_INCLUDE_VULKAN //->auto load <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#define WIDTH 800
#define HEIGHT 600
#include <vector>
#include <string>
#include "vk_struct.h"

class HelloTriangleApplication {
public:
	void run();
private:
	GLFWwindow* m_window;
	VkInstance m_vkInstance;
	VkPhysicalDevice m_vkPhysicDevice = VK_NULL_HANDLE;
	VkDevice m_vkDevice;
	VkDebugUtilsMessengerEXT m_vkDebugMessenger;
	VkQueue m_vkGraphicsQueue;
	VkQueue m_vkPresentQueue;
	VkSurfaceKHR m_vkSurface;
	VkSwapchainKHR m_vkSwapChain;
	std::vector<VkImage> m_vkSwapChainImages;
	std::vector<VkImageView> m_vkSwapChainImageViews;
	VkFormat m_swapChainImageFormat;
	VkExtent2D m_swapChainExtent;
	VkRenderPass m_vkRenderPass;
	VkPipelineLayout m_vkPipelineLayout;
	VkPipeline m_vkGraphicsPipeline;
	std::vector<VkFramebuffer> m_vkFramebuffers;
	VkCommandPool m_vkCommandPool;
	VkCommandBuffer m_vkCommandBuffer;
	VkSemaphore imageAvailableSemaphore;
	VkSemaphore renderFinishedSemaphore;
	VkFence inFlightFence;

	void printAvailableExtensions()const;
	VkDebugUtilsMessengerCreateInfoEXT getDefaultDebugMessengerCreateInfo()const;

	std::vector<VkExtensionProperties> getInstanceAvailableExtensions()const;
	std::vector<VkLayerProperties> getInstanceAvailableLayerProperties()const;
	std::vector<const char*> getInstanceRequiredExtensions()const;
	bool checkInstanceExtensionSupport(const std::vector<const char*>& _extensionToCheck)const;
	bool checkInstanceLayerSupport(const std::vector<const char*>& _layerToCheck)const;

	std::vector<VkExtensionProperties> getDeviceAvailableExtensions(const VkPhysicalDevice& device)const;
	std::vector<const char*> getDeviceRequiredExtensions()const;
	bool checkDeviceExtensionSupport(const VkPhysicalDevice& device, const std::vector<const char*>& _extensionToCheck)const;

	SwapChainSupportDetails querySwapChainSupport(const VkPhysicalDevice& device, const VkSurfaceKHR& surface)const;
	VkSurfaceFormatKHR chooseSwapChainFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)const;
	VkPresentModeKHR chooseSwapChainPresentMode(const std::vector<VkPresentModeKHR>& availableModes)const;
	VkExtent2D chooseSwapChainExtent(const VkSurfaceCapabilitiesKHR& capabilities)const;

	static std::vector<char> readFile(const std::string& filename);
	VkShaderModule createShaderModule(const std::vector<char>& code)const;

	void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imgIdx);

	void drawFrame();

	void initWindow();
	void initVulkan();

	void createInstance();
	void setupDebugMessenger();
	void createSurface();
	void pickPhysicalDevice();
	void createLogicalDevice();
	void createSwapChain();
	void createImageViews();
	void createRenderPass();
	void createGraphicsPipeline();
	void createFramebuffers();
	void createCommandPool();
	void createCommandBuffer();
	void createSyncObjects();

	bool isDeviceSuitable(const VkPhysicalDevice& device)const;
	
	QueueFamilyIndices findQueueFamilies(const VkPhysicalDevice& device)const;
	
	void mainLoop();
	void cleanUp();
};