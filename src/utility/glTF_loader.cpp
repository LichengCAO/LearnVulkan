#include "glTF_loader.h"
#include <glm/gtc/quaternion.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
#define COMPONENT_IMPLEMENTATION
#include "component.h"

COMPONENT_DEFINITION(Component, glTFContent::Camera);
COMPONENT_DEFINITION(Component, glTFContent::Mesh);
COMPONENT_DEFINITION(Component, glTFContent::Transform);
COMPONENT_DEFINITION(Component, glTFContent::Animation);

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

void glTFContent::_LoadNodes(const tinygltf::Model& _root)
{
	m_nodes.reserve(_root.nodes.size());
	for (size_t i = 0; i < _root.nodes.size(); ++i)
	{
		std::unique_ptr<Node> uptrNode = std::make_unique<Node>();
		const auto& curNode = _root.nodes[i];

		// load transform
		{
			if (curNode.matrix.size() == 16)
			{
				glTFContent::Transform transform{};
				size_t index = 0;
				glm::mat4 modelMatrix{};

				for (size_t col = 0; col < 4; ++col)
				{
					for (size_t row = 0; row < 4; ++row)
					{
						modelMatrix[col][row] = static_cast<float>(curNode.matrix[index]);
						++index;
					}
				}
				transform.m_transform = modelMatrix;

				uptrNode->AddComponent<glTFContent::Transform>(transform);
			}
			else if (curNode.translation.size() == 3 || curNode.scale.size() == 3 || curNode.rotation.size() == 4)
			{
				glTFContent::Transform transform{};
				glTFContent::Transform::SplitTransform splitTransform{};
				if (curNode.translation.size() == 3)
				{
					splitTransform.translation = glm::vec3(
						static_cast<float>(curNode.translation[0])
						, static_cast<float>(curNode.translation[1])
						, static_cast<float>(curNode.translation[2]));
				}
				if (curNode.scale.size() == 3)
				{
					splitTransform.scale = glm::vec3(
						static_cast<float>(curNode.scale[0])
						, static_cast<float>(curNode.scale[1])
						, static_cast<float>(curNode.scale[2]));
				}
				else
				{
					splitTransform.scale = glm::vec3(1.0f, 1.0f, 1.0f);
				}
				if (curNode.rotation.size() == 4)
				{
					splitTransform.rotation = glm::quat(
						static_cast<float>(curNode.rotation[3]),
						static_cast<float>(curNode.rotation[0]),
						static_cast<float>(curNode.rotation[1]),
						static_cast<float>(curNode.rotation[2]));
				}
				else
				{
					splitTransform.rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
				}
				transform.m_transform = splitTransform;

				uptrNode->AddComponent<glTFContent::Transform>(transform);
			}
		}

		// load mesh
		if (curNode.mesh != -1)
		{
			glTFContent::Mesh mesh{};
			const auto& curMesh = _root.meshes[curNode.mesh];

			_LoadMesh(_root, curMesh, mesh);

			uptrNode->AddComponent<glTFContent::Mesh>(mesh);
		}

		m_nodes.push_back(std::move(uptrNode));
	}

	// fill node hierarchy
	for (size_t i = 0; i < _root.nodes.size(); ++i)
	{
		auto& uptrNode = m_nodes[i];
		const auto& curNode = _root.nodes[i];

		uptrNode->pChildren.reserve(curNode.children.size());
		for (size_t j = 0; j < curNode.children.size(); ++j)
		{
			size_t index = curNode.children[j];
			uptrNode->pChildren.push_back(m_nodes[index].get());
			m_nodes[index]->pParent = uptrNode.get();
		}
	}
}

void glTFContent::_LoadScene(const tinygltf::Model& _root)
{
	for (size_t i = 0; i < _root.scenes.size(); ++i)
	{
		const auto& curScene = _root.scenes[i];
		std::unique_ptr<glTFContent::Scene> uptrScene = std::make_unique<glTFContent::Scene>();

		uptrScene->pNodes.reserve(curScene.nodes.size());
		for (size_t j = 0; j < curScene.nodes.size(); ++j)
		{
			size_t index = curScene.nodes[j];
			CHECK_TRUE(index < m_nodes.size(), "Do not have this node!");
			uptrScene->pNodes.push_back(m_nodes[index].get());
		}
	}
}

void glTFContent::_LoadMesh(const tinygltf::Model& _root, const tinygltf::Mesh& _mesh, glTFContent::Mesh& _outMesh)
{
	for (size_t i = 0; i < _mesh.primitives.size(); ++i)
	{
		glTFContent::Primitive primitive{};
		const auto& curPrimitive = _mesh.primitives[i];

		for (const auto& curAttribute : curPrimitive.attributes)
		{
			const auto& taccessor = _root.accessors[curAttribute.second];
			if (curAttribute.first.compare("POSITION") == 0)
			{
				CHECK_TRUE(TINYGLTF_COMPONENT_TYPE_FLOAT == taccessor.componentType, "Buffer does not use float type!");
				_LoadAccessor(_root, taccessor, primitive.positions);
			}
			else if (curAttribute.first.compare("NORMAL") == 0)
			{
				CHECK_TRUE(TINYGLTF_COMPONENT_TYPE_FLOAT == taccessor.componentType, "Buffer does not use float type!");
				_LoadAccessor(_root, taccessor, primitive.normals);
			}
			else if (curAttribute.first.compare("TEXCOORD_0") == 0)
			{
				CHECK_TRUE(TINYGLTF_COMPONENT_TYPE_FLOAT == taccessor.componentType, "Buffer does not use float type!");
				_LoadAccessor(_root, taccessor, primitive.texcoords);
			}
		}

		if (curPrimitive.indices > -1)
		{
			const auto& taccessor = _root.accessors[curPrimitive.indices];
			CHECK_TRUE(taccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT, "Type of index should be uint32_t.");
			_LoadAccessor(_root, taccessor, primitive.indices);
		}

		_outMesh.primitives.push_back(std::move(primitive));
	}
}

void glTFContent::Load(const std::string& _glTFPath)
{
	tinygltf::Model root;

	_LoadFile(_glTFPath, root);

	_LoadNodes(root);

	_LoadScene(root);
}

glm::mat4 glTFContent::Transform::GetModelMatrix() const
{
	if (auto ptr = std::get_if<SplitTransform>(&m_transform))
	{
		auto T = glm::translate(glm::mat4(1.0f), ptr->translation);
		auto R = glm::toMat4(ptr->rotation);
		auto S = glm::scale(glm::mat4(1.0f), ptr->scale);

		return T * R * S;
	}
	else if (auto ptr = std::get_if<glm::mat4>(&m_transform))
	{
		return *ptr;
	}
	CHECK_TRUE(false, "No model matrix!");
	return glm::mat4(1.0f);
}
