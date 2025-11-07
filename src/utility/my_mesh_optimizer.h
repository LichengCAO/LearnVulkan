#pragma once
#include <meshoptimizer.h>
#include "common.h"
#include "geometry.h"

// Wrapper class for meshoptimizer lib
class MeshOptimizer
{
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
	void BuildMeshlet(
		const std::vector<Vertex>& _vertex,
		const std::vector<uint32_t>& _index,
		MeshletDeviceData& _outMeshletData,
		std::vector<Meshlet>& _outMeshlet) const;

	void SimplifyMesh(
		const std::vector<Vertex>& _vertex,
		const std::vector<uint32_t>& _index,
		std::vector<uint32_t>& _outIndex,
		float& _error) const;

	void OptimizeMesh(std::vector<Vertex>& _vertex, std::vector<uint32_t>& _index) const;

	MeshletBounds ComputeMeshletBounds(
		const std::vector<Vertex>& _vertex,
		const MeshletDeviceData& _meshletData,
		const Meshlet& _meshlet) const;
};