#include "geometry.h"

void Meshlet::GetTriangle(
	uint32_t _index, 
	const std::vector<uint32_t>& _meshletVertices, 
	const std::vector<uint8_t>& _meshletTriangles, 
	std::array<uint32_t, 3>& _outTriangle) const
{
	CHECK_TRUE(_index < triangleCount, "Triangle index out of range!");

	for (int i = 0; i < 3; ++i)
	{
		uint8_t index = _meshletTriangles[triangleOffset * 3 + i];
		uint32_t vertexIndex = _meshletVertices[vertexOffset + index];

		_outTriangle[i] = vertexIndex;
	}
}
