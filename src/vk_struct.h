#pragma once
#include <optional>
#define GLFW_INCLUDE_VULKAN //->auto load <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <vector>

struct QueueFamilyIndices {
	std::optional<uint32_t> graphicsFamily;
	std::optional<uint32_t> presentFamily;
	bool isComplete() {
		return graphicsFamily.has_value() 
			&& presentFamily.has_value();
	}
};

struct SwapChainSupportDetails {
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};
