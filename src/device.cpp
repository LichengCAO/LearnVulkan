#include "device.h"
#include <iostream>
#include <cstdint>
#include <unordered_set>
#include <algorithm>
#include "image.h"
#include "pipeline_io.h"
#include <iomanip>
#define VOLK_IMPLEMENTATION
#include <volk.h>

std::unique_ptr<MyDevice> MyDevice::s_uptrInstance = nullptr;

MyDevice::MyDevice()
{
}

MyDevice::~MyDevice()
{
	if (m_initialized) Uninit();
}

std::vector<const char*> MyDevice::_GetInstanceRequiredExtensions() const
{
	uint32_t glfwExtensionsCount = 0;
	const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionsCount);
	std::vector<const char*> requiredExtensions;

	for (uint32_t i = 0; i < glfwExtensionsCount; ++i) {
		requiredExtensions.emplace_back(glfwExtensions[i]);
	}
	requiredExtensions.emplace_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
	requiredExtensions.emplace_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
	/*if (enableValidationLayer)*/ requiredExtensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	return requiredExtensions;
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

void MyDevice::_InitVolk()
{
	VK_CHECK(volkInitialize(), "Failed to initialize volk!");
}

void MyDevice::_InitGLFW()
{
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
	pWindow = glfwCreateWindow(800, 600, "My Vulkan", nullptr, nullptr);
	glfwSetWindowUserPointer(pWindow, this);
	glfwSetFramebufferSizeCallback(pWindow, OnFramebufferResized);
	glfwSetCursorPosCallback(pWindow, CursorPosCallBack);
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
	instanceBuilder.desire_api_version(VK_API_VERSION_1_2);
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
	volkLoadInstance(vkInstance);
}

void MyDevice::_CreateSurface()
{
	VK_CHECK(glfwCreateWindowSurface(vkInstance, pWindow, nullptr, &vkSurface), "Failed to create window surface!");
}

void MyDevice::_SelectPhysicalDevice()
{
	vkb::PhysicalDeviceSelector physicalDeviceSelector(m_instance);

	physicalDeviceSelector.set_surface(vkSurface);
	_AddBaseExtensionsAndFeatures(physicalDeviceSelector);
	_AddMeshShaderExtensionsAndFeatures(physicalDeviceSelector);
	_AddRayTracingExtensionsAndFeatures(physicalDeviceSelector);

	// select device based on setting
	{
		auto physicalDeviceSelectorReturn = physicalDeviceSelector.select();
		if (!physicalDeviceSelectorReturn)
		{
			std::cout << physicalDeviceSelectorReturn.error().message() << std::endl;
			throw std::runtime_error("Failed to find a suitable GPU!");
		}
		m_physicalDevice = physicalDeviceSelectorReturn.value();
		vkPhysicalDevice = m_physicalDevice.physical_device;
	}
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
	volkLoadDevice(vkDevice);
	vkGetDeviceQueue(vkDevice, queueFamilyIndices.presentFamily.value(), 0, &m_vkPresentQueue);
}

void MyDevice::_CreateSwapchain()
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
	swapchainBuilder.set_image_array_layer_count(1);
	auto swapchainBuilderReturn = swapchainBuilder.build();
	if (!swapchainBuilderReturn)
	{
		throw std::runtime_error(swapchainBuilderReturn.error().message());
	}
	m_swapchain = swapchainBuilderReturn.value();
	vkSwapchain = m_swapchain.swapchain;
	m_needRecreate = false;
	_UpdateSwapchainImages();
}

void MyDevice::RecreateSwapchain()
{
	int width = 0;
	int height = 0;
	glfwGetFramebufferSize(pWindow, &width, &height);
	while (width == 0 || height == 0) 
	{
		glfwGetFramebufferSize(pWindow, &width, &height);
		glfwWaitEvents();
	}
	vkDeviceWaitIdle(vkDevice);
	_DestroySwapchain();
	_CreateSwapchain();
}

