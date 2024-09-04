#include "trig_app.h"
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO
#include <stdexcept>
#include <iostream>
#include <unordered_set>
#include <string>
#include "help.h"
#include <cstdint>
#include <limits>
#include <algorithm>
#include <fstream>
#include "drawable.h"
#include <glm/gtc/matrix_transform.hpp>
#include <chrono>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>
#include <unordered_map>
#include <random>

#define VK_CHECK(vkcommand) \
do{\
if((vkcommand)!=VK_SUCCESS){\
   throw std::runtime_error(\
"something went wrong!");\
}\
}while(0)

#define VK_CHECK(vkcommand, message) \
do{\
if((vkcommand)!=VK_SUCCESS){\
   throw std::runtime_error(\
#message);\
}\
}while(0)

const std::vector<const char*> validationLayers = {
	"VK_LAYER_KHRONOS_validation"
};

//const std::vector<Vertex> vertices = {
//	{.pos = {-0.5,-0.5, 0}, .color = {1,0,0}, .texcoord = {1.0f,0.0f}},
//	{.pos = {0.5, -0.5f, 0}, .color = {1,0,0}, .texcoord = {0.0f,0.0f}},
//	{.pos = {0.5, 0.5, 0}, .color = {0,1,0}, .texcoord = {0.0f,1.0f}},
//	{.pos = {-0.5,0.5, 0}, .color = {0,0,1}, .texcoord = {1.0f,1.0f}},
//	{.pos = {-0.5,-0.5,  0.5f}, .color = {1,0,0}, .texcoord = {1.0f,0.0f}},
//	{.pos = {0.5, -0.5f, 0.5f}, .color = {1,0,0}, .texcoord = {0.0f,0.0f}},
//	{.pos = {0.5, 0.5,   0.5f}, .color = {0,1,0}, .texcoord = {0.0f,1.0f}},
//	{.pos = {-0.5,0.5,   0.5f}, .color = {0,0,1}, .texcoord = {1.0f,1.0f}},
//};
//
//const std::vector<uint16_t> indices = {
//	0,1,2,2,3,0,
//	4,5,6,6,7,4,
//};

const std::string MODEL_PATH = "E:/GitStorage/LearnVulkan/res/models/viking_room/viking_room.obj";
const std::string TEXTURE_PATH = "E:/GitStorage/LearnVulkan/res/models/viking_room/viking_room.png";



const bool enableValidationLayer = true;
const int MAX_FRAMES_IN_FLIGHT = 2;

static void FrameBufferResizedCallback(GLFWwindow* window, int width, int height) {
	auto app = reinterpret_cast<HelloTriangleApplication*>(glfwGetWindowUserPointer(window));
	app->setFrameResized(true);
}

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance
	, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo
	, const VkAllocationCallbacks* pAllocator
	, VkDebugUtilsMessengerEXT* pDebugMessenger
) {
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr) {
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	}
	else {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance
	, VkDebugUtilsMessengerEXT debugMessenger
	, const VkAllocationCallbacks* pAllocator
) {
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
		instance
		, "vkDestroyDebugUtilsMessengerEXT"
	);
	if (func != nullptr) {
		func(instance, debugMessenger, pAllocator);
	}
}

static VKAPI_ATTR VkBool32 VKAPI_CALL testCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType, 
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData
	) {
	if (messageSeverity > VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
		std::cout << "message based on severity" << std::endl;
		std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
	}
	return VK_FALSE;
}

void HelloTriangleApplication::run()
{
	initWindow();
	initVulkan();
	mainLoop();
	cleanUp();
}

void HelloTriangleApplication::printAvailableExtensions()const
{
	auto extensionProperties = getInstanceAvailableExtensions();
	std::cout << "Available extensions: " << std::endl;
	for (const auto& extensionProperty : extensionProperties) {
		std::cout << "\t" << extensionProperty.extensionName << std::endl;
	}
}

VkDebugUtilsMessengerCreateInfoEXT HelloTriangleApplication::getDefaultDebugMessengerCreateInfo() const
{
	VkDebugUtilsMessengerCreateInfoEXT createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	createInfo.messageSeverity = 0
		//| VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
		| VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT
		| VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
		| VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
		| VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT
		| VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
	createInfo.pfnUserCallback = testCallback;
	createInfo.pUserData = nullptr;
	return createInfo;
}

std::vector<VkExtensionProperties> HelloTriangleApplication::getInstanceAvailableExtensions()const
{
	uint32_t extensionCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
	std::vector<VkExtensionProperties> extensionProperties(extensionCount);
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensionProperties.data());
	return extensionProperties;
}

std::vector<VkLayerProperties> HelloTriangleApplication::getInstanceAvailableLayerProperties() const
{
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
	std::vector<VkLayerProperties> res(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, res.data());
	return res;
}

std::vector<const char*> HelloTriangleApplication::getInstanceRequiredExtensions() const
{
	uint32_t glfwExtensionsCount = 0;
	const char** glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionsCount);
	std::vector<const char*> requiredExtensions;
	for (uint32_t i = 0; i < glfwExtensionsCount;++i) {
		requiredExtensions.emplace_back(glfwExtensions[i]);
	}
	requiredExtensions.emplace_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
	if(enableValidationLayer)requiredExtensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	return requiredExtensions;
}

bool HelloTriangleApplication::checkInstanceExtensionSupport(const std::vector<const char*>& _extensionToCheck) const
{
	auto extensions = getInstanceAvailableExtensions();
	std::unordered_set<std::string> has;
	for (auto& extension : extensions) {
		has.insert(std::string(extension.extensionName));
	}
#if DEBUG
	for (const auto item : _extensionToCheck) {
		if (has.find(std::string(item)) == has.end()) {

			std::cout << "Instance doesn't have: " << item << std::endl;
			return false;
		}
		else {
			std::cout << "Instance has: " << item << std::endl;
		}
	}
#else
	for (const auto item : _extensionToCheck) {
		if (has.find(std::string(item)) == has.end()) {
			return false;
		}
}
#endif // DEBUG
	return true;
}

bool HelloTriangleApplication::checkInstanceLayerSupport(const std::vector<const char*>& _layerToCheck) const
{
	auto layers = getInstanceAvailableLayerProperties();
	for (const char* layerName : _layerToCheck) {
		bool layerFound = false;
		for (const auto& layerProperty : layers) {
			if (strcmp(layerName, layerProperty.layerName) == 0) {
				layerFound = true;
				break;
			}
		}
		if (!layerFound)return false;
	}
	return true;
}

std::vector<VkExtensionProperties> HelloTriangleApplication::getDeviceAvailableExtensions(const VkPhysicalDevice& device) const
{
	uint32_t extensionCount = 0;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
	std::vector<VkExtensionProperties> extensionProperties(extensionCount);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, extensionProperties.data());
	return extensionProperties;
}

std::vector<const char*> HelloTriangleApplication::getDeviceRequiredExtensions() const
{
	return {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
}

bool HelloTriangleApplication::checkDeviceExtensionSupport(const VkPhysicalDevice& device, const std::vector<const char*>& _extensionToCheck) const
{
	auto extensions = getDeviceAvailableExtensions(device);
	std::unordered_set<std::string> has;
	for (auto& extension : extensions) {
		has.insert(std::string(extension.extensionName));
	}
#if DEBUG
	for (const auto item : _extensionToCheck) {
		if (has.find(std::string(item)) == has.end()) {

			std::cout << "Device doesn't have: " << item << std::endl;
			return false;
		}
		else {
			std::cout << "Device has: " << item << std::endl;
		}
	}
#else
	for (const auto item : _extensionToCheck) {
		if (has.find(std::string(item)) == has.end()) {
			return false;
		}
	}
#endif // DEBUG
	return true;
}

SwapChainSupportDetails HelloTriangleApplication::querySwapChainSupport(const VkPhysicalDevice& device, const VkSurfaceKHR& surface) const
{
	SwapChainSupportDetails res;

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &res.capabilities);

	uint32_t formatCnt = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCnt, nullptr);
	if (formatCnt != 0) {
		res.formats.resize(formatCnt);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCnt, res.formats.data());
	}
	
	uint32_t modeCnt = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &modeCnt, nullptr);
	if (modeCnt != 0) {
		res.presentModes.resize(modeCnt);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &modeCnt, res.presentModes.data());
	}
	return res;
}

VkSurfaceFormatKHR HelloTriangleApplication::chooseSwapChainFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) const
{
	for (const auto& availableFormat : availableFormats) {
		if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			return availableFormat;
		}
	}
	return availableFormats[0];
}

