#pragma once
#include "geometry.h"
#include "transform.h"
#include <variant>
struct Mesh
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

class MeshUtility
{
private:
	static void _OptimizeMesh(Mesh& mesh);
	static void _OptimizeMeshToVertexCacheStage(Mesh& mesh);
	static MeshletBounds _ComputeMeshletBounds(
		const Mesh& inMesh,
		const Meshlet& inMeshlet,
		const std::vector<uint32_t>& inMeshletVertices,
		const std::vector<uint8_t>& inMeshletTriangles);

public:
	static bool Load(const std::string& objFile, std::vector<Mesh>& outMesh);
	/*
	* outMeshletVertices remap the vertices of the whole mesh to multiple subsets of vertices letting mesh shader to process locally,
	* outMeshletTriangles serves as something like indices buffers for these subsets of vertices.
	*/
	static void BuildMeshlets(
		const Mesh& inMesh,
		std::vector<Meshlet>& outMeshlets,
		std::vector<uint32_t>& outMeshletVertices,
		std::vector<uint8_t>& outMeshletTriangles);

	static void BuildMeshlets(
		const Mesh& inMesh,
		std::vector<Meshlet>& outMeshlets,
		std::vector<MeshletBounds>& outMeshletBounds,
		std::vector<uint32_t>& outMeshletVertices,
		std::vector<uint8_t>& outMeshletTriangles);
};

namespace CommonUtils
{
	// code from Nvidia vk_raytracing_tutorial
	template <class intType>
	constexpr intType AlignUp(intType x, size_t a)
	{
		return intType((x + intType(a) - 1) & ~intType(a - 1));
	}

	// code from Nvidia vk_raytracing_tutorial: VkTransformMatrixKHR toTransformMatrixKHR(glm::mat4 matrix)
	// Convert a Mat4x4 to the matrix required by acceleration structures
	inline VkTransformMatrixKHR ToTransformMatrixKHR(glm::mat4 matrix)
	{
		// VkTransformMatrixKHR uses a row-major memory layout, while glm::mat4
		// uses a column-major memory layout. We transpose the matrix so we can
		// memcpy the matrix's data directly.
		glm::mat4            temp = glm::transpose(matrix);
		VkTransformMatrixKHR out_matrix;
		memcpy(&out_matrix, &temp, sizeof(VkTransformMatrixKHR));
		return out_matrix;
	}

	void ReadFile(const std::string& _filePath, std::vector<uint8_t>& _output);
}