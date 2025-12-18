#pragma once
#include "common.h"
#include <map>
class DescriptorSet;
// Pipeline can have multiple DescriptorSetLayout, 
// when execute the pipeline we need to provide DescriptorSets for each of the DescriptorSetLayout in order
class DescriptorSetLayout
{
private:
	bool m_allowBindless = false;	// whether to add following descriptor bindings as bindless one
	std::vector<VkDescriptorBindingFlags> m_bindingFlags; // only used when using bindless descriptors

public:
	VkDescriptorSetLayout vkDescriptorSetLayout = VK_NULL_HANDLE;
	std::vector<VkDescriptorSetLayoutBinding> bindings;

private:

public:
	~DescriptorSetLayout();

public:
	// Optional, indicating whether the later descriptor bindings added are bindless or not
	// the descriptor set is NOT bindless by default
	DescriptorSetLayout& SetFollowingBindless(bool inBindless);

	DescriptorSetLayout& PreAddBinding(
		uint32_t descriptorCount,
		VkDescriptorType descriptorType,
		VkShaderStageFlags stageFlags,
		const VkSampler* pImmutableSamplers = nullptr,
		uint32_t* outBindingPtr = nullptr);

	// Add binding for the current layout.
	// For bindless binding, you may want to set descriptorCount large!
	DescriptorSetLayout& PreAddBinding(
		uint32_t binding,
		uint32_t descriptorCount,
		VkDescriptorType descriptorType,
		VkShaderStageFlags stageFlags,
		const VkSampler* pImmutableSamplers = nullptr);

	// Optional, set up binding flags for a specific binding
	// now only used for bindless descriptors
	DescriptorSetLayout& PreSetBindingFlags(
		uint32_t binding,
		VkDescriptorBindingFlags flags);

	void Init();

	DescriptorSet NewDescriptorSet() const;
	DescriptorSet* NewDescriptorSetPointer() const;

	// Set up the descriptor set, the result descriptor set is not Init() yet
	bool NewDescriptorSet(DescriptorSet& outDescriptorSet) const;

	// Create a pointer to the descriptor set, the result descriptor set is not Init() yet
	bool NewDescriptorSet(DescriptorSet*& outDescriptorSetPtr) const;

	void Uninit();
};

class DescriptorSet
{
private:
	enum class DescriptorType { BUFFER, IMAGE, TEXEL_BUFFER, ACCELERATION_STRUCTURE };
	struct DescriptorSetUpdate
	{
		uint32_t        binding = 0;
		uint32_t        arrayElementIndex = 0;
		DescriptorType  descriptorType = DescriptorType::BUFFER;
		std::vector<VkDescriptorBufferInfo> bufferInfos;
		std::vector<VkDescriptorImageInfo>  imageInfos;
		std::vector<VkBufferView>           bufferViews;
		std::vector<VkAccelerationStructureKHR> accelerationStructures;
	};

private:
	std::vector<DescriptorSetUpdate> m_updates;
	const DescriptorSetLayout* m_pLayout = nullptr;
	VkDescriptorPoolCreateFlags m_requiredPoolFlags = 0;

public:
	VkDescriptorSet vkDescriptorSet = VK_NULL_HANDLE;

public:
	// ~DescriptorSet(); No worry, allocator will handle this

public:
	void PresetLayout(const DescriptorSetLayout* _layout);
	
	VkDescriptorPoolCreateFlags GetRequiredPoolFlags() const;

	void PreSetRequiredPoolFlags(VkDescriptorPoolCreateFlags inFlags);

	void Init();
	
	void StartUpdate();
	
	void UpdateBinding(
		uint32_t bindingId, 
		const std::vector<VkDescriptorImageInfo>& dImageInfo,
		uint32_t inArrayIndexOffset = 0);
	
	void UpdateBinding(
		uint32_t bindingId, 
		const std::vector<VkBufferView>& bufferViews,
		uint32_t inArrayIndexOffset = 0);
	
	void UpdateBinding(
		uint32_t bindingId, 
		const std::vector<VkDescriptorBufferInfo>& bufferInfos,
		uint32_t inArrayIndexOffset = 0);
	
	void UpdateBinding(
		uint32_t bindingId, 
		const std::vector<VkAccelerationStructureKHR>& _accelStructs,
		uint32_t inArrayIndexOffset = 0);
	
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
	PoolSizes m_poolSizes{};
	std::map<VkDescriptorPoolCreateFlags, VkDescriptorPool> m_candidatePool;
	std::map<VkDescriptorPoolCreateFlags, std::vector<VkDescriptorPool>> m_usedPools;
	std::map<VkDescriptorPoolCreateFlags, std::vector<VkDescriptorPool>> m_freePools;

private:
	VkDescriptorPool _CreatePool(VkDescriptorPoolCreateFlags inPoolFlags);
	VkDescriptorPool _GrabPool(VkDescriptorPoolCreateFlags inPoolFlags);

public:
	void ResetPools();

	bool Allocate(
		VkDescriptorSetLayout inLayout, 
		VkDescriptorSet& outDescriptorSet, 
		VkDescriptorPoolCreateFlags inPoolFlags = 0,
		const void* inNextPtr = nullptr);

	void Init();

	void Uninit();
};