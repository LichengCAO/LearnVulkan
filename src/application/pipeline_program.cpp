#include "pipeline_program.h"
#include <algorithm>

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

void DescriptorSetManager::_InitDescriptorSetLayouts()
{
	for (size_t setId = 0; setId < m_vkDescriptorSetBindingInfo.size(); ++setId)
	{
		const auto& bindings = m_vkDescriptorSetBindingInfo[setId];
		std::unique_ptr<DescriptorSetLayout> uptrLayout = std::make_unique<DescriptorSetLayout>();

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
		}

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

void DescriptorSetManager::_PlanBinding(const BindPlan& _bindPlan)
{
	m_currentPlan.emplace_back(_bindPlan);
}

void DescriptorSetManager::_ProcessPlan()
{
	std::vector<DESCRIPTOR_BIND_SETTING> descriptorSetTypes;
	std::vector<DescriptorSetData*> pDescriptorSetDatas;
	size_t setCount = m_vkDescriptorSetBindingInfo.size();
	static bool bMapInited = false;
	static std::map<DESCRIPTOR_BIND_SETTING, std::map<DESCRIPTOR_BIND_SETTING, DESCRIPTOR_BIND_SETTING>> mapType;
	if (!bMapInited)
	{
		auto key = DESCRIPTOR_BIND_SETTING::CONSTANT_DESCRIPTOR_SET_ACROSS_FRAMES;
		mapType[key][DESCRIPTOR_BIND_SETTING::CONSTANT_DESCRIPTOR_SET_ACROSS_FRAMES] = 
			DESCRIPTOR_BIND_SETTING::CONSTANT_DESCRIPTOR_SET_ACROSS_FRAMES;
		mapType[key][DESCRIPTOR_BIND_SETTING::DEDICATE_DESCRIPTOR_SET_ACROSS_FRAMES] = 
			DESCRIPTOR_BIND_SETTING::DEDICATE_DESCRIPTOR_SET_ACROSS_FRAMES;
		mapType[key][DESCRIPTOR_BIND_SETTING::CONSTANT_DESCRIPTOR_SET_PER_FRAME] = 
			DESCRIPTOR_BIND_SETTING::CONSTANT_DESCRIPTOR_SET_PER_FRAME;
		mapType[key][DESCRIPTOR_BIND_SETTING::DEDICATE_DESCRIPTOR_SET_PER_FRAME] = 
			DESCRIPTOR_BIND_SETTING::DEDICATE_DESCRIPTOR_SET_PER_FRAME;
		
		key = DESCRIPTOR_BIND_SETTING::DEDICATE_DESCRIPTOR_SET_ACROSS_FRAMES;
		mapType[key][DESCRIPTOR_BIND_SETTING::CONSTANT_DESCRIPTOR_SET_ACROSS_FRAMES] =
			DESCRIPTOR_BIND_SETTING::DEDICATE_DESCRIPTOR_SET_ACROSS_FRAMES;
		mapType[key][DESCRIPTOR_BIND_SETTING::DEDICATE_DESCRIPTOR_SET_ACROSS_FRAMES] =
			DESCRIPTOR_BIND_SETTING::DEDICATE_DESCRIPTOR_SET_ACROSS_FRAMES;
		mapType[key][DESCRIPTOR_BIND_SETTING::CONSTANT_DESCRIPTOR_SET_PER_FRAME] =
			DESCRIPTOR_BIND_SETTING::DEDICATE_DESCRIPTOR_SET_PER_FRAME;
		mapType[key][DESCRIPTOR_BIND_SETTING::DEDICATE_DESCRIPTOR_SET_PER_FRAME] =
			DESCRIPTOR_BIND_SETTING::DEDICATE_DESCRIPTOR_SET_PER_FRAME;

		key = DESCRIPTOR_BIND_SETTING::CONSTANT_DESCRIPTOR_SET_PER_FRAME;
		mapType[key][DESCRIPTOR_BIND_SETTING::CONSTANT_DESCRIPTOR_SET_ACROSS_FRAMES] =
			DESCRIPTOR_BIND_SETTING::CONSTANT_DESCRIPTOR_SET_PER_FRAME;
		mapType[key][DESCRIPTOR_BIND_SETTING::DEDICATE_DESCRIPTOR_SET_ACROSS_FRAMES] =
			DESCRIPTOR_BIND_SETTING::DEDICATE_DESCRIPTOR_SET_PER_FRAME;
		mapType[key][DESCRIPTOR_BIND_SETTING::CONSTANT_DESCRIPTOR_SET_PER_FRAME] =
			DESCRIPTOR_BIND_SETTING::CONSTANT_DESCRIPTOR_SET_PER_FRAME;
		mapType[key][DESCRIPTOR_BIND_SETTING::DEDICATE_DESCRIPTOR_SET_PER_FRAME] =
			DESCRIPTOR_BIND_SETTING::DEDICATE_DESCRIPTOR_SET_PER_FRAME;

		key = DESCRIPTOR_BIND_SETTING::DEDICATE_DESCRIPTOR_SET_PER_FRAME;
		mapType[key][DESCRIPTOR_BIND_SETTING::CONSTANT_DESCRIPTOR_SET_ACROSS_FRAMES] =
			DESCRIPTOR_BIND_SETTING::DEDICATE_DESCRIPTOR_SET_PER_FRAME;
		mapType[key][DESCRIPTOR_BIND_SETTING::DEDICATE_DESCRIPTOR_SET_ACROSS_FRAMES] =
			DESCRIPTOR_BIND_SETTING::DEDICATE_DESCRIPTOR_SET_PER_FRAME;
		mapType[key][DESCRIPTOR_BIND_SETTING::CONSTANT_DESCRIPTOR_SET_PER_FRAME] =
			DESCRIPTOR_BIND_SETTING::DEDICATE_DESCRIPTOR_SET_PER_FRAME;
		mapType[key][DESCRIPTOR_BIND_SETTING::DEDICATE_DESCRIPTOR_SET_PER_FRAME] =
			DESCRIPTOR_BIND_SETTING::DEDICATE_DESCRIPTOR_SET_PER_FRAME;

		bMapInited = true;
	}

	// find out descriptor set allocate stratigies
	descriptorSetTypes.resize(setCount, DESCRIPTOR_BIND_SETTING::CONSTANT_DESCRIPTOR_SET_ACROSS_FRAMES);
	for (const auto& plan : m_currentPlan)
	{
		CHECK_TRUE(mapType.find(plan.bindType) != mapType.end() 
			&& mapType[plan.bindType].find(descriptorSetTypes[plan.set]) != mapType[plan.bindType].end(),
			"Not define for this pair of descriptor set allocations");
		descriptorSetTypes[plan.set] = mapType[plan.bindType][descriptorSetTypes[plan.set]];
	}

	// prepare descriptor sets
	for (size_t i = 0; i < setCount; ++i)
	{
		switch (descriptorSetTypes[i])
		{
		case DESCRIPTOR_BIND_SETTING::CONSTANT_DESCRIPTOR_SET_ACROSS_FRAMES:
		{
			size_t j = m_uptrDescriptorSetSetting0.size();
			while (j <= i )
			{
				std::unique_ptr<DescriptorSetData> setData = std::make_unique<DescriptorSetData>();
				auto pDescriptorSet = m_vecUptrDescriptorSetLayout[j]->NewDescriptorSetPointer();
				pDescriptorSet->Init();
				setData->uptrDescriptorSet.reset(pDescriptorSet);
				m_uptrDescriptorSetSetting0.push_back(std::move(setData));
				++j;
			}
			pDescriptorSetDatas.push_back(m_uptrDescriptorSetSetting0[i].get());
		}
			break;
		case DESCRIPTOR_BIND_SETTING::CONSTANT_DESCRIPTOR_SET_PER_FRAME:
		{
			size_t j = 0;
			m_uptrDescriptorSetSetting1.resize(m_uFrameInFlightCount);
			j = m_uptrDescriptorSetSetting1[m_uCurrentFrame].size();
			while (j <= i)
			{
				std::unique_ptr<DescriptorSetData> setData = std::make_unique<DescriptorSetData>();
				auto pDescriptorSet = m_vecUptrDescriptorSetLayout[j]->NewDescriptorSetPointer();
				pDescriptorSet->Init();
				setData->uptrDescriptorSet.reset(pDescriptorSet);
				m_uptrDescriptorSetSetting1[m_uCurrentFrame].push_back(std::move(setData));
				++j;
			}
			pDescriptorSetDatas.push_back(m_uptrDescriptorSetSetting1[m_uCurrentFrame][i].get());
		}
			break;
		case DESCRIPTOR_BIND_SETTING::DEDICATE_DESCRIPTOR_SET_PER_FRAME:
		{
			std::vector<size_t> planIds;// all plan to bind to this descriptor set
			size_t planId = 0;
			bool   needToCreateDescriptorSet = false;
			DescriptorSetData* pDescriptorSetData = nullptr;

			// find out all bind plan for this set
			for (const auto& plan : m_currentPlan)
			{
				if (plan.set == i)
				{
					planIds.push_back(planId);
				}
				++planId;
			}
			m_uptrDescriptorSetSetting2.resize(m_uFrameInFlightCount);
			m_uptrDescriptorSetSetting2[m_uCurrentFrame].resize(setCount);
			needToCreateDescriptorSet = (needToCreateDescriptorSet || m_uptrDescriptorSetSetting2[m_uCurrentFrame][i].size() == 0);
			
			// find valid descriptor set in current descriptor sets
			if (!needToCreateDescriptorSet)
			{
				for (const auto& uptrDescriptorSetData : m_uptrDescriptorSetSetting2[m_uCurrentFrame][i])
				{
					bool isValid = true;
					for (auto planId : planIds)
					{
						const auto& plan = m_currentPlan[planId];
						const auto& record = uptrDescriptorSetData->bindInfo[plan.location]; // https://helloacm.com/c-access-a-non-existent-key-in-stdmap-or-stdunordered_map/
						if (record.IsBound())
						{
							if (plan.bufferBind.size() > 0)
							{
								isValid = record.IsBindTo(plan.bufferBind);
							}
							else if (plan.imageBind.size() > 0)
							{
								isValid = record.IsBindTo(plan.imageBind);
							}
							else if (plan.viewBind.size() > 0)
							{
								isValid = record.IsBindTo(plan.viewBind);
							}
							else if (plan.accelStructBind.size() > 0)
							{
								isValid = record.IsBindTo(plan.accelStructBind);
							}
						}
						if (!isValid) break;
					}
					if (isValid)
					{
						pDescriptorSetData = uptrDescriptorSetData.get();
						break;
					}
				}
			}
			needToCreateDescriptorSet = (needToCreateDescriptorSet || pDescriptorSetData == nullptr);

			// create a new descriptor set if current descriptor sets doesn't meet requirements
			if (needToCreateDescriptorSet)
			{
				std::unique_ptr<DescriptorSetData> setData = std::make_unique<DescriptorSetData>();
				DescriptorSet* pDescriptorSet = m_vecUptrDescriptorSetLayout[i]->NewDescriptorSetPointer();
				pDescriptorSet->Init();
				setData->uptrDescriptorSet.reset(pDescriptorSet);
				pDescriptorSetData = setData.get();
				m_uptrDescriptorSetSetting2[m_uCurrentFrame][i].push_back(std::move(setData));
			}
			pDescriptorSetDatas.push_back(pDescriptorSetData);
		}
			break;
		case DESCRIPTOR_BIND_SETTING::DEDICATE_DESCRIPTOR_SET_ACROSS_FRAMES:
		{
			std::vector<size_t> planIds;// all plan to bind to this descriptor set
			size_t planId = 0;
			bool   needToCreateDescriptorSet = false;
			DescriptorSetData* pDescriptorSetData = nullptr;

			m_uptrDescriptorSetSetting3.resize(setCount);

			// find out all bind plan for this set
			for (const auto& plan : m_currentPlan)
			{
				if (plan.set == i)
				{
					planIds.push_back(planId);
				}
				++planId;
			}
			
			needToCreateDescriptorSet = m_uptrDescriptorSetSetting3[i].size() == 0;
			
			// try to find out if current descriptor set is ok for this bind
			if (!needToCreateDescriptorSet)
			{
				for (const auto& uptrDescriptorSetData : m_uptrDescriptorSetSetting3[i])
				{
					bool isValid = true;
					for (auto planId : planIds)
					{
						const auto& plan = m_currentPlan[planId];
						const auto& record = uptrDescriptorSetData->bindInfo[plan.location]; // https://helloacm.com/c-access-a-non-existent-key-in-stdmap-or-stdunordered_map/
						if (record.IsBound())
						{
							if (plan.bufferBind.size() > 0)
							{
								isValid = record.IsBindTo(plan.bufferBind);
							}
							else if (plan.imageBind.size() > 0)
							{
								isValid = record.IsBindTo(plan.imageBind);
							}
							else if (plan.viewBind.size() > 0)
							{
								isValid = record.IsBindTo(plan.viewBind);
							}
							else if (plan.accelStructBind.size() > 0)
							{
								isValid = record.IsBindTo(plan.accelStructBind);
							}
						}
						if (!isValid) break;
					}
					if (isValid)
					{
						pDescriptorSetData = uptrDescriptorSetData.get();
						break;
					}
				}
			}
			needToCreateDescriptorSet = needToCreateDescriptorSet || pDescriptorSetData == nullptr;
			
			// create a new descriptor set if current descriptor sets doesn't meet requirements
			if (needToCreateDescriptorSet)
			{
				std::unique_ptr<DescriptorSetData> setData = std::make_unique<DescriptorSetData>();
				DescriptorSet* pDescriptorSet = m_vecUptrDescriptorSetLayout[i]->NewDescriptorSetPointer();
				pDescriptorSet->Init();
				setData->uptrDescriptorSet.reset(pDescriptorSet);
				pDescriptorSetData = setData.get();
				m_uptrDescriptorSetSetting3[i].push_back(std::move(setData));
			}
			pDescriptorSetDatas.push_back(pDescriptorSetData);
		}
			break;
		default:
			CHECK_TRUE(false, "No such descriptor set type!");
			break;
		}
	}

	// now, do the binding
	for (auto& pDescriptorSetData : pDescriptorSetDatas)
	{
		pDescriptorSetData->uptrDescriptorSet->StartUpdate();
	}
	for (const auto& plan : m_currentPlan)
	{
		auto pDescriptorSetData = pDescriptorSetDatas[plan.set];
		auto pDescriptorSet = pDescriptorSetData->uptrDescriptorSet.get();
		auto curBindInfo = pDescriptorSetData->bindInfo[plan.location];
		if (plan.bufferBind.size() > 0 && (!curBindInfo.IsBindTo(plan.bufferBind)))
		{
			pDescriptorSet->UpdateBinding(plan.location, plan.bufferBind);
			curBindInfo.varBinding = plan.bufferBind[0];
		}
		else if (plan.imageBind.size() > 0 && (!curBindInfo.IsBindTo(plan.imageBind)))
		{
			pDescriptorSet->UpdateBinding(plan.location, plan.imageBind);
			curBindInfo.varBinding = plan.imageBind[0];
		}
		else if (plan.viewBind.size() > 0 && (!curBindInfo.IsBindTo(plan.viewBind)))
		{
			pDescriptorSet->UpdateBinding(plan.location, plan.viewBind);
			curBindInfo.varBinding = plan.viewBind[0];
		}
		else if (plan.accelStructBind.size() > 0 && (!curBindInfo.IsBindTo(plan.accelStructBind)))
		{
			pDescriptorSet->UpdateBinding(plan.location, plan.accelStructBind);
			//curBindInfo.varBinding = plan.accelStructBind[0];
		}
		else
		{
			assert(false);
		}
	}
	for (auto& pDescriptorSetData : pDescriptorSetDatas)
	{
		pDescriptorSetData->uptrDescriptorSet->FinishUpdate();
		m_pCurrentDescriptorSets.push_back(pDescriptorSetData->uptrDescriptorSet.get());
	}
}

DescriptorSetManager::~DescriptorSetManager()
{
	Uninit();
}

void DescriptorSetManager::Init(const std::vector<std::string>& _pipelineShaders, uint32_t _uFrameInFlightCount)
{
	ShaderReflector reflector{};

	reflector.Init(_pipelineShaders);
	reflector.ReflectDescriptorSets(m_mapNameToSetBinding, m_vkDescriptorSetBindingInfo);
	reflector.PrintReflectResult();
	reflector.Uninit();

	m_uFrameInFlightCount = _uFrameInFlightCount;
	m_uCurrentFrame = 0u;
}

void DescriptorSetManager::StartBind()
{
	m_vecDynamicOffset.clear();
	m_pCurrentDescriptorSets.clear();
}

void DescriptorSetManager::NextFrame()
{
	m_uCurrentFrame = (m_uCurrentFrame + 1) % m_uFrameInFlightCount;
}

void DescriptorSetManager::BindDescriptor(
	const std::string& _nameInShader,
	const std::vector<VkDescriptorBufferInfo>& _data,
	DESCRIPTOR_BIND_SETTING _setting)
{
	uint32_t uSet = ~0;
	uint32_t uBinding = ~0;

	CHECK_TRUE(_GetDescriptorLocation(_nameInShader, uSet, uBinding), "Cannot find binding!");

	BindDescriptor(uSet, uBinding, _data, _setting);
}

void DescriptorSetManager::BindDescriptor(
	uint32_t _set,
	uint32_t _binding,
	const std::vector<VkDescriptorBufferInfo>& _data,
	DESCRIPTOR_BIND_SETTING _setting)
{
	BindPlan plan{};
	plan.bindType = _setting;
	plan.set = _set;
	plan.location = _binding;
	plan.bufferBind = _data;
	_PlanBinding(plan);
}

void DescriptorSetManager::BindDescriptor(
	const std::string& _nameInShader,
	const std::vector<VkDescriptorBufferInfo>& _data,
	const std::vector<uint32_t>& _dynmaicOffsets,
	DESCRIPTOR_BIND_SETTING _setting)
{
	uint32_t uSet = ~0;
	uint32_t uBinding = ~0;

	CHECK_TRUE(_GetDescriptorLocation(_nameInShader, uSet, uBinding), "Cannot find binding!");

	BindDescriptor(uSet, uBinding, _data, _dynmaicOffsets, _setting);
}

void DescriptorSetManager::BindDescriptor(
	uint32_t _set,
	uint32_t _binding,
	const std::vector<VkDescriptorBufferInfo>& _data,
	const std::vector<uint32_t>& _dynmaicOffsets,
	DESCRIPTOR_BIND_SETTING _setting)
{
	DynamicOffsetRecord record{};
	BindPlan plan{};

	plan.bindType = _setting;
	plan.bufferBind = _data;
	plan.set = _set;
	plan.location = _binding;
	_PlanBinding(plan);
	
	// dynamic offset is cleared each time we bind, so we add dynamic offsets to record
	record.binding = _binding;
	record.set = _set;
	record.offsets = _dynmaicOffsets;
	m_vecDynamicOffset.push_back(record);

	CHECK_TRUE(
		m_vkDescriptorSetBindingInfo.size() > _set
		&& m_vkDescriptorSetBindingInfo[_set].find(_binding) != m_vkDescriptorSetBindingInfo[_set].end(),
		"Do not have descriptor info!");

	// now update the descriptor type to dynamic
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
}

void DescriptorSetManager::BindDescriptor(
	const std::string& _nameInShader,
	const std::vector<VkDescriptorImageInfo>& _data,
	DESCRIPTOR_BIND_SETTING _setting)
{
	uint32_t uSet = ~0;
	uint32_t uBinding = ~0;
	CHECK_TRUE(_GetDescriptorLocation(_nameInShader, uSet, uBinding), "Cannot find binding!");

	BindDescriptor(uSet, uBinding, _data, _setting);
}

void DescriptorSetManager::BindDescriptor(
	uint32_t _set,
	uint32_t _binding,
	const std::vector<VkDescriptorImageInfo>& _data,
	DESCRIPTOR_BIND_SETTING _setting)
{
	BindPlan plan{};
	plan.bindType = _setting;
	plan.imageBind = _data;
	plan.set = _set;
	plan.location = _binding;

	_PlanBinding(plan);
}

void DescriptorSetManager::BindDescriptor(
	const std::string& _nameInShader,
	const std::vector<VkBufferView>& _data,
	DESCRIPTOR_BIND_SETTING _setting)
{
	uint32_t uSet = ~0;
	uint32_t uBinding = ~0;

	CHECK_TRUE(_GetDescriptorLocation(_nameInShader, uSet, uBinding), "Cannot find binding!");

	BindDescriptor(uSet, uBinding, _data, _setting);
}

void DescriptorSetManager::BindDescriptor(
	uint32_t _set,
	uint32_t _binding,
	const std::vector<VkBufferView>& _data,
	DESCRIPTOR_BIND_SETTING _setting)
{
	BindPlan plan{};
	plan.viewBind = _data;
	plan.bindType = _setting;
	plan.set = _set;
	plan.location = _binding;
	_PlanBinding(plan);
}

void DescriptorSetManager::BindDescriptor(
	const std::string& _nameInShader,
	const std::vector<VkAccelerationStructureKHR>& _data,
	DESCRIPTOR_BIND_SETTING _setting)
{
	uint32_t uSet = ~0;
	uint32_t uBinding = ~0;

	CHECK_TRUE(_GetDescriptorLocation(_nameInShader, uSet, uBinding), "Cannot find binding!");

	BindDescriptor(uSet, uBinding, _data, _setting);
}

void DescriptorSetManager::BindDescriptor(
	uint32_t _set,
	uint32_t _binding,
	const std::vector<VkAccelerationStructureKHR>& _data,
	DESCRIPTOR_BIND_SETTING _setting)
{
	BindPlan plan{};
	plan.bindType = _setting;
	plan.accelStructBind = _data;
	plan.set = _set;
	plan.location = _binding;

	_PlanBinding(plan);
}

void DescriptorSetManager::EndBind()
{
	uint32_t uDescriptorSetCount = m_vkDescriptorSetBindingInfo.size();

	if (m_vecUptrDescriptorSetLayout.size() == 0)
	{
		_InitDescriptorSetLayouts();
	}

	_ProcessPlan();
}

void DescriptorSetManager::GetCurrentDescriptorSets(
	std::vector<VkDescriptorSet>& _outDescriptorSets,
	std::vector<uint32_t>& _outDynamicOffsets)
{
	// see specification of dynamic offsets
	// https://registry.khronos.org/vulkan/specs/latest/man/html/vkCmdBindDescriptorSets.html
	std::sort(m_vecDynamicOffset.begin(), m_vecDynamicOffset.end());

	for (const auto& dynamicOffsets : m_vecDynamicOffset)
	{
		_outDynamicOffsets.insert(_outDynamicOffsets.end(), dynamicOffsets.offsets.begin(), dynamicOffsets.offsets.end());
	}

	for (const auto& pDescriptorSetThisFrame : m_pCurrentDescriptorSets)
	{
		_outDescriptorSets.push_back(pDescriptorSetThisFrame->vkDescriptorSet);
	}
}

void DescriptorSetManager::GetDescriptorSetLayouts(std::vector<VkDescriptorSetLayout>& _outDescriptorSetLayouts)
{
	for (const auto& uptrDescriptorSetLayout : m_vecUptrDescriptorSetLayout)
	{
		_outDescriptorSetLayouts.push_back(uptrDescriptorSetLayout->vkDescriptorSetLayout);
	}
}

void DescriptorSetManager::Uninit()
{
	m_mapNameToSetBinding.clear();
	m_vkDescriptorSetBindingInfo.clear();
	m_uptrDescriptorSetSetting0.clear();
	m_uptrDescriptorSetSetting1.clear();
	m_uptrDescriptorSetSetting2.clear();
	m_uptrDescriptorSetSetting3.clear();
	_UninitDescriptorSetLayouts();
	m_currentPlan.clear();
	m_pCurrentDescriptorSets.clear();
	m_vecDynamicOffset.clear();
	m_uFrameInFlightCount = 0;
	m_uCurrentFrame = 0;
}

DescriptorSetManager::DescriptorSetData::~DescriptorSetData()
{
	if (uptrDescriptorSet.get())
	{
		uptrDescriptorSet.reset();
	}
	bindInfo.clear();
}

bool DescriptorSetManager::DescriptorBindRecord::IsBound() const
{
	bool result = false;

	//if (!std::holds_alternative<std::monostate>(varBinding))
	//{
	//	result = true;
	//}

	return result;
}

bool DescriptorSetManager::DescriptorBindRecord::IsBindTo(const std::vector<VkDescriptorImageInfo>& _input) const
{
	bool result = false;
	
	if (const auto pVar = std::get_if<VkDescriptorImageInfo>(&varBinding))
	{
		result = (pVar->imageView == _input[0].imageView);
	}

	return result;
}

bool DescriptorSetManager::DescriptorBindRecord::IsBindTo(const std::vector<VkBufferView>& _input) const
{
	bool result = false;

	if (const auto pVar = std::get_if<VkBufferView>(&varBinding))
	{
		VkBufferView vkView = *pVar;
		result = (vkView == _input[0]);
	}

	return result;
}

bool DescriptorSetManager::DescriptorBindRecord::IsBindTo(const std::vector<VkDescriptorBufferInfo>& _input) const
{
	bool result = false;

	if (const auto pVar = std::get_if<VkDescriptorBufferInfo>(&varBinding))
	{
		result = (pVar->buffer == _input[0].buffer);
	}

	return result;
}

bool DescriptorSetManager::DescriptorBindRecord::IsBindTo(const std::vector<VkAccelerationStructureKHR>& _input) const
{
	bool result = false;

	//if (const auto pVar = std::get_if<VkAccelerationStructureKHR>(&varBinding))
	//{
	//	VkAccelerationStructureKHR vkAccelStruct = *pVar;
	//	result = (vkAccelStruct == _input[0]);
	//}

	return result;
}