#pragma once
#include "common.h"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

struct Vertex final
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

struct StaticMesh final
{
	std::vector<Vertex> verts;
	std::vector<uint32_t> indices;
};

class Meshlet final
{
public:
	// GPU friendly compact meshlet data
	struct DeviceData
	{
		std::vector<uint32_t> meshletVertices;	// stores the vertex index of the original static mesh
		std::vector<uint8_t> meshletIndices;	// stores indices of meshletVertices that forms triangles, 3 in a group
	};
	// GPU friendly form of meshlet, pointing to data in Meshlet::DeviceData
	// To get ith triangle verts:
	// first find "meshlet index": Meshlet::DeviceData.meshletIndices[indexOffset + i * 3 + 0/1/2];
	// then find vertices in original mesh: Meshlet::DeviceData.meshletVertices["meshlet index" + vertexOffset];
	struct DeviceDataRef
	{
		uint32_t vertexOffset;
		uint32_t vertexCount;
		uint32_t indexOffset;
		uint32_t triangleCount;
	};

public:
	std::vector<uint32_t> vertices;
	std::vector<uint8_t> index;

public:
	// Get number _index triangle of this meshlet
	// _index: the index of the triangle in this meshlet, start from 0
	// _outTriangle: indices of vertices of original mesh that builds the triangle
	void GetTriangle(uint32_t _index, std::array<uint32_t, 3>& _outTriangle) const;

	// Get indices of this meshlet and push them into output, data already stored will be intact
	// _outIndices: indices of this meshlet, we can use this as index buffer to draw meshlet with the original mesh's vertex buffer,
	void GetIndices(std::vector<uint32_t>& _outIndices) const;

	// Compress the input meshlet, and add it to GPU friendly data
	// the original _outData and _outRef remains intact
	static void CompressToDeviceData(
		const Meshlet& _meshlet,
		Meshlet::DeviceData& _outData,
		Meshlet::DeviceDataRef& _outRef);

	// Get meshlet from device data
	static Meshlet GetFromDeviceData(
		const Meshlet::DeviceData& _data,
		const Meshlet::DeviceDataRef& _ref);
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