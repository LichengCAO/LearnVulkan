#pragma once

struct VirtualGeometry
{
private:
	struct MeshletGroup
	{

	};

	struct MeshletDataTable
	{
		std::vector<uint32_t> vertex;
		std::vector<uint8_t>	index;
	};

private:
	const StaticMesh* m_pBaseMesh = nullptr;

private:
	// There may be some vertices that share same positions but store different information,
	// but in this case we only want to differentiate them based on their positions,
	// i.e. if 2 vertices are close enough, it should be treated as the same vertex,
	// so i create this function,
	// _realIndex: vertex index in Static Mesh
	// return virtual index used to compare if the 2 vertices are same
	uint32_t _IdentifyVertexBasedOnPosition(uint32_t _realIndex) const;

	// Find edges of the meshlet
	void _FindMeshletEdge(
		const Meshlet& _meshlet, 
		const std::vector<uint32_t>& _meshletVertexIndex, 
		const std::vector<uint8_t>& _meshletLocalIndex,
		std::set<std::pair<uint32_t, uint32_t>>& _edges) const;

	// find how many edges shared by meshlet pair
	void _RecordMeshletConnections();

	// divide meshlet into groups based on edges they shader
	void _DivideMeshletGroup();

	// simplify triangles in meshlet groups
	void _SimplifyGroupTriangles();

	// build new meshlets from simplified triangles in the group
	void _BuildMeshletFromGroupTriangles();

public:
	void PresetStaticMesh(const StaticMesh& _original);
	void Init();
	void BuildVirtualGeometry()
};