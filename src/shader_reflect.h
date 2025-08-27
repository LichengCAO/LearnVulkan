#pragma once
#include "common.h"
#include <unordered_set>
#include <spirv_reflect.h>
//class Binder
//{
//	void StartBind();
//	void Bind(const VkDescriptorBufferInfo& _descriptor);
//	void Bind(const VkDescriptorImageInfo& _descriptor);
//	void Bind(const VkBufferView& _descriptor);
//	void Bind(const VkAccelerationStructureKHR& _descriptor);
//	void Bind(const VkImageView& _attachment);
//	void EndBind();
//};
//
//class Pass
//{
//	void AddShader();
//	void Run();
//};
class ImageView;
class CommandSubmission;
class DescriptorSet;
class DescriptorSetLayout;

class ShaderReflector final
{
private:
	std::vector<std::unique_ptr<SpvReflectShaderModule>> m_vecReflectModule;

private:
	SpvReflectShaderModule* _FindShaderModule(VkShaderStageFlagBits _stage) const;

public:
	~ShaderReflector();

	// start reflect shaders of the same pipeline
	void Init(const std::vector<std::string>& _spirvFiles);

	// reflect descriptor sets of the pipeline,
	// _mapSetBinding: output, map descriptor set name to {set, binding}, some descriptor sets may not have name
	// _descriptorSetData: output, descriptor bindings of set0, set1, ..., the sets are presented by order
	void ReflectDescriptorSets(
		std::unordered_map<std::string, std::pair<uint32_t, uint32_t>>& _mapSetBinding,
		std::vector<std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding>>& _descriptorSetData) const;

	// reflect input of the pipeline stage,
	// _mapLocation: output, map input name to location
	// _mapFormat: output, map location to VkFormat
	void ReflectInput(
		VkShaderStageFlagBits _stage,
		std::unordered_map<std::string, uint32_t>& _mapLocation,
		std::unordered_map<uint32_t, VkFormat>& _mapFormat) const;

	// reflect output of the pipeline stage,
	// _mapLocation: output, map output name to location
	// _mapFormat: output, map location to VkFormat
	void ReflectOuput(
		VkShaderStageFlagBits _stage, 
		std::unordered_map<std::string, uint32_t>& _mapLocation,
		std::unordered_map<uint32_t, VkFormat>& _mapFormat) const;

	// reflect push constant of the pipeline,
	// _mapIndex: output, map output name to index in _pushConstInfo
	// _pushConstInfo: output, store the stages and size of push constant
	void ReflectPushConst(
		std::unordered_map<std::string, uint32_t>& _mapIndex,
		std::vector<std::pair<VkShaderStageFlags, uint32_t>>& _pushConstInfo) const;

	void PrintReflectResult() const;

	void Uninit();
};

class DescriptorSetManager
{
private:
	struct DynamicOffsetRecord
	{
		uint32_t set = 0;
		uint32_t binding = 0;
		std::vector<uint32_t> offsets;

		bool operator<(const DynamicOffsetRecord& _other) const 
		{
			if (set != _other.set)
			{
				return set < _other.set;
			}
			return binding < _other.binding;
		}
	};

private:
	// get by reflection:
	std::unordered_map<std::string, std::pair<uint32_t, uint32_t>>			m_mapNameToSetBinding;		// name in shader -> set, binding
	std::vector<std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding>> m_vkDescriptorSetBindingInfo; // m_vkDescriptorSetBindingInfo[set]

	// device objects:
	std::vector<std::unique_ptr<DescriptorSetLayout>>				m_vecUptrDescriptorSetLayout;	// m_vecUptrDescriptorSetLayout[set]
	std::vector<std::vector<std::unique_ptr<DescriptorSet>>>		m_uptrDescriptorSetsEachFrame;	// m_uptrDescriptorSetsEachFrame[frame][set]
	
	// we cannot tell if the buffer is dynamic from reflection to create layout,
	// and descriptor set cannot be initialized and bound till the descriptor set layout is set,
	// so i postpone the real binding process till user sets all descriptor binding settings
	std::vector<std::function<void()>>	m_vecLateDescriptorUpdates;
	
