#include "descriptor_set.h"
#include "device.h"
#include "buffer.h"

void DescriptorSet::SetLayout(const DescriptorSetLayout* _layout)
{
	m_pLayout = _layout;
}

void DescriptorSet::Init()
{
	if (m_pLayout == nullptr)
	{
		throw std::runtime_error("No descriptor layout set!");
	}
	CHECK_TRUE(vkDescriptorSet == VK_NULL_HANDLE, "VkDescriptorSet is already created!");
	MyDevice::GetInstance().descriptorAllocator.Allocate(&vkDescriptorSet, m_pLayout->vkDescriptorSetLayout);
}

void DescriptorSet::StartUpdate()
{
	if (m_pLayout == nullptr)
	{
		throw std::runtime_error("Layout is not set!");
	}
	if (vkDescriptorSet == VK_NULL_HANDLE)
	{
		throw std::runtime_error("Descriptorset is not initialized");
	}
	m_updates.clear();
}

void DescriptorSet::UpdateBinding(uint32_t bindingId, const std::vector<VkDescriptorImageInfo>& dImageInfo)
{
	DescriptorSetUpdate newUpdate{};
	newUpdate.binding = bindingId;
	newUpdate.imageInfos = dImageInfo;
	newUpdate.descriptorType = DescriptorType::IMAGE;
	m_updates.push_back(newUpdate);
}

void DescriptorSet::UpdateBinding(uint32_t bindingId, const std::vector<VkBufferView>& bufferViews)
{
	DescriptorSetUpdate newUpdate{};

	newUpdate.binding = bindingId;
	newUpdate.bufferViews = bufferViews;
	newUpdate.descriptorType = DescriptorType::TEXEL_BUFFER;
	m_updates.push_back(newUpdate);
}

void DescriptorSet::UpdateBinding(uint32_t bindingId, const std::vector<VkDescriptorBufferInfo>& bufferInfos)
{
	DescriptorSetUpdate newUpdate{};
	newUpdate.binding = bindingId;
	newUpdate.bufferInfos = bufferInfos;
	newUpdate.descriptorType = DescriptorType::BUFFER;
	m_updates.push_back(newUpdate);
}

void DescriptorSet::UpdateBinding(uint32_t bindingId, const std::vector<VkAccelerationStructureKHR>& _accelStructs)
{
	DescriptorSetUpdate newUpdate{};
	newUpdate.binding = bindingId;
	newUpdate.accelerationStructures = _accelStructs;
	newUpdate.descriptorType = DescriptorType::ACCELERATION_STRUCTURE;
	m_updates.push_back(newUpdate);
}

