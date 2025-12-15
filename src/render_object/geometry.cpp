#include "geometry.h"

void Meshlet::GetTriangle(uint32_t _index, std::array<uint32_t, 3>& _outTriangle) const
{
	for (size_t i = 0; i < 3; ++i)
	{
		uint8_t idx = this->index[i + _index * 3];
		_outTriangle[i] = this->vertices[idx];
	}
}

void Meshlet::GetIndices(std::vector<uint32_t>& _outIndices) const
{
	for (auto idx : this->index)
	{
		_outIndices.push_back(this->vertices[idx]);
	}
	//_outIndices.insert(_outIndices.end(), this->vertices.begin(), this->vertices.end());
}

void Meshlet::CompressToDeviceData(const Meshlet& _meshlet, Meshlet::DeviceData& _outData, Meshlet::DeviceDataRef& _outRef)
{
	_outRef.indexOffset = _outData.meshletIndices.size();
	_outRef.vertexOffset = _outData.meshletVertices.size();
	_outRef.triangleCount = _meshlet.index.size() / 3;
	_outRef.vertexCount = _meshlet.vertices.size();

	_outData.meshletIndices.insert(
		_outData.meshletIndices.end(), 
		_meshlet.index.begin(), 
		_meshlet.index.end());
	_outData.meshletVertices.insert(
		_outData.meshletVertices.end(), 
		_meshlet.vertices.begin(), 
		_meshlet.vertices.end());
}

Meshlet Meshlet::GetFromDeviceData(const Meshlet::DeviceData& _data, const Meshlet::DeviceDataRef& _ref)
{
	Meshlet meshlet{};
	
	meshlet.index.insert(
		meshlet.index.end(), 
		_data.meshletIndices.begin() + _ref.indexOffset, 
		_data.meshletIndices.begin() + _ref.indexOffset + _ref.triangleCount * 3);
	meshlet.vertices.insert(
		meshlet.vertices.end(),
		_data.meshletVertices.begin() + _ref.vertexOffset,
		_data.meshletVertices.begin() + _ref.vertexOffset + _ref.vertexCount);

	return meshlet;
}