VkPresentModeKHR HelloTriangleApplication::chooseSwapChainPresentMode(const std::vector<VkPresentModeKHR>& availableModes) const
{
	for (const auto& availableMode : availableModes) {
		if (availableMode == VK_PRESENT_MODE_MAILBOX_KHR) {
			return availableMode;
		}
	}
	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D HelloTriangleApplication::chooseSwapChainExtent(const VkSurfaceCapabilitiesKHR& capabilities) const
{
	if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
		return capabilities.currentExtent;
	}
	int width, height;
	glfwGetFramebufferSize(m_window, &width, &height);
	VkExtent2D res{ 
		static_cast<uint32_t>(width),
		static_cast<uint32_t>(height)
	};

	res.width = std::clamp(res.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
	res.height = std::clamp(res.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
	return res;
}

std::vector<char> HelloTriangleApplication::readFile(const std::string& filename)
{
	std::ifstream file(filename, std::ios::ate | std::ios::binary);
	if (!file.is_open()) {
		throw std::runtime_error("failed to open file!");
	}

	size_t fileSize = (size_t)file.tellg();
	std::vector<char> ans(fileSize);
	
	file.seekg(0);
	file.read(ans.data(), fileSize);

	file.close();

	return ans;
}

VkShaderModule HelloTriangleApplication::createShaderModule(const std::vector<char>& code) const
{
	VkShaderModuleCreateInfo createInfo{
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.codeSize = code.size(),
		.pCode = reinterpret_cast<const uint32_t*>(code.data())
	};
	VkShaderModule shaderModule;
	if (vkCreateShaderModule(m_vkDevice, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
		throw std::runtime_error("failed to create shader module!");
	}
	return shaderModule;
}

void HelloTriangleApplication::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imgIdx){
	VkCommandBufferBeginInfo beginInfo{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = 0,
		.pInheritanceInfo = nullptr
	};

	if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
		throw std::runtime_error("failed to begin recording command buffer!");
	}
	{
		std::array<VkClearValue, 2> clearValues{};
		clearValues[0].color = { 0.f,0.f,0.f,1.f };
		clearValues[1].depthStencil = { 1.0f, 0 };

		VkRenderPassBeginInfo renderPassInfo{
			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
			.renderPass = m_vkRenderPass,
			.framebuffer = m_vkFramebuffers[imgIdx],
			.renderArea = {
				.offset = {0,0},
				.extent = m_vkSwapChainExtent
			},
			.clearValueCount = static_cast<uint32_t>(clearValues.size()),
			.pClearValues = clearValues.data()
		};
		vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
		{
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_vkGraphicsPipeline);
			//same in createGraphicsPipeline
			VkViewport viewport{
				.x = 0.f,
				.y = 0.f,
				.width = (float)m_vkSwapChainExtent.width,
				.height = (float)m_vkSwapChainExtent.height,
				.minDepth = 0.f,
				.maxDepth = 1.f
			};
			VkRect2D scissor{
				.offset = {0,0},
				.extent = m_vkSwapChainExtent
			};
			vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
			vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
			VkBuffer vertexBuffers[] = { m_vkVertexBuffer };
			VkDeviceSize offsets[] = { 0 };
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
			vkCmdBindIndexBuffer(commandBuffer, m_vkIndexBuffer, 0, VK_INDEX_TYPE_UINT32);
			//vkCmdDraw(commandBuffer, static_cast<uint32_t>(vertices.size()), 1, 0, 0);
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_vkPipelineLayout, 0, 1, &m_vkDescriptorSets[m_curFrame], 0, nullptr);
			vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
		}
		vkCmdEndRenderPass(commandBuffer);
	}
	if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
		throw std::runtime_error("failed to record command buffer!");
	}
}

void HelloTriangleApplication::drawFrame(){
	uint32_t imageIndex;
	VkResult result = VK_NOT_READY;

	/**
	 * =======================================
	 *				COMPUTE SHADER
	 * =======================================
	 */
	{
		vkWaitForFences(m_vkDevice, 1, &m_vkComputeInFlightFences[m_curFrame], VK_TRUE, UINT64_MAX);
		updateUniformBuffer(m_curFrame);
		vkResetFences(m_vkDevice, 1, &m_vkComputeInFlightFences[m_curFrame]);
		vkResetCommandBuffer(m_vkComputeCommandBuffers[m_curFrame], 0);
		recordComputeCommand(m_vkComputeCommandBuffers[m_curFrame], m_curFrame);
		VkSubmitInfo submitInfo{
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			.waitSemaphoreCount = 1,
			.commandBufferCount = 1,
			.pCommandBuffers = &m_vkComputeCommandBuffers[m_curFrame],
			.signalSemaphoreCount = 1,
			.pSignalSemaphores = &m_vkComputeFinishedSemaphores[m_curFrame],
		};
		VK_CHECK(vkQueueSubmit(m_vkComputeQueue, 1, &submitInfo, m_vkComputeInFlightFences[m_curFrame]), failed to submit compute command buffer!);
	}

	//wait for previous fences
	vkWaitForFences(m_vkDevice, 1, &m_vkInFlightFences[m_curFrame], VK_TRUE, UINT64_MAX);

	//aquire Image from swap chain
	result = vkAcquireNextImageKHR(m_vkDevice, m_vkSwapChain, UINT64_MAX, m_vkImageAvailableSemaphores[m_curFrame], VK_NULL_HANDLE, &imageIndex);
	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		recreateSwapChain();
		return;
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
		throw std::runtime_error("failed to acquire swap chain image!");
	}


	vkResetFences(m_vkDevice, 1, &m_vkInFlightFences[m_curFrame]);

	//get command buffer, record command
	vkResetCommandBuffer(m_vkCommandBuffers[m_curFrame], 0);
	recordCommandBuffer(m_vkCommandBuffers[m_curFrame], imageIndex);
	
	//submit command
	std::array<VkSemaphore, 2> waitSemaphores = { m_vkComputeFinishedSemaphores[m_curFrame], m_vkImageAvailableSemaphores[m_curFrame]};
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	VkSemaphore signalSemaphores[] = { m_vkRenderFinishedSemaphores[m_curFrame] };
	submitInfo = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.waitSemaphoreCount = waitSemaphores.size(),
		.pWaitSemaphores = waitSemaphores.data(),
		.pWaitDstStageMask = waitStages,
		.commandBufferCount = 1,
		.pCommandBuffers = &m_vkCommandBuffers[m_curFrame],
		.signalSemaphoreCount = 1,
		.pSignalSemaphores = signalSemaphores,
	};	
	result = vkQueueSubmit(m_vkGraphicsQueue, 1, &submitInfo, m_vkInFlightFences[m_curFrame]);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("failed to submit draw command buffer in drawFrame()!");
	}

	//present image
	VkSwapchainKHR swapChains[] = { m_vkSwapChain };
	VkPresentInfoKHR presentInfo{
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = signalSemaphores,
		.swapchainCount = 1,
		.pSwapchains = swapChains,
		.pImageIndices = &imageIndex,
		.pResults = nullptr
	};
	result = vkQueuePresentKHR(m_vkPresentQueue, &presentInfo);
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_bufferResized) {
		recreateSwapChain();
		m_bufferResized = false;
	}else if (result != VK_SUCCESS) {
		throw std::runtime_error("failed to present in drawFrame()!");
	}

	m_curFrame++;
	m_curFrame = m_curFrame % MAX_FRAMES_IN_FLIGHT;
}

void HelloTriangleApplication::initWindow()
{
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
	m_window = glfwCreateWindow(WIDTH, HEIGHT,"hello triangle", nullptr, nullptr);
	glfwSetWindowUserPointer(m_window, this);
	glfwSetFramebufferSizeCallback(m_window, FrameBufferResizedCallback);
}

void HelloTriangleApplication::initVulkan(){
	createInstance();
	setupDebugMessenger();
	createSurface();
	pickPhysicalDevice();
	createLogicalDevice();
	createSwapChain();
	createImageViews();
	createRenderPass();
	createDescriptorSetLayout();
	createGraphicsPipeline();
	createCommandPool();
	createColorResources();
	createDepthImage();
	createFrameBuffers();
	createTextureImage();
	createTextureImageView();
	createTextureSampler();
	loadModel();
	createVertexBuffer();
	createIndexBuffer();
	createUniformBuffers();
	createDescriptorPool();
	createDescriptorSets();
	bindDescriptorSets();
	createCommandBuffers();
	createSyncObjects();
}

void HelloTriangleApplication::createInstance()
{
	//optional
	VkApplicationInfo appInfo{};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "Hello Triangle";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "No Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_0;

	//mandatory
	VkInstanceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;
	createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
	auto requiredExtensions = getInstanceRequiredExtensions();
	createInfo.enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size());
	createInfo.ppEnabledExtensionNames = requiredExtensions.data();
	
	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = getDefaultDebugMessengerCreateInfo();
	//check layers
	if (enableValidationLayer) {
		if (!checkInstanceLayerSupport(validationLayers)) {
			throw std::runtime_error("validation layers required, but not available");
		}
		std::cout << "validation layer enabled" << std::endl;
		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();
		createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
	}
	else {
		createInfo.enabledLayerCount = 0;
		createInfo.pNext = nullptr;
	}

	//check extensions
	auto extensionsToCheck = getInstanceRequiredExtensions();
	if (!checkInstanceExtensionSupport(extensionsToCheck)) {
		throw std::runtime_error("extensions required, but not available");
	}
	else {
		std::cout << "instance extensions checked" << std::endl;
	}

	if (vkCreateInstance(&createInfo, nullptr, &m_vkInstance) != VK_SUCCESS) {
		throw std::runtime_error("failed to create instance!");
	}
}

