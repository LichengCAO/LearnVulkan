#include "vdb_loader.h"
#include <openvdb/openvdb.h>
#include <openvdb/tools/Dense.h>
#include <openvdb/Metadata.h>
#include <openvdb/tools/LevelSetSphere.h> // replace with your own dependencies for generating the OpenVDB grid
#include <nanovdb/tools/CreateNanoGrid.h> // converter from OpenVDB to NanoVDB (includes NanoVDB.h and GridManager.h)
#include <nanovdb/io/IO.h>
#include <nanovdb/util/NodeManager.h>
#include <nanovdb/NanoVDB.h>

#include <tbb/info.h>
#include <filesystem>
#include "common.h"
namespace fs = std::filesystem;
using GridType = openvdb::FloatGrid;
using TreeType = GridType::TreeType;

namespace {
	template<typename T>
	glm::mat4 _ConvertMat4(const T& _openvdbMat4)
	{
		glm::mat4 ret{};

		for (int row = 0; row < 4; ++row) {
			for (int col = 0; col < 4; ++col) {
				ret[col][row] = static_cast<float>(_openvdbMat4(col , row));
			}
		}

		return ret;
	}

	inline int _Convert8x8x8(int _x, int _y, int _z)
	{
		return _z + _y * 8 + _x * 64;
	}

	void _UpdateBitmask(uint32_t _x, uint32_t _y, uint32_t _z, uint32_t _dim, std::vector<uint8_t>& _toUpdate)
	{
		uint32_t bitPos = _z + _dim * _y + _dim * _dim * _x;
		uint32_t vecIndex = bitPos / 8;
		uint32_t bitMove = bitPos % 8;
		_toUpdate[vecIndex] |= static_cast<uint8_t>(1 << bitMove);
	}

	bool _EndWith(const std::string& _path, const std::string& _suffix)
	{
		return _path.size() >= _suffix.size()  && _path.substr(_path.size() - 4) ==  _suffix;
	}

	void _PrintCompactData(const MyVDBLoader::CompactData& _data)
	{
		std::cout << "Compact data information: " << std::endl;
		std::cout << "=====================" << std::endl;
		std::cout << "data size: " << _data.data.size() << std::endl;
		for (int i = 4; i >= 0; --i)
		{
			std::cout << "level " << i << ": " << std::endl;
			std::cout << "    offset: " << _data.offsets[i] << std::endl;
			std::cout << "    size: " << _data.dataSizes[i] << std::endl;
		}
	}
}

void MyVDBLoader::_ExportCompactDataFromGridHandle(const nanovdb::GridHandle<>& _handle, CompactData& _output)
{
	using GridT = nanovdb::NanoGrid<float>;
	using TreeT = GridT::TreeType;
	using RootT = TreeT::RootType;
	using Node2T = RootT::ChildNodeType;
	using Node1T = Node2T::ChildNodeType;
	using Node0T = Node1T::ChildNodeType;

	auto pFloatGrid = _handle.grid<float>();
	auto managerHandle = nanovdb::createNodeManager(*pFloatGrid);
	auto pManager = managerHandle.mgr<float>();

	_output.data.resize(_handle.size());
	memcpy(_output.data.data(), pFloatGrid, _handle.size());

	_output.dataSizes =
	{
		uint32_t(pFloatGrid->tree().nodeCount<Node0T>() * pFloatGrid->tree().getFirstNode<0>()->memUsage()),
		uint32_t(pFloatGrid->tree().nodeCount<Node1T>() * pFloatGrid->tree().getFirstNode<1>()->memUsage()),
		uint32_t(pFloatGrid->tree().nodeCount<Node2T>() * pFloatGrid->tree().getFirstNode<2>()->memUsage()),
		uint32_t(pFloatGrid->tree().root().memUsage()),
		uint32_t(GridT::memUsage()) 
	};

	{
		// auto* pLevel0 = &pManager->leaf(0);
		// auto* explicitly tells the compiler to deduce the type as a pointer, 
		// so ret will also have the type T*, it is identical to simply use auto:
		auto pLevel0 = &pManager->leaf(0);
		auto pLevel1 = &pManager->lower(0);
		auto pLevel2 = &pManager->upper(0);
		auto pRoot = &(pFloatGrid->tree().root());
		auto pGrid = pFloatGrid;
		uintptr_t baseAddr = uintptr_t(pGrid);

		_output.offsets =
		{
			uint32_t(uintptr_t(pLevel0) - baseAddr),
			uint32_t(uintptr_t(pLevel1) - baseAddr),
			uint32_t(uintptr_t(pLevel2) - baseAddr),
			uint32_t(uintptr_t(pRoot) - baseAddr),
			uint32_t(uintptr_t(pGrid) - baseAddr),
		};
	}

	_PrintCompactData(_output);
}

