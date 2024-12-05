#pragma once

#define GLFW_INCLUDE_VULKAN //->auto load <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#define WIDTH 800
#define HEIGHT 600
#define PARTICLE_COUNT 1000
#include <vector>
#include <string>
#include "vk_struct.h"
#include "drawable.h"
#include <VkBootstrap.h>
class HelloTriangleApplication {
public:
	void run();
	int bufferCnt = 0;
private:
	vkb::Instance m_instance;
	vkb::Device m_device;
	vkb::PhysicalDevice m_physicalDevice;
	vkb::DispatchTable m_dispatchTable;
	vkb::Swapchain m_swapchain;

	uint32_t m_curFrame = 0;
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
	VkExtent2D m_vkSwapChainExtent;
	VkRenderPass m_vkRenderPass;
	
	VkPipelineLayout m_vkPipelineLayout;
	VkPipeline m_vkGraphicsPipeline;
	std::vector<VkFramebuffer> m_vkFramebuffers;
	VkCommandPool m_vkCommandPool;
	std::vector<VkCommandBuffer> m_vkCommandBuffers;
	std::vector<VkSemaphore> m_vkImageAvailableSemaphores;
	std::vector<VkSemaphore> m_vkRenderFinishedSemaphores;
	std::vector<VkFence> m_vkInFlightFences;
	bool m_bufferResized = false;
	VkPhysicalDeviceProperties m_vkDeviceProperties;

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
	void createFrameBuffers();
	void createCommandPool();
	void createCommandBuffers();
	void createSyncObjects();
	void recreateSwapChain();
	void cleanUpSwapChain();
	void createVertexBuffer();
	void createIndexBuffer();
	void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
	void copyBuffer(VkBuffer& src, VkBuffer& dst, VkDeviceSize size);
	bool isDeviceSuitable(const VkPhysicalDevice& device)const;

	uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
	
	QueueFamilyIndices findQueueFamilies(const VkPhysicalDevice& device)const;
	
	//Uniform buffers Descriptor layout and buffer
	void createDescriptorSetLayout();
	VkDescriptorSetLayout m_vkDescriptorSetLayout;
	std::vector<VkBuffer> m_vkUniformBuffers;
	std::vector<VkDeviceMemory> m_vkUniformBuffersMemory;
	std::vector<void*> m_vkUniformBuffersMapped;
	void createUniformBuffers();
	void updateUniformBuffer(uint32_t currentImage);

	//Uniform buffers Descriptor pool and sets
	void createDescriptorPool();
	void createDescriptorSets();
	VkDescriptorPool m_vkDescriptorPool;
	std::vector<VkDescriptorSet> m_vkDescriptorSets;
	void bindDescriptorSets();

	//Texture mapping images
	void createTextureImage();
	VkImage m_vkTextureImage;
	VkDeviceMemory m_vkTextureImageMemory;
	void createImage(uint32_t width, uint32_t height, uint32_t mip, VkSampleCountFlagBits numSamples,
		VkFormat format,
		VkImageTiling tilling,
		VkImageUsageFlags usage,
		VkMemoryPropertyFlags properties,
		VkImage& image,
		VkDeviceMemory& imageMemory
		);
	VkCommandBuffer beginSingleTimeCommands();
	void endSingleTimeCommands(VkCommandBuffer commandBuffer);
	void transitionImageLayout(VkImage image, uint32_t mip, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
	void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

	//Texture mapping image view and sampler
	VkImageView m_vkTextureImageView;
	VkImageView createImageView(VkImage _image, uint32_t _mip = 1, VkFormat _format = VK_FORMAT_R8G8B8A8_SRGB, VkImageAspectFlags aspectMaskk = VK_IMAGE_ASPECT_COLOR_BIT);
	void createTextureImageView();
	VkSampler m_vkTextureSampler;
	void createTextureSampler();

	//Depth buffering
	VkImage m_vkDepthImage;
	VkImageView m_vkDepthImageView;
	VkDeviceMemory m_vkDepthImageMemory;
	void createDepthImage();
	VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tilling, VkFormatFeatureFlags features);
	VkFormat findDepthFormat();
	bool hasStencil(VkFormat format);

	//Load models
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
	VkBuffer m_vkVertexBuffer;
	VkDeviceMemory m_vkVertexBufferMemory;
	VkBuffer m_vkIndexBuffer;
	VkDeviceMemory m_vkIndexBufferMemory;
	void loadModel();

	//Generating Mipmaps
	uint32_t m_mipLevels;
	void generateMipmaps(VkImage image, VkFormat format, int32_t width, int32_t height, uint32_t mip);

	//Multisampling
	VkSampleCountFlagBits m_vkMSAASamples = VK_SAMPLE_COUNT_1_BIT;
	VkSampleCountFlagBits getMaxUsableSampleCount();
	VkImage m_vkColorImage; // stores the image for mulitsampling
	VkDeviceMemory m_vkColorImageMemory;
	VkImageView m_vkColorImageView;
	void createColorResources();

	//Compute Shader
	VkQueue m_vkComputeQueue;
	void createComputePipeline();
	std::vector<VkBuffer> m_vkShaderStorageBuffers;
	std::vector<VkDeviceMemory> m_vkShaderStorageBuffersMemory;
	void createShaderStorageBuffers();
	VkDescriptorSetLayout m_vkComputeDescriptorSetLayout; // stays alive during the program
	std::vector<VkDescriptorSet> m_vkComputeDescriptorSets;
	VkPipelineLayout m_vkComputePipelineLayout;
	VkPipeline m_vkComputePipeline;
	void recordComputeCommand(VkCommandBuffer commandBuffer, uint32_t imgIdx);
	std::vector<VkSemaphore> m_vkComputeFinishedSemaphores;
	std::vector<VkFence> m_vkComputeInFlightFences;
	std::vector<VkCommandBuffer> m_vkComputeCommandBuffers;
	void createComputeDescriptorSets();

	//Create Debug info
	struct DebugCreateInfo {
		uint64_t handle;
		VkObjectType type;
		std::string name;
	};
	void createDebugInfo(VkBuffer buffer, std::string name);
	void createDebugInfo(VkDeviceMemory memory, std::string name);
	void createDebugInfo(const DebugCreateInfo& _info);

	//Transfer Queue
	VkQueue m_vkTransferQueue;
	VkCommandPool m_vkCommandPool2;
	std::vector<VkCommandBuffer> m_vkCommandBuffers2;


	void mainLoop();
	void cleanUp();
public:
	void setFrameResized(bool val) { m_bufferResized = val; };
};