void HelloTriangleApplication::createSurface()
{
	if (glfwCreateWindowSurface(m_vkInstance, m_window, nullptr, &m_vkSurface) != VK_SUCCESS) {
		throw std::runtime_error("failed to create window surface!");
	}
}

void HelloTriangleApplication::createSwapChain()
{
	SwapChainSupportDetails swapChainSupport = querySwapChainSupport(m_vkPhysicDevice, m_vkSurface);

	VkSurfaceFormatKHR surfaceFormat = chooseSwapChainFormat(swapChainSupport.formats);
	VkPresentModeKHR presentMode = chooseSwapChainPresentMode(swapChainSupport.presentModes);
	VkExtent2D extent = chooseSwapChainExtent(swapChainSupport.capabilities);

	uint32_t imageCnt = swapChainSupport.capabilities.minImageCount + 1;
	if (swapChainSupport.capabilities.maxImageCount > 0 // means has maximum image count 
		&& imageCnt > swapChainSupport.capabilities.maxImageCount) {
		imageCnt = swapChainSupport.capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR createInfo{
		.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		.surface = m_vkSurface,
		.minImageCount = imageCnt,
		.imageFormat = surfaceFormat.format,
		.imageColorSpace = surfaceFormat.colorSpace,
		.imageExtent = extent,
		.imageArrayLayers = 1,
		.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		.preTransform = swapChainSupport.capabilities.currentTransform,
		.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR, //application window should not blend with other windows
		.presentMode = presentMode,
		.clipped = VK_TRUE, //part where application's window is behind other windows
		.oldSwapchain = VK_NULL_HANDLE
	};
	QueueFamilyIndices indices = findQueueFamilies(m_vkPhysicDevice);
	uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };
	if (indices.graphicsFamily != indices.presentFamily) {
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	else {
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.queueFamilyIndexCount = 0;
		createInfo.pQueueFamilyIndices = nullptr;
	}

	if (vkCreateSwapchainKHR(m_vkDevice, &createInfo, nullptr, &m_vkSwapChain) != VK_SUCCESS) {
		throw std::runtime_error("failed to create swap chain!");
	}

	uint32_t imgCnt = 0;
	vkGetSwapchainImagesKHR(m_vkDevice, m_vkSwapChain, &imgCnt, nullptr);
	m_vkSwapChainImages.resize(imgCnt);
	vkGetSwapchainImagesKHR(m_vkDevice, m_vkSwapChain, &imgCnt, m_vkSwapChainImages.data());

	//record something
	m_vkSwapChainExtent = extent;
	m_swapChainImageFormat = surfaceFormat.format;
}

void HelloTriangleApplication::createImageViews(){
	m_vkSwapChainImageViews.resize(m_vkSwapChainImages.size());
	for (size_t i = 0;i < m_vkSwapChainImages.size();++i) {
		m_vkSwapChainImageViews[i] = createImageView(m_vkSwapChainImages[i], 1, m_swapChainImageFormat); // we only generate texture mip
	}
}

void HelloTriangleApplication::createRenderPass()
{
	VkAttachmentDescription colorAttachment{
		.format = m_swapChainImageFormat,
		.samples = m_vkMSAASamples,
		.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL //You'll notice that we have changed the finalLayout from VK_IMAGE_LAYOUT_PRESENT_SRC_KHR 
																//to VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL. That's because multisampled images 
																//cannot be presented directly.We first need to resolve them to a regular image. 
																//This requirement does not apply to the depth buffer, since it won't be presented at any point.
	};

	VkAttachmentReference colorAttachmentRef{
		.attachment = 0,
		.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	};

	VkAttachmentDescription colorResolveAttachment{
	.format = m_swapChainImageFormat,
	.samples = VK_SAMPLE_COUNT_1_BIT,
	.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
	.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
	.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
	.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
	.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
	.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
	};

	VkAttachmentReference colorResolveAttachmentRef{
		.attachment = 2,
		.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	};

	VkAttachmentDescription depthAttachment{
		.format = findDepthFormat(),
		.samples = m_vkMSAASamples,
		.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
	};

	VkAttachmentReference depthAttachmentRef{
		.attachment = 1,
		.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
	};

	VkSubpassDescription subpass{
		.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
		.colorAttachmentCount = 1,
		.pColorAttachments = &colorAttachmentRef,
		.pResolveAttachments = &colorResolveAttachmentRef,
		.pDepthStencilAttachment = &depthAttachmentRef,
	};

	VkSubpassDependency dependencyInfo{
	.srcSubpass = VK_SUBPASS_EXTERNAL,
	.dstSubpass = 0,
	.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
	.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
	.srcAccessMask = 0,
	.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
	};

	std::array<VkAttachmentDescription, 3> attachments = { colorAttachment, depthAttachment, colorResolveAttachment }; //must ordered same as attachment ref

	VkRenderPassCreateInfo renderPassInfo{
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		.attachmentCount = static_cast<uint32_t>(attachments.size()),
		.pAttachments = attachments.data(),
		.subpassCount = 1,
		.pSubpasses = &subpass,
		.dependencyCount = 1,
		.pDependencies = &dependencyInfo
	};

	if (vkCreateRenderPass(m_vkDevice, &renderPassInfo, nullptr, &m_vkRenderPass) != VK_SUCCESS) {
		throw std::runtime_error("failed to create render pass!");
	}
}

void HelloTriangleApplication::createGraphicsPipeline()
{
	auto vertShaderCode = readFile("E:/GitStorage/LearnVulkan/bin/shaders/trig.vert.spv");
	auto fragShaderCode = readFile("E:/GitStorage/LearnVulkan/bin/shaders/trig.frag.spv");

	VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
	VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

	VkPipelineShaderStageCreateInfo vertShaderStageInfo{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.stage = VK_SHADER_STAGE_VERTEX_BIT,
		.module = vertShaderModule,
		.pName = "main", // entry point function in shader
	};

	VkPipelineShaderStageCreateInfo fragShaderStageInfo{
	.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
	.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
	.module = fragShaderModule,
	.pName = "main", // entry point function in shader
	};

	VkPipelineShaderStageCreateInfo shaderStageInfos[] = { vertShaderStageInfo, fragShaderStageInfo };
	auto vertexBindingDescriptInfo = Vertex::getVertexInputBindingDescription();
	auto vertexAttributeDescriptInfo = Vertex::getVertexInputAttributeDescription();

	VkPipelineVertexInputStateCreateInfo vertexInputInfo{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		.vertexBindingDescriptionCount = 1,
		.pVertexBindingDescriptions = &vertexBindingDescriptInfo,
		.vertexAttributeDescriptionCount = vertexAttributeDescriptInfo.size(),
		.pVertexAttributeDescriptions = vertexAttributeDescriptInfo.data()
	};

	VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		.primitiveRestartEnable = VK_FALSE
	};

	std::vector<VkDynamicState> dynamicStates = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};

	VkPipelineDynamicStateCreateInfo dynamicStateInfo{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
		.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
		.pDynamicStates = dynamicStates.data()
	};

	VkViewport viewport{
	.x = 0.f,
	.y = 0.f,
	.width = (float)m_vkSwapChainExtent.width,
	.height = (float)m_vkSwapChainExtent.height,
	.minDepth = 0.f,
	.maxDepth = 1.f
	};
	VkRect2D scissor{
		.offset = {0,0},
		.extent = m_vkSwapChainExtent
	};

	VkPipelineViewportStateCreateInfo viewportStateInfo{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
		.viewportCount = 1,
		//.pViewports = &viewport,
		.scissorCount = 1,
		//.pScissors = &scissor
	};

	VkPipelineRasterizationStateCreateInfo rasterizerInfo{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
		.depthClampEnable = VK_FALSE,
		.rasterizerDiscardEnable = VK_FALSE,
		.polygonMode = VK_POLYGON_MODE_FILL,
		.cullMode = VK_CULL_MODE_BACK_BIT,
		.frontFace = VK_FRONT_FACE_CLOCKWISE,
		.depthBiasEnable = VK_FALSE,//use for shadow mapping
		.depthBiasConstantFactor = 0.0f,
		.depthBiasClamp = 0.0f,
		.depthBiasSlopeFactor = 0.0f,
		.lineWidth = 1.0f, //use values bigger than 1.0, enable wideLines GPU features, extensions
	};
	rasterizerInfo.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizerInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;//otherwise the image won't be drawn

	VkPipelineMultisampleStateCreateInfo multisampleInfo{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
		.rasterizationSamples = m_vkMSAASamples,
		.sampleShadingEnable = VK_FALSE,
		.minSampleShading = 1.0f,
		.pSampleMask = nullptr,
		.alphaToCoverageEnable = VK_FALSE,
		.alphaToOneEnable = VK_FALSE
	};
	multisampleInfo.sampleShadingEnable = VK_TRUE; // enable sample shading in the pipeline
	multisampleInfo.minSampleShading = .2; // min fraction for sample shading; closer to one is smoother

	//DEPTH AND STENCIL

	//COLOR BLENDING
	VkPipelineColorBlendAttachmentState colorBlendAttachmentInfo{
		.blendEnable = VK_TRUE,
		.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
		.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
		.colorBlendOp = VK_BLEND_OP_ADD,
		.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
		.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
		.alphaBlendOp = VK_BLEND_OP_ADD,
		.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
	};

	VkPipelineColorBlendStateCreateInfo colorBlendStateInfo{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
		.logicOpEnable = VK_FALSE,
		.logicOp = VK_LOGIC_OP_COPY,
		.attachmentCount = 1,
		.pAttachments = &colorBlendAttachmentInfo,
		.blendConstants = {
			0,
			0,
			0,
			0
		}
	};

	//LAYOUT uniforms
	VkPipelineLayoutCreateInfo pipelineLayoutInfo{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.setLayoutCount = 1,
		.pSetLayouts = &m_vkDescriptorSetLayout,
		.pushConstantRangeCount = 0,
		.pPushConstantRanges = nullptr,
	};
	if (vkCreatePipelineLayout(m_vkDevice, &pipelineLayoutInfo, nullptr, &m_vkPipelineLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create pipeline layout!");
	}

	//DEPTH & STENCIL
	VkPipelineDepthStencilStateCreateInfo depthStencilInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
		.depthTestEnable = VK_TRUE,
		.depthWriteEnable = VK_TRUE, //if the new fragment pass depth test, it can be write to the depth buffer
		.depthCompareOp = VK_COMPARE_OP_LESS,
		.depthBoundsTestEnable = VK_FALSE, //keeps the fragments fall between minDepthBounds and maxDepthBounds
		.minDepthBounds = 0.f,
		.maxDepthBounds = 1.f,
	};

	//use for stencils
	{
		depthStencilInfo.stencilTestEnable = VK_FALSE;
		depthStencilInfo.front = {};
		depthStencilInfo.back = {};
	}


	//GRAPHICS PIPELINE LAYOUT
	VkGraphicsPipelineCreateInfo pipelineInfo{
		.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		.stageCount = 2,
		.pStages = shaderStageInfos,
		.pVertexInputState = &vertexInputInfo,
		.pInputAssemblyState = &inputAssemblyInfo,
		.pViewportState = &viewportStateInfo,
		.pRasterizationState = &rasterizerInfo,
		.pMultisampleState = &multisampleInfo,
		.pDepthStencilState = &depthStencilInfo,
		.pColorBlendState = &colorBlendStateInfo,
		.pDynamicState = &dynamicStateInfo,
		.layout = m_vkPipelineLayout,
		.renderPass = m_vkRenderPass,
		.subpass = 0,
		.basePipelineHandle = VK_NULL_HANDLE,//only when in graphics pipleininfo VK_PIPELINE_CREATE_DERIVATIVE_BIT flag is set
		.basePipelineIndex = -1
	};

	if (vkCreateGraphicsPipelines(m_vkDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_vkGraphicsPipeline) != VK_SUCCESS) {
		throw std::runtime_error("failed to create graphics pipeline!");
	}

	vkDestroyShaderModule(m_vkDevice, vertShaderModule, nullptr);
	vkDestroyShaderModule(m_vkDevice, fragShaderModule, nullptr);
}