MyVDBLoader::MyVDBLoader()
{
	openvdb::initialize();
}

void MyVDBLoader::Load(const std::string& _path)
{
	if (!fs::exists(fs::path(_path))) {
		return;
	}

	if (_path.empty())
	{
		return;
	}

	openvdb::io::File file(_path);
	file.open();
	for (auto nameIter = file.beginName(); nameIter != file.endName(); ++nameIter) 
	{
		openvdb::GridBase::Ptr baseGrid = file.readGrid(nameIter.gridName());

		baseGrid->pruneGrid();
		
		// Ensure it's a known type (e.g., FloatGrid)
		if (baseGrid->isType<openvdb::FloatGrid>()) {
			openvdb::FloatGrid::Ptr floatGrid = openvdb::gridPtrCast<openvdb::FloatGrid>(baseGrid);
			const auto& model = _ConvertMat4(floatGrid->transform().baseMap()->getAffineMap()->getMat4());
			const auto& vdbTree = floatGrid->tree();

			std::cout << "Grid Name: " << nameIter.gridName() << ", Leaf Count = " << floatGrid->tree().leafCount() << std::endl;
			std::cout << "Tree depth: " << vdbTree.treeDepth() << std::endl;
			std::vector<int> nodeCount(vdbTree.treeDepth(), 0);
			for (openvdb::FloatGrid::TreeType::NodeCIter iter = floatGrid->tree(); iter; ++iter)
			{
				nodeCount[iter.getLevel()]++;
			}
			int level = 0;
			for (auto& nCount : nodeCount)
			{
				std::cout << "Node Count at level " << level << " is " << nodeCount[level] << std::endl;
				level++;
			}
		}
	}

	file.close();
}

