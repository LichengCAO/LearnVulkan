#pragma once
#include "common.h";
#include "buffer.h"

class RayTracingAccelerationStructure
{
private:
	struct TLASInput
	{
		VkAccelerationStructureGeometryKHR				vkASGeometry;
		VkAccelerationStructureBuildRangeInfoKHR		vkASBuildRangeInfo;
		VkBuildAccelerationStructureFlagsKHR			vkBuildFlags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;

		std::unique_ptr<Buffer>							uptrInstanceBuffer;
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

		BLAS() {};
		BLAS(BLAS&& other) { 
			uptrBuffer = std::move(other.uptrBuffer);
			vkAccelerationStructure = other.vkAccelerationStructure;
			vkDeviceAddress = other.vkDeviceAddress;
		};
		~BLAS() { assert(vkAccelerationStructure == VK_NULL_HANDLE); };

		// create BLAS not build it
		void Init(VkDeviceSize size);
		void Uninit();
	};
	struct TLAS {
		std::unique_ptr<Buffer> uptrBuffer;
		VkAccelerationStructureKHR vkAccelerationStructure = VK_NULL_HANDLE;
		VkDeviceAddress vkDeviceAddress;

		~TLAS() { assert(vkAccelerationStructure == VK_NULL_HANDLE); };
		
		// create TLAS not build it
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
	struct InstanceData
	{
		uint32_t	uBLASIndex;
		uint32_t	uHitShaderGroupIndex;
		glm::mat4   transformMatrix;
		// i just use the index of input array for gl_InstanceCustomIndex;
	};

private:
	std::vector<BLASInput>	m_BLASInputs; // hold info temporarily, will be invalid after Init
	std::vector<BLAS>		m_BLASs;
	TLASInput				m_TLASInput; // hold info temporarily, will be invalid after Init
	TLAS					m_TLAS;
	VkQueryPool				m_vkQueryPool = VK_NULL_HANDLE;

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

	// Build all BLASs, after all BLASInputs are added
	void _BuildBLASs();

	// Create m_vkQueryPool if not have one with given size, or reset current pool to given size
	void _PrepareQueryPool(uint32_t uQueryCount);

	// Build TLAS
	void _BuildTLAS();

public:
	VkAccelerationStructureKHR vkAccelerationStructure = VK_NULL_HANDLE;
	
	~RayTracingAccelerationStructure() { assert(m_vkQueryPool == VK_NULL_HANDLE); assert(vkAccelerationStructure == VK_NULL_HANDLE); };

	// Add a BLAS to this acceleration structure, 
	// return the index of this BLAS in the vector of BLASs this acceleration structure holds
	uint32_t AddBLAS(const std::vector<TriangleData>& geomData);

	// Set up TLAS after BLASs are all added, TLAS will not be created or be built, we do it in Init
	void SetUpTLAS(const std::vector<InstanceData>& instData);
	
	// Build AS after all BLASs are added and TLAS's setup
	void Init();

	void Uninit();
};