void HelloTriangleApplication::createFrameBuffers()
{
	m_vkFramebuffers.resize(m_vkSwapChainImages.size());
	for (size_t i = 0;i < m_vkSwapChainImageViews.size();++i) {
		std::array<VkImageView,3 > attachements = {
			m_vkColorImageView,
			m_vkDepthImageView,
			m_vkSwapChainImageViews[i],
		}; //orders need to match the ref in createRenderPass

		VkFramebufferCreateInfo framebufferInfo{
			.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			.renderPass = m_vkRenderPass,
			.attachmentCount = static_cast<uint32_t>(attachements.size()),
			.pAttachments = attachements.data(),
			.width = m_vkSwapChainExtent.width,
			.height = m_vkSwapChainExtent.height,
			.layers = 1
		};

		if (vkCreateFramebuffer(m_vkDevice, &framebufferInfo, nullptr, &m_vkFramebuffers[i]) != VK_SUCCESS) {
			throw std::runtime_error("failed to create framebuffer!");
		}
	}
}

void HelloTriangleApplication::createCommandPool()
{
	QueueFamilyIndices queueFamilyIndices = findQueueFamilies(m_vkPhysicDevice);
	VkCommandPoolCreateInfo commandPoolInfo{
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
		.queueFamilyIndex = queueFamilyIndices.graphicsAndComputeFamily.value()
	};

	if (vkCreateCommandPool(m_vkDevice, &commandPoolInfo, nullptr, &m_vkCommandPool) != VK_SUCCESS) {
		throw std::runtime_error("failed to create command pool!");
	}
}

void HelloTriangleApplication::createCommandBuffers()
{
	m_vkCommandBuffers.resize(MAX_FRAMES_IN_FLIGHT);
	m_vkComputeCommandBuffers.resize(MAX_FRAMES_IN_FLIGHT);
	VkCommandBufferAllocateInfo allocInfo{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.commandPool = m_vkCommandPool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = static_cast<uint32_t>(m_vkCommandBuffers.size())
	};

	if (vkAllocateCommandBuffers(m_vkDevice, &allocInfo, m_vkCommandBuffers.data()) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate command buffers!");
	}

	if (vkAllocateCommandBuffers(m_vkDevice, &allocInfo, m_vkComputeCommandBuffers.data()) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate compute command buffers!");
	}

}

void HelloTriangleApplication::createSyncObjects(){
	m_vkImageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	m_vkInFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
	m_vkRenderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);

	m_vkComputeFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	m_vkComputeInFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

	VkSemaphoreCreateInfo semaphoreInfo{
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
	};

	VkFenceCreateInfo fenceInfo{
		.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		.flags = VK_FENCE_CREATE_SIGNALED_BIT // to make sure the first frame won't be blocked
	};

	for (auto i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
	{
		if (   vkCreateSemaphore(m_vkDevice, &semaphoreInfo, nullptr, &m_vkImageAvailableSemaphores[i]) != VK_SUCCESS
			|| vkCreateSemaphore(m_vkDevice, &semaphoreInfo, nullptr, &m_vkRenderFinishedSemaphores[i]) != VK_SUCCESS
			|| vkCreateFence(m_vkDevice, &fenceInfo, nullptr, &m_vkInFlightFences[i]) != VK_SUCCESS
			|| vkCreateSemaphore(m_vkDevice, &semaphoreInfo, nullptr, &m_vkComputeFinishedSemaphores[i]) != VK_SUCCESS
			|| vkCreateFence(m_vkDevice, &fenceInfo, nullptr, &m_vkComputeInFlightFences[i]) != VK_SUCCESS
			) {
			throw std::runtime_error("failed to create sync objects!");
		}
	}
}

void HelloTriangleApplication::recreateSwapChain()
{
	int width = 0;
	int height = 0;
	glfwGetFramebufferSize(m_window, &width, &height);
	while (width == 0 || height == 0) {
		glfwGetFramebufferSize(m_window, &width, &height);
		glfwWaitEvents();
	}

	vkDeviceWaitIdle(m_vkDevice);

	cleanUpSwapChain();

	createSwapChain();
	createImageViews();
	createDepthImage(); //will be cleaned in cleanupswapchain 
	createColorResources();
	createFrameBuffers();
}

void HelloTriangleApplication::cleanUpSwapChain()
{
	vkDestroyImageView(m_vkDevice, m_vkDepthImageView, nullptr);
	vkDestroyImage(m_vkDevice, m_vkDepthImage, nullptr);
	vkFreeMemory(m_vkDevice, m_vkDepthImageMemory, nullptr);
	for (auto framebuffer : m_vkFramebuffers) {
		vkDestroyFramebuffer(m_vkDevice, framebuffer, nullptr);
	}
	for (auto imageView : m_vkSwapChainImageViews) {
		vkDestroyImageView(m_vkDevice, imageView, nullptr);
	}
	vkDestroySwapchainKHR(m_vkDevice, m_vkSwapChain, nullptr);
}

void HelloTriangleApplication::createVertexBuffer()
{
	VkDeviceSize bufferSize = sizeof(Vertex) * vertices.size();
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	createBuffer(bufferSize, 
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		stagingBuffer,
		stagingBufferMemory
		);

	void* data;
	vkMapMemory(m_vkDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, vertices.data(), (size_t)bufferSize);
	vkUnmapMemory(m_vkDevice, stagingBufferMemory);

	createBuffer(bufferSize,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		m_vkVertexBuffer,
		m_vkVertexBufferMemory
	);

	copyBuffer(stagingBuffer, m_vkVertexBuffer, bufferSize);

	vkDestroyBuffer(m_vkDevice, stagingBuffer, nullptr);
	vkFreeMemory(m_vkDevice, stagingBufferMemory, nullptr);
}

