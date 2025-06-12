#include "ray_tracing.h"
#include "device.h"
#include "buffer.h"
#include "commandbuffer.h"
#include "utils.h"

void RayTracingAccelerationStructure::_InitScratchBuffer(
	VkDeviceSize maxBudget,
	const std::vector<VkAccelerationStructureBuildSizesInfoKHR>& buildSizeInfo,
	Buffer& scratchBufferToInit,
	std::vector<VkDeviceAddress>& slotAddresses
) const
{
	BufferInformation scratchBufferInfo{};
	uint32_t	uMinAlignment = 128; /*m_rtASProperties.minAccelerationStructureScratchOffsetAlignment*/
	VkDeviceSize maxScratchChunk = 0;
	VkDeviceSize fullScratchSize = 0;
	uint64_t	uSlotCount = 0;
	scratchBufferInfo.usage = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
	scratchBufferInfo.memoryProperty = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

	// setup maxScratchChunk, fullScratchSize
	for (const auto& sizeInfo : buildSizeInfo)
	{
		VkDeviceSize chunkSizeAligned = CommonUtils::AlignUp(sizeInfo.buildScratchSize, uMinAlignment);

		maxScratchChunk = std::max(maxScratchChunk, chunkSizeAligned);
		fullScratchSize += chunkSizeAligned;
	}
	CHECK_TRUE(maxBudget >= maxScratchChunk, "Max Budget is too small!");

	// Find the scratch buffer size
	if (fullScratchSize < maxBudget)
	{
		scratchBufferInfo.size = fullScratchSize;
		uSlotCount = buildSizeInfo.size();
	}
	else
	{
		uint64_t nChunkCount = std::min(buildSizeInfo.size(), maxBudget / maxScratchChunk);
		scratchBufferInfo.size = nChunkCount * maxScratchChunk;
		uSlotCount = nChunkCount;
	}

	scratchBufferToInit.Init(scratchBufferInfo);

	// fill slotAddresses
	slotAddresses.reserve(uSlotCount);
	if (fullScratchSize < maxBudget)
	{
		VkDeviceAddress curAddress = scratchBufferToInit.GetDeviceAddress();
		for (int i = 0; i < buildSizeInfo.size(); ++i)
		{
			const auto& sizeInfo = buildSizeInfo[i];
			VkDeviceSize chunkSizeAligned = CommonUtils::AlignUp(sizeInfo.buildScratchSize, uMinAlignment);

			slotAddresses[i] = curAddress;
			curAddress += chunkSizeAligned;
		}
	}
	else
	{
		VkDeviceAddress curAddress = scratchBufferToInit.GetDeviceAddress();
		for (int i = 0; i < uSlotCount; ++i)
		{
			slotAddresses.push_back(curAddress);	
			curAddress += maxScratchChunk;
		}
	}
}