void MyVDBLoader::LoadToLevel1(const std::string& _path, Level1Data& _output)
{
	openvdb::GridBase::Ptr baseGrid{};

	if (!fs::exists(fs::path(_path))) {
		return;
	}

	if (_path.empty())
	{
		return;
	}

	openvdb::io::File file(_path);
	file.open();

	// we only process the first grid
	auto nameIter = file.beginName();
	CHECK_TRUE(nameIter != file.endName(), "No grid is stored in the VDB!");
	
	// Ensure it's a known type (e.g., FloatGrid)
	baseGrid = file.readGrid(nameIter.gridName());
	baseGrid->pruneGrid();
	CHECK_TRUE(baseGrid->isType<openvdb::FloatGrid>(), "Grid type is not supported!");

	{
		openvdb::FloatGrid::Ptr floatGrid = openvdb::gridPtrCast<openvdb::FloatGrid>(baseGrid);
		std::vector<int> nodeCount;
		const auto& vdbTree = floatGrid->tree();

		// store grid model matrix
		_output.indexToWorld = _ConvertMat4(floatGrid->transform().baseMap()->getAffineMap()->getMat4());
		
		std::cout << "Grid Name: " << nameIter.gridName() << ", Leaf Count = " << floatGrid->tree().leafCount() << std::endl;
		std::cout << "Tree depth: " << vdbTree.treeDepth() << std::endl;
		
		// we only process depth 4 vdb for now
		CHECK_TRUE(vdbTree.treeDepth() == 4, "VDB of this tree depth is not supported!");
		
		nodeCount.resize(vdbTree.treeDepth(), 0);
		for (openvdb::FloatGrid::TreeType::NodeCIter iter = floatGrid->tree(); iter; ++iter)
		{
			nodeCount[iter.getLevel()]++;
		}
		for (int i = vdbTree.treeDepth() - 1; i >= 0; i--)
		{
			std::cout << "Node Count at level " << i << " is " << nodeCount[i] << std::endl;
		}

		// child of root is at level 2
		for (auto iter2 = vdbTree.cbeginRootChildren(); iter2; ++iter2)
		{
			// skip inactive level 2 nodes
			if (iter2->childCount() < 1) continue;

			// iterate active level 1 nodes
			for (auto iter1 = iter2->cbeginChildOn(); iter1; ++iter1)
			{
				Pool0Data pool0{};
				const auto& bbox = iter1->getNodeBoundingBox();
				pool0.level = iter1->LEVEL;
				pool0.index = glm::vec3(bbox.min().x(), bbox.min().y(), bbox.min().z());
				pool0.parentIndex = ~0;
				pool0.childOffset = _output.level0Pool0.size();
				pool0.activeChild.resize((1 << 12) / 8, 0);

				// skip level 0 nodes that doesn't have value
				for (auto iter0 = iter1->cbeginChildOn(); iter0; ++iter0)
				{
					Pool0Data leafPool0{};
					std::array<float, 512> voxelData;

					leafPool0.level = 0;
					leafPool0.index = glm::vec3(iter0.getCoord().x(), iter0.getCoord().y(), iter0.getCoord().z());
					leafPool0.parentIndex = _output.level1Pool0.size(); // parent index in level 1 pool0
					leafPool0.childOffset = _output.voxelData.size();	// offset of voxel value
					// leafPool0.activeChild; we store active voxel in an atlas
					
					// we fetch 8x8x8 voxel values and store it in the _output
					for (int x = 0; x < 8; ++x)
					{
						for (int y = 0; y < 8; ++y)
						{
							for (int z = 0; z < 8; ++z)
							{
								voxelData[_Convert8x8x8(x, y, z)] = vdbTree.getValue(iter0.getCoord().offsetBy(x, y, z));
							}
						}
					}
					_output.voxelData.push_back(voxelData);

					// store data to level 0 pool0 
					_output.level0Pool0.push_back(leafPool0);

					// set bitmask to show that this child is active
					uint32_t _x = (iter0.getCoord().x() - bbox.min().x()) / 8; // node index compared to level
					uint32_t _y = (iter0.getCoord().y() - bbox.min().y()) / 8; // node index compared to level
					uint32_t _z = (iter0.getCoord().z() - bbox.min().z()) / 8; // node index compared to level
					_UpdateBitmask(_x, _y, _z, 16, pool0.activeChild);
				}

				// store level 1 pool0
				_output.level1Pool0.push_back(pool0);
			}
		}
	}

	file.close();
}

void MyVDBLoader::_CreateNanoVDBFromOpenVDB(const std::string& _openVDB)
{
	CHECK_TRUE(_EndWith(_openVDB, ".vdb"), "This is not a openVDB file!");
	std::string nanoVDB = _openVDB.substr(0, _openVDB.size() - 4) + ".nvdb";
	openvdb::io::File file(_openVDB);
	
	file.open();

	// we only process the first grid
	auto nameIter = file.beginName();
	CHECK_TRUE(nameIter != file.endName(), "No grid is stored in the VDB!");

	// Ensure it's a known type (e.g., FloatGrid)
	auto baseGrid = file.readGrid(nameIter.gridName());
	baseGrid->pruneGrid();
	CHECK_TRUE(baseGrid->isType<openvdb::FloatGrid>(), "Grid type is not supported!");

	try
	{
		auto srcGrid = openvdb::gridPtrCast<openvdb::FloatGrid>(baseGrid);
		auto handle = nanovdb::tools::createNanoGrid(*srcGrid); // Convert from OpenVDB to NanoVDB and return a shared pointer to a GridHandle.
		auto* dstGrid = handle.grid<float>();										// Get a (raw) pointer to the NanoVDB grid form the GridManager.
		
		if (!dstGrid)
		{
			throw std::runtime_error("GridHandle does not contain a grid with value type float");
		}
		CompactData deviceData{};
		_ExportCompactDataFromGridHandle(handle, deviceData);

		nanovdb::io::writeGrid(nanoVDB, handle); // Write the NanoVDB grid to file and throw if writing fails
	}
	catch (const std::exception& e) {
		std::cerr << "An exception occurred: \"" << e.what() << "\"" << std::endl;
	}

	file.close();
}
