#pragma once
#include "geometry.h"
#include "transform.h"
#include <tiny_obj_loader.h>

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

class MeshUtility
{
private:
	static void _OptimizeMesh(Mesh& mesh);
	static void _OptimizeMeshToVertexCacheStage(Mesh& mesh);
public:
	static bool Load(const std::string& objFile, std::vector<Mesh>& outMesh);
	/*
	* outMeshletVertices remap the vertices of the whole mesh to multiple subsets of vertices letting mesh shader to process locally,
	* outMeshletTriangles serves as something like indices buffers for these subsets of vertices.
	*/
	static void BuildMeshlets(const Mesh& inMesh, std::vector<Meshlet>& outMeshlets, std::vector<uint32_t>& outMeshletVertices, std::vector<uint8_t>& outMeshletTriangles);
};
