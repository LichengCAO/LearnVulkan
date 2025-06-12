#pragma once
#include "common.h";
#include "buffer.h"

class RayTracingAccelerationStructure
{
private:
	class TLAS
	{

	};
	struct BLASInput
	{
		std::vector<VkAccelerationStructureGeometryKHR>			vkASGeometries;
		std::vector<VkAccelerationStructureBuildRangeInfoKHR>	vkASBuildRangeInfos;
		VkBuildAccelerationStructureFlagsKHR					vkBuildFlags = 0;
	};
	struct BLAS
	{
		std::unique_ptr<Buffer> uptrBuffer;
		VkAccelerationStructureKHR vkAccelerationStructure = VK_NULL_HANDLE;
		VkDeviceAddress vkDeviceAddress;

		~BLAS() { assert(vkAccelerationStructure == VK_NULL_HANDLE); };
		void Init(VkDeviceSize size);
		void Uninit();
	};

public:
	struct TriangleData
	{
		VkDeviceAddress vkDeviceAddressVertex;
		uint32_t		uVertexStride;
		uint32_t		uVertexCount;
		VkDeviceAddress vkDeviceAddressIndex;
		VkIndexType		vkIndexType;
		uint32_t		uIndexCount;
	};

private:
	std::vector<BLASInput> m_BLASInputs;
	std::vector<BLAS> m_BLASes;
	VkQueryPool       m_vkQueryPool = VK_NULL_HANDLE;

private:
	// Helper function to initialize scratch buffer, 
	// maxBudget is the maximum size buffer can be used to build AS concurrently
	// scratch buffer will be init inside,
	// scratch addresses are the slot addresses for each of the BLASes to build,
	// the number of slots may be smaller than the BLAS count, then several loops are required
	void _InitScratchBuffer(
		VkDeviceSize maxBudget,
		const std::vector<VkAccelerationStructureBuildSizesInfoKHR>& buildSizeInfo,
		Buffer& scratchBufferToInit,
		std::vector<VkDeviceAddress>& slotAddresses
	) const;

	// Build all BLASes, after all BLASInputs are added
	void _BuildBLASes();

	// Create m_vkQueryPool if not have one with given size, or reset current pool to given size
	void _PrepareQueryPool(uint32_t uQueryCount);

public:
	VkAccelerationStructureKHR vkAccelerationStructure = VK_NULL_HANDLE;
	~RayTracingAccelerationStructure() { assert(m_vkQueryPool == VK_NULL_HANDLE); assert(vkAccelerationStructure == VK_NULL_HANDLE); };

	// Add BLAS to this acceleration structure, 
	// return the index of this BLAS in the vector of BLASes this acceleration structure holds
	uint32_t AddBLAS(const std::vector<TriangleData>& geomData);
};