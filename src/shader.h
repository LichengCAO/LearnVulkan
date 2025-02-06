#pragma once
#include "trig_app.h"

enum ShaderType
{
	ST_NULL,
	ST_VERTEX,
	ST_FRAGMENT,
	ST_COMPUTE,
};

class ShaderModule
{
public:
	ShaderModule();
	~ShaderModule();
	VkShaderModule vkShaderModule;
};

class SimpleShader
{
public:
	SimpleShader();
	ShaderType type = ST_NULL;
private:
	ShaderModule m_shaderModule;
};