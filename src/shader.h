#pragma once
#include "common.h"

class SimpleShader
{
private:
	std::string m_spvFile;
	std::vector<char> _ReadFile(const std::string& filename) const;

public:
	VkShaderModule vkShaderModule = VK_NULL_HANDLE;
	VkShaderStageFlagBits stage = static_cast<VkShaderStageFlagBits>(0);
	std::string name = "main";
	~SimpleShader();
	void SetSPVFile(const std::string& file);
	void Init();
	void Uninit();
	VkPipelineShaderStageCreateInfo GetShaderStageInfo() const;
};