#pragma once
#include "common.h"
#include "spirv_reflect.h"
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

