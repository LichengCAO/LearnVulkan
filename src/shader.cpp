#include "shader.h"
#include "device.h"
#include <fstream>
std::vector<char> SimpleShader::_ReadFile(const std::string& filename) const
{
	std::ifstream file(filename, std::ios::ate | std::ios::binary);
	
	CHECK_TRUE(file.is_open(), "Failed to open file!");
	
	size_t fileSize = (size_t)file.tellg();
	std::vector<char> ans(fileSize);
	file.seekg(0);
	file.read(ans.data(), fileSize);
	file.close();

	return ans;
}
SimpleShader::~SimpleShader()
{
	assert(vkShaderModule == VK_NULL_HANDLE);
}

void SimpleShader::SetSPVFile(const std::string& file)
{
	m_spvFile = file;
}

void SimpleShader::Init()
{
	CHECK_TRUE(!m_spvFile.empty(), "SPV file is unset!");
	auto code = _ReadFile(m_spvFile);
	VkShaderModuleCreateInfo createInfo{
	.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
	.codeSize = code.size(),
	.pCode = reinterpret_cast<const uint32_t*>(code.data())
	};
	VK_CHECK(vkCreateShaderModule(MyDevice::GetInstance().vkDevice, &createInfo, nullptr, &vkShaderModule), Failed to create shader module!);
}

void SimpleShader::Uninit()
{
	if (vkShaderModule != VK_NULL_HANDLE)
	{
		vkDestroyShaderModule(MyDevice::GetInstance().vkDevice, vkShaderModule, nullptr);
		vkShaderModule = VK_NULL_HANDLE;
	}
}

VkPipelineShaderStageCreateInfo SimpleShader::GetShaderStageInfo() const
{
	VkPipelineShaderStageCreateInfo shaderStageInfo{ VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
	shaderStageInfo.stage = stage;
	shaderStageInfo.module = vkShaderModule;
	shaderStageInfo.pName = name.c_str();
	return shaderStageInfo;
}
