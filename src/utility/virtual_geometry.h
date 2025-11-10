#pragma once
#define IDXTYPEWIDTH 32
#define REALTYPEWIDTH 32
#include <metis/include/metis.h>
#include "common.h"
#include "geometry.h"

class VirtualGeometry
{
private:
	// Describes error of a simplified triangle group,
	// for meshlets, this is the error of the set of triangle group they generated from after simplification
	struct ErrorInfo
	{
		float error;					// error of meshlet group simplified
		MeshletBounds bounds; // boundary of simplified meshlet group
	};
	struct MyMeshlet
	{
		uint32_t lod = 0;
		
		// index pointing to error information of the meshlet
		uint32_t errorId = ~0;
		Meshlet meshlet;
		
		// edges on the boundary, built with virtual indices of 2 ends
		std::set<std::pair<uint32_t, uint32_t>> boundaries;
		
		// index of adjacent meshlets in array -> number of shared edges
		std::unordered_map<uint32_t, uint32_t> adjacentWeight;
		
		// I call meshlets built from the same simplified triangle group as siblings,
		// when converting to smaller pieces all siblings must do the same
		// eldestSibling is index of the first siblings in the vector
		// since siblings are stored consecutively I only store the siblingCount addition to eldestSibling
		uint32_t eldsetSibling;
		uint32_t siblingCount;
		
		// one of children whose LOD is higher, that is, meshlet built from the group of this meshlet and its siblings
		uint32_t child = ~0;
		
		// parent meshlets whose LOD is lower, that is, the original meshlet of the group this meshlet built from, 
		// LOD0 meshlets don't have parent, basically parent meshlets are the whole meshlets of a group
		std::vector<uint32_t> parents;
	};

private:
	const StaticMesh* m_pBaseMesh = nullptr;
	std::vector<ErrorInfo> m_errorInfo;							// error information of simplified meshlet
	std::vector<MeshletData> m_meshletTable;		// meshlet table for different LODs
	std::vector<std::vector<MyMeshlet>> m_meshlets;		// meshlets of different LODs
	std::vector<uint32_t> m_realToVirtual;						// maps real index to virtual index based on vertex positions, virtual index points
																			// to a vertex in the static mesh that has the postion that differentiate this virtual index from others

	// Debug use
	//std::vector<std::vector<Vertex>> m_verts;
	//std::vector<std::vector<uint32_t>> m_indices;
	//const std::vector<Vertex>& _GetVertices(uint32_t _lod) { return m_pBaseMesh->verts; };
	//const std::vector<uint32_t>& _GetIndices(uint32_t _lod) { return m_pBaseMesh->indices; };
private:
	// Add MyMeshlet objects, note it will also update parent MyMeshlet's child attribute
	// _lod: LOD of new meshlets
	// _error: error when simplifiy triangles to serve as the input to build new meshlets
	// _newMeshlets: meshlets built from simplified triangles
	// _parents: index of meshlets from the group before triangle simplification
	// _pFirstIndex: output, if not nullptr, fill first index of newly added MyMeshlet objects
	// _pNumAdded: output, if not nullptr, fill number of MyMeshlet objects added
	void _AddMyMeshlet(
		uint32_t _lod,
		float _error,
		const std::vector<Meshlet>& _newMeshlets, 
		const std::vector<uint32_t>& _parents,
		uint32_t* _pFirstIndex = nullptr,
		uint32_t* _pNumAdded = nullptr);

	// Build m_realToVirtual for the m_pBaseMesh
	void _BuildVirtualIndexMap();

	// Fill structure used in METIS,
	// _xadj, _adjncy describes a meshlet connection graph,
	// _xadj[i] is the beginning index of _adjncy that describes the connection of meshlet i
	// note that _xadj is 1 more longer than meshlet list because it needs last element to 
	// identify the end of last connection information of the last meshlet
	// _adjwgt describes weight of each connection
	// _lod: LOD of meshlets of the graph
	void _PrepareMETIS(
		uint32_t _lod, 
		std::vector<idx_t>& _xadj, 
		std::vector<idx_t>& _adjncy, 
		std::vector<idx_t>& _adjwgt) const;

	// There may be some vertices that share same the position but store different information(normal, uv),
	// but in this case we only want to differentiate them based on their positions,
	// i.e. if 2 vertices are close enough, it should be treated as the same vertex,
	// so i create this function,
	// _realIndex: vertex index in Static Mesh
	// return virtual index used to compare if the 2 vertices are same
	uint32_t _GetVirtualIndex(uint32_t _realIndex) const;

	// Find boundary of the meshlet, fill boundaries
	void _FindMeshletBoundary(
		const MeshletData& _meshletData,
		const Meshlet& _meshlet, 
		std::set<std::pair<uint32_t, uint32_t>>& _boundaries) const;

	// Find how many edges shared by meshlet pair, fill adjacentWeight
	void _RecordMeshletConnections(uint32_t _lod);

	// Divide meshlets into groups based on edges they shader
	// _lod: lod of meshlets to group
	// _meshletGroups: array of indices of meshlets in a group
	// return if division is successful
	bool _DivideMeshletGroup(
		uint32_t _lod,
		uint32_t _groupCount,
		std::vector<std::vector<uint32_t>>& _meshletGroups) const;

	// Simplify triangles in meshlet groups, return error compared to the original mesh
	// _lod: lod of meshlets to group
	// _meshletGroup: indices of meshlets in a group
	// _outIndex: indices that build simplified triangles
	float _SimplifyGroupTriangles(
		uint32_t _lod,
		const std::vector<uint32_t>& _meshletGroup,
		std::vector<uint32_t>& _outIndex) const;

	// Build new meshlets from simplified triangles in the group,
	// result will be added to the output, data already stored in output will be intact
	// _index: new set of index generated by _SimplifyGroupTriangle
	// _meshletData: local index points to the vertex in meshlet
	// _meshlet: output meshlet
	void _BuildMeshletFromGroup(
		const std::vector<Vertex>& _verts,
		const std::vector<uint32_t>& _index,
		MeshletData& _meshletData,
		std::vector<Meshlet>& _meshlet) const;

public:
	void PresetStaticMesh(const StaticMesh& _original);
	
	void Init();
};