void HelloTriangleApplication::createIndexBuffer()
{
	VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	createBuffer(bufferSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		stagingBuffer,
		stagingBufferMemory
	);

	void* data;
	vkMapMemory(m_vkDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, indices.data(), (size_t)bufferSize);
	vkUnmapMemory(m_vkDevice, stagingBufferMemory);

	createBuffer(bufferSize,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		m_vkIndexBuffer,
		m_vkIndexBufferMemory
	);

	copyBuffer(stagingBuffer, m_vkIndexBuffer, bufferSize);

	vkDestroyBuffer(m_vkDevice, stagingBuffer, nullptr);
	vkFreeMemory(m_vkDevice, stagingBufferMemory, nullptr);
}

void HelloTriangleApplication::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory)
{
	VkBufferCreateInfo bufferInfo = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.size = size,
		.usage = usage,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
	};

	if (vkCreateBuffer(m_vkDevice, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
		throw std::runtime_error("failed to create buffer!");
	}

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(m_vkDevice, buffer, &memRequirements);

	VkMemoryAllocateInfo allocInfo = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.allocationSize = memRequirements.size, //could be different from size on Host!
		.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits , properties),
	};

	if (vkAllocateMemory(m_vkDevice, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate buffer memory!");
	}

	VkDeviceSize offset = 0; // should be divisible by memRequirements.alignment
	vkBindBufferMemory(m_vkDevice, buffer, bufferMemory, offset);
}

void HelloTriangleApplication::copyBuffer(VkBuffer& src, VkBuffer& dst, VkDeviceSize size)
{

	VkCommandBuffer cpyCommandBuffer = beginSingleTimeCommands();

	VkBufferCopy cpyRegion = {
		.srcOffset = 0,
		.dstOffset = 0,
		.size = size,
	};

	vkCmdCopyBuffer(cpyCommandBuffer, src, dst, 1, &cpyRegion);

	endSingleTimeCommands(cpyCommandBuffer);
}

bool HelloTriangleApplication::isDeviceSuitable(const VkPhysicalDevice& device)const
{
	VkPhysicalDeviceProperties deviceProperties;//device name, type and supported vulkan version
	vkGetPhysicalDeviceProperties(device, &deviceProperties);

	VkPhysicalDeviceFeatures deviceFeatures;//64bit floats, multi viewport rendering
	vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

	QueueFamilyIndices indices = findQueueFamilies(device);

	auto deviceExtensionsToCheck = getDeviceRequiredExtensions();
	bool extensionsSupported = checkDeviceExtensionSupport(device, deviceExtensionsToCheck);
	bool swapChainAdequate = false;
	if (extensionsSupported) {
		SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device, m_vkSurface);
		swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
	}
	bool featuresSupported = deviceFeatures.samplerAnisotropy == true;

	return indices.isComplete() && extensionsSupported && swapChainAdequate && featuresSupported;
}

uint32_t HelloTriangleApplication::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(m_vkPhysicDevice, &memProperties);
	for (uint32_t res = 0; res < memProperties.memoryTypeCount; ++res) {
		bool suitableType = typeFilter & (1 << res);
		bool suitableMemoryProperty = (memProperties.memoryTypes[res].propertyFlags & properties) == properties;
		if (suitableType && suitableMemoryProperty) {
			return res;
		}
	}
	throw std::runtime_error("failed to find suitable memory type!");
	return uint32_t();
}

void HelloTriangleApplication::pickPhysicalDevice()
{
	//count devices
	uint32_t device_cnt = 0;
	vkEnumeratePhysicalDevices(m_vkInstance, &device_cnt, nullptr);
	if (device_cnt == 0) {
		throw std::runtime_error("failed to find GPUs with vulkan support!");
	}
	std::vector<VkPhysicalDevice> devices(device_cnt);
	vkEnumeratePhysicalDevices(m_vkInstance, &device_cnt, devices.data());
	for (const auto& device : devices) {
		std::cout << "check physical device..." << std::endl;
		if (isDeviceSuitable(device)) {
			m_vkPhysicDevice = device;
			vkGetPhysicalDeviceProperties(m_vkPhysicDevice, &m_vkDeviceProperties);
			m_vkMSAASamples = getMaxUsableSampleCount();
			std::cout << "physical device picked" << std::endl;
			break;
		}
	}
	if (m_vkPhysicDevice == VK_NULL_HANDLE) {
		throw std::runtime_error("failed to find a suitable GPU!");
	}
}

QueueFamilyIndices HelloTriangleApplication::findQueueFamilies(const VkPhysicalDevice& device) const
{
	QueueFamilyIndices indices;
	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());
	int i = 0;
	for (const auto& queueFamily : queueFamilies) {
		if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			indices.graphicsFamily = i;
		}
		if ((queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) && (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT)) {
			indices.graphicsAndComputeFamily = i;
		}
		VkBool32 supportPresent = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_vkSurface, &supportPresent);
		if (supportPresent) {
			indices.presentFamily = i;
		}
		if (indices.isComplete())break;
		++i;
	}
	return indices;
}

void HelloTriangleApplication::createLogicalDevice()
{
	QueueFamilyIndices indices = findQueueFamilies(m_vkPhysicDevice);
	float priority = 1.f;
	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::unordered_set<uint32_t> uniqueQueueFamilies{ indices.graphicsFamily.value(), indices.presentFamily.value(), indices.graphicsAndComputeFamily.value() };
	for (auto queueFamily : uniqueQueueFamilies) {
		VkDeviceQueueCreateInfo queueCreateInfo{
			.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
			.queueFamilyIndex = queueFamily,
			.queueCount = 1,
			.pQueuePriorities = &priority
		};
		queueCreateInfos.push_back(queueCreateInfo);
	}

	VkPhysicalDeviceFeatures deviceFeatures = {
		.sampleRateShading = VK_TRUE, // enable sample shading feature for the device
		.samplerAnisotropy = VK_TRUE,
	};
	VkDeviceCreateInfo createInfo{
		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size()),
		.pQueueCreateInfos = queueCreateInfos.data(),
		.pEnabledFeatures = &deviceFeatures
	};

	//enable validation layer
	if (enableValidationLayer) {
		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();
	}
	else {
		createInfo.enabledLayerCount = 0;
	}

	//enable extensions
	auto requiredExtensions = getDeviceRequiredExtensions();
	createInfo.enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size());
	createInfo.ppEnabledExtensionNames = requiredExtensions.data();


	if (vkCreateDevice(m_vkPhysicDevice, &createInfo, nullptr, &m_vkDevice) != VK_SUCCESS) {
		throw std::runtime_error("failed to create logical device!");
	}

	vkGetDeviceQueue(m_vkDevice, indices.graphicsFamily.value(), 0, &m_vkGraphicsQueue);
	vkGetDeviceQueue(m_vkDevice, indices.presentFamily.value(), 0, &m_vkPresentQueue);
	vkGetDeviceQueue(m_vkDevice, indices.graphicsAndComputeFamily.value(), 0, &m_vkComputeQueue);
}

void HelloTriangleApplication::setupDebugMessenger()
{
	if (!enableValidationLayer)return;
	VkDebugUtilsMessengerCreateInfoEXT createInfo = getDefaultDebugMessengerCreateInfo();
	if (CreateDebugUtilsMessengerEXT(m_vkInstance, &createInfo, nullptr, &m_vkDebugMessenger) != VK_SUCCESS) {
		throw std::runtime_error("failed to setup debug messenger");
	}
}

void HelloTriangleApplication::createDescriptorSetLayout()
{
	VkDescriptorSetLayoutBinding computedResultBinding = {
		.binding = 0,
		.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		.descriptorCount = 2,
		.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
		.pImmutableSamplers = nullptr,
	};

	std::array<VkDescriptorSetLayoutBinding, 1> bindings = { computedResultBinding};
	VkDescriptorSetLayoutCreateInfo layoutCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.bindingCount = bindings.size(),
		.pBindings = bindings.data(),
	};

	VK_CHECK(vkCreateDescriptorSetLayout(m_vkDevice, &layoutCreateInfo, nullptr, &m_vkDescriptorSetLayout), failed to create descriptor set layout!);

	std::array<VkDescriptorSetLayoutBinding, 3> layoutBindings{};
	layoutBindings[0].binding = 0;
	layoutBindings[0].descriptorCount = 1;
	layoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	layoutBindings[0].pImmutableSamplers = nullptr;
	layoutBindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

	layoutBindings[1].binding = 1;
	layoutBindings[1].descriptorCount = 1;
	layoutBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	layoutBindings[1].pImmutableSamplers = nullptr;
	layoutBindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

	layoutBindings[2].binding = 2;
	layoutBindings[2].descriptorCount = 1;
	layoutBindings[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	layoutBindings[2].pImmutableSamplers = nullptr;
	layoutBindings[2].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

	VkDescriptorSetLayoutCreateInfo createInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.bindingCount = 3,
		.pBindings = layoutBindings.data(),
	};

	VK_CHECK(vkCreateDescriptorSetLayout(m_vkDevice, &createInfo, nullptr, &m_vkComputeDescriptorSetLayout), failed to create compute descriptor set layout!);
}

