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
#include "utils.h"
namespace fs = std::filesystem;
using GridType = openvdb::FloatGrid;
using TreeType = GridType::TreeType;

namespace {
	void _PrintUint(const std::vector<uint8_t>& _data, size_t& _offset, std::ostream& os)
	{
		os << *(uint32_t*)(_data.data() + _offset);
		_offset += 4;
	}

	void _PrintInt(const std::vector<uint8_t>& _data, size_t& _offset, std::ostream& os)
	{
		os << *(int32_t*)(_data.data() + _offset);
		_offset += 4;
	}

	void _PrintUvec2(const std::vector<uint8_t>& _data, size_t& _offset, std::ostream& os)
	{
		os << "[";
		_PrintUint(_data, _offset, os);
		os << ", ";
		_PrintUint(_data, _offset, os);
		os << "]";
	}

	void _PrintIvec2(const std::vector<uint8_t>& _data, size_t& _offset, std::ostream& os)
	{
		os << "[";
		_PrintInt(_data, _offset, os);
		os << ", ";
		_PrintInt(_data, _offset, os);
		os << "]";
	}

	void _PrintFloat(const std::vector<uint8_t>& _data, size_t& _offset, std::ostream& os)
	{
		_offset = common_utils::AlignUp(_offset, 4);
		os << *(float*)(_data.data() + _offset);
		_offset += 4;
	}

	void _PrintDouble(const std::vector<uint8_t>& _data, size_t& _offset, std::ostream& os)
	{
		_offset = common_utils::AlignUp(_offset, 8);
		os << *(double*)(_data.data() + _offset);
		_offset += 8;
	}

	void _PrintVec3(const std::vector<uint8_t>& _data, size_t& _offset, std::ostream& os)
	{
		os << "[";
		_PrintFloat(_data, _offset, os);
		os << ", ";
		_PrintFloat(_data, _offset, os);
		os << ", ";
		_PrintFloat(_data, _offset, os);
		os << "]";
	}

	void _PrintDvec3(const std::vector<uint8_t>& _data, size_t& _offset, std::ostream& os)
	{
		os << "[";
		_PrintDouble(_data, _offset, os);
		os << ", ";
		_PrintDouble(_data, _offset, os);
		os << ", ";
		_PrintDouble(_data, _offset, os);
		os << "]";
	}

	void _PrintMat3(const std::vector<uint8_t>& _data, size_t& _offset, std::ostream& os, int _indent = 2)
	{
		for (int i = 0; i < 3; ++i)
		{
			for (int j = 0; j < _indent; ++j)
			{
				os << "  ";
			}
			_PrintVec3(_data, _offset, os);
			os << std::endl;
		}
	}

	void _PrintDmat3(const std::vector<uint8_t>& _data, size_t& _offset, std::ostream& os, int _indent = 2)
	{
		for (int i = 0; i < 3; ++i)
		{
			for (int j = 0; j < _indent; ++j)
			{
				os << "  ";
			}
			_PrintDvec3(_data, _offset, os);
			os << std::endl;
		}
	}

	void _PrintGridData(const std::vector<uint8_t>& _data)
	{
		size_t offset = 0;
		std::cout << "Grid Data: \r\n";
		std::cout << "======================== \r\n";
		std::cout << "  magic: ";
		_PrintUvec2(_data, offset, std::cout);
		std::cout << std::endl;
		std::cout << "  checksum: ";
		_PrintUvec2(_data, offset, std::cout);
		std::cout << std::endl;
		std::cout << "  version: ";
		_PrintUint(_data, offset, std::cout);
		std::cout << std::endl;
		std::cout << "  flags: ";
		_PrintUint(_data, offset, std::cout);
		std::cout << std::endl;
		std::cout << "  grid index: ";
		_PrintUint(_data, offset, std::cout);
		std::cout << std::endl;
		std::cout << "  grid count: ";
		_PrintUint(_data, offset, std::cout);
		std::cout << std::endl;
		std::cout << "  grid size: ";
		_PrintUvec2(_data, offset, std::cout);
		std::cout << std::endl;
		offset += 256;

		std::cout << "  Map: \r\n";
		std::cout << "    matf:\r\n";
		_PrintMat3(_data, offset, std::cout);
		std::cout << "    invmatf:\r\n";
		_PrintMat3(_data, offset, std::cout);
		std::cout << "    vecf: ";
		_PrintVec3(_data, offset, std::cout);
		std::cout << std::endl;
		std::cout << "    typerf: ";
		_PrintFloat(_data, offset, std::cout);
		std::cout << std::endl;
		std::cout << "    matd:\r\n";
		_PrintDmat3(_data, offset, std::cout);
		std::cout << "    invmatd:\r\n";
		_PrintDmat3(_data, offset, std::cout);
		std::cout << "    vecd: ";
		_PrintDvec3(_data, offset, std::cout);
		std::cout << std::endl;
		std::cout << "    typerd: ";
		_PrintDouble(_data, offset, std::cout);
		std::cout << std::endl;

		std::cout << "  world bbox: \r\n";
		std::cout << "    ";
		_PrintDvec3(_data, offset, std::cout);
		std::cout << std::endl;
		std::cout << "    ";
		_PrintDvec3(_data, offset, std::cout);
		std::cout << std::endl;
		std::cout << "  voxel size: ";
		_PrintDvec3(_data, offset, std::cout);
		std::cout << std::endl;

		std::cout << " grid class: ";
		_PrintUint(_data, offset, std::cout);
		std::cout << std::endl;

		std::cout << " grid type: ";
		_PrintUint(_data, offset, std::cout);
		std::cout << std::endl;

		std::cout << "  blind meta data offset: ";
		_PrintIvec2(_data, offset, std::cout);
		std::cout << std::endl;
		std::cout << "  blind meta data count: ";
		_PrintUint(_data, offset, std::cout);
		std::cout << std::endl;
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
		_PrintGridData(_data.data);
	}
}

