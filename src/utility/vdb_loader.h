#pragma once
#include "common.h"
#include <nanovdb/io/IO.h>
class MyVDBLoader
{
public:
	// VDB data structure https://jangafx.com/insights/vdb-a-deep-dive
	// ref: https://diglib.eg.org/server/api/core/bitstreams/8bbf3ba6-a4df-45c4-9703-856002f04b1d/content
	// openVDB iterator iterate based on coordinate order,
	// so i think if we can get the relative index we can map it to an index in next level pool0
	// perhaps do a parallel prefix sum (scan) to find out which data to read
	// the iterate order is z -> y -> x

	// GPU friendly data to update to GPU
	struct CompactData
	{
		std::vector<uint8_t> data;				    // data to push to the GPU

		std::array<uint32_t, 5> offsets;			// leaf, lower, upper, root, grid
		std::array<uint32_t, 5> dataSizes;

		glm::vec3 minBound;
		glm::vec3 maxBound;
		float maxValue;
		float minValue;
	};
private:
	std::string _CreateNanoVDBFromOpenVDB(const std::string& _openVDB) const;
	void _ExportCompactDataFromGridHandle(const nanovdb::GridHandle<>& _handle, CompactData& _output) const;
	void _ExportNanoVDB(const std::string& _nvdbFile, CompactData& _output) const;

public:
	MyVDBLoader();
	void Load(const std::string& _file, CompactData& _output) const;
};