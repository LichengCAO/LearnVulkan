#pragma once
#include "shader_reflect.h"
#include "pipeline_io.h"
#include <variant>

class DescriptorSetManager
{
public:
	enum class DESCRIPTOR_BIND_SETTING {
		CONSTANT_DESCRIPTOR_SET_ACROSS_FRAMES,	// 1 descriptor set for all frames
		CONSTANT_DESCRIPTOR_SET_PER_FRAME,		// 1 descriptor set for each frame
		DEDICATE_DESCRIPTOR_SET_PER_FRAME,		// 1 descriptor set for each bind each frame
		DEDICATE_DESCRIPTOR_SET_ACROSS_FRAMES,	// 1 descriptor set for each bind for all frames
	};

private:
	struct DescriptorBindRecord
	{
		std::variant<VkDescriptorImageInfo, VkBufferView, VkDescriptorBufferInfo, VkAccelerationStructureKHR> varBinding;

		// return true if the descriptor is bound with something
		bool IsBound() const;
		
		// return true if the descriptor is bound with the input
		bool IsBindTo(const std::vector<VkDescriptorImageInfo>& _input) const;
		bool IsBindTo(const std::vector<VkBufferView>& _input) const;
		bool IsBindTo(const std::vector<VkDescriptorBufferInfo>& _input) const;
		bool IsBindTo(const std::vector<VkAccelerationStructureKHR>& _input) const;
	};
	struct DescriptorSetData
	{
		std::unique_ptr<DescriptorSet> uptrDescriptorSet;
		std::map<uint32_t, DescriptorBindRecord> bindInfo; // to record binding states of this descriptor set
		~DescriptorSetData();
	};
	struct BindPlan
	{
		DESCRIPTOR_BIND_SETTING bindType;
		uint32_t set;
		uint32_t location;
		std::vector<VkDescriptorImageInfo>		imageBind;
		std::vector<VkBufferView>				viewBind;
		std::vector<VkDescriptorBufferInfo>		bufferBind;
		std::vector<VkAccelerationStructureKHR> accelStructBind;
	};

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
	std::unordered_map<std::string, std::pair<uint32_t, uint32_t>>	m_mapNameToSetBinding;		// name in shader -> set, binding
	std::vector<std::map<uint32_t, VkDescriptorSetLayoutBinding>>	m_vkDescriptorSetBindingInfo; // m_vkDescriptorSetBindingInfo[set]

	// device objects:
	std::vector<std::unique_ptr<DescriptorSetLayout>>							m_vecUptrDescriptorSetLayout;// m_vecUptrDescriptorSetLayout[set]
	std::vector<std::unique_ptr<DescriptorSetData>>								m_uptrDescriptorSetSetting0; // 1 descriptor set for all frames
	std::vector<std::vector<std::unique_ptr<DescriptorSetData>>>				m_uptrDescriptorSetSetting1; // 1 descriptor set for each frame
	std::vector<std::vector<std::vector<std::unique_ptr<DescriptorSetData>>>>	m_uptrDescriptorSetSetting2; // 1 descriptor set for each bind each frame
	std::vector<std::vector<std::unique_ptr<DescriptorSetData>>>				m_uptrDescriptorSetSetting3; // 1 descriptor set for each bind for all frames

	std::vector<BindPlan> m_currentPlan; // done after endbind, cleared at begin bind
	
	std::vector<DescriptorSet*> m_pCurrentDescriptorSets; // valid descriptor set in current bound, cleared at begin bind
	std::vector<DynamicOffsetRecord> m_vecDynamicOffset;

	uint32_t m_uFrameInFlightCount = 1;
	uint32_t m_uCurrentFrame = 0;

private:
	DescriptorSetManager() {};

	// given descriptor name, get the set and binding of the descriptor
	// return true if the output is valid, return false if we cannot map the name
	bool _GetDescriptorLocation(const std::string& _name, uint32_t& _set, uint32_t& _binding) const;

	// create descriptor set layouts, init them this will be only done once
	void _InitDescriptorSetLayouts();

	// destroy descriptor set layout and descriptor sets, uninit them
	void _UninitDescriptorSetLayouts();

	// descriptor set cannot be bound till we have descriptor set inited,
	// I postpone the binding since we cannot build descriptor set till we know
	// all binding types which will not be known till the first call of EndBind()
	void _PlanBinding(const BindPlan& _bindPlan);

