#pragma once
#include <meshoptimizer.h>
#include "common.h"
#include "geometry.h"

class MeshletBuilder
{
public:
	// _vertex: vertices of the whole mesh, start with vec3 position
	// _index: index of the part of the mesh we want to build meshlet for
	// _outMeshletVertex: index points to the Vertex in _vertex
	// _outMeshletLocalIndex: index points to the vertex the meshlet owns
	// _outMeshlet: each element is an interval of _outMeshletVertex, _outMeshletLocalIndex that presents a meshlet
	void BuildMeshlet(
		const std::vector<Vertex>& _vertex,
		const std::vector<uint32_t>& _index,
		std::vector<uint32_t>& _outMeshletVertex,
		std::vector<uint8_t>& _outMeshletLocalIndex,
		std::vector<Meshlet>& _outMeshlet) const;
};