#include "my_mesh_optimizer.h"
#include "utils.h"

void MeshOptimizer::BuildMeshlets(
	const std::vector<Vertex>& _vertex, 
	const std::vector<uint32_t>& _index, 
	MeshletData& _outMeshletData, 
	std::vector<Meshlet>& _outMeshlet) const
{
	std::vector<Meshlet> outMeshlets{};
	std::vector<uint32_t> outMeshletVertices{};
	std::vector<uint8_t> outMeshletTriangles{};
	std::vector<meshopt_Meshlet> meshoptMeshlets{};
	const float coneWeight = 0.0f;
	const size_t maxIndices = 64;
	const size_t maxTriangles = 124; // must be divisible by 4
	size_t maxMeshlets = meshopt_buildMeshletsBound(_index.size(), maxIndices, maxTriangles);
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
		outMeshlet.triangleOffset = m.triangle_offset + _outMeshletData.meshletIndices.size();
		outMeshlet.vertexCount = m.vertex_count;
		outMeshlet.vertexOffset = m.vertex_offset + _outMeshletData.meshletVertices.size();
		meshopt_optimizeMeshlet(
			static_cast<unsigned int*>(&outMeshletVertices[m.vertex_offset]),
			static_cast<unsigned char*>(&outMeshletTriangles[m.triangle_offset]),
			m.triangle_count,
			m.vertex_count);
		outMeshlets.push_back(outMeshlet);
	}

	_outMeshletData.meshletVertices.insert(_outMeshletData.meshletVertices.end(), outMeshletVertices.begin(), outMeshletVertices.end());
	_outMeshletData.meshletIndices.insert(_outMeshletData.meshletIndices.end(), outMeshletTriangles.begin(), outMeshletTriangles.end());
	_outMeshlet.insert(_outMeshlet.end(), outMeshlets.begin(), outMeshlets.end());
}

float MeshOptimizer::SimplifyMesh(
	const std::vector<Vertex>& _vertex, 
	const std::vector<uint32_t>& _index, 
	size_t _targetIndexCount, 
	std::vector<uint32_t>& _outIndex) const
{
	float error = 0.0f;
	size_t newIndexSize = 0;
	std::vector<uint32_t> dstIndex;
	std::vector<uint32_t> srcIndex;
	
	srcIndex.resize(_index.size());
	dstIndex.resize(_index.size());
	meshopt_generateShadowIndexBuffer(
		srcIndex.data(),
		_index.data(),
		_index.size(),
		_vertex.data(),
		_vertex.size(),
		sizeof(glm::vec3),
		sizeof(Vertex));

	newIndexSize = meshopt_simplify(
		dstIndex.data(),
		srcIndex.data(),
		srcIndex.size(),
		reinterpret_cast<const float*>(_vertex.data()),
		_vertex.size(),
		sizeof(Vertex),
		_targetIndexCount,
		FLT_MAX,
		meshopt_SimplifyLockBorder | meshopt_SimplifyPermissive,
		&error);
	dstIndex.resize(newIndexSize);
	
	// if normal simplification failed
	if (newIndexSize == srcIndex.size())
	{
		newIndexSize = meshopt_simplify(
			srcIndex.data(),
			dstIndex.data(),
			dstIndex.size(),
			reinterpret_cast<const float*>(_vertex.data()),
			_vertex.size(),
			sizeof(Vertex),
			_targetIndexCount,
			FLT_MAX,
			meshopt_SimplifyLockBorder | meshopt_SimplifyPermissive | meshopt_SimplifyPrune,
			&error);
		if (newIndexSize > 0)
		{
			srcIndex.resize(newIndexSize);
			std::swap(srcIndex, dstIndex);
		}
	}

	_outIndex.insert(_outIndex.end(), dstIndex.begin(), dstIndex.end());
	
	return error;
}

float MeshOptimizer::SimplifyMesh(
	const std::vector<Vertex>& _vertex, 
	const std::vector<uint32_t>& _index, 
	size_t _targetIndexCount, 
	std::vector<Vertex>& _outVertex, 
	std::vector<uint32_t>& _outIndex) const
{
	struct FlatVertex
	{
		std::array<float, 8> data; // position, uv, normal
	};
	static const float weights[5] = { 1, 1, 1, 1, 1 };

	float error = 0;
	std::vector<FlatVertex> srcVerts;
	std::vector<FlatVertex> dstVerts;
	std::vector<uint32_t> dstIndex;
	bool hasUV = _vertex[0].uv.has_value();
	bool hasNormal = _vertex[0].normal.has_value();
	size_t indexCount = 0;
	
	// fill _vertex into flat form
	srcVerts.resize(_vertex.size());
	for (size_t i = 0; i < _vertex.size(); ++i)
	{
		FlatVertex& mvert = srcVerts[i];
		mvert.data[0] = _vertex[i].position.x;
		mvert.data[1] = _vertex[i].position.y;
		mvert.data[2] = _vertex[i].position.z;
		if (hasUV)
		{
			mvert.data[3] = _vertex[i].uv.value().x;
			mvert.data[4] = _vertex[i].uv.value().y;
		}
		if (hasNormal)
		{
			mvert.data[5] = _vertex[i].normal.value().x;
			mvert.data[6] = _vertex[i].normal.value().y;
			mvert.data[7] = _vertex[i].normal.value().z;
		}
	}

	// simplify
	dstVerts = srcVerts;
	dstIndex = _index;
	indexCount = meshopt_simplifyWithUpdate(
		dstIndex.data(),
		dstIndex.size(),
		reinterpret_cast<float*>(dstVerts.data()),
		dstVerts.size(),
		sizeof(FlatVertex),
		&dstVerts[0].data[3],
		sizeof(FlatVertex),
		weights,
		5,
		NULL,
		_targetIndexCount,
		FLT_MAX,
		meshopt_SimplifyLockBorder | meshopt_SimplifyPermissive,
		&error);

	// try simplify more aggressively
	if (indexCount > _targetIndexCount)
	{
		dstVerts = srcVerts;
		dstIndex = _index;
		indexCount = meshopt_simplifyWithUpdate(
			dstIndex.data(),
			dstIndex.size(),
			reinterpret_cast<float*>(dstVerts.data()),
			dstVerts.size(),
			sizeof(FlatVertex),
			&dstVerts[0].data[3],
			sizeof(FlatVertex),
			weights,
			5,
			NULL,
			_targetIndexCount,
			FLT_MAX,
			meshopt_SimplifyLockBorder | meshopt_SimplifyPermissive | meshopt_SimplifyPrune,
			&error);
	}
	dstIndex.resize(indexCount);

	std::swap(dstVerts, srcVerts);
	//meshopt_optimizeVertexFetch(
	//	dstVerts.data(),
	//	dstIndex.data(),
	//	dstIndex.size(),
	//	srcVerts.data(),
	//)

	_outVertex.resize(_vertex.size());
	_outIndex.resize(_index.size());

    return error;
}

