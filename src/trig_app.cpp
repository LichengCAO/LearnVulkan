#include "trig_app.h"
#include <stdexcept>
#include <iostream>
#include <unordered_set>
#include <string>
#include "help.h"

const std::vector<const char*> validationLayers = {
	"VK_LAYER_KHRONOS_validation"
};

const bool enableValidationLayer = true;

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
	auto extensionProperties = getAvailableExtensions();
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

std::vector<VkExtensionProperties> HelloTriangleApplication::getAvailableExtensions()const
{
	uint32_t extensionCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
	std::vector<VkExtensionProperties> extensionProperties(extensionCount);
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensionProperties.data());
	return extensionProperties;
}

std::vector<VkLayerProperties> HelloTriangleApplication::getAvailableLayerProperties() const
{
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
	std::vector<VkLayerProperties> res(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, res.data());
	return res;
}

std::vector<const char*> HelloTriangleApplication::getRequiredExtensions() const
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

bool HelloTriangleApplication::checkExtensionSupport(const std::vector<const char*>& _extensionToCheck) const
{
	auto extensions = getAvailableExtensions();
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
	auto layers = getAvailableLayerProperties();
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

void HelloTriangleApplication::initWindow()
{
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	m_window = glfwCreateWindow(WIDTH, HEIGHT,"hello triangle", nullptr, nullptr);
}

void HelloTriangleApplication::initVulkan()
{
	createInstance();
	setupDebugMessenger();
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
	auto requiredExtensions = getRequiredExtensions();
	createInfo.enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size());
	createInfo.ppEnabledExtensionNames = requiredExtensions.data();
	
	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = getDefaultDebugMessengerCreateInfo();
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

	if (vkCreateInstance(&createInfo, nullptr, &m_vkInstance) != VK_SUCCESS) {
		throw std::runtime_error("failed to create instance!");
	}
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
	}
}

void HelloTriangleApplication::cleanUp()
{
	if (enableValidationLayer)DestroyDebugUtilsMessengerEXT(m_vkInstance, m_vkDebugMessenger, nullptr);
	vkDestroyInstance(m_vkInstance, nullptr);
	glfwDestroyWindow(m_window);
	glfwTerminate();
}
