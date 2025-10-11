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

struct StaticMesh
{
	std::vector<Vertex> verts;
	std::vector<uint32_t> indices;
};

struct Meshlet
{
	uint32_t vertexOffset;
	uint32_t vertexCount;

	uint32_t triangleOffset;
	uint32_t triangleCount;
};

struct MeshletBounds
{
	// frustum culling
	glm::vec3 center;
	float radius;
	// back-face culling
	// e.g. if (dot(normalize(coneApex - cameraPosition), coneAxis) >= coneCutoff) reject(); code from zeux/meshoptimizer
	glm::vec3 coneApex;
	glm::vec3 coneAxis;
	float coneCutoff;
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