	// in this function, I create descriptor sets based on descriptor layouts and
	// descriptor set allocate stratigies, which suggest us to cache or update descriptor sets
	// when neccessary
	void _ProcessPlan();

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
		DESCRIPTOR_BIND_SETTING _setting = DESCRIPTOR_BIND_SETTING::DEDICATE_DESCRIPTOR_SET_PER_FRAME);

	void BindDescriptor(
		uint32_t _set, uint32_t _binding,
		const std::vector<VkDescriptorBufferInfo>& _data,
		DESCRIPTOR_BIND_SETTING _setting = DESCRIPTOR_BIND_SETTING::DEDICATE_DESCRIPTOR_SET_PER_FRAME);

	// bind data to a dynamic descriptor, if _bForce is true then vkUpdateDescriptorSets will be called, 
	// otherwise vkUpdateDescriptorSets will only be called the first time for each frame in flight,
	// size of _dynmaicOffsets must equals size of _data
	// https://github.com/KhronosGroup/Vulkan-Samples/blob/main/samples/performance/descriptor_management/README.adoc
	void BindDescriptor(
		const std::string& _nameInShader,
		const std::vector<VkDescriptorBufferInfo>& _data,
		const std::vector<uint32_t>& _dynmaicOffsets,
		DESCRIPTOR_BIND_SETTING _setting = DESCRIPTOR_BIND_SETTING::CONSTANT_DESCRIPTOR_SET_PER_FRAME);

	void BindDescriptor(
		uint32_t _set, 
		uint32_t _binding,
		const std::vector<VkDescriptorBufferInfo>& _data,
		const std::vector<uint32_t>& _dynmaicOffsets,
		DESCRIPTOR_BIND_SETTING _setting = DESCRIPTOR_BIND_SETTING::CONSTANT_DESCRIPTOR_SET_PER_FRAME);

	// bind data to descriptor, if _bForce is true then vkUpdateDescriptorSets will be called, 
	// otherwise vkUpdateDescriptorSets will only be called the first time for each frame in flight
	void BindDescriptor(
		const std::string& _nameInShader,
		const std::vector<VkDescriptorImageInfo>& _data,
		DESCRIPTOR_BIND_SETTING _setting = DESCRIPTOR_BIND_SETTING::DEDICATE_DESCRIPTOR_SET_PER_FRAME);

	void BindDescriptor(
		uint32_t _set, 
		uint32_t _binding,
		const std::vector<VkDescriptorImageInfo>& _data,
		DESCRIPTOR_BIND_SETTING _setting = DESCRIPTOR_BIND_SETTING::DEDICATE_DESCRIPTOR_SET_PER_FRAME);

	void BindDescriptor(
		const std::string& _nameInShader,
		const std::vector<VkBufferView>& _data,
		DESCRIPTOR_BIND_SETTING _setting = DESCRIPTOR_BIND_SETTING::DEDICATE_DESCRIPTOR_SET_PER_FRAME);

	void BindDescriptor(
		uint32_t _set, 
		uint32_t _binding,
		const std::vector<VkBufferView>& _data,
		DESCRIPTOR_BIND_SETTING _setting = DESCRIPTOR_BIND_SETTING::DEDICATE_DESCRIPTOR_SET_PER_FRAME);

	void BindDescriptor(
		const std::string& _nameInShader,
		const std::vector<VkAccelerationStructureKHR>& _data,
		DESCRIPTOR_BIND_SETTING _setting = DESCRIPTOR_BIND_SETTING::DEDICATE_DESCRIPTOR_SET_PER_FRAME);

	void BindDescriptor(
		uint32_t _set, 
		uint32_t _binding,
		const std::vector<VkAccelerationStructureKHR>& _data,
		DESCRIPTOR_BIND_SETTING _setting = DESCRIPTOR_BIND_SETTING::DEDICATE_DESCRIPTOR_SET_PER_FRAME);

	void EndBind();

	void GetCurrentDescriptorSets(std::vector<VkDescriptorSet>& _outDescriptorSets, std::vector<uint32_t>& _outDynamicOffsets);

	// only be valid after the first call of EndBind();
	void GetDescriptorSetLayouts(std::vector<VkDescriptorSetLayout>& _outDescriptorSetLayouts);

	void Uninit();
};

class PipelineProgram
{
public:


private:
	bool m_bPipelineLayoutReady = false;
	bool m_bPipelineReady = false;
	std::unique_ptr<ShaderReflector> m_uptrReflector;

private:
	void _InitDescriptorSetLayouts();

	void _UninitDescriptorSetLayouts();

	void _InitPipeline();

	void _UninitPipeline();

public:
	void Init(const std::vector<std::string>& _shaderPaths);

	bool ApplySelf(CommandSubmission* _pCmd);

	void Uninit();
};