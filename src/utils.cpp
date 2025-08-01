#include "utils.h"
#include <meshoptimizer.h>
#include <tiny_gltf.h>
#include <numeric>
#include "transform.h"

bool MeshUtility::Load(const std::string& objFile, std::vector<Mesh>& outMesh)
{
	bool bSuccess = false;
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warn, err;
	CHECK_TRUE(tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, objFile.c_str()), warn + err);
	
	for (const auto& shape : shapes)
	{
		Mesh mesh{};
		std::unordered_map<Vertex, uint32_t> uniqueVertices{};

		for (const auto& index : shape.mesh.indices)
		{
			Vertex vertex{};
			vertex.position = {
				attrib.vertices[3 * index.vertex_index + 0],
				attrib.vertices[3 * index.vertex_index + 1],
				attrib.vertices[3 * index.vertex_index + 2]
			};
			if (index.texcoord_index != -1)
			{
				vertex.uv = {
					attrib.texcoords[2 * index.texcoord_index + 0],
					1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
				};
			}
			if (index.normal_index != -1)
			{
				vertex.normal = {
					attrib.normals[3 * index.normal_index + 0],
					attrib.normals[3 * index.normal_index + 1],
					attrib.normals[3 * index.normal_index + 2],
				};
			}
			mesh.indices.push_back(static_cast<uint32_t>(mesh.verts.size()));
			mesh.verts.push_back(vertex);
		}

		// apply optimizer
		_OptimizeMesh(mesh);
		outMesh.push_back(mesh);
	}
	bSuccess = true;
	return bSuccess;
}

void MeshUtility::BuildMeshlets(
	const Mesh& inMesh,
	std::vector<Meshlet>& outMeshlets,
	std::vector<uint32_t>& outMeshletVertices,
	std::vector<uint8_t>& outMeshletTriangles
)
{
	const std::vector<uint32_t>& indices = inMesh.indices;
	const std::vector<Vertex>& vertices = inMesh.verts;
	const float coneWeight = 0.0f;
	const size_t maxIndices = 64;
	const size_t maxTriangles = 124; // must be divisible by 4
	std::vector<meshopt_Meshlet> meshoptMeshlets;
	size_t maxMeshlets = meshopt_buildMeshletsBound(indices.size(), maxIndices, maxTriangles);
	size_t meshletCount = 0;
	
	meshoptMeshlets.resize(maxMeshlets, meshopt_Meshlet{});
	outMeshletVertices.resize(maxMeshlets * maxIndices, 0);
	outMeshletTriangles.resize(maxMeshlets * maxTriangles * 3, 0);

	meshletCount = meshopt_buildMeshlets(
		meshoptMeshlets.data(),
		static_cast<unsigned int*>(outMeshletVertices.data()),
		static_cast<unsigned char*>(outMeshletTriangles.data()),
		indices.data(),
		indices.size(),
		reinterpret_cast<const float*>(vertices.data()),
		vertices.size(),
		sizeof(Vertex),
		maxIndices,
		maxTriangles,
		coneWeight
	);
	meshoptMeshlets.resize(meshletCount);
	outMeshletVertices.resize(meshoptMeshlets.back().vertex_offset + meshoptMeshlets.back().vertex_count);
	// This expression is performing an operation that ensures the result is rounded up to the next multiple of 4.
	// & ~3 Performing a bitwise AND (&) with this value effectively clears the last two bits of the number, aligning it down to a multiple of 4.
	outMeshletTriangles.resize(meshoptMeshlets.back().triangle_offset + ((meshoptMeshlets.back().triangle_count * 3 + 3) & ~3));

	// further optimize meshlets
	for (const auto& m : meshoptMeshlets)
	{
		Meshlet outMeshlet{};

		outMeshlet.triangleCount = m.triangle_count;
		outMeshlet.triangleOffset = m.triangle_offset;
		outMeshlet.vertexCount = m.vertex_count;
		outMeshlet.vertexOffset = m.vertex_offset;
		meshopt_optimizeMeshlet(
			static_cast<unsigned int*>(&outMeshletVertices[m.vertex_offset]), 
			static_cast<unsigned char*>(&outMeshletTriangles[m.triangle_offset]),
			m.triangle_count,
			m.vertex_count
		);
		outMeshlets.push_back(outMeshlet);
	}
}