void RayTracingAccelerationStructure::_BuildBLASes()
{
	VkBuildAccelerationStructureFlagsKHR vkBuildASFlags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR | VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_COMPACTION_BIT_KHR;
	VkDeviceSize maxBudget = 256'000'000;
	std::vector<VkAccelerationStructureBuildGeometryInfoKHR> buildGeomInfos;
	std::vector<VkAccelerationStructureBuildSizesInfoKHR> buildSizeInfos;
	std::vector<VkDeviceAddress> scratchAddresses;
	Buffer scratchBuffer{};
	bool bNeedCompact = false;
	int  nAllBLASCount = m_BLASInputs.size();

	// setup scratch buffer that doesn't exceed maxBudget
	buildGeomInfos.reserve(nAllBLASCount);
	buildSizeInfos.reserve(nAllBLASCount);
	for (int i = 0; i < nAllBLASCount; ++i)
	{
		std::vector<uint32_t> maxPrimCount(m_BLASInputs[i].vkASBuildRangeInfos.size());
		VkAccelerationStructureBuildSizesInfoKHR	buildSizeInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR };
		VkAccelerationStructureBuildGeometryInfoKHR buildGeomInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR };
		buildGeomInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
		buildGeomInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
		buildGeomInfo.flags = vkBuildASFlags  |  m_BLASInputs[i].vkBuildFlags;
		buildGeomInfo.geometryCount = static_cast<uint32_t>(m_BLASInputs[i].vkASGeometries.size());
		buildGeomInfo.pGeometries = m_BLASInputs[i].vkASGeometries.data();
		buildGeomInfo.ppGeometries = nullptr;
		buildGeomInfo.srcAccelerationStructure = VK_NULL_HANDLE;
		buildGeomInfo.dstAccelerationStructure = VK_NULL_HANDLE; // we don't have AS yet, so we do this later, see Task1
		// buildGeomInfo.scratchData: we don't have scratch buffer yet, so we do this later, see Task2
		
		// setup maxPrimCount
		for (int j = 0; j < m_BLASInputs[i].vkASBuildRangeInfos.size(); ++j)
		{
			maxPrimCount[j] = m_BLASInputs[i].vkASBuildRangeInfos[j].primitiveCount;
		}
		
		// https://registry.khronos.org/vulkan/specs/latest/man/html/vkGetAccelerationStructureBuildSizesKHR.html
		assert(maxPrimCount.size() == buildGeomInfo.geometryCount);
		vkGetAccelerationStructureBuildSizesKHR(MyDevice::GetInstance().vkDevice, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &buildGeomInfo, maxPrimCount.data(), &buildSizeInfo);

		bNeedCompact = (buildGeomInfo.flags & VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_COMPACTION_BIT_KHR) || bNeedCompact;

		buildGeomInfos.push_back(buildGeomInfo);
		buildSizeInfos.push_back(buildSizeInfo);
	}
	_InitScratchBuffer(maxBudget, buildSizeInfos, scratchBuffer, scratchAddresses);

	// Task2: now we have scratch buffer, we can set scratch data for buildGeomInfos
	for (int i = 0; i < nAllBLASCount; ++i)
	{
		auto& buildGeomInfo = buildGeomInfos[i];
		buildGeomInfo.scratchData.deviceAddress = scratchAddresses[i % scratchAddresses.size()];
	}

	// create BLASes and build them
	{
		int BLASIndex = 0;
		uint32_t queryIndex = 0u;
		int loopCount = (nAllBLASCount + scratchAddresses.size() - 1) / scratchAddresses.size();
		CommandSubmission cmd{};
		
		// prepare query pool
		if (bNeedCompact)
		{
			_PrepareQueryPool(nAllBLASCount);
		}

		cmd.Init();

		for (int i = 0; i < loopCount; ++i)
		{
			VkMemoryBarrier vkMemBarrier{ VK_STRUCTURE_TYPE_MEMORY_BARRIER };
			std::vector<VkAccelerationStructureKHR> BLASesToProcess; // stores the created but not built yet BLASes
			std::vector<const VkAccelerationStructureBuildRangeInfoKHR*> buildRangeInfosToProcess;
			std::vector<VkAccelerationStructureBuildGeometryInfoKHR> buildGeomInfosToProcess;
			std::vector<BLAS> BLASesInitInThisIter;

			BLASesToProcess.reserve(nAllBLASCount);
			buildRangeInfosToProcess.reserve(nAllBLASCount);
			buildGeomInfosToProcess.reserve(nAllBLASCount);
			BLASesInitInThisIter.reserve(nAllBLASCount);

			// try to create as many BLASes as scratch buffer can hold
			for (int j = 0; j < scratchAddresses.size() && BLASIndex < nAllBLASCount; ++j)
			{
				BLAS curBLAS{};

				// create BLAS
				curBLAS.Init(buildSizeInfos[BLASIndex].accelerationStructureSize);

				// Task1: now we have AS, we can set dstAS for buildGeom
				buildGeomInfos[BLASIndex].dstAccelerationStructure = curBLAS.vkAccelerationStructure;

				// use scratch buffer to build BLAS
				BLASesToProcess.push_back(curBLAS.vkAccelerationStructure);
				buildRangeInfosToProcess.push_back(m_BLASInputs[BLASIndex].vkASBuildRangeInfos.data());
				buildGeomInfosToProcess.push_back(buildGeomInfos[BLASIndex]);
				BLASesInitInThisIter.push_back(std::move(curBLAS));

				BLASIndex++;
			}

			// this will wait till previous commands be done, so that scratch buffer is available
			cmd.StartCommands({});

			// build BLASes
			cmd.BuildAccelerationStructures(buildGeomInfosToProcess, buildRangeInfosToProcess);

			// wait build to be done
			vkMemBarrier.srcAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
			vkMemBarrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;
			cmd.AddPipelineBarrier(VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, { vkMemBarrier });

			// store compact info in the query pool
			if (bNeedCompact)
			{
				cmd.WriteAccelerationStructuresProperties(BLASesToProcess, VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_KHR, m_vkQueryPool, queryIndex);
			}

			cmd.SubmitCommands();

			// compact BLAS
			if (bNeedCompact)
			{
				std::vector<VkDeviceSize> compactSizes(BLASesToProcess.size());

				// wait query pool write to be done
				cmd.WaitTillAvailable();
				vkGetQueryPoolResults(
					MyDevice::GetInstance().vkDevice,
					m_vkQueryPool,
					queryIndex,
					static_cast<uint32_t>(compactSizes.size()),
					compactSizes.size() * sizeof(VkDeviceSize),
					compactSizes.data(),
					sizeof(VkDeviceSize),
					VK_QUERY_RESULT_WAIT_BIT
				);

				cmd.StartCommands({});

				for (int i = queryIndex; i < BLASIndex; ++i)
				{
					size_t localIndex = i - queryIndex;
					VkDeviceSize compactSize = compactSizes[localIndex];
					if (compactSize > 0)
					{
						BLAS compactBLAS{};
						VkCopyAccelerationStructureInfoKHR copyInfo{ VK_STRUCTURE_TYPE_COPY_ACCELERATION_STRUCTURE_INFO_KHR };

						compactBLAS.Init(compactSize);

						copyInfo.pNext = nullptr;
						copyInfo.mode = VK_COPY_ACCELERATION_STRUCTURE_MODE_COMPACT_KHR;
						copyInfo.src = BLASesInitInThisIter[localIndex].vkAccelerationStructure;
						copyInfo.dst = compactBLAS.vkAccelerationStructure;

						cmd.CopyAccelerationStructure(copyInfo);
						m_BLASes.push_back(std::move(compactBLAS));
					}
				}
				queryIndex = BLASIndex;

				cmd.SubmitCommands();
			}

			// wait build to be finished
			cmd.WaitTillAvailable();

			// process BLASes Init in this iter
			if (bNeedCompact)
			{
				// we already push the compacted BLAS copy into the m_BLASes, therefore BLASes in BLASesInitInThisIter is useless
				for (auto& uselessBLAS : BLASesInitInThisIter)
				{
					uselessBLAS.Uninit();
				}
				BLASesInitInThisIter.clear();
			}
			else
			{
				for (int i = 0; i < BLASesInitInThisIter.size(); ++i)
				{
					m_BLASes.push_back(std::move(BLASesInitInThisIter[i]));
				}
			}
		}

		cmd.Uninit();
	}
	
	scratchBuffer.Uninit();
}

