#pragma once
#include "common.h"
#include "component.h"
#include <variant>
#include <tiny_gltf.h>
#include "utils.h"
class glTFLoader
{
private:
	struct Material
	{
		std::string name;
		glm::vec4 color;
		// TODO
	};
	struct Primitive
	{
		std::vector<glm::vec3> positions;
		std::vector<glm::vec3> normals;
		std::vector<glm::vec2> texcoords;
		std::vector<uint32_t>  indices;
		std::optional<Material> optMaterial;
	};

	//node's components
	struct Camera : public Component
	{
		COMPONENT_DECLARATION;
		// TODO
	};
	struct Mesh : public Component
	{
		COMPONENT_DECLARATION;
		std::vector<Primitive> primitives;
	};
	struct Transform : public Component
	{
		COMPONENT_DECLARATION;
		struct SplitTransform
		{
			glm::vec3 translation;
			glm::quat rotation;
			glm::vec3 scale;
		};

		std::variant<glm::mat4, SplitTransform> m_transform;

		glm::mat4 GetModelMatrix() const;
	};
	struct Animation : public Component
	{
		COMPONENT_DECLARATION;
		// TODO
	};

	struct Node : public ComponentManager
	{
		Node* pParent;
		std::vector<Node*> pChildren;
	};

	struct Scene
	{
		std::vector<Node*> pNodes;
	};

public:
	// set up pointer to get the data from glTF scene
	struct SceneData
	{
		std::vector<::StaticMesh>* pStaticMeshes = nullptr; // optional, get static mesh with glTF primitives
		std::vector<glm::mat4>* pModelMatrices = nullptr;   // optional, get model matrices of glTF primitives
		std::vector<glm::vec4>* pMeshColors = nullptr;		// optional, get mesh color of glTF primitives
		std::vector<std::string>* pMaterialNames = nullptr; // optional, get name of materials
	};

private:
	std::vector<std::unique_ptr<Node>> m_nodes;
	std::vector<std::unique_ptr<Scene>> m_scenes;

private:
	static void _LoadFile(const std::string& _file, tinygltf::Model& _out);

	template<class DataType>
	static void _LoadAccessor(const tinygltf::Model& _root, const tinygltf::Accessor& _accessor, std::vector<DataType>& _outVec);

	static void _LoadMesh(const tinygltf::Model& _root, const tinygltf::Mesh& _mesh, glTFLoader::Mesh& _outMesh);

	void _LoadNodes(const tinygltf::Model& _root);

	void _LoadScene(const tinygltf::Model& _root);

	// recursively fetch primitives from this node and its parent nodes that can build static meshes
	// return the static meshes built by these primitives and their model matrices
	void _FetchStaticMeshes(
		const glTFLoader::Node* _pNode,
		const glm::mat4& _parentModelMatrix,
		std::vector<::StaticMesh>& _outStaticMeshes,
		std::vector<glm::mat4>& _outModelMatrices) const;

	void _FetchSceneData(
		const glTFLoader::Node* _pNode,
		const glm::mat4& _parentModelMatrix,
		SceneData& _outputSceneData) const;

public:
	void Load(const std::string& _glTFPath);

	void GetSceneSimpleMeshes(std::vector<::StaticMesh>& _staticMeshes, std::vector<glm::mat4>& _modelMatrices);

	// Get scene data from glTF scene, see glTFLoader::SceneData for detail
	void GetSceneData(SceneData& _outputSceneData) const;
};

template<class DataType>
void glTFLoader::_LoadAccessor(const tinygltf::Model& _root, const tinygltf::Accessor& _accessor, std::vector<DataType>& _outVec)
{
	const auto& tbufferView = _root.bufferViews[_accessor.bufferView];
	const auto& tbuffer = _root.buffers[tbufferView.buffer];
	int byteStride = _accessor.ByteStride(tbufferView);
	const uint8_t* pBufferDataSrc = &tbuffer.data.at(0) + tbufferView.byteOffset + _accessor.byteOffset;

	CHECK_TRUE(byteStride > 0, "Stride of index buffer is zero!");
	_outVec.resize(_accessor.count);

	for (int i = 0; i < _accessor.count; ++i)
	{
		memcpy(&_outVec[i], pBufferDataSrc, sizeof(DataType));
		pBufferDataSrc += byteStride;
	}
}