void HelloTriangleApplication::createUniformBuffers()
{
	VkDeviceSize bufferSize = sizeof(UniformBufferObject);

	m_vkUniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
	m_vkUniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
	m_vkUniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		createBuffer(bufferSize,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT,
			m_vkUniformBuffers[i],
			m_vkUniformBuffersMemory[i]
			);

		VK_CHECK(vkMapMemory(m_vkDevice, m_vkUniformBuffersMemory[i], 0, bufferSize, 0, &m_vkUniformBuffersMapped[i]), failed to map uniform memory!);

	}

}

void HelloTriangleApplication::updateUniformBuffer(uint32_t currentImage)
{
	static auto startTime = std::chrono::high_resolution_clock::now();

	auto curTime = std::chrono::high_resolution_clock::now();

	float time = std::chrono::duration<float, std::chrono::seconds::period>(curTime - startTime).count();

	UniformBufferObject ubo = {
		.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
		.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
		.proj = glm::perspective(glm::radians(45.0f), m_vkSwapChainExtent.width / (float)m_vkSwapChainExtent.height, 0.1f, 10.0f),
	};

	ubo.proj[1][1] *= -1; //otherwise the image will be upside down

	memmove(m_vkUniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
}

void HelloTriangleApplication::createDescriptorPool()
{
	VkDescriptorPoolSize unifBufferPoolSize = {
		.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		.descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT)
	};

	VkDescriptorPoolSize StorageBufferPoolSize = {
		.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		.descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) * 2
	};

	std::array<VkDescriptorPoolSize, 2> poolSizes = {
		unifBufferPoolSize,
		StorageBufferPoolSize
	};
	VkDescriptorPoolCreateInfo poolInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT),
		.poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
		.pPoolSizes = poolSizes.data(),
	};

	VK_CHECK(vkCreateDescriptorPool(m_vkDevice, &poolInfo, nullptr, &m_vkDescriptorPool), failed to create descriptor pool!);
}

void HelloTriangleApplication::createDescriptorSets()
{
	std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, m_vkDescriptorSetLayout);
	VkDescriptorSetAllocateInfo allocInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.descriptorPool = m_vkDescriptorPool,
		.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT),
		.pSetLayouts = layouts.data(),
	};

	m_vkDescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
	VK_CHECK(vkAllocateDescriptorSets(m_vkDevice, &allocInfo, m_vkDescriptorSets.data()), failed to allocate descriptor sets!);
}

void HelloTriangleApplication::bindDescriptorSets()
{
	for (auto i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
		VkDescriptorBufferInfo bufferInfo = {
			.buffer = m_vkUniformBuffers[i],
			.offset = 0,
			.range = sizeof(UniformBufferObject),
		};
		VkWriteDescriptorSet descriptorSetWrite = {
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = m_vkDescriptorSets[i],
			.dstBinding = 0,
			.dstArrayElement = 0,
			.descriptorCount = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.pBufferInfo = &bufferInfo,
			.pTexelBufferView = nullptr, //optional
		};

		descriptorSetWrite.pImageInfo = nullptr; //optional

		VkDescriptorImageInfo imageInfo = {
			.sampler = m_vkTextureSampler,
			.imageView = m_vkTextureImageView,
			.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		};
		VkWriteDescriptorSet imageDescriptSetWrite = {
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = m_vkDescriptorSets[i],
			.dstBinding = 1,
			.dstArrayElement = 0,
			.descriptorCount = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.pImageInfo = &imageInfo,
			.pTexelBufferView = nullptr, //optional
		};

		std::array<VkWriteDescriptorSet, 2> descriptorSetWrites = { descriptorSetWrite, imageDescriptSetWrite };

		vkUpdateDescriptorSets(m_vkDevice, static_cast<uint32_t>(descriptorSetWrites.size()), descriptorSetWrites.data(), 0, nullptr);
	}

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
	{
		VkDescriptorBufferInfo unifBufferInfo = {
			.buffer = m_vkUniformBuffers[i],
			.offset = 0,
			.range = sizeof(UniformBufferObject),
		};
		VkWriteDescriptorSet unifDescriptorSetWrite = {
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = m_vkComputeDescriptorSets[i],
			.dstBinding = 0,
			.dstArrayElement = 0,
			.descriptorCount = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.pBufferInfo = &unifBufferInfo,
			.pTexelBufferView = nullptr, //optional
		};

		VkDescriptorBufferInfo lastFrameBuffer = {
			.buffer = m_vkShaderStorageBuffers[(i - 1) % MAX_FRAMES_IN_FLIGHT],
			.offset = 0,
			.range = sizeof(Particle) * PARTICLE_COUNT,
		};
		VkWriteDescriptorSet lastFrameDescriptorSetWrite = {
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = m_vkComputeDescriptorSets[i],
			.dstBinding = 1,
			.dstArrayElement = 0,
			.descriptorCount = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.pBufferInfo = &lastFrameBuffer,
			.pTexelBufferView = nullptr, //optional
		};

		VkDescriptorBufferInfo thisFrameBuffer = {
			.buffer = m_vkShaderStorageBuffers[i],
			.offset = 0,
			.range = sizeof(Particle) * PARTICLE_COUNT,
		};
		VkWriteDescriptorSet thisFrameDescriptorSetWrite = {
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = m_vkComputeDescriptorSets[i],
			.dstBinding = 2,
			.dstArrayElement = 0,
			.descriptorCount = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.pBufferInfo = &thisFrameBuffer,
			.pTexelBufferView = nullptr, //optional
		};

		std::array<VkWriteDescriptorSet, 3> descriptorSetWrites = { unifDescriptorSetWrite, lastFrameDescriptorSetWrite, thisFrameDescriptorSetWrite };
		vkUpdateDescriptorSets(m_vkDevice, static_cast<uint32_t>(descriptorSetWrites.size()), descriptorSetWrites.data(), 0, nullptr);
	}
}

void HelloTriangleApplication::createTextureImage()
{
	int texWidth = 0, texHeight = 0, texChannels = 0;
	auto* pixels = stbi_load(TEXTURE_PATH.c_str(), &texWidth, &texHeight, &texChannels,
		STBI_rgb_alpha);
	VkDeviceSize imageSize = texWidth * texHeight * 4;
	m_mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;
	
	if (!pixels) {
		throw std::runtime_error("failed to load texture image!");
	}

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMem;
	createBuffer(imageSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		stagingBuffer,
		stagingBufferMem
		);

	void* data;
	vkMapMemory(m_vkDevice, stagingBufferMem, 0, imageSize, 0, &data);
	memcpy(data, pixels, static_cast<size_t>(imageSize));
	vkUnmapMemory(m_vkDevice, stagingBufferMem);

	stbi_image_free(pixels);
	createImage(texWidth, texHeight, m_mipLevels, VK_SAMPLE_COUNT_1_BIT,
		VK_FORMAT_R8G8B8A8_SRGB,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		m_vkTextureImage,
		m_vkTextureImageMemory
	);

	//transition texture image to VK_IMAGE_LAYOUT_TRANSFER_DEST_OPTIMAL
	transitionImageLayout(m_vkTextureImage, m_mipLevels,
		VK_FORMAT_R8G8B8A8_SRGB,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
		);
	
	//copy image from staging buffer to transitioned vkimage
	copyBufferToImage(stagingBuffer, m_vkTextureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));

	//enable sampling from the image
	//transitionImageLayout(m_vkTextureImage, m_mipLevels,
	//	VK_FORMAT_R8G8B8A8_SRGB,
	//	VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
	//	VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
	//);

	vkDestroyBuffer(m_vkDevice, stagingBuffer, nullptr);
	vkFreeMemory(m_vkDevice, stagingBufferMem, nullptr);

	generateMipmaps(m_vkTextureImage, VK_FORMAT_R8G8B8A8_SRGB, texWidth, texHeight, m_mipLevels);
}

void HelloTriangleApplication::createImage(uint32_t width, uint32_t height, uint32_t mip, VkSampleCountFlagBits numSamples,
	VkFormat format, VkImageTiling tilling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory)
{
	VkImageCreateInfo imgInfo = {
	.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
	.imageType = VK_IMAGE_TYPE_2D,
	.extent = {
		.width = static_cast<uint32_t>(width),
		.height = static_cast<uint32_t>(height),
		.depth = 1
	},
	.mipLevels = mip,
	.arrayLayers = 1,
	};

	imgInfo.format = format;
	imgInfo.tiling = tilling;
	imgInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imgInfo.usage = usage;
	imgInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imgInfo.samples = numSamples;
	imgInfo.flags = 0;

	VK_CHECK(vkCreateImage(m_vkDevice, &imgInfo, nullptr, &image), failed to crate image!);

	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(m_vkDevice, image, &memRequirements);

	VkMemoryAllocateInfo allocInfo = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.allocationSize = memRequirements.size,
		.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties)
	};

	VK_CHECK(vkAllocateMemory(m_vkDevice, &allocInfo, nullptr, &imageMemory), failed to allocate image stbi_info_from_memory!);

	vkBindImageMemory(m_vkDevice, image, imageMemory, 0);
}