void MeshUtility::BuildMeshlets(
	const Mesh& inMesh,
	std::vector<Meshlet>& outMeshlets,
	std::vector<MeshletBounds>& outMeshletBounds,
	std::vector<uint32_t>& outMeshletVertices,
	std::vector<uint8_t>& outMeshletTriangles
)
{
	size_t offset = outMeshlets.size();
	
	BuildMeshlets(inMesh, outMeshlets, outMeshletVertices, outMeshletTriangles);
	outMeshletBounds.reserve(outMeshletBounds.size() + outMeshlets.size() - offset);
	for (int i = offset; i < outMeshlets.size(); ++i)
	{
		outMeshletBounds.push_back(_ComputeMeshletBounds(inMesh, outMeshlets[i], outMeshletVertices, outMeshletTriangles));
	}
}

void MeshUtility::_OptimizeMesh(Mesh& mesh)
{
	std::vector<uint32_t> remap(std::max(mesh.indices.size(), mesh.verts.size()));
	std::vector<uint32_t> dstIndices(mesh.indices.size());
	std::vector<Vertex> dstVerts(mesh.verts.size());
	size_t indexCount = mesh.indices.size();
	size_t vertexCount = meshopt_generateVertexRemapCustom(
		remap.data(),
		mesh.indices.data(),
		mesh.indices.size(),
		reinterpret_cast<const float*>(mesh.verts.data()),
		mesh.verts.size(),
		sizeof(Vertex),
		[&](unsigned int l, unsigned int r) {
			const Vertex& lv = mesh.verts[l];
			const Vertex& rv = mesh.verts[r];
			return lv == rv;
		}
	);
	dstIndices.resize(indexCount);
	dstVerts.resize(vertexCount);
	meshopt_remapVertexBuffer(dstVerts.data(), mesh.verts.data(), mesh.verts.size(), sizeof(Vertex), remap.data());
	meshopt_remapIndexBuffer(dstIndices.data(), mesh.indices.data(), indexCount, remap.data());
	meshopt_optimizeVertexCache(mesh.indices.data(), dstIndices.data(), indexCount, vertexCount);
	meshopt_optimizeOverdraw(
		dstIndices.data(),
		mesh.indices.data(),
		indexCount,
		reinterpret_cast<const float*>(dstVerts.data()),
		vertexCount,
		sizeof(Vertex),
		1.0f
	);
	vertexCount = meshopt_optimizeVertexFetch(
		mesh.verts.data(),
		dstIndices.data(),
		indexCount,
		dstVerts.data(),
		vertexCount,
		sizeof(Vertex)
	);
	mesh.verts.resize(vertexCount);
	mesh.indices = std::move(dstIndices);
}

void MeshUtility::_OptimizeMeshToVertexCacheStage(Mesh& mesh)
{
	std::vector<uint32_t> remap(std::max(mesh.indices.size(), mesh.verts.size()));
	std::vector<uint32_t> dstIndices(mesh.indices.size());
	std::vector<Vertex> dstVerts(mesh.verts.size());
	size_t indexCount = mesh.indices.size();
	size_t vertexCount = meshopt_generateVertexRemapCustom(
		remap.data(),
		mesh.indices.data(),
		mesh.indices.size(),
		reinterpret_cast<const float*>(mesh.verts.data()),
		mesh.verts.size(),
		sizeof(Vertex),
		[&](unsigned int l, unsigned int r) {
			const Vertex& lv = mesh.verts[l];
			const Vertex& rv = mesh.verts[r];
			return lv == rv;
		}
	);
	dstIndices.resize(indexCount);
	dstVerts.resize(vertexCount);
	meshopt_remapVertexBuffer(dstVerts.data(), mesh.verts.data(), mesh.verts.size(), sizeof(Vertex), remap.data());
	meshopt_remapIndexBuffer(dstIndices.data(), mesh.indices.data(), indexCount, remap.data());
	meshopt_optimizeVertexCache(mesh.indices.data(), dstIndices.data(), indexCount, vertexCount);
	mesh.verts = std::move(dstVerts);
}

