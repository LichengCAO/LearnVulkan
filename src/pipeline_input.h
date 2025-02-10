#pragma once
#include "trig_app.h"
class Buffer;

struct DescriptorSetEntry
{
	uint32_t descriptorCount = 1;
	VkDescriptorType descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	VkShaderStageFlags stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
};

class DescriptorSetLayout
{
private:
	// std::vector<DescriptorSet*> m_pDescriptorSets;
public:
	std::optional<VkDescriptorSetLayout> vkDescriptorSetLayout;
	std::vector<DescriptorSetEntry> bindings;
	void AddBinding(const DescriptorSetEntry& _binding);
	void Init();
	void Uninit();
};

class DescriptorSet
{
private:
	struct DescriptorSetUpdate
	{
		VkDescriptorBufferInfo bufferInfo;
		VkWriteDescriptorSet   writeDescriptorSet;
	};

private:
	std::vector<DescriptorSetUpdate> m_updates;
	DescriptorSetLayout* m_pLayout = nullptr;

public:
	std::optional<VkDescriptorSet> vkDescriptorSet;
	void SetLayout(const DescriptorSetLayout* _layout);
	void StartDescriptorSetUpdate();
	void DescriptorSetUpdate_WriteBinding(int bindingId, const Buffer* pBuffer);
	void FinishDescriptorSetUpdate();
	void Init();
	void Uninit();
};

struct VertexInputEntry
{
	uint32_t location = 0;
	VkFormat format = VK_FORMAT_R32_SFLOAT;
	uint32_t offset = 0;
};

class VertexInputLayout
{
public:
	uint32_t stride = 0;
	VkVertexInputRate inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	std::vector<VertexInputEntry> locations;
};

class VertexInput
{
private:
	VertexInputLayout* m_pVertexInputLayout = nullptr;

public:
	void SetLayout(const VertexInputLayout* _layout);
	void SetBuffer(const Buffer* pBuffer);
	void SetIndexBuffer(const Buffer* pIndexBuffer);
};