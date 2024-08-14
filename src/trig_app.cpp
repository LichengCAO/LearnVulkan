#include "trig_app.h"
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

const std::vector<Vertex> vertices = {
	{.pos = {-0.5,-0.5}, .color = {1,0,0}},
	{.pos = {0.5, -0.5f}, .color = {1,0,0}},
	{.pos = {0.5, 0.5}, .color = {0,1,0}},
	{.pos = {-0.5,0.5}, .color = {0,0,1}},
};

const std::vector<uint16_t> indices = {
	0,1,2,2,3,0
};

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
		VkClearValue clearColor = { {{0.f,0.f,0.f,1.f}} };
		VkRenderPassBeginInfo renderPassInfo{
			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
			.renderPass = m_vkRenderPass,
			.framebuffer = m_vkFramebuffers[imgIdx],
			.renderArea = {
				.offset = {0,0},
				.extent = m_swapChainExtent
			},
			.clearValueCount = 1,
			.pClearValues = &clearColor
		};
		vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
		{
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_vkGraphicsPipeline);
			//same in createGraphicsPipeline
			VkViewport viewport{
				.x = 0.f,
				.y = 0.f,
				.width = (float)m_swapChainExtent.width,
				.height = (float)m_swapChainExtent.height,
				.minDepth = 0.f,
				.maxDepth = 1.f
			};
			VkRect2D scissor{
				.offset = {0,0},
				.extent = m_swapChainExtent
			};
			vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
			vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
			VkBuffer vertexBuffers[] = { m_vkVertexBuffer };
			VkDeviceSize offsets[] = { 0 };
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
			vkCmdBindIndexBuffer(commandBuffer, m_vkIndexBuffer, 0, VK_INDEX_TYPE_UINT16);
			//vkCmdDraw(commandBuffer, static_cast<uint32_t>(vertices.size()), 1, 0, 0);
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
	VkSemaphore waitSemaphores[] = { m_vkImageAvailableSemaphores[m_curFrame] };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	VkSemaphore signalSemaphores[] = { m_vkRenderFinishedSemaphores[m_curFrame] };
	VkSubmitInfo submitInfo{
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = waitSemaphores,
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
	createGraphicsPipeline();
	createFramebuffers();
	createCommandPool();
	createVertexBuffer();
	createIndexBuffer();
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
	m_swapChainExtent = extent;
	m_swapChainImageFormat = surfaceFormat.format;
}

void HelloTriangleApplication::createImageViews(){
	m_vkSwapChainImageViews.resize(m_vkSwapChainImages.size());
	for (size_t i = 0;i < m_vkSwapChainImages.size();++i) {
		VkImageViewCreateInfo createInfo{
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.image = m_vkSwapChainImages[i],
			.viewType = VK_IMAGE_VIEW_TYPE_2D,
			.format = m_swapChainImageFormat,
			.components = VkComponentMapping{
				.r = VK_COMPONENT_SWIZZLE_IDENTITY,
				.g = VK_COMPONENT_SWIZZLE_IDENTITY,
				.b = VK_COMPONENT_SWIZZLE_IDENTITY,
				.a = VK_COMPONENT_SWIZZLE_IDENTITY
			},
			.subresourceRange = VkImageSubresourceRange{
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.baseMipLevel = 0,
				.levelCount = 1,
				.baseArrayLayer = 0,
				.layerCount = 1
			}
		};

		if (vkCreateImageView(m_vkDevice, &createInfo, nullptr, &m_vkSwapChainImageViews[i]) != VK_SUCCESS) {
			throw std::runtime_error("failed to create image views!");
		}
	}
}