	std::vector<DynamicOffsetRecord> m_vecDynamicOffset;

	uint32_t m_uFrameInFlightCount = 1;
	uint32_t m_uCurrentFrame = 0;
	bool m_bDescriptorSetInitialized = false;
	std::vector<bool> m_vecDescriptorSetIsBoundForFrame;

private:
	DescriptorSetManager() {};

	// given descriptor name, get the set and binding of the descriptor
	// return true if the output is valid, return false if we cannot map the name
	bool _GetDescriptorLocation(const std::string& _name, uint32_t& _set, uint32_t& _binding) const;

	// given descriptor set number and frame count, get the pointer of the descriptor
	// return true if the output is valid, return false if we do not reflect of this descriptor set from shader
	bool _GetDescriptorSet(uint32_t _set, uint32_t _frame, DescriptorSet*& _pDescriptorSet) const;

	bool _IsFirstTimeBind() const;

	// create descriptor set layouts, init them this will be only done once
	void _InitDescriptorSetLayouts();

	// destroy descriptor set layout and descriptor sets, uninit them
	void _UninitDescriptorSetLayouts();

public:
	DescriptorSetManager(DescriptorSetManager& other) = delete;
	~DescriptorSetManager();

	void Init(const std::vector<std::string>& _pipelineShaders, uint32_t _uFrameInFlightCount = 1);

	void NextFrame();

	void StartBind();

	// bind data to descriptor, if _bForce is true then vkUpdateDescriptorSets will be called, 
	// otherwise vkUpdateDescriptorSets will only be called the first time for each frame in flight
	void BindDescriptor(
		const std::string& _nameInShader, 
		const std::vector<VkDescriptorBufferInfo>& _data, 
		bool _bForce = false);
	
	void BindDescriptor(
		uint32_t _set, uint32_t _binding,
		const std::vector<VkDescriptorBufferInfo>& _data,
		bool _bForce = false);

	// bind data to a dynamic descriptor, if _bForce is true then vkUpdateDescriptorSets will be called, 
	// otherwise vkUpdateDescriptorSets will only be called the first time for each frame in flight,
	// size of _dynmaicOffsets must equals size of _data
	// https://github.com/KhronosGroup/Vulkan-Samples/blob/main/samples/performance/descriptor_management/README.adoc
	void BindDescriptor(
		const std::string& _nameInShader, 
		const std::vector<VkDescriptorBufferInfo>& _data, 
		const std::vector<uint32_t>& _dynmaicOffsets, 
		bool _bForce = false);
	
	void BindDescriptor(
		uint32_t _set, uint32_t _binding,
		const std::vector<VkDescriptorBufferInfo>& _data,
		const std::vector<uint32_t>& _dynmaicOffsets,
		bool _bForce = false);

	// bind data to descriptor, if _bForce is true then vkUpdateDescriptorSets will be called, 
	// otherwise vkUpdateDescriptorSets will only be called the first time for each frame in flight
	void BindDescriptor(
		const std::string& _nameInShader, 
		const std::vector<VkDescriptorImageInfo>& _data, 
		bool _bForce = false);
	
	void BindDescriptor(
		uint32_t _set, uint32_t _binding,
		const std::vector<VkDescriptorImageInfo>& _data,
		bool _bForce = false);
	
	void BindDescriptor(
		const std::string& _nameInShader, 
		const std::vector<VkBufferView>& _data, 
		bool _bForce = false);
	
	void BindDescriptor(
		uint32_t _set, uint32_t _binding,
		const std::vector<VkBufferView>& _data,
		bool _bForce = false);
	
	void BindDescriptor(
		const std::string& _nameInShader, 
		const std::vector<VkAccelerationStructureKHR>& _data, 
		bool _bForce = false);
	
	void BindDescriptor(
		uint32_t _set, uint32_t _binding,
		const std::vector<VkAccelerationStructureKHR>& _data,
		bool _bForce = false);

	void EndBind();

	void GetCurrentDescriptorSets(std::vector<VkDescriptorSet>& _outDescriptorSets, std::vector<uint32_t>& _outDynamicOffsets);

	void Uninit();
};