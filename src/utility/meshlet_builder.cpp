#include "meshlet_builder.h"

void MeshletBuilder::BuildMeshlet(
	const std::vector<Vertex>& _vertex, 
	const std::vector<uint32_t>& _index, 
	std::vector<uint32_t>& _outMeshletVertex,
	std::vector<uint8_t>& _outMeshletLocalIndex, 
	std::vector<Meshlet>& _outMeshlet) const
{
	std::vector<Meshlet> outMeshlets{};
	std::vector<uint32_t> outMeshletVertices{};
	std::vector<uint8_t> outMeshletTriangles{};
	std::vector<meshopt_Meshlet> meshoptMeshlets{};
	const float coneWeight = 0.0f;
	const size_t maxIndices = 64;
	const size_t maxTriangles = 124; // must be divisible by 4
	size_t maxMeshlets = meshopt_buildMeshletsBound(_vertex.size(), maxIndices, maxTriangles);
	size_t meshletCount = 0;

	meshoptMeshlets.resize(maxMeshlets, meshopt_Meshlet{});
	outMeshletVertices.resize(maxMeshlets * maxIndices, 0);
	outMeshletTriangles.resize(maxMeshlets * maxTriangles * 3, 0);

	meshletCount = meshopt_buildMeshlets(
		meshoptMeshlets.data(),
		static_cast<unsigned int*>(outMeshletVertices.data()),
		static_cast<unsigned char*>(outMeshletTriangles.data()),
		_index.data(),
		_index.size(),
		reinterpret_cast<const float*>(_vertex.data()),
		_vertex.size(),
		sizeof(Vertex),
		maxIndices,
		maxTriangles,
		coneWeight);

	meshoptMeshlets.resize(meshletCount);
	outMeshletVertices.resize(meshoptMeshlets.back().vertex_offset + meshoptMeshlets.back().vertex_count);
	// This expression is performing an operation that ensures the result is rounded up to the next multiple of 4.
	//outMeshletTriangles.resize(meshoptMeshlets.back().triangle_offset + ((meshoptMeshlets.back().triangle_count * 3 + 3) & ~3));
	outMeshletTriangles.resize(meshoptMeshlets.back().triangle_offset + meshoptMeshlets.back().triangle_count * 3);

	// further optimize meshlets
	for (const auto& m : meshoptMeshlets)
	{
		Meshlet outMeshlet{};

		outMeshlet.triangleCount = m.triangle_count;
		outMeshlet.triangleOffset = m.triangle_offset + _outMeshletLocalIndex.size();
		outMeshlet.vertexCount = m.vertex_count;
		outMeshlet.vertexOffset = m.vertex_offset + _outMeshletVertex.size();
		meshopt_optimizeMeshlet(
			static_cast<unsigned int*>(&outMeshletVertices[m.vertex_offset]),
			static_cast<unsigned char*>(&outMeshletTriangles[m.triangle_offset]),
			m.triangle_count,
			m.vertex_count);
		outMeshlets.push_back(outMeshlet);
	}

	_outMeshletVertex.insert(_outMeshletVertex.end(), outMeshletVertices.begin(), outMeshletVertices.end());
	_outMeshletLocalIndex.insert(_outMeshletLocalIndex.end(), outMeshletTriangles.begin(), outMeshletTriangles.end());
	_outMeshlet.insert(_outMeshlet.end(), outMeshlets.begin(), outMeshlets.end());
}