void HelloTriangleApplication::createRenderPass()
{
	VkAttachmentDescription colorAttachment{
		.format = m_swapChainImageFormat,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
	};

	VkAttachmentReference colorAttachmentRef{
		.attachment = 0,
		.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	};

	VkSubpassDescription subpass{
		.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
		.colorAttachmentCount = 1,
		.pColorAttachments = &colorAttachmentRef
	};

	VkSubpassDependency dependencyInfo{
	.srcSubpass = VK_SUBPASS_EXTERNAL,
	.dstSubpass = 0,
	.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
	.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
	.srcAccessMask = 0,
	.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
	};


	VkRenderPassCreateInfo renderPassInfo{
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		.attachmentCount = 1,
		.pAttachments = &colorAttachment,
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
	.width = (float)m_swapChainExtent.width,
	.height = (float)m_swapChainExtent.height,
	.minDepth = 0.f,
	.maxDepth = 1.f
	};
	VkRect2D scissor{
		.offset = {0,0},
		.extent = m_swapChainExtent
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

	VkPipelineMultisampleStateCreateInfo multisampleInfo{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
		.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
		.sampleShadingEnable = VK_FALSE,
		.minSampleShading = 1.0f,
		.pSampleMask = nullptr,
		.alphaToCoverageEnable = VK_FALSE,
		.alphaToOneEnable = VK_FALSE
	};

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
		.setLayoutCount = 0,
		.pSetLayouts = nullptr,
		.pushConstantRangeCount = 0,
		.pPushConstantRanges = nullptr,
	};
	if (vkCreatePipelineLayout(m_vkDevice, &pipelineLayoutInfo, nullptr, &m_vkPipelineLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create pipeline layout!");
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
		.pDepthStencilState = nullptr,
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

void HelloTriangleApplication::createFramebuffers()
{
	m_vkFramebuffers.resize(m_vkSwapChainImages.size());
	for (size_t i = 0;i < m_vkSwapChainImageViews.size();++i) {
		VkImageView attachements[] = {
			m_vkSwapChainImageViews[i]
		};

		VkFramebufferCreateInfo framebufferInfo{
			.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			.renderPass = m_vkRenderPass,
			.attachmentCount = 1,
			.pAttachments = attachements,
			.width = m_swapChainExtent.width,
			.height = m_swapChainExtent.height,
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
		.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value()
	};

	if (vkCreateCommandPool(m_vkDevice, &commandPoolInfo, nullptr, &m_vkCommandPool) != VK_SUCCESS) {
		throw std::runtime_error("failed to create command pool!");
	}
}

void HelloTriangleApplication::createCommandBuffers()
{
	m_vkCommandBuffers.resize(MAX_FRAMES_IN_FLIGHT);
	VkCommandBufferAllocateInfo allocInfo{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.commandPool = m_vkCommandPool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = static_cast<uint32_t>(m_vkCommandBuffers.size())
	};

	if (vkAllocateCommandBuffers(m_vkDevice, &allocInfo, m_vkCommandBuffers.data()) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate command buffers!");
	}
}

void HelloTriangleApplication::createSyncObjects(){
	m_vkImageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	m_vkInFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
	m_vkRenderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);

	VkSemaphoreCreateInfo semaphoreInfo{
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
	};

	VkFenceCreateInfo fenceInfo{
		.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		.flags = VK_FENCE_CREATE_SIGNALED_BIT // to make sure the first frame won't be blocked
	};

	for (auto i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
	{
		if (vkCreateSemaphore(m_vkDevice, &semaphoreInfo, nullptr, &m_vkImageAvailableSemaphores[i]) != VK_SUCCESS
			|| vkCreateSemaphore(m_vkDevice, &semaphoreInfo, nullptr, &m_vkRenderFinishedSemaphores[i]) != VK_SUCCESS
			|| vkCreateFence(m_vkDevice, &fenceInfo, nullptr, &m_vkInFlightFences[i]) != VK_SUCCESS) {
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
	createFramebuffers();
}

void HelloTriangleApplication::cleanUpSwapChain()
{
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
	VkCommandBufferAllocateInfo allocInfo = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.commandPool = m_vkCommandPool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = 1,
	};

	VkCommandBuffer cpyCommandBuffer;
	if (vkAllocateCommandBuffers(m_vkDevice, &allocInfo, &cpyCommandBuffer) != VK_SUCCESS) {
		throw std::runtime_error("create copy command buffer failed!");
	}

	VkCommandBufferBeginInfo beginInfo = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
	};

	VK_CHECK(vkBeginCommandBuffer(cpyCommandBuffer, &beginInfo), create cpy buffer failed);

	VkBufferCopy cpyRegion = {
		.srcOffset = 0,
		.dstOffset = 0,
		.size = size,
	};

	vkCmdCopyBuffer(cpyCommandBuffer, src, dst, 1, &cpyRegion);

	vkEndCommandBuffer(cpyCommandBuffer);

	VkSubmitInfo submitInfo = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.commandBufferCount = 1,
		.pCommandBuffers = &cpyCommandBuffer,
	};

	VkQueue copyQueue = m_vkGraphicsQueue;
	VK_CHECK(vkQueueSubmit(copyQueue, 1, &submitInfo, VK_NULL_HANDLE), failed to submit copy command at line 1022);
	vkQueueWaitIdle(copyQueue);
	vkFreeCommandBuffers(m_vkDevice, m_vkCommandPool, 1, &cpyCommandBuffer);
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

	return indices.isComplete() && extensionsSupported && swapChainAdequate;
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
	std::unordered_set<uint32_t> uniqueQueueFamilies{ indices.graphicsFamily.value(), indices.presentFamily.value() };
	for (auto queueFamily : uniqueQueueFamilies) {
		VkDeviceQueueCreateInfo queueCreateInfo{
			.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
			.queueFamilyIndex = queueFamily,
			.queueCount = 1,
			.pQueuePriorities = &priority
		};
		queueCreateInfos.push_back(queueCreateInfo);
	}

	VkPhysicalDeviceFeatures deviceFeatures{};
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
}

void HelloTriangleApplication::setupDebugMessenger()
{
	if (!enableValidationLayer)return;
	VkDebugUtilsMessengerCreateInfoEXT createInfo = getDefaultDebugMessengerCreateInfo();
	if (CreateDebugUtilsMessengerEXT(m_vkInstance, &createInfo, nullptr, &m_vkDebugMessenger) != VK_SUCCESS) {
		throw std::runtime_error("failed to setup debug messenger");
	}
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
	}
	vkDestroyBuffer(m_vkDevice, m_vkVertexBuffer, nullptr);
	vkFreeMemory(m_vkDevice, m_vkVertexBufferMemory, nullptr);
	vkDestroyBuffer(m_vkDevice, m_vkIndexBuffer, nullptr);
	vkFreeMemory(m_vkDevice, m_vkIndexBufferMemory, nullptr);
	vkDestroyCommandPool(m_vkDevice, m_vkCommandPool, nullptr);
	vkDestroyPipeline(m_vkDevice, m_vkGraphicsPipeline, nullptr);
	vkDestroyPipelineLayout(m_vkDevice, m_vkPipelineLayout, nullptr);
	vkDestroyRenderPass(m_vkDevice, m_vkRenderPass, nullptr);
	vkDestroyDevice(m_vkDevice, nullptr);
	vkDestroySurfaceKHR(m_vkInstance, m_vkSurface, nullptr);
	vkDestroyInstance(m_vkInstance, nullptr);
	glfwDestroyWindow(m_window);
	glfwTerminate();
}
