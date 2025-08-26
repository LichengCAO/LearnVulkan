#include "shader_reflect.h"
#include "shader.h"
#include "pipeline.h"
#include "pipeline_io.h"
#include "utils.h"
#include <spirv_reflect.h>
#include <fstream>
#include <algorithm>

namespace
{
#define PRINT_STAGE(os, stage, bit) if (COMPARE_BITS(stage, bit))\
									{\
										os << #bit <<" ";\
									}
#define PRINT_TYPE(os, vkt, vkt2) if (vkt == vkt2)\
									{\
										os << #vkt2 << " ";\
									}

	struct DescriptorSetLayoutData {
		std::vector<std::string> names;
		std::vector<VkDescriptorSetLayoutBinding> bindings;
	};

	void _PrintStage(std::ostream& os, VkShaderStageFlags stages)
	{
		PRINT_STAGE(os, stages, VK_SHADER_STAGE_VERTEX_BIT);
		PRINT_STAGE(os, stages, VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT);
		PRINT_STAGE(os, stages, VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT);
		PRINT_STAGE(os, stages, VK_SHADER_STAGE_GEOMETRY_BIT);
		PRINT_STAGE(os, stages, VK_SHADER_STAGE_FRAGMENT_BIT);
		PRINT_STAGE(os, stages, VK_SHADER_STAGE_COMPUTE_BIT);
		PRINT_STAGE(os, stages, VK_SHADER_STAGE_RAYGEN_BIT_KHR);
		PRINT_STAGE(os, stages, VK_SHADER_STAGE_ANY_HIT_BIT_KHR);
		PRINT_STAGE(os, stages, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);
		PRINT_STAGE(os, stages, VK_SHADER_STAGE_MISS_BIT_KHR);
		PRINT_STAGE(os, stages, VK_SHADER_STAGE_INTERSECTION_BIT_KHR);
		PRINT_STAGE(os, stages, VK_SHADER_STAGE_CALLABLE_BIT_KHR);
		PRINT_STAGE(os, stages, VK_SHADER_STAGE_TASK_BIT_EXT);
		PRINT_STAGE(os, stages, VK_SHADER_STAGE_MESH_BIT_EXT);
		PRINT_STAGE(os, stages, VK_SHADER_STAGE_SUBPASS_SHADING_BIT_HUAWEI);
		PRINT_STAGE(os, stages, VK_SHADER_STAGE_CLUSTER_CULLING_BIT_HUAWEI);
	}
	
