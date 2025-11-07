#pragma once
#include "geometry.h"
#include "transform.h"
#include <variant>

class MeshUtility
{
private:
	static void _OptimizeMesh(StaticMesh& mesh);
	static void _OptimizeMeshToVertexCacheStage(StaticMesh& mesh);
	static MeshletBounds _ComputeMeshletBounds(
		const StaticMesh& inMesh,
		const Meshlet& inMeshlet,
		const std::vector<uint32_t>& inMeshletVertices,
		const std::vector<uint8_t>& inMeshletTriangles);

public:
	static bool Load(const std::string& objFile, std::vector<StaticMesh>& outMesh);
	/*
	* outMeshletVertices remap the vertices of the whole mesh to multiple subsets of vertices letting mesh shader to process locally,
	* outMeshletTriangles serves as something like indices buffers for these subsets of vertices.
	*/
	static void BuildMeshlets(
		const StaticMesh& inMesh,
		std::vector<Meshlet>& outMeshlets,
		std::vector<uint32_t>& outMeshletVertices,
		std::vector<uint8_t>& outMeshletTriangles);

	static void BuildMeshlets(
		StaticMesh& inMesh,
		std::vector<Meshlet>& outMeshlets,
		std::vector<MeshletBounds>& outMeshletBounds,
		std::vector<uint32_t>& outMeshletVertices,
		std::vector<uint8_t>& outMeshletTriangles);
};

namespace common_utils
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

	// Read file from file path
	void ReadFile(const std::string& _filePath, std::vector<uint8_t>& _output);

	// Get extension name from file path
	std::string GetFileExtension(const std::string& _filePath);

	// Used for unordered_... when std::pair as key
	// e.g. unordered_map<pair<T1, T2>, int, common_utils::PairHash>
	struct PairHash
	{
		template<typename T1, typename T2>
		std::size_t operator() (const std::pair<T1, T2>& p) const 
		{
			auto h1 = std::hash<T1>{}(p.first);
			auto h2 = std::hash<T2>{}(p.second);
			return h1 ^ h2;
		}
	};
}