void MeshOptimizer::OptimizeMesh(std::vector<Vertex>& _vertex, std::vector<uint32_t>& _index) const
{
	std::vector<uint32_t> remap(std::max(_vertex.size(), _index.size()));
	std::vector<uint32_t> dstIndices(_index.size());
	std::vector<Vertex> dstVerts(_vertex.size());
	size_t indexCount = _index.size();
	size_t vertexCount = meshopt_generateVertexRemapCustom(
		remap.data(),
		_index.data(),
		_index.size(),
		reinterpret_cast<const float*>(_vertex.data()),
		_vertex.size(),
		sizeof(Vertex),
		[&](unsigned int l, unsigned int r) {
			const Vertex& lv = _vertex[l];
			const Vertex& rv = _vertex[r];
			return lv == rv;
		}
	);
	dstIndices.resize(indexCount);
	dstVerts.resize(vertexCount);
	meshopt_remapVertexBuffer(dstVerts.data(), _vertex.data(), _vertex.size(), sizeof(Vertex), remap.data());
	meshopt_remapIndexBuffer(dstIndices.data(), _index.data(), indexCount, remap.data());
	meshopt_optimizeVertexCache(_index.data(), dstIndices.data(), indexCount, vertexCount);
	meshopt_optimizeOverdraw(
		dstIndices.data(),
		_index.data(),
		indexCount,
		reinterpret_cast<const float*>(dstVerts.data()),
		vertexCount,
		sizeof(Vertex),
		1.0f
	);
	vertexCount = meshopt_optimizeVertexFetch(
		_vertex.data(),
		dstIndices.data(),
		indexCount,
		dstVerts.data(),
		vertexCount,
		sizeof(Vertex)
	);
	_vertex.resize(vertexCount);
	_index = std::move(dstIndices);
}

MeshletBounds MeshOptimizer::ComputeMeshletBounds(
	const std::vector<Vertex>& _vertex,
	const MeshletData& _meshletData, 
	const Meshlet& _meshlet) const
{
	MeshletBounds retBounds{};
	meshopt_Bounds bounds{};

	bounds = meshopt_computeMeshletBounds(
		&_meshletData.meshletVertices[_meshlet.vertexOffset],
		&_meshletData.meshletIndices[_meshlet.triangleOffset],
		_meshlet.triangleCount,
		reinterpret_cast<const float*>(_vertex.data()),
		_vertex.size(),
		sizeof(Vertex));
	retBounds.center = glm::vec3(bounds.center[0], bounds.center[1], bounds.center[2]);
	retBounds.radius = bounds.radius;
	retBounds.coneApex = glm::vec3(bounds.cone_apex[0], bounds.cone_apex[1], bounds.cone_apex[2]);
	retBounds.coneAxis = glm::vec3(bounds.cone_axis[0], bounds.cone_axis[1], bounds.cone_axis[2]);
	retBounds.coneCutoff = bounds.cone_cutoff;

	return retBounds;
}

MeshletBounds MeshOptimizer::ComputeBounds(const std::vector<Vertex>& _vertex, const std::vector<uint32_t>& _index) const
{
	MeshletBounds retBounds{};
	meshopt_Bounds bounds{};
	std::vector<glm::vec3> pos;

	pos.resize(_index.size());
	for (int i = 0; i < _index.size(); ++i)
	{
		pos[i] = _vertex[_index[i]].position;
	}
	bounds = meshopt_computeSphereBounds(
		reinterpret_cast<const float*>(pos.data()), 
		pos.size(), 
		sizeof(glm::vec3), 
		NULL, 
		0);

	retBounds.center = glm::vec3(bounds.center[0], bounds.center[1], bounds.center[2]);
	retBounds.radius = bounds.radius;
	retBounds.coneApex = glm::vec3(bounds.cone_apex[0], bounds.cone_apex[1], bounds.cone_apex[2]);
	retBounds.coneAxis = glm::vec3(bounds.cone_axis[0], bounds.cone_axis[1], bounds.cone_axis[2]);
	retBounds.coneCutoff = bounds.cone_cutoff;

	return retBounds;
}
