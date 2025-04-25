#include "utils.h"
#include <meshoptimizer.h>
bool MeshLoader::Load(const std::string& objFile, std::vector<Mesh>& outMesh)
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

void MeshLoader::_OptimizeMesh(Mesh& mesh)
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
