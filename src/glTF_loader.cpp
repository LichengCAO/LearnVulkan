#include "glTF_loader.h"

void glTFContent::_LoadFile(const std::string& _file, tinygltf::Model& _out)
{
	tinygltf::TinyGLTF tloader;
	std::string strErr;
	std::string strWarn;
	bool bResult = tloader.LoadASCIIFromFile(&_out, &strErr, &strWarn, _file);
	if (!strWarn.empty())
	{
		std::cout << "WARNING: " << strWarn << std::endl;
	}
	if (!strErr.empty())
	{
		std::cout << "ERROR: " << strErr << std::endl;
	}
	if (!bResult)
	{
		std::cout << "Failed to load glTF: " << _file << std::endl;
	}
	else
	{
		std::cout << "Loaded glTF: " << _file << std::endl;
	}
}

void glTFContent::Load(const std::string& _glTFPath)
{
	tinygltf::Model root;

	_LoadFile(_glTFPath, root);
}
