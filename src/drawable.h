#pragma once
#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN //->auto load <vulkan/vulkan.h>
#endif
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <array>
struct Vertex {
	glm::vec3 pos;
	alignas(16) glm::vec3 color;
	alignas(8) glm::vec2 texcoord;

	static VkVertexInputBindingDescription getVertexInputBindingDescription();
	static std::array<VkVertexInputAttributeDescription,3> getVertexInputAttributeDescription();
};

//alignas(4) float
//alignas(8) vec2
//alignas(16) vec3 vec4 
//alignas(16) struct  
//alignas(16) mat4
struct UniformBufferObject {
	alignas(16) glm::mat4 model;
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 proj;
};

