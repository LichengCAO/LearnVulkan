#pragma once
#include "common.h"
class Binder
{
	void StartBind();
	void Bind(const VkDescriptorBufferInfo& _descriptor);
	void Bind(const VkDescriptorImageInfo& _descriptor);
	void Bind(const VkBufferView& _descriptor);
	void Bind(const VkAccelerationStructureKHR& _descriptor);
	void Bind(const VkImageView& _attachment);
	void EndBind();
};

class Pass
{
	void AddShader();
	void Run();
};

class SpirvReflector
{
private:


public:
	struct ReflectDescriptorSet
	{
		uint32_t uSetIndex = ~0;
		std::vector<VkDescriptorSetLayoutBinding> bindings;
	};

public:
	void ReflectSpirv(const std::string& _path);

};