MeshletBounds MeshUtility::_ComputeMeshletBounds(const Mesh& inMesh, const Meshlet& inMeshlet, const std::vector<uint32_t>& inMeshletVertices, const std::vector<uint8_t>& inMeshletTriangles)
{
	MeshletBounds meshletBounds{};
	meshopt_Bounds bounds{};
	bounds = meshopt_computeMeshletBounds(
		&inMeshletVertices[inMeshlet.vertexOffset],
		&inMeshletTriangles[inMeshlet.triangleOffset],
		inMeshlet.triangleCount,
		reinterpret_cast<const float*>(inMesh.verts.data()),
		inMesh.verts.size(),
		sizeof(Vertex)
	);
	meshletBounds.center = glm::vec3(bounds.center[0], bounds.center[1], bounds.center[2]);
	meshletBounds.radius = bounds.radius;
	meshletBounds.coneApex = glm::vec3(bounds.cone_apex[0], bounds.cone_apex[1], bounds.cone_apex[2]);
	meshletBounds.coneAxis = glm::vec3(bounds.cone_axis[0], bounds.cone_axis[1], bounds.cone_axis[2]);
	meshletBounds.coneCutoff = bounds.cone_cutoff;
	return meshletBounds;
}

namespace {
	void _LoadMesh(
		const tinygltf::Model& _Tmodel,
		const tinygltf::Mesh& _Tmesh,
		GLTFScene& _outScene)
	{
		for (size_t i = 0; i < _Tmesh.primitives.size(); ++i)
		{
			const auto& tprimitive = _Tmesh.primitives[i];
			Mesh newMesh{};

			// initialize position, normal, uv
			for (const auto& tattribute : tprimitive.attributes)
			{
				const auto& taccessor = _Tmodel.accessors[tattribute.second];
				const auto& tbufferView = _Tmodel.bufferViews[taccessor.bufferView];
				const auto& tbuffer = _Tmodel.buffers[tbufferView.buffer];
				int byteStride = taccessor.ByteStride(tbufferView);
				const uint8_t* pBufferDataSrc = &tbuffer.data.at(0) + tbufferView.byteOffset;
				
				if (byteStride <= 0) continue;

				// copy attribute to Mesh
				if (tattribute.first.compare("POSITION") == 0)
				{
					int n = tbufferView.byteLength / byteStride;

					CHECK_TRUE(TINYGLTF_COMPONENT_TYPE_FLOAT == taccessor.componentType, "Buffer does not use float type!");
					if (newMesh.verts.size() == 0)
					{
						newMesh.verts.resize(n, Vertex{});
					}
					CHECK_TRUE(newMesh.verts.size() == n, "Number of attributes doesn't match number of vertices!");
					
					for (int j = 0; j < n; ++j)
					{
						glm::vec3 f3Position{};

						memcpy(&f3Position, pBufferDataSrc, sizeof(glm::vec3));
						newMesh.verts[j].position = f3Position;

						pBufferDataSrc += byteStride;
					}
				}
				else if (tattribute.first.compare("NORMAL") == 0)
				{
					int n = tbufferView.byteLength / byteStride;

					CHECK_TRUE(TINYGLTF_COMPONENT_TYPE_FLOAT == taccessor.componentType, "Buffer does not use float type!");
					if (newMesh.verts.size() == 0)
					{
						newMesh.verts.resize(n, Vertex{});
					}
					CHECK_TRUE(newMesh.verts.size() == n, "Number of attributes doesn't match number of vertices!");

					for (int j = 0; j < n; ++j)
					{
						glm::vec3 f3Normal{};
						
						memcpy(&f3Normal, pBufferDataSrc, sizeof(glm::vec3));
						newMesh.verts[j].normal = f3Normal;

						pBufferDataSrc += byteStride;
					}
				}
				else if (tattribute.first.compare("TEXCOORD_0") == 0)
				{
					int n = tbufferView.byteLength / byteStride;

					CHECK_TRUE(TINYGLTF_COMPONENT_TYPE_FLOAT == taccessor.componentType, "Buffer does not use float type!");
					if (newMesh.verts.size() == 0)
					{
						newMesh.verts.resize(n, Vertex{});
					}
					CHECK_TRUE(newMesh.verts.size() == n, "Number of attributes doesn't match number of vertices!");

					for (int j = 0; j < n; ++j)
					{
						glm::vec2 f2Texcoord{};

						memcpy(&f2Texcoord, pBufferDataSrc, sizeof(glm::vec2));
						newMesh.verts[j].uv = f2Texcoord;

						pBufferDataSrc += byteStride;
					}
				}
			}

			// initialize index
			if (tprimitive.indices > -1)
			{
				const auto& taccessor = _Tmodel.accessors[tprimitive.indices];
				const auto& tbufferView = _Tmodel.bufferViews[taccessor.bufferView];
				const auto& tbuffer = _Tmodel.buffers[tbufferView.buffer];
				int byteStride = taccessor.ByteStride(tbufferView);
				const uint8_t* pBufferDataSrc = &tbuffer.data.at(0) + tbufferView.byteOffset;

				CHECK_TRUE(byteStride > 0, "Stride of index buffer is zero!");
				CHECK_TRUE(taccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT, "Type of index should be uint32_t.");
				int n = tbufferView.byteLength / byteStride;
				newMesh.indices.resize(n, 0);

				for (int j = 0; j < n; ++j)
				{
					memcpy(&newMesh.indices[j], pBufferDataSrc, sizeof(uint32_t));
					pBufferDataSrc += byteStride;
				}
			}
			else
			{
				newMesh.indices.resize(newMesh.verts.size());
				std::iota(newMesh.indices.begin(), newMesh.indices.end(), 0);
			}

			_outScene.meshes.push_back(std::move(newMesh));
			_outScene.modelMatrices.push_back(glm::identity<glm::mat4>());
		}
	}