std::vector<Image> MyDevice::GetSwapchainImages() const
{
	std::vector<VkImage> swapchainImages;
	std::vector<Image> ret;
	_GetVkSwapchainImages(swapchainImages);
	
	ret.reserve(swapchainImages.size());
	for (const auto& vkImage : swapchainImages)
	{
		Image tmpImage{};
		Image::Information imageInfo;
		imageInfo.width = m_swapchain.extent.width;
		imageInfo.height = m_swapchain.extent.height;
		imageInfo.usage = m_swapchain.image_usage_flags;
		imageInfo.format = m_swapchain.image_format;
		tmpImage.SetImageInformation(imageInfo);
		tmpImage.vkImage = vkImage;
		ret.push_back(tmpImage);
	}

	return ret;
}

void MyDevice::GetSwapchainImagePointers(std::vector<Image*>& _output) const
{
	_output.clear();
	_output.reserve(m_uptrSwapchainImages.size());
	for (auto& uptrImage : m_uptrSwapchainImages)
	{
		_output.push_back(uptrImage.get());
	}
}

std::optional<uint32_t> MyDevice::AquireAvailableSwapchainImageIndex(VkSemaphore finishSignal)
{
	uint32_t imageIndex = 0;
	std::optional<uint32_t> ret;
	VkResult result = vkAcquireNextImageKHR(vkDevice, vkSwapchain, UINT64_MAX, finishSignal, VK_NULL_HANDLE, &imageIndex);
	if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) 
	{
		throw std::runtime_error("Failed to acquire swap chain image!");
	}
	else if (result != VK_ERROR_OUT_OF_DATE_KHR)
	{
		ret = imageIndex;
	}
	else
	{
		m_needRecreate = true;
	}
	return ret;
}

void MyDevice::PresentSwapchainImage(const std::vector<VkSemaphore>& waitSemaphores, uint32_t imageIdx)
{
	VkSwapchainKHR swapChains[] = { vkSwapchain };
	VkPresentInfoKHR presentInfo{ VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
	presentInfo.waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size());
	presentInfo.pWaitSemaphores = waitSemaphores.data();
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;
	presentInfo.pImageIndices = &imageIdx;
	presentInfo.pResults = nullptr;

	VkResult result = vkQueuePresentKHR(m_vkPresentQueue, &presentInfo);
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) 
	{
		m_needRecreate = true;
	}
	else if (result != VK_SUCCESS) 
	{
		throw std::runtime_error("Failed to present swapchain!");
	}
}

bool MyDevice::NeedRecreateSwapchain() const
{
	return m_needRecreate;
}

void MyDevice::_DestroySwapchain()
{
	for (auto& uptrImage : m_uptrSwapchainImages)
	{
		if (uptrImage.get() != nullptr)
		{
			uptrImage->Uninit();
			uptrImage.reset();
		}
	}
	m_uptrSwapchainImages.clear();
	vkb::destroy_swapchain(m_swapchain);
}

void MyDevice::_AddBaseExtensionsAndFeatures(vkb::PhysicalDeviceSelector& _selector) const
{
	VkPhysicalDeviceFeatures requiredFeatures{};
	VkPhysicalDeviceDescriptorIndexingFeaturesEXT physicalDeviceDescriptorIndexingFeatures{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT };
	
	requiredFeatures.geometryShader = VK_TRUE;
	requiredFeatures.samplerAnisotropy = VK_TRUE;
	requiredFeatures.sampleRateShading = VK_TRUE;
	requiredFeatures.fragmentStoresAndAtomics = VK_TRUE;
	requiredFeatures.shaderInt64 = VK_TRUE;
	physicalDeviceDescriptorIndexingFeatures.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
	physicalDeviceDescriptorIndexingFeatures.runtimeDescriptorArray = VK_TRUE;
	physicalDeviceDescriptorIndexingFeatures.descriptorBindingVariableDescriptorCount = VK_TRUE;

	_selector.add_required_extensions(
		{ 
			VK_KHR_SWAPCHAIN_EXTENSION_NAME,
			VK_KHR_MAINTENANCE1_EXTENSION_NAME,
			VK_KHR_MAINTENANCE3_EXTENSION_NAME,
			VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
		});
	_selector.set_required_features(requiredFeatures);
	_selector.add_required_extension_features(physicalDeviceDescriptorIndexingFeatures);
}

