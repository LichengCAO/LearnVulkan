#include "virtual_geometry.h"
#include "my_mesh_optimizer.h"
#include "utils.h"
#include <functional>

void VirtualGeometry::_AddMyMeshlet(uint32_t _lod, const Meshlet& _meshlet, uint32_t _parent, float error, const std::vector<uint32_t>& _siblings)
{
	uint32_t 
}

void VirtualGeometry::_BuildVirtualIndexMap()
{
	const StaticMesh& staticMesh = *m_pBaseMesh;
	std::set<uint32_t> existVirtualIndex;
 	size_t n = staticMesh.verts.size();
	
	m_realToVirtual.clear();
	m_realToVirtual.resize(n, 0);
	for (size_t i = 0; i < n; ++i)
	{
		const glm::vec3& curPosition = staticMesh.verts[i].position;
		bool hasVirtualIndex = false;
		for (auto virtualIndex : existVirtualIndex)
		{
			float dist = glm::distance(curPosition, staticMesh.verts[virtualIndex].position);
			if (dist < 0.001f)
			{
				m_realToVirtual[i] = virtualIndex;
				hasVirtualIndex = true;
				break;
			}
		}
		if (!hasVirtualIndex)
		{
			m_realToVirtual[i] = i;
			existVirtualIndex.insert(i);
		}
	}
}

void VirtualGeometry::_PrepareMETIS(
	uint32_t _lod, 
	std::vector<idx_t>& _xadj, 
	std::vector<idx_t>& _adjncy, 
	std::vector<idx_t>& _adjwgt) const
{
	const uint32_t n = m_meshlets[_lod].size();
	_xadj.clear();
	_adjncy.clear();
	_adjwgt.clear();
	_xadj.resize(n + 1);
	_adjncy.reserve(8 * n);
	_adjwgt.reserve(8 * n);

	_xadj.push_back(static_cast<idx_t>(_adjncy.size()));
	for (uint32_t i = 0; i < n; ++i)
	{
		const auto& curMeshlet = m_meshlets[_lod][i];
		for (const auto& p : curMeshlet.adjacentWeight)
		{
			_adjncy.push_back(static_cast<idx_t>(p.first));		// index of the adjacent meshlet
			_adjwgt.push_back(static_cast<idx_t>(p.second));	// number of shared edges
		}
		_xadj.push_back(static_cast<idx_t>(_adjncy.size()));
	}
}

uint32_t VirtualGeometry::_GetVirtualIndex(uint32_t _realIndex) const
{
	return m_realToVirtual[_realIndex];
}

void VirtualGeometry::_FindMeshletBoundary(
	const MeshletDeviceData& _meshletData, 
	const Meshlet& _meshlet, 
	std::set<std::pair<uint32_t, uint32_t>>& _boundaries) const
{
	std::unordered_map<std::pair<uint32_t, uint32_t>, uint32_t, common_utils::PairHash> edgeSeen;
	uint32_t n = _meshlet.triangleCount;

	for (uint32_t i = 0; i < n; ++i)
	{
		std::array<uint32_t, 3> indices;
		std::array<std::pair<uint32_t, uint32_t>, 3> sides;

		_meshlet.GetTriangle(i, _meshletData, indices);
		sides[0] = { _GetVirtualIndex(indices[0]), _GetVirtualIndex(indices[1]) };
		sides[1] = { _GetVirtualIndex(indices[1]), _GetVirtualIndex(indices[2]) };
		sides[2] = { _GetVirtualIndex(indices[2]), _GetVirtualIndex(indices[0]) };

		for (int j = 0; j < 3; ++j)
		{
			uint32_t head = std::min(sides[j].first, sides[j].second);
			uint32_t tail = std::max(sides[j].first, sides[j].second);

			sides[j] = { head, tail };
			if (edgeSeen.find(sides[j]) == edgeSeen.end())
			{
				edgeSeen.insert({ sides[j], 1 });
			}
			else
			{
				edgeSeen[sides[j]] = edgeSeen[sides[j]] + 1;
			}
		}
	}

	for (const auto& p : edgeSeen)
	{
		// shows up only once
		if (p.second == 1)
		{
			_boundaries.insert(p.first);
		}
	}
}