	void _LoadNode(
		const tinygltf::Model& _Tmodel,
		const tinygltf::Node& _Tnode,
		GLTFScene& _outScene)
	{
		size_t meshOffset = _outScene.meshes.size();
		bool bHasTransform = false;
		glm::mat4 modelMatrix{};

		if ((_Tnode.mesh >= 0) && (_Tnode.mesh < _Tmodel.meshes.size()))
		{
			_LoadMesh(_Tmodel, _Tmodel.meshes[_Tnode.mesh], _outScene);
		}

		for (size_t i = 0; i < _Tnode.children.size(); ++i)
		{
			_LoadNode(_Tmodel, _Tmodel.nodes[_Tnode.children[i]], _outScene);
		}

		// apply parent transformation
		if (_Tnode.matrix.size() == 16)
		{
			size_t index = 0;
			bHasTransform = true;
			for (int col = 0; col < 4; ++col)
			{
				for (int row = 0; row < 4; ++row)
				{
					modelMatrix[col][row] = static_cast<float>(_Tnode.matrix[index]);
					++index;
				}
			}
		}
		else
		{
			Transform transform{};

			if (_Tnode.scale.size() == 3)
			{
				bHasTransform = true;
				transform.SetScale(_Tnode.scale[0], _Tnode.scale[1], _Tnode.scale[2]);
			}
			if (_Tnode.rotation.size() == 4)
			{
				bHasTransform = true;
				transform.SetRotation(_Tnode.rotation[0], _Tnode.rotation[1], _Tnode.rotation[2], _Tnode.rotation[3]);
			}
			if (_Tnode.translation.size() == 3)
			{
				bHasTransform = true;
				transform.SetPosition(_Tnode.translation[0], _Tnode.translation[1], _Tnode.translation[2]);
			}

			modelMatrix = transform.GetModelMatrix();
		}
		if (bHasTransform)
		{
			for (size_t i = meshOffset; i < _outScene.modelMatrices.size(); ++i)
			{
				_outScene.modelMatrices[i] = modelMatrix * _outScene.modelMatrices[i];
			}
		}

	}
}

bool SceneUtility::Load(const std::string& _strGlTFFile, GLTFScene& _outScene)
{
	bool bSuccess = false;
	tinygltf::Model tmodel;

	// load to tmodel
	{
		tinygltf::TinyGLTF tloader;
		std::string strErr;
		std::string strWarn;
		bool bResult = tloader.LoadASCIIFromFile(&tmodel, &strErr, &strWarn, _strGlTFFile);
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
			std::cout << "Failed to load glTF: " << _strGlTFFile << std::endl;
		}
		else
		{
			std::cout << "Loaded glTF: " << _strGlTFFile << std::endl;
		}
	}

	for (size_t i = 0; i < tmodel.meshes.size(); ++i)
	{
		const auto& tmesh = tmodel.meshes[i];
		_LoadMesh(tmodel, tmesh, _outScene);
	}

	bSuccess = true;
	return bSuccess;
}