void MyDevice::_AddRayTracingExtensionsAndFeatures(vkb::PhysicalDeviceSelector& _selector) const
{
	// Ray tracing
	VkPhysicalDeviceAccelerationStructureFeaturesKHR accelerationStructureFeatures{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR };
	VkPhysicalDeviceRayTracingPipelineFeaturesKHR rayTracingPipelineFeatures{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR };
	VkPhysicalDeviceHostQueryResetFeatures hostQueryResetFeatures{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_HOST_QUERY_RESET_FEATURES };
	VkPhysicalDeviceVulkan12Features vulkan12Featrues{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES };

	accelerationStructureFeatures.accelerationStructure = VK_TRUE;
	rayTracingPipelineFeatures.rayTracingPipeline = VK_TRUE;
	hostQueryResetFeatures.hostQueryReset = VK_TRUE;
	vulkan12Featrues.bufferDeviceAddress = VK_TRUE;
	_selector.add_required_extensions(
		{ 
			VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
			VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
			VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
		});
	_selector.add_required_extension_features(accelerationStructureFeatures);
	_selector.add_required_extension_features(rayTracingPipelineFeatures);
	_selector.add_required_extension_features(hostQueryResetFeatures);
	_selector.add_required_extension_features(vulkan12Featrues);
}

void MyDevice::_AddMeshShaderExtensionsAndFeatures(vkb::PhysicalDeviceSelector& _selector) const
{
	VkPhysicalDeviceMeshShaderFeaturesEXT meshShaderFeatures{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT };
	VkPhysicalDeviceVulkan12Features vulkan12Featrues{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES };

	meshShaderFeatures.taskShader = VK_TRUE;
	meshShaderFeatures.meshShader = VK_TRUE;
	vulkan12Featrues.shaderInt8 = VK_TRUE;
	vulkan12Featrues.storageBuffer8BitAccess = VK_TRUE;
	vulkan12Featrues.descriptorIndexing = VK_TRUE;

	_selector.add_required_extensions(
		{
			VK_EXT_MESH_SHADER_EXTENSION_NAME,
			VK_KHR_SPIRV_1_4_EXTENSION_NAME,
			VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME
		});
	_selector.add_required_extension_features(meshShaderFeatures);
	_selector.add_required_extension_features(vulkan12Featrues);
}

void MyDevice::_GetVkSwapchainImages(std::vector<VkImage>& _vkImages) const
{
	uint32_t uImageCount = 0;
	_vkImages.clear();
	vkGetSwapchainImagesKHR(vkDevice, vkSwapchain, &uImageCount, nullptr);
	_vkImages.resize(static_cast<size_t>(uImageCount));
	vkGetSwapchainImagesKHR(vkDevice, vkSwapchain, &uImageCount, _vkImages.data());
}

void MyDevice::_UpdateSwapchainImages()
{
	std::vector<VkImage> vkImages;
	for (std::unique_ptr<Image>& uptrImage : m_uptrSwapchainImages)
	{
		if (uptrImage.get() != nullptr)
		{
			uptrImage->Uninit();
			uptrImage.reset();
		}
	}
	_GetVkSwapchainImages(vkImages);
	m_uptrSwapchainImages.resize(vkImages.size());
	for (size_t i = 0; i < vkImages.size(); ++i)
	{
		std::unique_ptr<Image>& uptrImage = m_uptrSwapchainImages[i];
		Image::Information imageInfo{};
		imageInfo.width = m_swapchain.extent.width;
		imageInfo.height = m_swapchain.extent.height;
		imageInfo.usage = m_swapchain.image_usage_flags;
		imageInfo.format = m_swapchain.image_format;
		imageInfo.inSwapchain = true;
		uptrImage = std::make_unique<Image>();
		uptrImage->SetImageInformation(imageInfo);
		uptrImage->vkImage = vkImages[i];
		uptrImage->Init();
	}
}

VkExtent2D MyDevice::GetSwapchainExtent() const
{
	return m_swapchain.extent;
}