void VirtualGeometry::_RecordMeshletConnections(uint32_t _lod)
{
	std::unordered_map<
		std::pair<uint32_t, uint32_t>, 
		std::vector<uint32_t>, 
		common_utils::PairHash> edgeMeshlet; // edge to meshlets that have the edge

	// fill edge meshlet map
	for (uint32_t i = 0; i < m_meshlets[_lod].size(); ++i)
	{
		const auto& currentMeshlet = m_meshlets[_lod][i];
		for (const auto& edge : currentMeshlet.boundaries)
		{
			if (edgeMeshlet.find(edge) == edgeMeshlet.end())
			{
				edgeMeshlet.insert({ edge, { i } });
			}
			else
			{
				edgeMeshlet[edge].push_back(i);
			}
		}
	}

	// for each edge, update meshlets connected to it
	for (const auto& p : edgeMeshlet)
	{
		for (uint32_t i = 0; i < p.second.size(); ++i)
		{
			uint32_t meshletId = p.second[i];
			auto& meshlet = m_meshlets[_lod][meshletId];
			for (uint32_t j = 0; j < p.second.size(); ++j)
			{
				if (j == i) continue;
				uint32_t meshletId2 = p.second[j];
				if (meshlet.adjacentWeight.find(meshletId2) == meshlet.adjacentWeight.end())
				{
					meshlet.adjacentWeight.insert({ meshletId2, 1 });
				}
				else
				{
					meshlet.adjacentWeight[meshletId2]++;
				}
			}
		}
	}
}

bool VirtualGeometry::_DivideMeshletGroup(uint32_t _lod, std::vector<std::vector<uint32_t>>& _meshletGroups)
{
	std::vector<idx_t> xadj;
	std::vector<idx_t> adjncy;
	std::vector<idx_t> adjwgt;
	std::vector<idx_t> part;
	idx_t nvtxs = m_meshlets[_lod].size();
	idx_t nparts = std::max( static_cast<idx_t>(m_meshlets[_lod].size() / 4), 1);
	idx_t edgecut = 0;
	idx_t options[METIS_NOPTIONS];
	std::function<int(idx_t*, idx_t*, idx_t*, idx_t*, idx_t*, idx_t*, idx_t*, idx_t*, real_t*, real_t*, idx_t*, idx_t*, idx_t*)> metisFunc;
	int result = 0;

	METIS_SetDefaultOptions(options);  // Initialize default options
	part.resize(nvtxs);
	_PrepareMETIS(_lod, xadj, adjncy, adjwgt);

	if (nparts > 8)
	{
		metisFunc = METIS_PartGraphKway;
	}
	else
	{
		metisFunc = METIS_PartGraphRecursive;
	}

	result = metisFunc(
		&nvtxs,				// nvtxs, Number of vertices
		NULL,					// ncon, Number of balancing constraints (usually NULL)
		xadj.data(),			// xadj, Pointer to xadj array
		adjncy.data(),		// adjncy, Pointer to adjncy array
		NULL,					// vwgt, Vertex weights (NULL if not used)
		NULL,					// vsize, Sizes of vertex weights (NULL if not used)
		adjwgt.data(),		// adjwgt, Edge weights (NULL if not used)
		&nparts,				// nparts, Number of parts
		NULL,					// tpwgts, Imbalance tolerance (NULL uses default)
		NULL,					// ubvec, Edge weight factor (NULL uses default)
		options,				// options, Options array
		&edgecut,			// edgecut, Output: Edge cut value
		part.data());			// part, Output: Partition vector

	if (result == METIS_OK)
	{
		_meshletGroups.resize(nparts, {});
		for (uint32_t i = 0; i < nvtxs; ++i)
		{
			idx_t groupId = part[i];
			_meshletGroups[groupId].push_back(i);
		}
	}

	return result == METIS_OK;
}

