#pragma once
#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN //->auto load <vulkan/vulkan.h>
#endif
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <array>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>
struct Vertex {
	glm::vec3 pos;
	alignas(16) glm::vec3 color = {0.0f, 0.0f, 0.0f};
	alignas(8) glm::vec2 texcoord;
	bool operator==(const Vertex& other) const {
		return pos == other.pos && color == other.color && texcoord == other.texcoord;
	}
	static VkVertexInputBindingDescription getVertexInputBindingDescription();
	static std::array<VkVertexInputAttributeDescription,3> getVertexInputAttributeDescription();
};

struct Particle {
	glm::vec2 pos;
	glm::vec2 velocity;
	glm::vec4 color;

	static std::vector<VkVertexInputBindingDescription> getVertexInputBindingDescription();
	static std::vector<VkVertexInputAttributeDescription> getVertexInputAttributeDescription();
};

namespace std {
	template<> struct hash<Vertex> {
		size_t operator()(Vertex const& vertex) const {
			return ((hash<glm::vec3>()(vertex.pos) ^
				(hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^
				(hash<glm::vec2>()(vertex.texcoord) << 1);
		}
	};
}

//alignas(4) float
//alignas(8) vec2
//alignas(16) vec3 vec4 
//alignas(16) struct  
//alignas(16) mat4
struct UniformBufferObject {
	float deltaTime = 0.0f;
	//alignas(16) glm::mat4 model;
	//alignas(16) glm::mat4 view;
	//alignas(16) glm::mat4 proj;
};

