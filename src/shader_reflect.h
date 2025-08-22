#pragma once
#include "common.h"
#include <unordered_set>
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

class SpirvReflector
{
private:


public:
	struct ReflectDescriptorSet
	{
		uint32_t uSetIndex = ~0;
		std::vector<VkDescriptorSetLayoutBinding> bindings;
	};

private:
	static std::vector<char> _ReadFile(const std::string& filename);

public:
	void ReflectPipeline(const std::vector<std::string>& _shaderStage) const;
	
	//void ReflectSpirv(const std::string& _path);
};

class GraphicsPipelineProgram
{
public:
	void SetUpShaders(const std::vector<std::string>& _paths);

	void StartBind(CommandSubmission* _pCmd);

	void BindDescriptor(const std::string& _nameInShader, const std::vector<VkDescriptorBufferInfo>& _data);
	void BindDescriptor(const std::string& _nameInShader, const std::vector<VkDescriptorImageInfo>& _data);
	void BindDescriptor(const std::string& _nameInShader, const std::vector<VkBufferView>& _data);
	void BindDescriptor(const std::string& _nameInShader, const std::vector<VkAccelerationStructureKHR>& _data);

	void BindVertex();

	void BindColorAttachment(
		const std::string& _nameInShader,
		ImageView* _pImageView,
		VkAttachmentLoadOp loadOp,
		VkAttachmentStoreOp storeOp,
		VkImageLayout initialLayout,
		VkImageLayout finalLayout,
		VkSampleCountFlagBits samples);

	void EndBind(CommandSubmission* _pCmd);

	void ApplySelf(CommandSubmission* _pCmd);
};

class ComputePipelineProgram
{
private:
	std::unique_ptr<ComputePipeline>										m_uptrPipeline;
	std::unordered_map<std::string, std::pair<uint32_t, uint32_t>>			m_mapNameToSetBinding;
	std::vector<std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding>> m_vkDescriptorSetBindingInfo;

	std::vector<std::unique_ptr<DescriptorSetLayout>>				m_vecUptrDescriptorSetLayout;
	std::vector<bool>												m_vecDescriptorSetsAvailability;
	std::vector<std::vector<std::vector<DescriptorSet*>>>			m_pDescriptorSetsForFrame;
	std::vector<std::unique_ptr<DescriptorSet>>						m_vecUptrDescriptorSet;

	uint32_t m_uFrameInFlightCount = 1;
	uint32_t m_uCurrentFrame = 0;

	std::vector<std::vector<std::unordered_set<uint32_t>>> m_bDescriptorBound;

private:
	// given descriptor name, get the set and binding of the descriptor
	// return true if the output is valid, return false if we cannot map the name
	bool _GetDescriptorLocation(const std::string& _name, uint32_t& _set, uint32_t& _binding) const;

	// create descriptor set layout based on shader reflector and then build pipeline
	void _ReflectPipeline();

	void _InitDescriptorSets();

	void _UninitDescriptorSets();

	DescriptorSet* _GetDescriptorSet(uint32_t _set);

public:
	ComputePipelineProgram(ComputePipelineProgram& other) = delete;
	
	void SetUpShaders(const std::vector<std::string>& _paths);
	
	void GoToNextFrame();

	void StartBind(CommandSubmission* _pCmd);

	// bind data to descriptor, if _bForce is true then vkUpdateDescriptorSets will be called, 
	// otherwise vkUpdateDescriptorSets will only be called the first time for each frame in flight
	void BindDescriptor(
		const std::string& _nameInShader, 
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

	// bind data to descriptor, if _bForce is true then vkUpdateDescriptorSets will be called, 
	// otherwise vkUpdateDescriptorSets will only be called the first time for each frame in flight
	void BindDescriptor(
		const std::string& _nameInShader, 
		const std::vector<VkDescriptorImageInfo>& _data, 
		bool _bForce = false);

	// bind data to descriptor, if _bForce is true then vkUpdateDescriptorSets will be called, 
	// otherwise vkUpdateDescriptorSets will only be called the first time for each frame in flight
	void BindDescriptor(
		const std::string& _nameInShader, 
		const std::vector<VkBufferView>& _data, 
		bool _bForce = false);

	// bind data to descriptor, if _bForce is true then vkUpdateDescriptorSets will be called, 
	// otherwise vkUpdateDescriptorSets will only be called the first time for each frame in flight
	void BindDescriptor(
		const std::string& _nameInShader, 
		const std::vector<VkAccelerationStructureKHR>& _data, 
		bool _bForce = false);

	void EndBind(CommandSubmission* _pCmd);

	void ApplySelf(CommandSubmission* _pCmd);
};

class RayTracingPipelineProgram
{
public:
	void SetUpShaders(const std::vector<std::string>& _paths);

	void StartBind(CommandSubmission* _pCmd);

	void BindDescriptor(const std::string& _nameInShader, const std::vector<VkDescriptorBufferInfo>& _data);
	void BindDescriptor(const std::string& _nameInShader, const std::vector<VkDescriptorImageInfo>& _data);
	void BindDescriptor(const std::string& _nameInShader, const std::vector<VkBufferView>& _data);
	void BindDescriptor(const std::string& _nameInShader, const std::vector<VkAccelerationStructureKHR>& _data);

	void BindColorAttachment(
		const std::string& _nameInShader,
		ImageView* _pImageView,
		VkAttachmentLoadOp loadOp,
		VkAttachmentStoreOp storeOp,
		VkImageLayout initialLayout,
		VkImageLayout finalLayout,
		VkSampleCountFlagBits samples);

	void EndBind(CommandSubmission* _pCmd);

	void ApplySelf(CommandSubmission* _pCmd);
};