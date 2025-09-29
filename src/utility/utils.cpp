#include "utils.h"
#include <meshoptimizer.h>
#include <tiny_obj_loader.h>
#include <tiny_gltf.h>
#include <numeric>
#include "transform.h"
#include <fstream>

bool MeshUtility::Load(const std::string& objFile, std::vector<Mesh>& outMesh)
{
	bool bSuccess = false;
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warn, err;
	CHECK_TRUE(tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, objFile.c_str()), warn + err);
	
	for (const auto& shape : shapes)
	{
		Mesh mesh{};
		std::unordered_map<Vertex, uint32_t> uniqueVertices{};

		for (const auto& index : shape.mesh.indices)
		{
			Vertex vertex{};
			vertex.position = {
				attrib.vertices[3 * index.vertex_index + 0],
				attrib.vertices[3 * index.vertex_index + 1],
				attrib.vertices[3 * index.vertex_index + 2]
			};
			if (index.texcoord_index != -1)
			{
				vertex.uv = {
					attrib.texcoords[2 * index.texcoord_index + 0],
					1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
				};
			}
			if (index.normal_index != -1)
			{
				vertex.normal = {
					attrib.normals[3 * index.normal_index + 0],
					attrib.normals[3 * index.normal_index + 1],
					attrib.normals[3 * index.normal_index + 2],
				};
			}
			mesh.indices.push_back(static_cast<uint32_t>(mesh.verts.size()));
			mesh.verts.push_back(vertex);
		}

		// apply optimizer
		_OptimizeMesh(mesh);
		outMesh.push_back(mesh);
	}
	bSuccess = true;
	return bSuccess;
}

void MeshUtility::BuildMeshlets(
	const Mesh& inMesh,
	std::vector<Meshlet>& outMeshlets,
	std::vector<uint32_t>& outMeshletVertices,
	std::vector<uint8_t>& outMeshletTriangles
)
{
	const std::vector<uint32_t>& indices = inMesh.indices;
	const std::vector<Vertex>& vertices = inMesh.verts;
	const float coneWeight = 0.0f;
	const size_t maxIndices = 64;
	const size_t maxTriangles = 124; // must be divisible by 4
	std::vector<meshopt_Meshlet> meshoptMeshlets;
	size_t maxMeshlets = meshopt_buildMeshletsBound(indices.size(), maxIndices, maxTriangles);
	size_t meshletCount = 0;
	
	meshoptMeshlets.resize(maxMeshlets, meshopt_Meshlet{});
	outMeshletVertices.resize(maxMeshlets * maxIndices, 0);
	outMeshletTriangles.resize(maxMeshlets * maxTriangles * 3, 0);

	meshletCount = meshopt_buildMeshlets(
		meshoptMeshlets.data(),
		static_cast<unsigned int*>(outMeshletVertices.data()),
		static_cast<unsigned char*>(outMeshletTriangles.data()),
		indices.data(),
		indices.size(),
		reinterpret_cast<const float*>(vertices.data()),
		vertices.size(),
		sizeof(Vertex),
		maxIndices,
		maxTriangles,
		coneWeight
	);
	meshoptMeshlets.resize(meshletCount);
	outMeshletVertices.resize(meshoptMeshlets.back().vertex_offset + meshoptMeshlets.back().vertex_count);
	// This expression is performing an operation that ensures the result is rounded up to the next multiple of 4.
	// & ~3 Performing a bitwise AND (&) with this value effectively clears the last two bits of the number, aligning it down to a multiple of 4.
	outMeshletTriangles.resize(meshoptMeshlets.back().triangle_offset + ((meshoptMeshlets.back().triangle_count * 3 + 3) & ~3));

	// further optimize meshlets
	for (const auto& m : meshoptMeshlets)
	{
		Meshlet outMeshlet{};

		outMeshlet.triangleCount = m.triangle_count;
		outMeshlet.triangleOffset = m.triangle_offset;
		outMeshlet.vertexCount = m.vertex_count;
		outMeshlet.vertexOffset = m.vertex_offset;
		meshopt_optimizeMeshlet(
			static_cast<unsigned int*>(&outMeshletVertices[m.vertex_offset]), 
			static_cast<unsigned char*>(&outMeshletTriangles[m.triangle_offset]),
			m.triangle_count,
			m.vertex_count
		);
		outMeshlets.push_back(outMeshlet);
	}
}

void MeshUtility::BuildMeshlets(
	const Mesh& inMesh,
	std::vector<Meshlet>& outMeshlets,
	std::vector<MeshletBounds>& outMeshletBounds,
	std::vector<uint32_t>& outMeshletVertices,
	std::vector<uint8_t>& outMeshletTriangles
)
{
	size_t offset = outMeshlets.size();
	
	BuildMeshlets(inMesh, outMeshlets, outMeshletVertices, outMeshletTriangles);
	outMeshletBounds.reserve(outMeshletBounds.size() + outMeshlets.size() - offset);
	for (int i = offset; i < outMeshlets.size(); ++i)
	{
		outMeshletBounds.push_back(_ComputeMeshletBounds(inMesh, outMeshlets[i], outMeshletVertices, outMeshletTriangles));
	}
}

