#include "drawable.h"

VkVertexInputBindingDescription Vertex::getVertexInputBindingDescription()
{
	VkVertexInputBindingDescription res = {
		.binding = 0,
		.stride = sizeof(Vertex),
		.inputRate = VK_VERTEX_INPUT_RATE_VERTEX
	};
	return res;
}

std::array<VkVertexInputAttributeDescription, 3> Vertex::getVertexInputAttributeDescription()
{
	std::array<VkVertexInputAttributeDescription, 3> res{};
	res[0] = {
		.location = 0,
		.binding = 0,
		.format = VK_FORMAT_R32G32B32_SFLOAT,
		.offset = offsetof(Vertex, pos),
	};

	res[1] = {
		.location = 1,
		.binding = 0,
		.format = VK_FORMAT_R32G32B32_SFLOAT,
		.offset = offsetof(Vertex, color),
	};

	res[2] = {
		.location = 2,
		.binding = 0,
		.format = VK_FORMAT_R32G32_SFLOAT,
		.offset = offsetof(Vertex,texcoord),
	};

	return res;
}

