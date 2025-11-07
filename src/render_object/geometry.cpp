#include "geometry.h"

void Meshlet::GetTriangle(uint32_t _index, const MeshletDeviceData& _meshletData, std::array<uint32_t, 3>& _outTriangle) const
{
	CHECK_TRUE(_index < triangleCount, "Triangle index out of range!");

	for (int i = 0; i < 3; ++i)
	{
		uint8_t index = _meshletData.meshletIndices[triangleOffset * 3 + i];
		uint32_t vertexIndex = _meshletData.meshletVertices[vertexOffset + index];

		_outTriangle[i] = vertexIndex;
	}
}

void Meshlet::GetIndices(const MeshletDeviceData& _meshletData, std::vector<uint32_t>& _outIndices) const
{
	for (uint32_t i = 0; i < triangleCount; ++i)
	{
		std::array<uint32_t, 3> triangle;
		GetTriangle(i, _meshletData, triangle);
		_outIndices.insert(_outIndices.end(), triangle.begin(), triangle.end());
	}
}
