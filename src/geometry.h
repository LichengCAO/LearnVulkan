#pragma once
#include "commandbuffer.h"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

struct Vertex
{
	alignas(16) glm::vec3 pos{};
	alignas(16) glm::vec3 normal{};
	alignas(8)  glm::vec2 uv{};
	bool operator==(const Vertex& other) const { return pos == other.pos && uv == other.uv && normal == other.normal; }
};
namespace std
{
	template<> struct hash<Vertex>
	{
		size_t operator()(Vertex const& vertex) const
		{
			return hash<glm::vec3>()(vertex.pos) ^
				(hash<glm::vec2>()(vertex.uv) << 1)
				;
		}
	};
}