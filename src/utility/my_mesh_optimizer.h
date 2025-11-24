#pragma once
#include <meshoptimizer.h>
#include <functional>
#include "common.h"
#include "geometry.h"

// Wrapper class for meshoptimizer lib
class MeshOptimizer
{
private:
	void _LockBoundary(
		const std::vector<Vertex>& _vertex, 
		const std::vector<uint32_t>& _index, 
		std::vector<uint8_t>& _locks) const;
public:
	// Build meshlets from vertices and indices,
	// _outMeshletData, _outMeshlet don't need 
	// to be empty as long as the vertices that they built from are the same as the current call.
	// Which means that we can insert only part of the whole index buffer to build meshlets that for partial of the mesh 
	// and come back later to build the other part
	// _vertex: vertices of the whole mesh, start with vec3 position
	// _index: index of the part of the mesh we want to build meshlet for
	// _outMeshletData: data used by meshlet to get indices of the original mesh
	// _outMeshlet: each element is an interval of _outMeshletVertex, _outMeshletLocalIndex that presents a meshlet
	// _maxPrimitiveCount: max primitive/triangle count of each meshlet
	// _maxIndexCount: max index count of each meshlet
	void BuildMeshlets(
		const std::vector<Vertex>& _vertex,
		const std::vector<uint32_t>& _index,
		Meshlet::DeviceData& _outMeshletData,
		std::vector<Meshlet::DeviceDataRef>& _outMeshlet,
		size_t _maxPrimitiveCount = 124,
		size_t _maxIndexCount = 64) const;
	void BuildMeshlets(
		const std::vector<Vertex>& _vertex,
		const std::vector<uint32_t>& _index,
		std::vector<Meshlet>& _outMeshlet,
		size_t _maxPrimitiveCount = 124,
		size_t _maxIndexCount = 64) const;

	// Simplify mesh/meshlet while leaving border intact,
	// use original vertices to draw simplified mesh
	// return relative error from the original mesh
	// previous elements in _outIndex will remain intact
	// _vertex: vertices of the original mesh
	// _index: indices of the orignal mesh
	// _targetIndexCount: target index number returned may not reach this
	// _outIndex: output simplified index, can draw new mesh with the original _vertex
	float SimplifyMesh(
		const std::vector<Vertex>& _vertex,
		const std::vector<uint32_t>& _index,
		size_t _targetIndexCount,
		std::vector<uint32_t>& _outIndex) const;

	// Simplify mesh/meshlet while leaving border intact, 
	// generate a new set of vertices used to draw simplified mesh
	// return relative error from the original mesh,
	// previous elements in _outVertex and _outIndex will remain intact
	// _vertex: vertices of the original mesh
	// _index: indices of the orignal mesh
	// _targetIndexCount: target index number returned may not reach this
	// _outVertex: output simplified vertices 
	// _outIndex: output simplified index, can draw new mesh with _outVertex
	float SimplifyMesh(
		const std::vector<Vertex>& _vertex,
		const std::vector<uint32_t>& _index,
		size_t _targetIndexCount,
		std::vector<Vertex>& _outVertex,
		std::vector<uint32_t>& _outIndex) const;

	// Optimize mesh by reordering vertex and index to GPU friendly layout
	// removes duplicated vertices
	void OptimizeMesh(std::vector<Vertex>& _vertex, std::vector<uint32_t>& _index) const;

	// Compute meshlet bounds for culling
	MeshletBounds ComputeMeshletBounds(
		const std::vector<Vertex>& _vertex,
		const Meshlet& _meshlet) const;
	MeshletBounds ComputeMeshletBounds(
		const std::vector<Vertex>& _vertex,
		const Meshlet::DeviceData& _meshletData,
		const Meshlet::DeviceDataRef& _meshlet) const;

	// Compute boundary of cluster of mesh
	MeshletBounds ComputeBounds(
		const std::vector<Vertex>& _vertex,
		const std::vector<uint32_t>& _index) const;

	// Remove duplicate vertices based on _equal
	template<typename T>
	void RemoveDuplicateVertices(
		std::vector<T>& _vertex, 
		std::vector<uint32_t>& _index, 
		std::function<bool(const T&, const T&)> _equal) const 
	{
		std::vector<uint32_t> remap(std::max(_vertex.size(), _index.size()));
		std::vector<uint32_t> dstIndices(_index.size());
		std::vector<T> dstVerts(_vertex.size());
		size_t indexCount = _index.size();
		size_t vertexCount = meshopt_generateVertexRemapCustom(
			remap.data(),
			_index.data(),
			_index.size(),
			reinterpret_cast<const float*>(_vertex.data()),
			_vertex.size(),
			sizeof(T),
			[&](unsigned int l, unsigned int r) { return _equal(_vertex[l], _vertex[r]); }
		);
		dstIndices.resize(indexCount);
		dstVerts.resize(vertexCount);
		meshopt_remapVertexBuffer(dstVerts.data(), _vertex.data(), _vertex.size(), sizeof(T), remap.data());
		meshopt_remapIndexBuffer(dstIndices.data(), _index.data(), indexCount, remap.data());
		_vertex = dstVerts;
		_index = dstIndices;
	};

	// Map index to the first vertex
	// whose first _equalByte bytes are equal to the 
	// vertex that this index currently points to
	// this function will not reorder triangles
	// that _originalIndex represents
	template<typename T>
	void RemapIndex(
		const std::vector<T>& _vertex,
		const std::vector<uint32_t>& _originIndex,
		std::vector<uint32_t>& _outIndex,
		size_t _equalByte = sizeof(glm::vec3)) const
	{
		_outIndex.resize(_originIndex.size());
		meshopt_generateShadowIndexBuffer(
			_outIndex.data(), 
			_originIndex.data(),
			_originIndex.size(),
			_vertex.data(), 
			_vertex.size(), 
			_equalByte,
			sizeof(T));
	}
};