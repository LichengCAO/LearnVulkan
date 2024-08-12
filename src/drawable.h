#pragma once
#define GLFW_INCLUDE_VULKAN //->auto load <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <array>
struct Vertex {
	glm::vec2 pos;
	glm::vec3 color;

	static VkVertexInputBindingDescription getVertexInputBindingDescription();
	static std::array<VkVertexInputAttributeDescription,2> getVertexInputAttributeDescription();
};
