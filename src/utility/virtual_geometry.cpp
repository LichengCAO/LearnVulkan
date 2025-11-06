
#define IDXTYPEWIDTH 32
#define REALTYPEWIDTH 32
#include <metis/include/metis.h>
#include "virtual_geometry.h"
#include <meshoptimizer.h>
#include "common.h"
#include "utils.h"

void VirtualGeometry::_SimplifyGroupTriangles()
{
	std::vector<uint32_t> srcIndex;
	std::vector<Vertex> srcVertex;
	std::vector<uint32_t> index;
	size_t indexCount = 0;
	float error = 0;

	index.reserve(srcIndex.size());

	indexCount = meshopt_simplify(
		index.data(),
		srcIndex.data(),
		srcIndex.size(),
		(float*)srcVertex.data(),
		srcVertex.size(),
		sizeof(Vertex),
		srcIndex.size() / 2,
		0.2,
		meshopt_SimplifyLockBorder,
		&error);

	index.resize(indexCount);
}

void VirtualGeometry::_FillCSR(
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
