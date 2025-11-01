#pragma once
#include "common.h"

class DescriptorSet;
// Pipeline can have multiple DescriptorSetLayout, 
// when execute the pipeline we need to provide DescriptorSets for each of the DescriptorSetLayout in order
class DescriptorSetLayout
{
public:
	VkDescriptorSetLayout vkDescriptorSetLayout = VK_NULL_HANDLE;
	std::vector<VkDescriptorSetLayoutBinding> bindings;

public:
	~DescriptorSetLayout();

public:
	uint32_t PreAddBinding(
		uint32_t descriptorCount,
		VkDescriptorType descriptorType,
		VkShaderStageFlags stageFlags,
		const VkSampler* pImmutableSamplers = nullptr);

	uint32_t PreAddBinding(
		uint32_t binding,
		uint32_t descriptorCount,
		VkDescriptorType descriptorType,
		VkShaderStageFlags stageFlags,
		const VkSampler* pImmutableSamplers = nullptr);

	void Init();

	DescriptorSet NewDescriptorSet() const;
	DescriptorSet* NewDescriptorSetPointer() const;

	void Uninit();
};

class DescriptorSet
{
private:
	enum class DescriptorType { BUFFER, IMAGE, TEXEL_BUFFER, ACCELERATION_STRUCTURE };
	struct DescriptorSetUpdate
	{
		uint32_t        binding = 0;
		uint32_t        startElement = 0;
		DescriptorType  descriptorType = DescriptorType::BUFFER;
		std::vector<VkDescriptorBufferInfo> bufferInfos;
		std::vector<VkDescriptorImageInfo>  imageInfos;
		std::vector<VkBufferView>           bufferViews;
		std::vector<VkAccelerationStructureKHR> accelerationStructures;
	};

private:
	std::vector<DescriptorSetUpdate> m_updates;
	const DescriptorSetLayout* m_pLayout = nullptr;

public:
	VkDescriptorSet vkDescriptorSet = VK_NULL_HANDLE;

public:
	// ~DescriptorSet(); No worry, allocator will handle this

public:
	void PresetLayout(const DescriptorSetLayout* _layout);

	void Init();

	void StartUpdate();
	void UpdateBinding(uint32_t bindingId, const std::vector<VkDescriptorImageInfo>& dImageInfo);
	void UpdateBinding(uint32_t bindingId, const std::vector<VkBufferView>& bufferViews);
	void UpdateBinding(uint32_t bindingId, const std::vector<VkDescriptorBufferInfo>& bufferInfos);
	void UpdateBinding(uint32_t bindingId, const std::vector<VkAccelerationStructureKHR>& _accelStructs);
	void FinishUpdate();
};

class DescriptorSetAllocator
{
	// https://vkguide.dev/docs/extra-chapter/abstracting_descriptors/
public:
	struct PoolSizes {
		std::vector<std::pair<VkDescriptorType, uint32_t>> sizes =
		{
			{ VK_DESCRIPTOR_TYPE_SAMPLER, 500 },
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4000 },
			{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 4000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
			{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 500 }
		};
	};

private:
	VkDescriptorPool _CreatePool();
	VkDescriptorPool _GrabPool();
	VkDescriptorPool m_currentPool = VK_NULL_HANDLE;
	PoolSizes m_poolSizes{};
	std::vector<VkDescriptorPool> m_usedPools;
	std::vector<VkDescriptorPool> m_freePools;

public:
	void ResetPools();

	bool Allocate(VkDescriptorSet* _vkSet, VkDescriptorSetLayout layout);

	void Init();
	void Uninit();
};