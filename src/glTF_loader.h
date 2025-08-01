#pragma once
#include "common.h"
#include "component.h"
#include <variant>
#include <tiny_gltf.h>

class glTFContent
{
private:
	struct Node : public ComponentManager
	{
		Node* pParent;
		std::vector<Node*> children;
	};

	struct Camera : public Component
	{
		COMPONENT_DECLARATION;
	};

	struct Material
	{

	};

	struct Primitive
	{
		std::vector<glm::vec3> positions;
		std::vector<glm::vec3> normals;
		std::vector<glm::vec2> texcoords;
		std::vector<uint32_t>  indices;
		std::optional<Material> optMaterial;
	};

	struct Mesh : public Component
	{
		COMPONENT_DECLARATION;
		std::vector<Primitive> primitives;
	};

	struct Transform : public Component
	{
		COMPONENT_DECLARATION;
	private:
		struct SplitTransform
		{
			glm::vec3 translation;
			glm::quat rotation;
			glm::vec3 scale;
		};

	private:
		std::variant<glm::mat4, SplitTransform> m_transform;

	public:
		glm::mat4 GetModelMatrix();
	};

	struct Animation : public Component
	{
		COMPONENT_DECLARATION;
	};

	struct Scene
	{
		std::vector<Node*> pNodes;
	};

private:
	std::vector<std::unique_ptr<Node>> m_nodes;
	std::vector<std::unique_ptr<Scene>> m_scenes;

private:
	static void _LoadFile(const std::string& _file, tinygltf::Model& _out);

	template<class DataType>
	static void _LoadAccessor(const tinygltf::Model& _root, const tinygltf::Accessor& _accessor, std::vector<DataType>& _outVec);

public:
	void Load(const std::string& _glTFPath);
};

template<class DataType>
void glTFContent::_LoadAccessor(const tinygltf::Model& _root, const tinygltf::Accessor& _accessor, std::vector<DataType>& _outVec)
{
	const auto& tbufferView = _root.bufferViews[_accessor.bufferView];
	const auto& tbuffer = _root.buffers[tbufferView.buffer];
	int byteStride = _accessor.ByteStride(tbufferView);
	const uint8_t* pBufferDataSrc = &tbuffer.data.at(0) + tbufferView.byteOffset;

	CHECK_TRUE(byteStride > 0, "Stride of index buffer is zero!");
	int n = tbufferView.byteLength / byteStride;
	_outVec.resize(n);

	for (int i = 0; i < n; ++i)
	{
		memcpy(&_outVec[i], pBufferDataSrc, sizeof(DataType));
		pBufferDataSrc += byteStride;
	}
}