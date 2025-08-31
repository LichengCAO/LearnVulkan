#pragma once
#include "common.h"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

struct Vertex
{
	// make sure position is the first attribute, it is used by mesh optimizer
	alignas(16) glm::vec3 position{};
	alignas(16) std::optional<glm::vec3> normal{};
	alignas(16) std::optional<glm::vec2> uv{};
	bool operator==(const Vertex& other) const 
	{
		return position == other.position && uv == other.uv && normal == other.normal; 
	}
};

namespace std
{
	template<> struct hash<Vertex>
	{
		size_t operator()(Vertex const& vertex) const
		{
			size_t hashValue = 0;
			hashValue = hash<glm::vec3>()(vertex.position);
			if (vertex.normal.has_value())
			{
				hashValue ^= (hash<glm::vec3>()(vertex.normal.value()) << 1);
				hashValue >>= 1;
			}
			if (vertex.uv.has_value())
			{
				hashValue ^= (hash<glm::vec2>()(vertex.uv.value()) << 1);
			}
			return hashValue;
		}
	};
}