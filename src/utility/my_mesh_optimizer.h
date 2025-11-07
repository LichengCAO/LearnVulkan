#pragma once
#include <meshoptimizer.h>
#include "common.h"
#include "geometry.h"

// Adaptor for meshoptimizer lib
class MeshOptimizer
{
public:
	// Build meshlets from vertices and indices,
	// _outMeshletVertex, _outMeshletLocalIndex, _outMeshlet don't need 
	// to be empty as long as the vertices that they built from are the same as the current call.
	// Which means that we can insert only part of the whole index buffer to build meshlets that for partial of the mesh 
	// and come back later to build the other part
	// _vertex: vertices of the whole mesh, start with vec3 position
	// _index: index of the part of the mesh we want to build meshlet for
	// _outMeshletVertex: meshlet vertex which is actually an index pointing to the Vertex in _vertex
	// _outMeshletLocalIndex: index points to meshlet vertex
	// _outMeshlet: each element is an interval of _outMeshletVertex, _outMeshletLocalIndex that presents a meshlet
	void BuildMeshlet(
		const std::vector<Vertex>& _vertex,
		const std::vector<uint32_t>& _index,
		std::vector<uint32_t>& _outMeshletVertex,
		std::vector<uint8_t>& _outMeshletLocalIndex,
		std::vector<Meshlet>& _outMeshlet) const;

	void SimplifyMesh(
		const std::vector<Vertex>& _vertex,
		const std::vector<uint32_t>& _index,
		std::vector<uint32_t>& _outIndex,
		float& _error) const;
};