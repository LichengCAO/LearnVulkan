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
	// inVertices: vertices of the whole mesh, start with vec3 position
	// inIndices: index of the part of the mesh we want to build meshlet for
	// _outMeshletData: data used by meshlet to get indices of the original mesh, data already stored remains intact
	// _outMeshlet: each element is an interval of Meshlet::DeviceData that presents a meshlet
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
	// previous elements in outIndices will remain intact
	// inVertices: vertices of the original mesh
	// inIndices: indices of the orignal mesh
	// inTargetIndexCount: target index number returned may not reach this
	// outIndices: output simplified index, can draw new mesh with the original inVertices
	float SimplifyMesh(
		const std::vector<Vertex>& inVertices,
		const std::vector<uint32_t>& inIndices,
		size_t inTargetIndexCount,
		std::vector<uint32_t>& outIndices) const;

	// Simplify mesh/meshlet while leaving border intact, 
	// generate a new set of vertices used to draw simplified mesh
	// return relative error from the original mesh,
	// previous elements in outVertices and outIndices will remain intact
	// inVertices: vertices of the original mesh
	// inIndices: indices of the orignal mesh
	// inTargetIndexCount: target index number returned may not reach this
	// outVertices: output simplified vertices 
	// outIndices: output simplified index, can draw new mesh with outVertices
	float SimplifyMesh(
		const std::vector<Vertex>& inVertices,
		const std::vector<uint32_t>& inIndices,
		size_t inTargetIndexCount,
		std::vector<Vertex>& outVertices,
		std::vector<uint32_t>& outIndices) const;

	// Optimize mesh by reordering vertex and index to GPU friendly layout
	// removes duplicated vertices
	void OptimizeMesh(
		std::vector<Vertex>& inoutVertices, 
		std::vector<uint32_t>& inoutIndices) const;

	// Compute meshlet bounds for culling
	MeshletBounds ComputeMeshletBounds(
		const std::vector<Vertex>& inVertices,
		const Meshlet& inMeshlet) const;
	MeshletBounds ComputeMeshletBounds(
		const std::vector<Vertex>& inVertices,
		const Meshlet::DeviceData& inMeshletData,
		const Meshlet::DeviceDataRef& inMeshletDataRef) const;

	// Compute boundary of cluster of mesh
	MeshletBounds ComputeBounds(
		const std::vector<Vertex>& inVertices,
		const std::vector<uint32_t>& inIndices) const;

	// Remove duplicate vertices based on inFuncEqual
	template<typename T>
	void RemoveDuplicateVertices(
		std::vector<T>& inoutVertices, 
		std::vector<uint32_t>& inoutIndices, 
		std::function<bool(const T&, const T&)> inFuncEqual) const 
	{
		std::vector<uint32_t> remap(std::max(inoutVertices.size(), inoutIndices.size()));
		std::vector<uint32_t> dstIndices(inoutIndices.size());
		std::vector<T> dstVerts(inoutVertices.size());
		size_t indexCount = inoutIndices.size();
		size_t vertexCount = meshopt_generateVertexRemapCustom(
			remap.data(),
			inoutIndices.data(),
			inoutIndices.size(),
			reinterpret_cast<const float*>(inoutVertices.data()),
			inoutVertices.size(),
			sizeof(T),
			[&](unsigned int l, unsigned int r) { return inFuncEqual(inoutVertices[l], inoutVertices[r]); }
		);
		dstIndices.resize(indexCount);
		dstVerts.resize(vertexCount);
		meshopt_remapVertexBuffer(dstVerts.data(), inoutVertices.data(), inoutVertices.size(), sizeof(T), remap.data());
		meshopt_remapIndexBuffer(dstIndices.data(), inoutIndices.data(), indexCount, remap.data());
		inoutVertices = dstVerts;
		inoutIndices = dstIndices;
	};

	// Map index to the first vertex
	// whose first inEqualBytes bytes are equal to the 
	// vertex that this index currently points to
	// this function will not reorder triangles
	// that inOriginIndices represents
	template<typename T>
	void RemapIndex(
		const std::vector<T>& inVertices,
		const std::vector<uint32_t>& inOriginIndices,
		std::vector<uint32_t>& outIndices,
		size_t inEqualBytes = sizeof(glm::vec3)) const
	{
		outIndices.resize(inOriginIndices.size());
		meshopt_generateVertexRemapCustom(
			outIndices.data(),
			inOriginIndices.data(),
			inOriginIndices.size(),
			reinterpret_cast<const float*>(inVertices.data()),
			inVertices.size(),
			sizeof(T),
			[&](unsigned int l, unsigned int r) {
				return std::memcmp(
					reinterpret_cast<const uint8_t*>(&inVertices[l]), 
					reinterpret_cast<const uint8_t*>(&inVertices[r]), 
					inEqualBytes) == 0;
			});
	}

	void GeneratePositionRemap(
		const std::vector<Vertex>& inVertices,
		std::vector<uint32_t>& outPositionRemap) const;
};