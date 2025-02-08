#include "device.h"
#include <iostream>
#include <cstdint>
#include <unordered_set>
#include <algorithm>

std::vector<const char*> MyDevice::_GetInstanceRequiredExtensions() const
{
	uint32_t glfwExtensionsCount = 0;
	const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionsCount);
	std::vector<const char*> requiredExtensions;

	for (uint32_t i = 0; i < glfwExtensionsCount; ++i) {
		requiredExtensions.emplace_back(glfwExtensions[i]);
	}
	requiredExtensions.emplace_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
	/*if (enableValidationLayer)*/ requiredExtensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	return requiredExtensions;
}

std::vector<const char*> MyDevice::_GetPhysicalDeviceRequiredExtensions() const
{
	return {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
}

QueueFamilyIndices MyDevice::_GetQueueFamilyIndices(VkPhysicalDevice physicalDevice) const
{
	QueueFamilyIndices indices;
	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());
	int i = 0;
	for (const auto& queueFamily : queueFamilies) {
		if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			indices.graphicsFamily = i;
		}
		if ((queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) && (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT)) {
			indices.graphicsAndComputeFamily = i;
		}
		VkBool32 supportPresent = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, vkSurface, &supportPresent);
		if (supportPresent) {
			indices.presentFamily = i;
		}
		if (!(queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) && (queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT))
		{
			// The good news is that any queue family with VK_QUEUE_GRAPHICS_BIT or VK_QUEUE_COMPUTE_BIT capabilities already implicitly support VK_QUEUE_TRANSFER_BIT operations.
			indices.transferFamily = i;
		}
		if (indices.isComplete())break;
		++i;
	}
	return indices;
}

SwapChainSupportDetails MyDevice::_QuerySwapchainSupport(VkPhysicalDevice phyiscalDevice, VkSurfaceKHR surface) const
{
	SwapChainSupportDetails res;

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(phyiscalDevice, surface, &res.capabilities);

	uint32_t formatCnt = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(phyiscalDevice, surface, &formatCnt, nullptr);
	if (formatCnt != 0) {
		res.formats.resize(formatCnt);
		vkGetPhysicalDeviceSurfaceFormatsKHR(phyiscalDevice, surface, &formatCnt, res.formats.data());
	}

	uint32_t modeCnt = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR(phyiscalDevice, surface, &modeCnt, nullptr);
	if (modeCnt != 0) {
		res.presentModes.resize(modeCnt);
		vkGetPhysicalDeviceSurfacePresentModesKHR(phyiscalDevice, surface, &modeCnt, res.presentModes.data());
	}
	return res;
}

VkSurfaceFormatKHR MyDevice::_ChooseSwapchainFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) const
{
	for (const auto& availableFormat : availableFormats) 
	{
		if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) 
		{
			return availableFormat;
		}
	}
	return availableFormats[0];
}