	void _PrintDescriptorType(std::ostream& os, VkDescriptorType vkType)
	{
		PRINT_TYPE(os, vkType, VK_DESCRIPTOR_TYPE_SAMPLER);
		PRINT_TYPE(os, vkType, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		PRINT_TYPE(os, vkType, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE);
		PRINT_TYPE(os, vkType, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
		PRINT_TYPE(os, vkType, VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER);
		PRINT_TYPE(os, vkType, VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER);
		PRINT_TYPE(os, vkType, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
		PRINT_TYPE(os, vkType, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
		PRINT_TYPE(os, vkType, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC);
		PRINT_TYPE(os, vkType, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC);
		PRINT_TYPE(os, vkType, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT);
		PRINT_TYPE(os, vkType, VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK);
		PRINT_TYPE(os, vkType, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR);
		PRINT_TYPE(os, vkType, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV);
		PRINT_TYPE(os, vkType, VK_DESCRIPTOR_TYPE_SAMPLE_WEIGHT_IMAGE_QCOM);
		PRINT_TYPE(os, vkType, VK_DESCRIPTOR_TYPE_BLOCK_MATCH_IMAGE_QCOM);
		PRINT_TYPE(os, vkType, VK_DESCRIPTOR_TYPE_MUTABLE_EXT);
		PRINT_TYPE(os, vkType, VK_DESCRIPTOR_TYPE_PARTITIONED_ACCELERATION_STRUCTURE_NV);
	}
	
	// reflect descriptor set layout based on shaders of a pipeline
	// _shaderPaths: paths of all shader files of a pipeline,
	// _mapSetBinding: output, maps descriptor name to {set, binding}
	// _descriptorSetData: output, stores layout information in order, i.e. set0, set1, ...
	void _ReflectDescriptorSet(
		const std::vector<std::string>& _shaderPaths,
		std::unordered_map<std::string, std::pair<uint32_t, uint32_t>>& _mapSetBinding,
		std::vector<std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding>>& _descriptorSetData)
	{
		std::unordered_map<uint32_t, DescriptorSetLayoutData> mapDescriptorSets;

		// fill mapDescriptorSets
		for (auto& _shader : _shaderPaths)
		{
			SpvReflectShaderModule reflectModule{};
			std::vector<uint8_t> spirvData;
			auto eResult = SPV_REFLECT_RESULT_SUCCESS;
			uint32_t uSetCount = 0u;
			std::vector<SpvReflectDescriptorSet*> pSets;

			CommonUtils::ReadFile(_shader, spirvData);

			eResult = spvReflectCreateShaderModule(spirvData.size(), spirvData.data(), &reflectModule);
			assert(eResult == SPV_REFLECT_RESULT_SUCCESS);

			eResult = spvReflectEnumerateDescriptorSets(&reflectModule, &uSetCount, nullptr);
			assert(eResult == SPV_REFLECT_RESULT_SUCCESS);

			pSets.resize(uSetCount);
			eResult = spvReflectEnumerateDescriptorSets(&reflectModule, &uSetCount, pSets.data());
			assert(eResult == SPV_REFLECT_RESULT_SUCCESS);

			// store descriptor set of this stage into map
			for (auto _pSet : pSets)
			{
				const SpvReflectDescriptorSet& curSet = *_pSet;
				std::vector<SpvReflectDescriptorBinding*> pBindings(_pSet->bindings, _pSet->bindings + static_cast<size_t>(_pSet->binding_count));
				if (mapDescriptorSets.find(curSet.set) == mapDescriptorSets.end())
				{
					DescriptorSetLayoutData setData{};
					for (auto pBinding : pBindings)
					{
						VkDescriptorSetLayoutBinding vkBinding{};
						vkBinding.binding = pBinding->binding;
						vkBinding.descriptorType = static_cast<VkDescriptorType>(pBinding->descriptor_type);
						vkBinding.stageFlags |= static_cast<VkShaderStageFlagBits>(reflectModule.shader_stage);
						vkBinding.descriptorCount = 1;
						for (uint32_t uDim = 0u; uDim < pBinding->array.dims_count; ++uDim)
						{
							vkBinding.descriptorCount *= (pBinding->array.dims[uDim]);
						}

						setData.bindings.push_back(std::move(vkBinding));
						setData.names.emplace_back(pBinding->name);
					}

					mapDescriptorSets.insert(std::make_pair(curSet.set, setData));
				}
				else
				{
					auto& setData = mapDescriptorSets[curSet.set];
					for (auto pBinding : pBindings)
					{
						int index = -1;

						// check if we already have such binding stored
						for (int nBindingIndex = 0; nBindingIndex < setData.bindings.size(); ++nBindingIndex)
						{
							auto& vkBinding = setData.bindings[nBindingIndex];
							if (vkBinding.binding == pBinding->binding)
							{
								index = nBindingIndex;
								break;
							}
						}

						if (index != -1)
						{
							// we update the existing binding
							VkDescriptorSetLayoutBinding& vkBinding = setData.bindings[index];
							uint32_t uDescriptorCount = 1;

							for (uint32_t uDim = 0u; uDim < pBinding->array.dims_count; ++uDim)
							{
								uDescriptorCount *= (pBinding->array.dims[uDim]);
							}
							CHECK_TRUE(vkBinding.descriptorCount == uDescriptorCount, "The descriptor doesn't match!");
							CHECK_TRUE(vkBinding.descriptorType == static_cast<VkDescriptorType>(pBinding->descriptor_type), "The descriptor doesn't match!");
							vkBinding.stageFlags |= static_cast<VkShaderStageFlagBits>(reflectModule.shader_stage);
						}
						else
						{
							// we add a new binding
							VkDescriptorSetLayoutBinding vkBinding{};

							vkBinding.binding = pBinding->binding;
							vkBinding.descriptorType = static_cast<VkDescriptorType>(pBinding->descriptor_type);
							vkBinding.stageFlags |= static_cast<VkShaderStageFlagBits>(reflectModule.shader_stage);
							vkBinding.descriptorCount = 1;
							for (uint32_t uDim = 0u; uDim < pBinding->array.dims_count; ++uDim)
							{
								vkBinding.descriptorCount *= (pBinding->array.dims[uDim]);
							}

							setData.bindings.push_back(std::move(vkBinding));
							setData.names.emplace_back(pBinding->name);
						}
					}
				}
			}

			spvReflectDestroyShaderModule(&reflectModule);
		}

		// build descriptor set layout
		{
			// find out how many descriptor layouts do we have
			uint32_t uMaxSet = 0u;
			int setCount = 0;
			for (const auto& _pair : mapDescriptorSets)
			{
				uint32_t uSetId = _pair.first;
				uMaxSet = std::max(uMaxSet, uSetId);
				++setCount;
			}
			if (setCount == 0) return;

			// update _uptrDescriptorSetLayouts, _mapSetBinding
			_descriptorSetData.reserve(static_cast<size_t>(uMaxSet + 1));
			for (uint32_t uSetId = 0u; uSetId <= uMaxSet; ++uSetId)
			{
				std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> curSet{};

				// fill descriptor setting in the layout and then initialize layout
				if (mapDescriptorSets.find(uSetId) != mapDescriptorSets.end())
				{
					const auto& descriptorData = mapDescriptorSets[uSetId];
					for (size_t i = 0; i < descriptorData.bindings.size(); ++i)
					{
						const auto& vkInfo = descriptorData.bindings[i];
						const auto& strDescriptorName = descriptorData.names[i];

						curSet.insert({ vkInfo.binding, vkInfo });
						_mapSetBinding[strDescriptorName] = { uSetId, vkInfo.binding };
					}
				}

				_descriptorSetData.push_back(std::move(curSet));
			}
		}

		//print info for debug
		{
			static const std::string& strIndent = "\t";
			for (const auto& mapPair : mapDescriptorSets)
			{
				uint32_t setId = mapPair.first;
				const auto& setData = mapPair.second;
				std::cout << "set: " << setId << std::endl;
				for (size_t i = 0; i < setData.bindings.size(); ++i)
				{
					const auto& vkBinding = setData.bindings[i];
					std::cout << strIndent << "binding name: " << setData.names[i] << std::endl;
					std::cout << strIndent << "location: " << vkBinding.binding << std::endl;
					std::cout << strIndent << "descriptor count: " << vkBinding.descriptorCount << std::endl;
					std::cout << strIndent << "descriptor type: ";
					_PrintDescriptorType(std::cout, vkBinding.descriptorType);
					std::cout << std::endl;
					std::cout << strIndent << "stage: ";
					_PrintStage(std::cout, vkBinding.stageFlags);
					std::cout << "\r\n\r\n";
				}
				std::cout << std::endl;
			}
		}
	}


#undef PRINT_STAGE(os, stage, bit)
#undef PRINT_TYPE(os, vkt, vkt2)
}

bool DescriptorSetManager::_GetDescriptorLocation(const std::string& _name, uint32_t& _set, uint32_t& _binding) const
{
	const auto itr = m_mapNameToSetBinding.find(_name);
	bool bResult = itr != m_mapNameToSetBinding.end();

	if (bResult)
	{
		_set = (*itr).second.first;
		_binding = (*itr).second.second;
	}

	return bResult;
}

bool DescriptorSetManager::_GetDescriptorSet(uint32_t _set, uint32_t _frame, DescriptorSet*& _pDescriptorSet) const
{
	bool bResult = _frame < m_uptrDescriptorSetsEachFrame.size() && _set < m_uptrDescriptorSetsEachFrame[0].size();

	if (bResult)
	{
		_pDescriptorSet = m_uptrDescriptorSetsEachFrame[_frame][_set].get();
	}

	return bResult;
}

bool DescriptorSetManager::_IsFirstTimeBind() const
{
	return m_vecDescriptorSetIsBoundForFrame[m_uCurrentFrame];
}

void DescriptorSetManager::_InitDescriptorSetLayouts()
{
	std::cout << "Create Descriptor Set Layout: \r\n";
	for (size_t setId = 0; setId < m_vkDescriptorSetBindingInfo.size(); ++setId)
	{
		const auto& bindings = m_vkDescriptorSetBindingInfo[setId];
		std::unique_ptr<DescriptorSetLayout> uptrLayout = std::make_unique<DescriptorSetLayout>();

		std::cout << "\tSet: " << setId << std::endl;

		for (const auto& binding : bindings)
		{
			uint32_t bindingId = binding.first;
			const VkDescriptorSetLayoutBinding& vkBinding = binding.second;

			uptrLayout->AddBinding(
				vkBinding.binding, 
				vkBinding.descriptorCount, 
				vkBinding.descriptorType, 
				vkBinding.stageFlags, 
				vkBinding.pImmutableSamplers);

			std::cout << "\t\tBinding: " << vkBinding.binding << std::endl;
			std::cout << "\t\tDescriptor Count: " << vkBinding.descriptorCount << std::endl;
			std::cout << "\t\tDescriptor Type: ";
			_PrintDescriptorType(std::cout, vkBinding.descriptorType);
			std::cout << std::endl;
			std::cout << "\t\tStages: ";
			_PrintStage(std::cout, vkBinding.stageFlags);
			std::cout << std::endl;
		}
		std::cout << std::endl;

		uptrLayout->Init();

		m_vecUptrDescriptorSetLayout.push_back(std::move(uptrLayout));
	}
}

void DescriptorSetManager::_UninitDescriptorSetLayouts()
{
	for (auto& uptrLayout : m_vecUptrDescriptorSetLayout)
	{
		uptrLayout->Uninit();
	}
	m_vecUptrDescriptorSetLayout.clear();
}

DescriptorSetManager::~DescriptorSetManager()
{
	Uninit();
}

void DescriptorSetManager::Init(const std::vector<std::string>& _pipelineShaders, uint32_t _uFrameInFlightCount)
{
	_ReflectDescriptorSet(_pipelineShaders, m_mapNameToSetBinding, m_vkDescriptorSetBindingInfo);
	m_uFrameInFlightCount = _uFrameInFlightCount;
	m_uCurrentFrame = 0u;
	m_vecDescriptorSetIsBoundForFrame.resize(m_uFrameInFlightCount, false);
}

void DescriptorSetManager::StartBind()
{
	m_vecDynamicOffset.clear();
}

void DescriptorSetManager::NextFrame()
{
	m_uCurrentFrame = (m_uCurrentFrame + 1) % m_uFrameInFlightCount;
}

void DescriptorSetManager::BindDescriptor(
	const std::string& _nameInShader, 
	const std::vector<VkDescriptorBufferInfo>& _data, 
	bool _bForce)
{
	uint32_t uSet = ~0;
	uint32_t uBinding = ~0;

	if (!(_bForce || _IsFirstTimeBind())) return;

	CHECK_TRUE(_GetDescriptorLocation(_nameInShader, uSet, uBinding), "Cannot find binding!");

	BindDescriptor(uSet, uBinding, _data, _bForce);
}

void DescriptorSetManager::BindDescriptor(
	uint32_t _set, 
	uint32_t _binding, 
	const std::vector<VkDescriptorBufferInfo>& _data, 
	bool _bForce)
{
	if (!(_bForce || _IsFirstTimeBind())) return;

	m_vecLateDescriptorUpdates.push_back(
		[_set, _binding, _data, this]
		{
			DescriptorSet* pDescriptorSet = nullptr;
			CHECK_TRUE(_GetDescriptorSet(_set, m_uCurrentFrame, pDescriptorSet), "Cannot find descriptor set!");
			pDescriptorSet->UpdateBinding(_binding, _data);
		});
}

void DescriptorSetManager::BindDescriptor(
	const std::string& _nameInShader, 
	const std::vector<VkDescriptorBufferInfo>& _data, 
	const std::vector<uint32_t>& _dynmaicOffsets, 
	bool _bForce)
{
	uint32_t uSet = ~0;
	uint32_t uBinding = ~0;

	CHECK_TRUE(_GetDescriptorLocation(_nameInShader, uSet, uBinding), "Cannot find binding!");

	BindDescriptor(uSet, uBinding, _data, _dynmaicOffsets, _bForce);
}

void DescriptorSetManager::BindDescriptor(
	uint32_t _set, 
	uint32_t _binding, 
	const std::vector<VkDescriptorBufferInfo>& _data, 
	const std::vector<uint32_t>& _dynmaicOffsets, 
	bool _bForce)
{
	bool bIsFirstTimeBind = _IsFirstTimeBind();

	// dynamic offset is cleared each time we bind, so we add dynamic offsets to record
	{
		DynamicOffsetRecord record{};

		record.binding = _binding;
		record.set = _set;
		record.offsets = _dynmaicOffsets;

		m_vecDynamicOffset.push_back(record);
	}
	
	if (!(_bForce || bIsFirstTimeBind)) return;

	CHECK_TRUE(
		m_vkDescriptorSetBindingInfo.size() > _set
		&& m_vkDescriptorSetBindingInfo[_set].find(_binding) != m_vkDescriptorSetBindingInfo[_set].end(),
		"Do not have descriptor info!");

	// now update the descriptor type to dynamic
	if (bIsFirstTimeBind)
	{
		auto& vkInfo = m_vkDescriptorSetBindingInfo[_set][_binding];
		switch (vkInfo.descriptorType)
		{
		case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
		{
			vkInfo.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
			break;
		}
		case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
		{
			vkInfo.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
			break;
		}
		case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
		case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
		{
			break;
		}
		default:
		{
			CHECK_TRUE(false, "This cannot be a dynamic bound!");
			break;
		}
		}
	}

	m_vecLateDescriptorUpdates.push_back(
		[_set, _binding, _data, this]
		{
			DescriptorSet* pDescriptorSet = nullptr;
			CHECK_TRUE(_GetDescriptorSet(_set, m_uCurrentFrame, pDescriptorSet), "Cannot find descriptor set!");
			pDescriptorSet->UpdateBinding(_binding, _data);
		});
}

void DescriptorSetManager::BindDescriptor(
	const std::string& _nameInShader,
	const std::vector<VkDescriptorImageInfo>& _data,
	bool _bForce)
{
	uint32_t uSet = ~0;
	uint32_t uBinding = ~0;
	if (!(_bForce || _IsFirstTimeBind())) return;
	CHECK_TRUE(_GetDescriptorLocation(_nameInShader, uSet, uBinding), "Cannot find binding!");

	BindDescriptor(uSet, uBinding, _data, _bForce);
}

void DescriptorSetManager::BindDescriptor(
	uint32_t _set,
	uint32_t _binding,
	const std::vector<VkDescriptorImageInfo>& _data,
	bool _bForce)
{
	if (!(_bForce || _IsFirstTimeBind())) return;

	m_vecLateDescriptorUpdates.push_back(
		[_set, _binding, _data, this]
		{
			DescriptorSet* pDescriptorSet = nullptr;
			CHECK_TRUE(_GetDescriptorSet(_set, m_uCurrentFrame, pDescriptorSet), "Cannot find descriptor set!");
			pDescriptorSet->UpdateBinding(_binding, _data);
		});
}

void DescriptorSetManager::BindDescriptor(
	const std::string& _nameInShader,
	const std::vector<VkBufferView>& _data,
	bool _bForce)
{
	uint32_t uSet = ~0;
	uint32_t uBinding = ~0;
	if (!(_bForce || _IsFirstTimeBind())) return;
	CHECK_TRUE(_GetDescriptorLocation(_nameInShader, uSet, uBinding), "Cannot find binding!");

	BindDescriptor(uSet, uBinding, _data, _bForce);
}

void DescriptorSetManager::BindDescriptor(
	uint32_t _set,
	uint32_t _binding,
	const std::vector<VkBufferView>& _data,
	bool _bForce)
{	
	if (!(_bForce || _IsFirstTimeBind())) return;

	m_vecLateDescriptorUpdates.push_back(
		[_set, _binding, _data, this]
		{ 
			DescriptorSet* pDescriptorSet = nullptr;
			CHECK_TRUE(_GetDescriptorSet(_set, m_uCurrentFrame, pDescriptorSet), "Cannot find descriptor set!");
			pDescriptorSet->UpdateBinding(_binding, _data);
		});
}

void DescriptorSetManager::BindDescriptor(
	const std::string& _nameInShader,
	const std::vector<VkAccelerationStructureKHR>& _data,
	bool _bForce)
{
	uint32_t uSet = ~0;
	uint32_t uBinding = ~0;
	if (!(_bForce || _IsFirstTimeBind())) return;
	CHECK_TRUE(_GetDescriptorLocation(_nameInShader, uSet, uBinding), "Cannot find binding!");

	BindDescriptor(uSet, uBinding, _data, _bForce);
}

void DescriptorSetManager::BindDescriptor(
	uint32_t _set,
	uint32_t _binding,
	const std::vector<VkAccelerationStructureKHR>& _data,
	bool _bForce)
{
	if (!(_bForce || _IsFirstTimeBind())) return;

	m_vecLateDescriptorUpdates.push_back(
		[_set, _binding, _data, this]
		{
			DescriptorSet* pDescriptorSet = nullptr;
			CHECK_TRUE(_GetDescriptorSet(_set, m_uCurrentFrame, pDescriptorSet), "Cannot find descriptor set!");
			pDescriptorSet->UpdateBinding(_binding, _data);
		});
}

void DescriptorSetManager::EndBind()
{
	uint32_t uDescriptorSetCount = m_vkDescriptorSetBindingInfo.size();

	if (!m_bDescriptorSetInitialized)
	{
		_InitDescriptorSetLayouts();

		m_uptrDescriptorSetsEachFrame.clear();
		m_uptrDescriptorSetsEachFrame.reserve(m_uFrameInFlightCount);
		for (uint32_t i = 0u; i < m_uFrameInFlightCount; ++i)
		{
			m_uptrDescriptorSetsEachFrame.push_back({});
			m_uptrDescriptorSetsEachFrame[i].reserve(m_vecUptrDescriptorSetLayout.size());
			for (size_t j = 0u; j < m_vecUptrDescriptorSetLayout.size(); ++j)
			{
				std::unique_ptr<DescriptorSet> uptrDescriptorSet = nullptr;
				uptrDescriptorSet.reset(m_vecUptrDescriptorSetLayout[j]->NewDescriptorSetPointer());
				uptrDescriptorSet->Init();
				m_uptrDescriptorSetsEachFrame[i].push_back(std::move(uptrDescriptorSet));
			}
		}

		m_bDescriptorSetInitialized = true;
	}

	for (uint32_t i = 0u; i < uDescriptorSetCount; ++i)
	{
		DescriptorSet* pDescriptorSet = nullptr;
		CHECK_TRUE(_GetDescriptorSet(i, m_uCurrentFrame, pDescriptorSet), "Cannot get descriptor set!");
		pDescriptorSet->StartUpdate();
	}

	for (auto& func : m_vecLateDescriptorUpdates)
	{
		func();
	}
	m_vecLateDescriptorUpdates.clear();

	for (uint32_t i = 0u; i < uDescriptorSetCount; ++i)
	{
		DescriptorSet* pDescriptorSet = nullptr;
		CHECK_TRUE(_GetDescriptorSet(i, m_uCurrentFrame, pDescriptorSet), "Cannot get descriptor set!");
		pDescriptorSet->FinishUpdate();
	}

	// descriptor set is bound for this frame
	m_vecDescriptorSetIsBoundForFrame[m_uCurrentFrame] = true;
}

void DescriptorSetManager::GetCurrentDescriptorSets(
	std::vector<VkDescriptorSet>& _outDescriptorSets, 
	std::vector<uint32_t>& _outDynamicOffsets)
{
	const auto& uptrDescriptorSetsThisFrame = m_uptrDescriptorSetsEachFrame[m_uCurrentFrame];
	
	// see specification of dynamic offsets
	// https://registry.khronos.org/vulkan/specs/latest/man/html/vkCmdBindDescriptorSets.html
	std::sort(m_vecDynamicOffset.begin(), m_vecDynamicOffset.end());
	
	for (const auto& dynamicOffsets : m_vecDynamicOffset)
	{
		_outDynamicOffsets.insert(_outDynamicOffsets.end(), dynamicOffsets.offsets.begin(), dynamicOffsets.offsets.end());
	}

	for (const auto& uptrDescriptorSetThisFrame : uptrDescriptorSetsThisFrame)
	{
		_outDescriptorSets.push_back(uptrDescriptorSetThisFrame->vkDescriptorSet);
	}
}

void DescriptorSetManager::Uninit()
{
	m_mapNameToSetBinding.clear();
	m_uFrameInFlightCount = 0;
	m_uCurrentFrame = 0;
	m_vecLateDescriptorUpdates.clear();
	m_vkDescriptorSetBindingInfo.clear();
	m_vecDescriptorSetIsBoundForFrame.clear();

	if (m_bDescriptorSetInitialized)
	{
		m_uptrDescriptorSetsEachFrame.clear();
		m_bDescriptorSetInitialized = false;
	}
	for (auto& uptrDescriptorSetLayout : m_vecUptrDescriptorSetLayout)
	{
		uptrDescriptorSetLayout->Uninit();
	}
	m_vecUptrDescriptorSetLayout.clear();
}
