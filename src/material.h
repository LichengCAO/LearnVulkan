#pragma once
#include "component.h"
#include "common.h"
#include <variant>
class MaterialBaseColor : virtual public Component
{
	COMPONENT_DECLARATION;

private:
	std::variant<glm::vec3, std::string> m_val;
};

class MaterialMetallic : virtual public Component
{
	COMPONENT_DECLARATION;
private:
	std::variant<float, std::string> m_val;
};

class MaterialRoughness : virtual public Component
{
	COMPONENT_DECLARATION;
private:
	float m_roughness;
	std::string m_texture;
};

class MaterialNormal : virtual public Component
{
	COMPONENT_DECLARATION;
private:
	std::string m_texture;
};

class MaterialEmissive : virtual public Component
{
	COMPONENT_DECLARATION;
private:
	glm::vec3 m_emissive;
	std::string m_texture;
};

class MaterialOcclusion : virtual public Component
{
	COMPONENT_DECLARATION;
private:
	std::string m_texture;
};

class Material : public ComponentManager
{

};