void MeshUtility::_OptimizeMesh(Mesh& mesh)
{
	std::vector<uint32_t> remap(std::max(mesh.indices.size(), mesh.verts.size()));
	std::vector<uint32_t> dstIndices(mesh.indices.size());
	std::vector<Vertex> dstVerts(mesh.verts.size());
	size_t indexCount = mesh.indices.size();
	size_t vertexCount = meshopt_generateVertexRemapCustom(
		remap.data(),
		mesh.indices.data(),
		mesh.indices.size(),
		reinterpret_cast<const float*>(mesh.verts.data()),
		mesh.verts.size(),
		sizeof(Vertex),
		[&](unsigned int l, unsigned int r) {
			const Vertex& lv = mesh.verts[l];
			const Vertex& rv = mesh.verts[r];
			return lv == rv;
		}
	);
	dstIndices.resize(indexCount);
	dstVerts.resize(vertexCount);
	meshopt_remapVertexBuffer(dstVerts.data(), mesh.verts.data(), mesh.verts.size(), sizeof(Vertex), remap.data());
	meshopt_remapIndexBuffer(dstIndices.data(), mesh.indices.data(), indexCount, remap.data());
	meshopt_optimizeVertexCache(mesh.indices.data(), dstIndices.data(), indexCount, vertexCount);
	meshopt_optimizeOverdraw(
		dstIndices.data(),
		mesh.indices.data(),
		indexCount,
		reinterpret_cast<const float*>(dstVerts.data()),
		vertexCount,
		sizeof(Vertex),
		1.0f
	);
	vertexCount = meshopt_optimizeVertexFetch(
		mesh.verts.data(),
		dstIndices.data(),
		indexCount,
		dstVerts.data(),
		vertexCount,
		sizeof(Vertex)
	);
	mesh.verts.resize(vertexCount);
	mesh.indices = std::move(dstIndices);
}

void MeshUtility::_OptimizeMeshToVertexCacheStage(Mesh& mesh)
{
	std::vector<uint32_t> remap(std::max(mesh.indices.size(), mesh.verts.size()));
	std::vector<uint32_t> dstIndices(mesh.indices.size());
	std::vector<Vertex> dstVerts(mesh.verts.size());
	size_t indexCount = mesh.indices.size();
	size_t vertexCount = meshopt_generateVertexRemapCustom(
		remap.data(),
		mesh.indices.data(),
		mesh.indices.size(),
		reinterpret_cast<const float*>(mesh.verts.data()),
		mesh.verts.size(),
		sizeof(Vertex),
		[&](unsigned int l, unsigned int r) {
			const Vertex& lv = mesh.verts[l];
			const Vertex& rv = mesh.verts[r];
			return lv == rv;
		}
	);
	dstIndices.resize(indexCount);
	dstVerts.resize(vertexCount);
	meshopt_remapVertexBuffer(dstVerts.data(), mesh.verts.data(), mesh.verts.size(), sizeof(Vertex), remap.data());
	meshopt_remapIndexBuffer(dstIndices.data(), mesh.indices.data(), indexCount, remap.data());
	meshopt_optimizeVertexCache(mesh.indices.data(), dstIndices.data(), indexCount, vertexCount);
	mesh.verts = std::move(dstVerts);
}

MeshletBounds MeshUtility::_ComputeMeshletBounds(
	const Mesh& inMesh, 
	const Meshlet& inMeshlet, 
	const std::vector<uint32_t>& inMeshletVertices, 
	const std::vector<uint8_t>& inMeshletTriangles)
{
	MeshletBounds meshletBounds{};
	meshopt_Bounds bounds{};
	bounds = meshopt_computeMeshletBounds(
		&inMeshletVertices[inMeshlet.vertexOffset],
		&inMeshletTriangles[inMeshlet.triangleOffset],
		inMeshlet.triangleCount,
		reinterpret_cast<const float*>(inMesh.verts.data()),
		inMesh.verts.size(),
		sizeof(Vertex)
	);
	meshletBounds.center = glm::vec3(bounds.center[0], bounds.center[1], bounds.center[2]);
	meshletBounds.radius = bounds.radius;
	meshletBounds.coneApex = glm::vec3(bounds.cone_apex[0], bounds.cone_apex[1], bounds.cone_apex[2]);
	meshletBounds.coneAxis = glm::vec3(bounds.cone_axis[0], bounds.cone_axis[1], bounds.cone_axis[2]);
	meshletBounds.coneCutoff = bounds.cone_cutoff;
	return meshletBounds;
}

void CommonUtils::ReadFile(const std::string& _filePath, std::vector<uint8_t>& _output)
{
	std::ifstream file(_filePath, std::ios::ate | std::ios::binary);

	CHECK_TRUE(file.is_open(), "Failed to open file!");

	size_t fileSize = (size_t)file.tellg();
	_output.resize(fileSize);
	file.seekg(0);
	file.read(reinterpret_cast<char*>(_output.data()), fileSize);
	file.close();
}