void RayTracingAccelerationStructure::_PrepareQueryPool(uint32_t uQueryCount)
{
	VkDevice vkDevice = MyDevice::GetInstance().vkDevice;
	if (m_vkQueryPool == VK_NULL_HANDLE)
	{
		VkQueryPoolCreateInfo qpci = { VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO };
		qpci.queryType = VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_KHR;
		qpci.queryCount = uQueryCount;
		vkCreateQueryPool(vkDevice, &qpci, nullptr, &m_vkQueryPool);
	}
	
	if (m_vkQueryPool)
	{
		vkResetQueryPool(vkDevice, m_vkQueryPool, 0, uQueryCount);
	}
}

uint32_t RayTracingAccelerationStructure::AddBLAS(const std::vector<TriangleData>& geomData)
{
	uint32_t uRet = static_cast<uint32_t>(m_BLASInputs.size());
	BLASInput blas{};

	for (const auto& geom : geomData)
	{
		VkAccelerationStructureGeometryTrianglesDataKHR triangles{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR };
		triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
		triangles.vertexData.deviceAddress = geom.vkDeviceAddressVertex;
		triangles.vertexStride = geom.uVertexStride;
		triangles.indexType = geom.vkIndexType;
		triangles.indexData.deviceAddress = geom.vkDeviceAddressIndex;
		triangles.transformData = {};
		triangles.maxVertex = geom.uVertexCount - 1;
		triangles.pNext = nullptr;

		VkAccelerationStructureGeometryKHR asGeom{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR };
		asGeom.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
		asGeom.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
		asGeom.geometry.triangles = triangles;
		asGeom.pNext = nullptr;

		VkAccelerationStructureBuildRangeInfoKHR offset;
		offset.firstVertex = 0u;
		offset.primitiveOffset = 0u;
		offset.primitiveCount = geom.uIndexCount / 3u;
		offset.transformOffset = 0u;

		blas.vkASGeometries.push_back(asGeom);
		blas.vkASBuildRangeInfos.push_back(offset);
	}

	m_BLASInputs.push_back(blas);

    return uRet;
}

void RayTracingAccelerationStructure::BLAS::Init(VkDeviceSize size)
{
	BufferInformation bufferInfo{};
	VkAccelerationStructureCreateInfoKHR info{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR };
	VkAccelerationStructureDeviceAddressInfoKHR vkASAddrInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR };

	info.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
	info.size = size;
	uptrBuffer = std::make_unique<Buffer>();

	bufferInfo.memoryProperty = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	bufferInfo.usage = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
	bufferInfo.size = info.size;

	uptrBuffer->Init(bufferInfo);
	info.buffer = uptrBuffer->vkBuffer;

	vkCreateAccelerationStructureKHR(MyDevice::GetInstance().vkDevice, &info, nullptr, &vkAccelerationStructure);

	vkASAddrInfo.accelerationStructure = vkAccelerationStructure;
	vkASAddrInfo.pNext = nullptr;
	vkDeviceAddress = vkGetAccelerationStructureDeviceAddressKHR(MyDevice::GetInstance().vkDevice, &vkASAddrInfo);
}

void RayTracingAccelerationStructure::BLAS::Uninit()
{
	if (uptrBuffer.get() != nullptr)
	{
		uptrBuffer->Uninit();
		uptrBuffer.reset();
	}
	if (vkAccelerationStructure != VK_NULL_HANDLE)
	{
		vkDestroyAccelerationStructureKHR(MyDevice::GetInstance().vkDevice, vkAccelerationStructure, nullptr);
		vkAccelerationStructure = VK_NULL_HANDLE;
	}
}