VkCommandBuffer HelloTriangleApplication::beginSingleTimeCommands()
{
	VkCommandBufferAllocateInfo allocInfo = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.commandPool = m_vkCommandPool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = 1
	};
	VkCommandBuffer commandBuffer;
	VK_CHECK(vkAllocateCommandBuffers(m_vkDevice, &allocInfo, &commandBuffer),failed to allocate single time command buffer!);

	VkCommandBufferBeginInfo beginInfo = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
	};

	VK_CHECK(vkBeginCommandBuffer(commandBuffer, &beginInfo), failed to begin single time command!);
	
	return commandBuffer;
}

void HelloTriangleApplication::endSingleTimeCommands(VkCommandBuffer commandBuffer)
{
	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.commandBufferCount = 1,
		.pCommandBuffers = &commandBuffer,
	};

	VK_CHECK(vkQueueSubmit(m_vkGraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE),failed to submit single time command to queue!);
	vkQueueWaitIdle(m_vkGraphicsQueue);

	vkFreeCommandBuffers(m_vkDevice, m_vkCommandPool, 1, &commandBuffer);
}

void HelloTriangleApplication::transitionImageLayout(VkImage image, uint32_t mip, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout)
{
	VkCommandBuffer commandBuffer = beginSingleTimeCommands();
	VkImageMemoryBarrier barrier = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
		.oldLayout = oldLayout,
		.newLayout = newLayout,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED, //set other if you want to transfer queue family
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.image = image,
		.subresourceRange = {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.baseMipLevel = 0,
			.levelCount = mip,
			.baseArrayLayer = 0,
			.layerCount = 1,
		}
	};

	VkPipelineStageFlags srcStage = VK_PIPELINE_STAGE_NONE;
	VkPipelineStageFlags dstStage = VK_PIPELINE_STAGE_NONE;
	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		if (hasStencil(format))
		{
			barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
		}
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
																//The reading happens in the VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT stage 
																//and the writing in the VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT
		dstStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;	// the earlist pipeline stage that matches the specified operations
		srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT; 
		
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else {
		throw std::invalid_argument("unsupported layout transition!");
	}

	vkCmdPipelineBarrier(commandBuffer,
		srcStage, //happen before transition
		dstStage, //happen after transition
		0,
		0, nullptr,
		0, nullptr,
		1, &barrier
		);
	endSingleTimeCommands(commandBuffer);
}

void HelloTriangleApplication::copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
{
	VkCommandBuffer commandBuffer = beginSingleTimeCommands();
	VkBufferImageCopy region = {
		.bufferOffset = 0,
		.bufferRowLength = 0,
		.bufferImageHeight = 0,
		.imageSubresource = {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.mipLevel = 0,
			.baseArrayLayer = 0,
			.layerCount = 1
		},
		.imageOffset = {0,0,0},
		.imageExtent = {width, height, 1},
	};

	vkCmdCopyBufferToImage(
		commandBuffer,
		buffer,
		image,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		1,
		&region
	);

	endSingleTimeCommands(commandBuffer);
}

VkImageView HelloTriangleApplication::createImageView(VkImage _image,uint32_t _mip, VkFormat _format, VkImageAspectFlags aspectMask)
{
	VkImageViewCreateInfo viewInfo = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.image = _image,
		.viewType = VK_IMAGE_VIEW_TYPE_2D,
		.format = _format,
		.subresourceRange = {
			.aspectMask = aspectMask,
			.baseMipLevel = 0,
			.levelCount = _mip,
			.baseArrayLayer = 0,
			.layerCount = 1
		}
	};
	VkImageView imageView;
	VK_CHECK(vkCreateImageView(m_vkDevice, &viewInfo, nullptr, &imageView), failed to create image view!);
	return imageView;
}

void HelloTriangleApplication::createTextureImageView()
{
	m_vkTextureImageView = createImageView(m_vkTextureImage, m_mipLevels);
}

void HelloTriangleApplication::createTextureSampler()
{
	VkSamplerCreateInfo createInfo = {
		.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
		.magFilter = VK_FILTER_LINEAR,
		.minFilter = VK_FILTER_LINEAR,
		.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
		.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
		.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
		.anisotropyEnable = VK_TRUE,
		.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
		.unnormalizedCoordinates = VK_FALSE, //set to false means to sample image with [0,1), set to true means to sample image with [0, texWidth)
	};

	createInfo.maxAnisotropy = m_vkDeviceProperties.limits.maxSamplerAnisotropy;
	
	//used in filtering operations
	createInfo.compareEnable = VK_FALSE;
	createInfo.compareOp = VK_COMPARE_OP_ALWAYS;

	createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	createInfo.mipLodBias = 0.f;
	createInfo.minLod = static_cast<float>(0);
	createInfo.maxLod = static_cast<float>(m_mipLevels);

	VK_CHECK(vkCreateSampler(m_vkDevice, &createInfo, nullptr, &m_vkTextureSampler), failed to create texture sampler!);
}

void HelloTriangleApplication::createDepthImage()
{
	VkFormat depthFormat = findDepthFormat();
	createImage(m_vkSwapChainExtent.width, m_vkSwapChainExtent.height, 1, m_vkMSAASamples,
		depthFormat,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		m_vkDepthImage,
		m_vkDepthImageMemory
		);
	m_vkDepthImageView = createImageView(m_vkDepthImage, 1, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);

	transitionImageLayout(m_vkDepthImage, 1, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
}

VkFormat HelloTriangleApplication::findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tilling, VkFormatFeatureFlags features)
{
	for (auto format : candidates)
	{
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(m_vkPhysicDevice, format, &props);
		if (tilling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
		{
			return format;
		}
		else if (tilling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
		{
			return format;
		}
	}
	throw std::runtime_error("failed to find supported format!");
}

VkFormat HelloTriangleApplication::findDepthFormat()
{
	return findSupportedFormat(
		{VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
		VK_IMAGE_TILING_OPTIMAL,
		VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
	);
}

bool HelloTriangleApplication::hasStencil(VkFormat format)
{
	return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

void HelloTriangleApplication::loadModel()
{
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warn, err;

	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, MODEL_PATH.c_str()))
	{
		throw std::runtime_error(warn + err);
	}
	std::unordered_map<Vertex, uint32_t> vertexMap;
	for (const auto& shape : shapes)
	{
		for (const auto& index : shape.mesh.indices)
		{
			Vertex vertex{};
			vertex.pos = {
				attrib.vertices[3 * index.vertex_index + 0],
				attrib.vertices[3 * index.vertex_index + 1],
				attrib.vertices[3 * index.vertex_index + 2],
			};
			vertex.texcoord = {
				attrib.texcoords[2 * index.texcoord_index + 0],
				1.0 - attrib.texcoords[2 * index.texcoord_index + 1],
			};

			if (vertexMap.find(vertex) == vertexMap.end())
			{
				vertexMap[vertex] = static_cast<uint32_t>(vertices.size());
				vertices.push_back(vertex);
			}

			indices.push_back(vertexMap[vertex]);
		}
	}
}

void HelloTriangleApplication::generateMipmaps(VkImage image, VkFormat format, int32_t width, int32_t height, uint32_t mip)
{
	VkFormatProperties formatProperties;
	vkGetPhysicalDeviceFormatProperties(m_vkPhysicDevice, format, &formatProperties);
	if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT))
	{
		throw std::runtime_error("texture image format doesn't support linear blitting!");
	}

	VkCommandBuffer cmdBuf = beginSingleTimeCommands();

	VkImageMemoryBarrier barrier{
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.image = image,
		.subresourceRange = {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.levelCount = 1,
			.baseArrayLayer = 0,
			.layerCount = 1,
		},
	};

	int32_t mipWidth = width;
	int32_t mipHeight = height;
	
	for (uint32_t i = 1; i < mip; ++i)
	{
		barrier.subresourceRange.baseMipLevel = i - 1;
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

		vkCmdPipelineBarrier(cmdBuf,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
			0, nullptr,
			0, nullptr,
			1, &barrier);

		VkImageBlit blit{
			.srcSubresource = {
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.mipLevel = i - 1,
				.baseArrayLayer = 0,
				.layerCount = 1
			},
			.srcOffsets = {{0,0,0}, {mipWidth, mipHeight, 1}},
			
			.dstSubresource = {
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.mipLevel = i,
				.baseArrayLayer = 0,
				.layerCount = 1
			},
			.dstOffsets = {{0,0,0}, {mipWidth > 1 ? mipWidth /2 : 1, mipHeight > 1 ? mipHeight/2 : 1, 1}},
		};

		vkCmdBlitImage(cmdBuf, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR);

		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		vkCmdPipelineBarrier(
			cmdBuf,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
			0, nullptr,
			0, nullptr,
			1, &barrier
		);

		if (mipWidth > 1) mipWidth >>= 1;
		if (mipHeight > 1) mipHeight >>= 1;
	}

	//transfer the last mip level texture into read optimal
	barrier.subresourceRange.baseMipLevel = mip - 1;
	barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

	vkCmdPipelineBarrier(
		cmdBuf,
		VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
		0, nullptr,
		0, nullptr,
		1, &barrier
	);

	endSingleTimeCommands(cmdBuf);
}

