
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
