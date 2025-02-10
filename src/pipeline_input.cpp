#include "pipeline_input.h"
#include "device.h"
#include "buffer.h"
void DescriptorSet::SetLayout(const DescriptorSetLayout* _layout)
{
	m_pLayout = _layout;
}
void DescriptorSet::StartDescriptorSetUpdate()
{
	if (m_pLayout == nullptr)
	{
		throw std::runtime_error("Layout is not set!");
	}
	if (!vkDescriptorSet.has_value())
	{
		throw std::runtime_error("Descriptorset is not initialized");
	}
	m_updates.clear();
}
void DescriptorSet::DescriptorSetUpdate_WriteBinding(int bindingId, const Buffer* pBuffer)
{
	DescriptorSetUpdate tmpUpdate;
	DescriptorSetEntry binding = m_pLayout->bindings[bindingId];

	m_updates.push_back(tmpUpdate);

	DescriptorSetUpdate& newUpdate = m_updates.back();

	newUpdate.bufferInfo.buffer = pBuffer->vkBuffer;
	newUpdate.bufferInfo.offset = 0;
	newUpdate.bufferInfo.range = pBuffer->GetBufferInformation().size;

	newUpdate.writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	newUpdate.writeDescriptorSet.dstSet = vkDescriptorSet.value();
	newUpdate.writeDescriptorSet.dstBinding = bindingId;
	newUpdate.writeDescriptorSet.dstArrayElement = 0;
	newUpdate.writeDescriptorSet.descriptorCount = binding.descriptorCount;
	newUpdate.writeDescriptorSet.descriptorType = binding.descriptorType;
	newUpdate.writeDescriptorSet.pBufferInfo = &newUpdate.bufferInfo;

}
void DescriptorSet::FinishDescriptorSetUpdate()
{
	std::vector<VkWriteDescriptorSet> writes;
	for (auto& eUpdate : m_updates)
	{
		writes.push_back(eUpdate.writeDescriptorSet);
	}
	vkUpdateDescriptorSets(MyDevice::GetInstance().vkDevice, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
	m_updates.clear();
}
void DescriptorSet::Init()
{
	if (m_pLayout == nullptr)
	{
		throw std::runtime_error("No descriptor layout set!");
	}

	VkDescriptorSetAllocateInfo allocInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.descriptorPool = MyDevice::GetInstance().vkDescriptorPool,
		.descriptorSetCount = 1,
		.pSetLayouts = &m_pLayout->vkDescriptorSetLayout.value(),
	};

	VkDescriptorSet descriptorSet;

	VK_CHECK(vkAllocateDescriptorSets(MyDevice::GetInstance().vkDevice, &allocInfo, &descriptorSet), Failed to allocate descriptor set!);

	vkDescriptorSet = descriptorSet;
}
void DescriptorSet::Uninit()
{
	if (vkDescriptorSet.has_value())
	{
		vkDescriptorSet.reset();
	}
}

void DescriptorSetLayout::AddBinding(const DescriptorSetEntry& _binding)
{
	bindings.push_back(_binding);
}

void DescriptorSetLayout::Init()
{
	if (bindings.size() == 0)
	{
		throw std::runtime_error("No bindings set!");
	}
	if (vkDescriptorSetLayout.has_value())
	{
		throw std::runtime_error("Layout is already initialized.");
	}

	std::vector<VkDescriptorSetLayoutBinding> layoutBindings;

	for (uint32_t i = 0; i < bindings.size(); ++i)
	{
		VkDescriptorSetLayoutBinding layoutBinding;
		layoutBinding.binding = i;
		layoutBinding.descriptorCount = bindings[i].descriptorCount;
		layoutBinding.descriptorType = bindings[i].descriptorType;
		layoutBinding.pImmutableSamplers = nullptr;
		layoutBinding.stageFlags = bindings[i].stageFlags;
	}

	VkDescriptorSetLayoutCreateInfo createInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.bindingCount = static_cast<uint32_t>(layoutBindings.size()),
		.pBindings = layoutBindings.data(),
	};

	VkDescriptorSetLayout setLayout;

	VK_CHECK(vkCreateDescriptorSetLayout(MyDevice::GetInstance().vkDevice, &createInfo, nullptr, &setLayout), Failed to create compute descriptor set m_pLayout!);

	vkDescriptorSetLayout = setLayout;
}

void DescriptorSetLayout::Uninit()
{
	if (vkDescriptorSetLayout.has_value())
	{
		vkDestroyDescriptorSetLayout(MyDevice::GetInstance().vkDevice, vkDescriptorSetLayout.value(), nullptr);
		vkDescriptorSetLayout.reset();
	}
}