VkSampleCountFlagBits HelloTriangleApplication::getMaxUsableSampleCount()
{
	VkPhysicalDeviceProperties physicalDeviceProperties;
	vkGetPhysicalDeviceProperties(m_vkPhysicDevice, &physicalDeviceProperties);
	VkSampleCountFlags counts = physicalDeviceProperties.limits.framebufferColorSampleCounts
		& physicalDeviceProperties.limits.framebufferDepthSampleCounts;
	if (counts & VK_SAMPLE_COUNT_64_BIT) { return VK_SAMPLE_COUNT_64_BIT; }
	if (counts & VK_SAMPLE_COUNT_32_BIT) { return VK_SAMPLE_COUNT_32_BIT; }
	if (counts & VK_SAMPLE_COUNT_16_BIT) { return VK_SAMPLE_COUNT_16_BIT; }
	if (counts & VK_SAMPLE_COUNT_8_BIT)  { return VK_SAMPLE_COUNT_8_BIT; }
	if (counts & VK_SAMPLE_COUNT_4_BIT)  { return VK_SAMPLE_COUNT_4_BIT; }
	if (counts & VK_SAMPLE_COUNT_2_BIT)  { return VK_SAMPLE_COUNT_2_BIT; }
	return VK_SAMPLE_COUNT_1_BIT;
}

void HelloTriangleApplication::createColorResources()
{
	VkFormat colorFormat = m_swapChainImageFormat;

	createImage(
		m_vkSwapChainExtent.width,
		m_vkSwapChainExtent.height,
		1,
		m_vkMSAASamples,
		colorFormat,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		m_vkColorImage,
		m_vkColorImageMemory
	);
	m_vkColorImageView = createImageView(m_vkColorImage, 1, colorFormat, VK_IMAGE_ASPECT_COLOR_BIT);
}

void HelloTriangleApplication::createComputePipeline()
{
	auto computeShaderCode = readFile("E:/GitStorage/LearnVulkan/bin/shaders/trig.comp.spv");

	VkShaderModule compShaderModule = createShaderModule(computeShaderCode);

	VkPipelineShaderStageCreateInfo compShaderStageInfo{
	.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
	.stage = VK_SHADER_STAGE_COMPUTE_BIT,
	.module = compShaderModule,
	.pName = "main", // entry point function in shader
	};

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.setLayoutCount = 1,
		.pSetLayouts = &m_vkComputeDescriptorSetLayout,
	};
	if (vkCreatePipelineLayout(m_vkDevice, &pipelineLayoutInfo, nullptr, &m_vkComputePipelineLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create pipeline layout!");
	}

	VkComputePipelineCreateInfo pipelineInfo{
		.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
		.stage = compShaderStageInfo,
		.layout = m_vkComputePipelineLayout,
	};

	VK_CHECK(vkCreateComputePipelines(m_vkDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_vkComputePipeline), failed to create compute pipeline!);

	vkDestroyShaderModule(m_vkDevice, compShaderModule, nullptr);
}

void HelloTriangleApplication::createShaderStorageBuffers()
{
	m_vkShaderStorageBuffers.resize(MAX_FRAMES_IN_FLIGHT);
	m_vkShaderStorageBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);

	std::default_random_engine rndEngine((unsigned)time(nullptr));
	std::uniform_real_distribution<float> rndDist(0.0f, 1.0f);

	std::vector<Particle> particles(PARTICLE_COUNT);

	for (auto& particle : particles)
	{
		float r = 0.25f * sqrt(rndDist(rndEngine));
		float theta = rndDist(rndEngine) * 2 * 3.14159265358979323846;
		float x = r * cos(theta) * HEIGHT / WIDTH;
		float y = r * sin(theta);
		particle.pos = glm::vec2(x, y);
		particle.velocity = glm::normalize(glm::vec2(x, y)) * 0.00025f;
		particle.color = glm::vec4(rndDist(rndEngine), rndDist(rndEngine), rndDist(rndEngine), 1.0f);
	}

	VkDeviceSize bufferSize = sizeof(Particle) * PARTICLE_COUNT;
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	createBuffer(bufferSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
		stagingBuffer,
		stagingBufferMemory
		);

	void* data;
	vkMapMemory(m_vkDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, particles.data(), static_cast<size_t>(bufferSize));
	vkUnmapMemory(m_vkDevice, stagingBufferMemory);

	for (size_t i = 0; i < PARTICLE_COUNT; ++i)
	{
		createBuffer(bufferSize,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			m_vkShaderStorageBuffers[i],
			m_vkShaderStorageBuffersMemory[i]
			);

		copyBuffer(stagingBuffer, m_vkShaderStorageBuffers[i], bufferSize);
	}
}

void HelloTriangleApplication::recordComputeCommand(VkCommandBuffer commandBuffer, uint32_t frameIdx)
{
	VkCommandBufferBeginInfo beginInfo{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO
	};

	VK_CHECK(vkBeginCommandBuffer(commandBuffer, &beginInfo), failed to begin recording command buffer!);

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_vkComputePipeline);
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_vkComputePipelineLayout, 0, 1, &m_vkComputeDescriptorSets[frameIdx], 0, nullptr);

	vkCmdDispatch(commandBuffer, PARTICLE_COUNT / 256, 1, 1);

	VK_CHECK(vkEndCommandBuffer(commandBuffer), failed to record command buffer!);
}

void HelloTriangleApplication::mainLoop()
{
	while (!glfwWindowShouldClose(m_window))
	{
		glfwPollEvents();
		drawFrame();
	}

	vkDeviceWaitIdle(m_vkDevice);
}

void HelloTriangleApplication::cleanUp()
{
	cleanUpSwapChain();
	if (enableValidationLayer)DestroyDebugUtilsMessengerEXT(m_vkInstance, m_vkDebugMessenger, nullptr);
	for (auto i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
	{
		vkDestroySemaphore(m_vkDevice, m_vkImageAvailableSemaphores[i], nullptr);
		vkDestroySemaphore(m_vkDevice, m_vkRenderFinishedSemaphores[i], nullptr);
		vkDestroyFence(m_vkDevice, m_vkInFlightFences[i], nullptr);

		vkDestroySemaphore(m_vkDevice, m_vkComputeFinishedSemaphores[i], nullptr);
		vkDestroyFence(m_vkDevice, m_vkComputeInFlightFences[i], nullptr);

		vkDestroyBuffer(m_vkDevice, m_vkUniformBuffers[i], nullptr);
		vkFreeMemory(m_vkDevice, m_vkUniformBuffersMemory[i], nullptr);
	}
	for (auto i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
	{
		vkDestroyBuffer(m_vkDevice, m_vkShaderStorageBuffers[i], nullptr);
		vkFreeMemory(m_vkDevice, m_vkShaderStorageBuffersMemory[i], nullptr);
	}
	vkDestroyBuffer(m_vkDevice, m_vkVertexBuffer, nullptr);
	vkFreeMemory(m_vkDevice, m_vkVertexBufferMemory, nullptr);
	vkDestroyBuffer(m_vkDevice, m_vkIndexBuffer, nullptr);
	vkFreeMemory(m_vkDevice, m_vkIndexBufferMemory, nullptr);
	vkDestroyCommandPool(m_vkDevice, m_vkCommandPool, nullptr);
	vkDestroyPipeline(m_vkDevice, m_vkGraphicsPipeline, nullptr);
	vkDestroyPipelineLayout(m_vkDevice, m_vkPipelineLayout, nullptr);
	vkDestroyPipeline(m_vkDevice, m_vkComputePipeline, nullptr);
	vkDestroyPipelineLayout(m_vkDevice, m_vkComputePipelineLayout, nullptr);

	vkDestroyDescriptorPool(m_vkDevice, m_vkDescriptorPool, nullptr); // descriptor sets will be destroyed automatically
	vkDestroyDescriptorSetLayout(m_vkDevice, m_vkDescriptorSetLayout, nullptr);
	vkDestroyDescriptorSetLayout(m_vkDevice, m_vkComputeDescriptorSetLayout, nullptr);
	vkDestroyImageView(m_vkDevice, m_vkTextureImageView, nullptr); // destroy image view before image
	vkDestroyImage(m_vkDevice, m_vkTextureImage, nullptr);
	vkFreeMemory(m_vkDevice, m_vkTextureImageMemory, nullptr);
	vkDestroyImageView(m_vkDevice, m_vkColorImageView, nullptr);
	vkDestroyImage(m_vkDevice, m_vkColorImage, nullptr);
	vkFreeMemory(m_vkDevice, m_vkColorImageMemory, nullptr);
	vkDestroySampler(m_vkDevice, m_vkTextureSampler, nullptr);
	vkDestroyRenderPass(m_vkDevice, m_vkRenderPass, nullptr);
	vkDestroyDevice(m_vkDevice, nullptr);
	vkDestroySurfaceKHR(m_vkInstance, m_vkSurface, nullptr);
	vkDestroyInstance(m_vkInstance, nullptr);
	glfwDestroyWindow(m_window);
	glfwTerminate();
}
