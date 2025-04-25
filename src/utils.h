#pragma once
#include "geometry.h"
#include "transform.h"
#include <tiny_obj_loader.h>

struct Mesh
{
	std::vector<Vertex> verts;
	std::vector<uint32_t> indices;
};

class MeshLoader
{
private:
	static void _OptimizeMesh(Mesh& mesh);
public:
	static bool Load(const std::string& objFile, std::vector<Mesh>& outMesh);
};