void DescriptorSet::FinishUpdate()
{
	std::vector<VkWriteDescriptorSet> writes;
	std::vector<std::unique_ptr<VkWriteDescriptorSetAccelerationStructureKHR>> uptrASwrites;
	CHECK_TRUE(m_pLayout != nullptr, "Layout is not initialized!");
	for (auto& eUpdate : m_updates)
	{
		VkWriteDescriptorSet wds{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
		wds.dstSet = vkDescriptorSet;
		wds.dstBinding = eUpdate.binding;
		wds.dstArrayElement = eUpdate.startElement;
		CHECK_TRUE(m_pLayout->bindings.size() > static_cast<size_t>(eUpdate.binding), "The descriptor doesn't have this binding!");
		wds.descriptorType = m_pLayout->bindings[eUpdate.binding].descriptorType;
		// wds.descriptorCount = m_pLayout->bindings[eUpdate.binding].descriptorCount; the number of elements to update doens't need to be the same as the total number of the elements
		switch (eUpdate.descriptorType)
		{
		case DescriptorType::BUFFER:
		{
			wds.descriptorCount = static_cast<uint32_t>(eUpdate.bufferInfos.size());
			wds.pBufferInfo = eUpdate.bufferInfos.data();
			break;
		}
		case DescriptorType::IMAGE:
		{
			wds.descriptorCount = static_cast<uint32_t>(eUpdate.imageInfos.size());
			wds.pImageInfo = eUpdate.imageInfos.data();
			break;
		}
		case DescriptorType::TEXEL_BUFFER:
		{
			wds.descriptorCount = static_cast<uint32_t>(eUpdate.bufferViews.size());
			wds.pTexelBufferView = eUpdate.bufferViews.data();
			break;
		}
		case DescriptorType::ACCELERATION_STRUCTURE:
		{
			std::unique_ptr<VkWriteDescriptorSetAccelerationStructureKHR> uptrASInfo = std::make_unique<VkWriteDescriptorSetAccelerationStructureKHR>();
			uptrASInfo->sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
			uptrASInfo->accelerationStructureCount = static_cast<uint32_t>(eUpdate.accelerationStructures.size());
			uptrASInfo->pAccelerationStructures = eUpdate.accelerationStructures.data();
			uptrASInfo->pNext = nullptr;
			// pNext<VkWriteDescriptorSetAccelerationStructureKHR>.accelerationStructureCount must be equal to descriptorCount
			wds.descriptorCount = static_cast<uint32_t>(eUpdate.accelerationStructures.size());
			wds.pNext = uptrASInfo.get();
			uptrASwrites.push_back(std::move(uptrASInfo));
			break;
		}
		default:
		{
			std::runtime_error("No such descriptor type!");
			break;
		}
		}
		writes.push_back(wds);
	}
	vkUpdateDescriptorSets(MyDevice::GetInstance().vkDevice, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
	m_updates.clear();
}

DescriptorSetLayout::~DescriptorSetLayout()
{
	assert(vkDescriptorSetLayout == VK_NULL_HANDLE);
}

uint32_t DescriptorSetLayout::AddBinding(
	uint32_t descriptorCount,
	VkDescriptorType descriptorType,
	VkShaderStageFlags stageFlags,
	const VkSampler* pImmutableSamplers)
{
	uint32_t binding = static_cast<uint32_t>(bindings.size());

	return AddBinding(binding, descriptorType, stageFlags, pImmutableSamplers);
}

uint32_t DescriptorSetLayout::AddBinding(
	uint32_t binding,
	uint32_t descriptorCount,
	VkDescriptorType descriptorType,
	VkShaderStageFlags stageFlags,
	const VkSampler* pImmutableSamplers)
{
	VkDescriptorSetLayoutBinding layoutBinding{};

	layoutBinding.binding = binding;
	layoutBinding.descriptorCount = descriptorCount;
	layoutBinding.descriptorType = descriptorType;
	layoutBinding.pImmutableSamplers = pImmutableSamplers;
	layoutBinding.stageFlags = stageFlags;

	bindings.push_back(layoutBinding);

	return binding;
}

void DescriptorSetLayout::Init()
{
	CHECK_TRUE(bindings.size() != 0, "No bindings set!");
	CHECK_TRUE(vkDescriptorSetLayout == VK_NULL_HANDLE, "Layout is already initialized!");

	VkDescriptorSetLayoutCreateInfo createInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.bindingCount = static_cast<uint32_t>(bindings.size()),
		.pBindings = bindings.data(),
	};

	CHECK_TRUE(vkDescriptorSetLayout == VK_NULL_HANDLE, "VkDescriptorSetLayout is already created!");
	VK_CHECK(vkCreateDescriptorSetLayout(MyDevice::GetInstance().vkDevice, &createInfo, nullptr, &vkDescriptorSetLayout), "Failed to create descriptor set layout!");
}

DescriptorSet DescriptorSetLayout::NewDescriptorSet() const
{
	DescriptorSet result{};
	result.SetLayout(this);
	return result;
}

DescriptorSet* DescriptorSetLayout::NewDescriptorSetPointer() const
{
	DescriptorSet* pDescriptor = new DescriptorSet();
	pDescriptor->SetLayout(this);
	return pDescriptor;
}

void DescriptorSetLayout::Uninit()
{
	if (vkDescriptorSetLayout != VK_NULL_HANDLE)
	{
		vkDestroyDescriptorSetLayout(MyDevice::GetInstance().vkDevice, vkDescriptorSetLayout, nullptr);
		vkDescriptorSetLayout = VK_NULL_HANDLE;
	}
	bindings.clear();
}

VkDescriptorPool DescriptorSetAllocator::_CreatePool()
{
	VkDevice device = MyDevice::GetInstance().vkDevice;

	std::vector<VkDescriptorPoolSize> sizes;
	sizes.reserve(m_poolSizes.sizes.size());
	for (auto sz : m_poolSizes.sizes)
	{
		sizes.push_back({ sz.first, sz.second });
	}
	VkDescriptorPoolCreateInfo pool_info = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
	pool_info.flags = 0;
	pool_info.maxSets = 1000;
	pool_info.poolSizeCount = static_cast<uint32_t>(sizes.size());
	pool_info.pPoolSizes = sizes.data();

	VkDescriptorPool descriptorPool;
	vkCreateDescriptorPool(device, &pool_info, nullptr, &descriptorPool);

	return descriptorPool;
}

VkDescriptorPool DescriptorSetAllocator::_GrabPool()
{
	//there are reusable pools available
	if (m_freePools.size() > 0)
	{
		//grab pool from the back of the vector and remove it from there.
		VkDescriptorPool pool = m_freePools.back();
		m_freePools.pop_back();
		return pool;
	}
	else
	{
		//no pools available, so create a new one
		return _CreatePool();
	}
}

void DescriptorSetAllocator::ResetPools()
{
	VkDevice device = MyDevice::GetInstance().vkDevice;
	//reset all used pools and add them to the free pools
	for (auto p : m_usedPools)
	{
		vkResetDescriptorPool(device, p, 0);
		m_freePools.push_back(p);
	}

	//clear the used pools, since we've put them all in the free pools
	m_usedPools.clear();

	//reset the current pool handle back to null
	m_currentPool = VK_NULL_HANDLE;
}

bool DescriptorSetAllocator::Allocate(VkDescriptorSet* _vkSet, VkDescriptorSetLayout layout)
{
	VkDevice device = MyDevice::GetInstance().vkDevice;
	//initialize the currentPool handle if it's null
	if (m_currentPool == VK_NULL_HANDLE)
	{
		m_currentPool = _GrabPool();
		m_usedPools.push_back(m_currentPool);
	}

	VkDescriptorSetAllocateInfo allocInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
	allocInfo.pNext = nullptr;
	allocInfo.pSetLayouts = &layout;
	allocInfo.descriptorPool = m_currentPool;
	allocInfo.descriptorSetCount = 1;

	//try to allocate the descriptor set
	VkResult allocResult = vkAllocateDescriptorSets(device, &allocInfo, _vkSet);
	bool needReallocate = false;

	switch (allocResult)
	{
	case VK_SUCCESS:
		//all good, return
		return true;
	case VK_ERROR_FRAGMENTED_POOL:
	case VK_ERROR_OUT_OF_POOL_MEMORY:
		//reallocate pool
		needReallocate = true;
		break;
	default:
		//unrecoverable error
		return false;
	}

	if (needReallocate)
	{
		//allocate a new pool and retry
		m_currentPool = _GrabPool();
		m_usedPools.push_back(m_currentPool);

		allocResult = vkAllocateDescriptorSets(device, &allocInfo, _vkSet);

		//if it still fails then we have big issues
		if (allocResult == VK_SUCCESS)
		{
			return true;
		}
	}

	return false;
}

void DescriptorSetAllocator::Init()
{
}

void DescriptorSetAllocator::Uninit()
{
	VkDevice device = MyDevice::GetInstance().vkDevice;
	//delete every pool held
	for (auto p : m_freePools)
	{
		vkDestroyDescriptorPool(device, p, nullptr);
	}
	for (auto p : m_usedPools)
	{
		vkDestroyDescriptorPool(device, p, nullptr);
	}
}