void MyVDBLoader::_ExportCompactDataFromGridHandle(const nanovdb::GridHandle<>& _handle, CompactData& _output) const
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

	CHECK_TRUE(pFloatGrid, "Failed to load float grid!");

	// nano vdb data
	_output.data.resize(_handle.size());
	memcpy(_output.data.data(), pFloatGrid, _handle.size());

	// data size
	_output.dataSizes =
	{
		uint32_t(pFloatGrid->tree().nodeCount<Node0T>() * pFloatGrid->tree().getFirstNode<0>()->memUsage()),
		uint32_t(pFloatGrid->tree().nodeCount<Node1T>() * pFloatGrid->tree().getFirstNode<1>()->memUsage()),
		uint32_t(pFloatGrid->tree().nodeCount<Node2T>() * pFloatGrid->tree().getFirstNode<2>()->memUsage()),
		uint32_t(pFloatGrid->tree().root().memUsage()),
		uint32_t(GridT::memUsage()) 
	};

	// data offset
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

	// bounding box
	_output.minBound = glm::vec3(pFloatGrid->worldBBox().min()[0], pFloatGrid->worldBBox().min()[1], pFloatGrid->worldBBox().min()[2]);
	_output.maxBound = glm::vec3(pFloatGrid->worldBBox().max()[0], pFloatGrid->worldBBox().max()[1], pFloatGrid->worldBBox().max()[2]);
	
	// max voxel value & min voxel value
	float minValue = std::numeric_limits<float>::max();
	float maxValue = std::numeric_limits<float>::lowest();
	for (uint32_t i = 0; i < pFloatGrid->tree().nodeCount<Node0T>(); ++i)
	{
		auto pLeaf = pFloatGrid->tree().getFirstLeaf() + i;
		for (auto voxelOn = pLeaf->cbeginValueOn(); voxelOn; voxelOn++)
		{
			float val = pFloatGrid->tree().getValue(voxelOn.getCoord()); 
			minValue = std::min(val, minValue);
			maxValue = std::max(val, maxValue);
		}
	}
	_output.maxValue = maxValue;
	_output.minValue = minValue;

	_PrintCompactData(_output);
}

void MyVDBLoader::_ExportNanoVDB(const std::string& _nvdbFile, CompactData& _output) const
{
	auto gridHandle = nanovdb::io::readGrid(_nvdbFile);
	const nanovdb::GridMetaData* pMetaData = nullptr;
	const nanovdb::FloatGrid* pFloatGrid = nullptr;

	CHECK_TRUE(gridHandle, "Failed to open nanovdb file!");

	_ExportCompactDataFromGridHandle(gridHandle, _output);
}

MyVDBLoader::MyVDBLoader()
{
	openvdb::initialize();
}

void MyVDBLoader::Load(const std::string& _file, CompactData& _output) const
{
	auto strExtension = common_utils::GetFileExtension(_file);
	std::string strPath;
	if (strExtension == "vdb")
	{
		strPath = _CreateNanoVDBFromOpenVDB(_file);
	}
	else if (strExtension == "nvdb")
	{
		strPath = _file;
	}
	else
	{
		CHECK_TRUE(false, "This is not a vdb file!");
	}

	_ExportNanoVDB(strPath, _output);
}

std::string MyVDBLoader::_CreateNanoVDBFromOpenVDB(const std::string& _openVDB) const
{
	CHECK_TRUE(common_utils::GetFileExtension(_openVDB) == "vdb", "This is not a openVDB file!");
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
		//srcGrid->setTransform(openvdb::math::Transform::createLinearTransform(0.05));
		auto handle = nanovdb::tools::createNanoGrid(*srcGrid); // Convert from OpenVDB to NanoVDB and return a shared pointer to a GridHandle.
		auto* dstGrid = handle.grid<float>();										// Get a (raw) pointer to the NanoVDB grid form the GridManager.

		if (!dstGrid)
		{
			throw std::runtime_error("GridHandle does not contain a grid with value type float");
		}

		nanovdb::io::writeGrid(nanoVDB, handle); // Write the NanoVDB grid to file and throw if writing fails
	}
	catch (const std::exception& e) 
	{
		std::cerr << "An exception occurred: \"" << e.what() << "\"" << std::endl;
	}

	file.close();

	return nanoVDB;
}
