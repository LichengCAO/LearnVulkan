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
	struct Pool0Data
	{
		uint8_t level = 0;				  // leaf level is 0, root level is 3
		glm::vec3 index;				  // bounding box's min corner, note this is not relative index in a level
		uint32_t parentIndex = ~0;		  // parent index in pool0
		uint32_t childOffset = 0;		  // child index offset in pool1
		std::vector<uint8_t> activeChild; // present how many child is active, one bit for one child, so the length of this should be CHILDREN_COUNT / 8
	};

	struct Level1Data
	{
		glm::mat4 indexToWorld;				// apply this model matrix to index to convert it to world position
		std::vector<Pool0Data> level1Pool0; // store the level nodes
		std::vector<Pool0Data> level0Pool0; // points to data in voxel data
		std::vector<std::array<float, 512>> voxelData; // for now i assume the data is only float not considering vec3 or other
	};

	// GPU friendly data to update to GPU
	struct CompactData
	{
		std::vector<uint8_t> data;				    // data to push to the GPU

		std::array<uint32_t, 5> offsets;			// leaf, lower, upper, root, grid
		std::array<uint32_t, 5> dataSizes;

		glm::vec3 minBound;
		glm::vec3 maxBound;
	};
private:
	std::string _CreateNanoVDBFromOpenVDB(const std::string& _openVDB) const;
	void _ExportCompactDataFromGridHandle(const nanovdb::GridHandle<>& _handle, CompactData& _output) const;
	void _ExportNanoVDB(const std::string& _nvdbFile, CompactData& _output) const;

public:
	MyVDBLoader();
	void Load(const std::string& _file, CompactData& _output) const;
};