VkFormat MyDevice::GetDepthFormat() const
{
	return FindSupportFormat(
		{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
		VK_IMAGE_TILING_OPTIMAL,
		VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
	);
}

VkFormat MyDevice::GetSwapchainFormat() const
{
	return m_swapchain.image_format;
}

VkFormat MyDevice::FindSupportFormat(const std::vector<VkFormat>& candidates, VkImageTiling tilling, VkFormatFeatureFlags features) const
{
	for (auto format : candidates)
	{
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(vkPhysicalDevice, format, &props);
		if (tilling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
		{
			return format;
		}
		else if (tilling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
		{
			return format;
		}
	}
	throw std::runtime_error("Failed to find supported format!");
}

void MyDevice::_CreateCommandPools()
{
	std::set<uint32_t> uniqueFamilyIndex;
	std::vector<std::optional<uint32_t>> familyIndices =
	{ 
		queueFamilyIndices.graphicsAndComputeFamily,
		queueFamilyIndices.graphicsFamily,
		queueFamilyIndices.presentFamily,
		queueFamilyIndices.transferFamily,
	};
	for (const auto& opt : familyIndices)
	{
		if (opt.has_value())
		{
			uint32_t key = opt.value();
			uniqueFamilyIndex.insert(key);
		}
	}
	for (auto key : uniqueFamilyIndex)
	{
		VkCommandPool cmdPool;
		VkCommandPoolCreateInfo commandPoolInfo{ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
		commandPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		commandPoolInfo.queueFamilyIndex = key;
		VK_CHECK(vkCreateCommandPool(vkDevice, &commandPoolInfo, nullptr, &cmdPool), "Failed to create command pool!");
		vkCommandPools[key] = cmdPool;
	}
}

void MyDevice::_InitDescriptorAllocator()
{
	descriptorAllocator.Init();
}

void MyDevice::_DestroyCommandPools()
{
	for (const auto& p : vkCommandPools)
	{
		vkDestroyCommandPool(vkDevice, p.second, nullptr);
	}
}

void MyDevice::Init()
{
	_InitVolk();
	_InitGLFW();
	_CreateInstance();
	_CreateSurface();
	_SelectPhysicalDevice();
	_CreateLogicalDevice();
	_CreateSwapchain();
	_InitDescriptorAllocator();
	_CreateCommandPools();
	m_initialized = true;
}

void MyDevice::Uninit()
{
	_DestroyCommandPools();
	descriptorAllocator.Uninit();
	_DestroySwapchain();
	vkb::destroy_device(m_device);
	vkb::destroy_surface(m_instance, vkSurface);
	vkb::destroy_instance(m_instance);
	glfwDestroyWindow(pWindow);
	glfwTerminate();
	m_initialized = false;
}

UserInput MyDevice::GetUserInput() const
{
	UserInput ret = m_userInput;
	ret.W = (glfwGetKey(pWindow, GLFW_KEY_W) == GLFW_PRESS);
	ret.S = (glfwGetKey(pWindow, GLFW_KEY_S) == GLFW_PRESS);
	ret.A = (glfwGetKey(pWindow, GLFW_KEY_A) == GLFW_PRESS);
	ret.D = (glfwGetKey(pWindow, GLFW_KEY_D) == GLFW_PRESS);
	ret.Q = (glfwGetKey(pWindow, GLFW_KEY_Q) == GLFW_PRESS);
	ret.E = (glfwGetKey(pWindow, GLFW_KEY_E) == GLFW_PRESS);
	ret.LMB = (glfwGetMouseButton(pWindow, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS);
	ret.RMB = (glfwGetMouseButton(pWindow, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS);
	return ret;
}

MyDevice& MyDevice::GetInstance()
{
	if (s_uptrInstance.get() == nullptr)
	{
		s_uptrInstance = std::make_unique<MyDevice>();
	}
	return *s_uptrInstance;
}

void MyDevice::OnFramebufferResized(GLFWwindow* _pWindow, int width, int height)
{
	auto mydevice = reinterpret_cast<MyDevice*>(glfwGetWindowUserPointer(_pWindow));
	mydevice->m_needRecreate = true;
}

void MyDevice::CursorPosCallBack(GLFWwindow* _pWindow, double _xPos, double _yPos)
{
	// only called when the pos changes
	auto mydevice = reinterpret_cast<MyDevice*>(glfwGetWindowUserPointer(_pWindow));
	auto& userInput = mydevice->m_userInput;
	userInput.xPos = _xPos;
	userInput.yPos = _yPos;
}