VkPresentModeKHR MyDevice::_ChooseSwapchainPresentMode(const std::vector<VkPresentModeKHR>& availableModes) const
{
	for (const auto& availableMode : availableModes) 
	{
		if (availableMode == VK_PRESENT_MODE_MAILBOX_KHR) 
		{
			return availableMode;
		}
	}
	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D MyDevice::_ChooseSwapchainExtent(const VkSurfaceCapabilitiesKHR& capabilities) const
{
	if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) 
	{
		return capabilities.currentExtent;
	}
	int width, height;
	glfwGetFramebufferSize(pWindow, &width, &height);
	VkExtent2D res{
		static_cast<uint32_t>(width),
		static_cast<uint32_t>(height)
	};

	res.width = std::clamp(res.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
	res.height = std::clamp(res.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
	return res;
}

void MyDevice::_InitGLFW()
{
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
	pWindow = glfwCreateWindow(WIDTH, HEIGHT, "hello triangle", nullptr, nullptr);
	// glfwSetWindowUserPointer(pWindow, this);
	// glfwSetFramebufferSizeCallback(pWindow, FrameBufferResizedCallback);
}

void MyDevice::_CreateInstance()
{
	vkb::InstanceBuilder instanceBuilder;
	instanceBuilder.enable_extensions(_GetInstanceRequiredExtensions());
	instanceBuilder.request_validation_layers(true);
	instanceBuilder.set_app_name("Hello Triangle");
	instanceBuilder.set_app_version(VK_MAKE_VERSION(1, 0, 0));
	instanceBuilder.set_engine_name("No Engine");
	instanceBuilder.set_engine_version(VK_MAKE_VERSION(1, 0, 0));
	instanceBuilder.desire_api_version(VK_API_VERSION_1_0);
	instanceBuilder.set_debug_callback(
		[](
			VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
			VkDebugUtilsMessageTypeFlagsEXT messageType,
			const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
			void* pUserData) -> VkBool32
		{
			if (messageSeverity > VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
			{
				std::cout << "message based on severity" << std::endl;
				std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
			}
			return VK_FALSE;
		}
	);
	auto instanceBuildResult = instanceBuilder.build();

	if (!instanceBuildResult)
	{
		throw std::runtime_error("Failed to create instance!");
	}
	
	m_instance = instanceBuildResult.value();
	vkInstance = m_instance.instance;
}

void MyDevice::_CreateSurface()
{
	VK_CHECK(glfwCreateWindowSurface(vkInstance, pWindow, nullptr, &vkSurface), Failed to create window surface!);
}

void MyDevice::_SelectPhysicalDevice()
{
	vkb::PhysicalDeviceSelector physicalDeviceSelector(m_instance);
	VkPhysicalDeviceFeatures requiredFeatures{
		.geometryShader = VK_TRUE,
		.samplerAnisotropy = VK_TRUE
	};
	requiredFeatures.sampleRateShading = VK_TRUE;
	auto vecRequiredExtensions = _GetPhysicalDeviceRequiredExtensions();

	physicalDeviceSelector.set_surface(vkSurface);
	physicalDeviceSelector.set_required_features(requiredFeatures);
	physicalDeviceSelector.prefer_gpu_device_type();
	for (auto requiredExtension : vecRequiredExtensions)
	{
		physicalDeviceSelector.add_required_extension(requiredExtension);
	}

	auto physicalDeviceSelectorReturn = physicalDeviceSelector.select();
	if (!physicalDeviceSelectorReturn)
	{
		std::cout << physicalDeviceSelectorReturn.error().message() << std::endl;
		throw std::runtime_error("Failed to find a suitable GPU!");
	}
	m_physicalDevice = physicalDeviceSelectorReturn.value();
	vkPhysicalDevice = m_physicalDevice.physical_device;
}

void MyDevice::_CreateLogicalDevice()
{
	vkb::DeviceBuilder deviceBuilder{ m_physicalDevice };
	std::vector<vkb::CustomQueueDescription> queueDescription;
	float priority = 1.0f;
	queueFamilyIndices = _GetQueueFamilyIndices(vkPhysicalDevice);
	if (!queueFamilyIndices.isComplete())
	{
		throw std::runtime_error("Failed to find all queue families");
	}
	std::unordered_set<uint32_t> uniqueQueueFamilies{ 
		queueFamilyIndices.graphicsFamily.value(), 
		queueFamilyIndices.presentFamily.value(), 
		queueFamilyIndices.graphicsAndComputeFamily.value(), 
		queueFamilyIndices.transferFamily.value() 
	};
	for (auto queueFamily : uniqueQueueFamilies) 
	{
		queueDescription.emplace_back(vkb::CustomQueueDescription{ queueFamily, {priority} });
	}
	deviceBuilder.custom_queue_setup(queueDescription);
	
	auto deviceBuilderReturn = deviceBuilder.build();
	if (!deviceBuilderReturn)
	{
		std::cout << deviceBuilderReturn.error().message() << std::endl;
		throw std::runtime_error("Failed to create logic device!");
	}
	
	m_device = deviceBuilderReturn.value();
	vkDevice = m_device.device;
	m_dispatchTable = m_device.make_table();
}

void MyDevice::_CreateSwapChain()
{
	SwapChainSupportDetails swapChainSupport = _QuerySwapchainSupport(vkPhysicalDevice, vkSurface);
	uint32_t imageCnt = swapChainSupport.capabilities.minImageCount + 1;
	VkSurfaceFormatKHR surfaceFormat = _ChooseSwapchainFormat(swapChainSupport.formats);
	VkPresentModeKHR presentMode = _ChooseSwapchainPresentMode(swapChainSupport.presentModes);
	VkExtent2D extent = _ChooseSwapchainExtent(swapChainSupport.capabilities);
	vkb::SwapchainBuilder swapchainBuilder{ m_device };
	// swapchainBuilder.set_old_swapchain(m_swapchain); // it says that we need to set this, but if i leave it empty it still works, weird
	swapchainBuilder.set_desired_format(surfaceFormat);
	swapchainBuilder.set_desired_present_mode(presentMode);
	swapchainBuilder.set_composite_alpha_flags(VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR);
	swapchainBuilder.set_desired_min_image_count(imageCnt);
	swapchainBuilder.set_desired_extent(extent.width, extent.height);
	// swapchainBuilder.set_image_array_layer_count(1);
	auto swapchainBuilderReturn = swapchainBuilder.build();
	if (!swapchainBuilderReturn)
	{
		throw std::runtime_error(swapchainBuilderReturn.error().message());
	}
	m_swapchain = swapchainBuilderReturn.value();
	vkSwapchain = m_swapchain.swapchain;
}

void MyDevice::Init()
{
	_InitGLFW();
	_CreateInstance();
	_CreateSurface();
	_SelectPhysicalDevice();
	_CreateLogicalDevice();
	_CreateSwapChain();
}

void MyDevice::Uninit()
{
	vkb::destroy_swapchain(m_swapchain);
	vkb::destroy_device(m_device);
	vkb::destroy_surface(m_instance, vkSurface);
	vkb::destroy_instance(m_instance);
	glfwDestroyWindow(pWindow);
	glfwTerminate();
}

MyDevice& MyDevice::GetInstance()
{
	return s_instance;
}