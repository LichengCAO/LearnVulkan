#include "vdb_loader.h"
#include <openvdb/openvdb.h>
#include <openvdb/tools/Dense.h>
#include <openvdb/Metadata.h>
#include <filesystem>
namespace fs = std::filesystem;

void MyVDBLoader::Load(const std::string& _path)
{
	if (!fs::exists(fs::path(_path))) {
		return;
	}

	if (_path.empty())
	{
		return;
	}

	openvdb::initialize();
	openvdb::io::File file(_path);
	file.open();

	auto grids = file.readAllGridMetadata();
	for (auto grid : *grids) {
		std::cout << "Grid " << grid->getName() << " " << grid->valueType() << std::endl;
	}

	file.close();
}
