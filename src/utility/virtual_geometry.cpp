
#define IDXTYPEWIDTH 32
#define REALTYPEWIDTH 32
#include <metis/include/metis.h>
#include "virtual_geometry.h"
#include "my_mesh_optimizer.h"
#include "common.h"
#include "utils.h"

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
	std::vector<uint32_t>& _xadj, 
	std::vector<uint32_t>& _adjncy, 
	std::vector<uint32_t>& _adjwgt) const
{
	const uint32_t n = m_meshlets[_lod].size();
	_xadj.clear();
	_adjncy.clear();
	_adjwgt.clear();
	_xadj.resize(n + 1);
	_adjncy.reserve(8 * n);
	_adjwgt.reserve(8 * n);

	_xadj.push_back(_adjncy.size());
	for (uint32_t i = 0; i < n; ++i)
	{
		const auto& curMeshlet = m_meshlets[_lod][i];
		for (const auto& p : curMeshlet.adjacentWeight)
		{
			_adjncy.push_back(p.first);		// index of the adjacent meshlet
			_adjwgt.push_back(p.second);	// number of shared edges
		}
		_xadj.push_back(_adjncy.size());
	}
}

uint32_t VirtualGeometry::_GetVirtualIndex(uint32_t _realIndex) const
{
	return m_realToVirtual[_realIndex];
}

void VirtualGeometry::_FindMeshletBoundary(
	const Meshlet& _meshlet, 
	const std::vector<uint32_t>& _meshletVertexIndex, 
	const std::vector<uint8_t>& _meshletLocalIndex, 
	std::set<std::pair<uint32_t, uint32_t>>& _boundaries) const
{
	std::unordered_map<std::pair<uint32_t, uint32_t>, uint32_t, common_utils::PairHash> edgeSeen;
	uint32_t n = _meshlet.triangleCount;

	for (uint32_t i = 0; i < n; ++i)
	{
		std::array<uint32_t, 3> indices;
		std::array<std::pair<uint32_t, uint32_t>, 3> sides;

		_meshlet.GetTriangle(i, _meshletVertexIndex, _meshletLocalIndex, indices);
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

void VirtualGeometry::_DivideMeshletGroup(uint32_t _lod, std::vector<std::vector<uint32_t>>& _meshletGroups)
{
	std::vector<uint32_t> xadj;
	std::vector<uint32_t> adjncy;
	std::vector<uint32_t> adjwgt;
	uint32_t nvtxs = m_meshlets[_lod].size();
	uint32_t ncon = 1;

	uint32_t nPart = std::max( static_cast<uint32_t>(m_meshlets[_lod].size() / 4), 1u);

	_PrepareMETIS(_lod, xadj, adjncy, adjwgt);
	if (nPart > 8)
	{
		METIS_PartGraphKway(
			&nvtxs,				//nvtxs
			&ncon,				//ncon
			xadj.data(),			//xadj
			adjncy.data(),		//adjncy
			nullptr,				//vwgt
			nullptr,				//vsize
			adjwgt.data(),		//adjwgt
			&nPart,				//nparts
			nullptr,				//tpwgts
			nullptr,				//ubvec
			nullptr,				//options
			nullptr,				//edgecut
			nullptr);				//part
	}
	else
	{

	}
}

void VirtualGeometry::_SimplifyGroupTriangles(
	uint32_t _lod, 
	const std::vector<uint32_t>& _meshletGroup, 
	std::vector<uint32_t>& _outIndex)
{

}