float VirtualGeometry::_SimplifyGroupTriangles(
	uint32_t _lod, 
	const std::vector<uint32_t>& _meshletGroup, 
	std::vector<uint32_t>& _outIndex)
{
	std::vector<uint32_t> meshletIndex;
	MeshOptimizer optimizer{};
	float error = 0.0f;

	for (size_t i = 0; i < _meshletGroup.size(); ++i)
	{
		const auto& meshlet = m_meshlets[_lod][_meshletGroup[i]].meshlet;
		meshlet.GetIndices(m_meshletTable[_lod], meshletIndex);
	}

	optimizer.SimplifyMesh(m_pBaseMesh->verts, meshletIndex, _outIndex, error);

	return error;
}

void VirtualGeometry::_BuildMeshletFromGroup(
	const std::vector<uint32_t>& _index, 
	MeshletDeviceData& _meshletData, 
	std::vector<Meshlet>& _meshlet)
{
	MeshOptimizer optimizer{};
	optimizer.BuildMeshlet(m_pBaseMesh->verts, _index, _meshletData, _meshlet);
}

void VirtualGeometry::PresetStaticMesh(const StaticMesh& _original)
{
	m_pBaseMesh = &_original;
}

void VirtualGeometry::Init()
{
	int maxLOD = 5;
	MeshOptimizer optimizer{};
	std::vector<std::vector<uint32_t>> meshletGroups;
	m_meshlets.resize(maxLOD + 1);
	m_meshletTable.resize(maxLOD + 1);

	_BuildVirtualIndexMap();

	for (int i = 0; i < (maxLOD + 1); ++i)
	{
		std::vector<Meshlet> meshlets{};

		// Build meshlets for current LOD
		if (i == 0)
		{
			optimizer.BuildMeshlet(m_pBaseMesh->verts, m_pBaseMesh->indices, m_meshletTable[i], meshlets);

			// fill up LOD i meshlets
			{
				uint32_t firstSibling = m_meshlets[i].size();
				std::vector<uint32_t> siblings;
				for (int j = 0; j < meshlets.size(); ++j)
				{
					MyMeshlet myMeshlet{};

					myMeshlet.lod = i;
					myMeshlet.meshlet = meshlets[j];
					siblings.push_back(m_meshlets.size());
					m_meshlets[i].push_back(myMeshlet);
				}

				for (int j = 0; j < meshlets.size(); ++j)
				{
					auto& myMeshlet = m_meshlets[i][firstSibling + j];
					myMeshlet.siblings = siblings;
					myMeshlet.parent
				}
			}
		}
		else
		{
			// For each group of meshlets, build a new list of triangles approximating the original group
			// For each simplified group, break them apart into new meshlets
			for (const auto& group : meshletGroups)
			{
				std::vector<uint32_t> simplifiedIndex;

				// For each group of meshlets, build a new list of triangles approximating the original group
				_SimplifyGroupTriangles(i, group, simplifiedIndex);

				// For each simplified group, break them apart into new meshlets
				_BuildMeshletFromGroup(simplifiedIndex, m_meshletTable[i], meshlets);
			}
		}

		// if LOD reaches max we don't need to group meshlet any more, break;
		if (i == maxLOD) break;

		// For each meshlet, find the set of all edges making up the triangles within the meshlet
		for (int j = 0; j < m_meshlets[i].size(); ++j)
		{
			_FindMeshletBoundary(m_meshletTable[i], m_meshlets[i][j].meshlet, m_meshlets[i][j].boundaries);
		}

		// For each meshlet, find the set of connected meshlets(sharing an edge)
		_RecordMeshletConnections(i);

		// Divide meshlets into groups of roughly 4
		meshletGroups.clear();
		auto divideSuccess = _DivideMeshletGroup(i, meshletGroups);

		// Remove data we don't fill
		if (!divideSuccess)
		{
			m_meshlets.resize(i + 1);
			m_meshletTable.resize(i + 1);
			std::cout << "ERROR: Cannot divide meshlet groups, break!" << std::endl;
			break;
		}
	}
}
