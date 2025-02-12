#include "pipeline_io.h"
#include "device.h"
#include "buffer.h"
#include "image.h"
DescriptorSet::~DescriptorSet()
{
	assert(vkDescriptorSet == VK_NULL_HANDLE);
}
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
	if (vkDescriptorSet == VK_NULL_HANDLE)
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
	newUpdate.writeDescriptorSet.dstSet = vkDescriptorSet;
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
		.pSetLayouts = &m_pLayout->vkDescriptorSetLayout,
	};

	VK_CHECK(vkAllocateDescriptorSets(MyDevice::GetInstance().vkDevice, &allocInfo, &vkDescriptorSet), Failed to allocate descriptor set!);
}
void DescriptorSet::Uninit()
{
	if (vkDescriptorSet != VK_NULL_HANDLE)
	{
		vkFreeDescriptorSets(MyDevice::GetInstance().vkDevice, MyDevice::GetInstance().vkDescriptorPool, 1, &vkDescriptorSet);
		vkDescriptorSet == VK_NULL_HANDLE;
	}
}

DescriptorSetLayout::~DescriptorSetLayout()
{
	assert(vkDescriptorSetLayout == VK_NULL_HANDLE);
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
	if (vkDescriptorSetLayout != VK_NULL_HANDLE)
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

	VK_CHECK(vkCreateDescriptorSetLayout(MyDevice::GetInstance().vkDevice, &createInfo, nullptr, &vkDescriptorSetLayout), Failed to create compute descriptor set m_pLayout!);
}
void DescriptorSetLayout::Uninit()
{
	if (vkDescriptorSetLayout != VK_NULL_HANDLE)
	{
		vkDestroyDescriptorSetLayout(MyDevice::GetInstance().vkDevice, vkDescriptorSetLayout, nullptr);
		vkDescriptorSetLayout = VK_NULL_HANDLE;
	}
}

void VertexInputLayout::AddLocation(const VertexInputEntry& _location)
{
	locations.push_back(_location);
}
VkVertexInputBindingDescription VertexInputLayout::GetVertexInputBindingDescription(uint32_t _binding) const
{
	VkVertexInputBindingDescription ret;
	ret.binding = _binding;
	ret.stride = stride;
	ret.inputRate = inputRate;
	return ret;
}
std::vector<VkVertexInputAttributeDescription> VertexInputLayout::GetVertexInputAttributeDescriptions(uint32_t _binding) const
{
	std::vector<VkVertexInputAttributeDescription> ret;
	for (int i = 0; i < locations.size(); ++i)
	{
		VkVertexInputAttributeDescription attr;
		attr.binding = _binding;
		attr.location = static_cast<uint32_t>(i);
		attr.format = locations[i].format;
		attr.offset = locations[i].offset;
		ret.push_back(attr);
	}
	return ret;
}

RenderPass::~RenderPass()
{
	assert(vkRenderPass == VK_NULL_HANDLE);
}
uint32_t RenderPass::AddAttachment(AttachmentInformation _info)
{
	uint32_t ret = static_cast<uint32_t>(attachments.size());
	assert(vkRenderPass == VK_NULL_HANDLE);
	attachments.push_back(_info);
	return ret;
}
uint32_t RenderPass::AddSubpass(SubpassInformation _subpass)
{
	uint32_t ret = static_cast<uint32_t>(subpasses.size());
	assert(vkRenderPass == VK_NULL_HANDLE);
	VkSubpassDependency subpassDependency;
	subpassDependency.srcSubpass = subpasses.empty() ? VK_SUBPASS_EXTERNAL : static_cast<uint32_t>(subpasses.size() - 1);
	subpassDependency.dstSubpass = static_cast<uint32_t>(subpasses.size());
	subpassDependency.srcStageMask =  VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpassDependency.srcAccessMask = 0;
	subpassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	if (_subpass.optDepthStencilAttachment.has_value())
	{
		subpassDependency.srcStageMask = subpassDependency.srcAccessMask | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		subpassDependency.dstStageMask = subpassDependency.dstStageMask | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		subpassDependency.dstAccessMask = subpassDependency.dstAccessMask | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	}

	m_vkSubpassDependencies.push_back(subpassDependency);
	subpasses.push_back(_subpass);

	return ret;
}
void RenderPass::Init()
{
	std::vector<VkAttachmentDescription> vkAttachments;
	for (int i = 0; i < attachments.size(); ++i)
	{
		VkAttachmentDescription vkAttachment;
		vkAttachment.format = attachments[i].format;
		vkAttachment.samples = attachments[i].samples;
		vkAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		vkAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		vkAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		vkAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		vkAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		vkAttachment.finalLayout = attachments[i].finalLayout;
		vkAttachments.push_back(vkAttachment);
	}
	std::vector<VkSubpassDescription> vkSubpasses;
	for (int i = 0; i < subpasses.size(); ++i)
	{
		VkSubpassDescription vkSubpass;
		vkSubpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		vkSubpass.colorAttachmentCount = static_cast<uint32_t>(subpasses[i].colorAttachments.size());
		vkSubpass.pColorAttachments = subpasses[i].colorAttachments.data();
		if (subpasses[i].optDepthStencilAttachment.has_value())
		{
			vkSubpass.pDepthStencilAttachment = &subpasses[i].optDepthStencilAttachment.value();
		}
		if (subpasses[i].resolveAttachments.size() > 0)
		{
			vkSubpass.pResolveAttachments = subpasses[i].resolveAttachments.data();
		}
		vkSubpasses.push_back(vkSubpass);
	}
	VkRenderPassCreateInfo renderPassInfo{ VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
	renderPassInfo.attachmentCount = static_cast<uint32_t>(vkAttachments.size());
	renderPassInfo.pAttachments = vkAttachments.data();
	renderPassInfo.subpassCount = static_cast<uint32_t>(vkSubpasses.size());
	renderPassInfo.pSubpasses = vkSubpasses.data();
	renderPassInfo.dependencyCount = static_cast<uint32_t>(m_vkSubpassDependencies.size());
	renderPassInfo.pDependencies = m_vkSubpassDependencies.data();

	VK_CHECK(vkCreateRenderPass(MyDevice::GetInstance().vkDevice, &renderPassInfo, nullptr, &vkRenderPass), Failed to create render pass!);
}
void RenderPass::Uninit()
{
	m_vkSubpassDependencies.clear();
	attachments.clear();
	subpasses.clear();
	if (vkRenderPass != VK_NULL_HANDLE)
	{
		vkDestroyRenderPass(MyDevice::GetInstance().vkDevice, vkRenderPass, nullptr);
		vkRenderPass = VK_NULL_HANDLE;
	}
}
Framebuffer RenderPass::NewFramebuffer(const std::vector<const ImageView*> _imageViews) const
{
	CHECK_TRUE(vkRenderPass != VK_NULL_HANDLE, Renderpass is not initialized!);
	CHECK_TRUE(_imageViews.size() == attachments.size(), Number of imageviews is not the same as number of attachments);
	for (int i = 0; i < _imageViews.size(); ++i)
	{
		CHECK_TRUE(attachments[i].format == _imageViews[i]->pImage->GetImageInformation().format, Format of the imageview is not the same as the format of attachment);
	}
	Framebuffer framebuffer;
	framebuffer.pRenderPass = this;
	framebuffer.attachments = _imageViews;
	return framebuffer;
}
Framebuffer::~Framebuffer()
{
	assert(vkFramebuffer == VK_NULL_HANDLE);
}
void Framebuffer::Init()
{
	CHECK_TRUE(pRenderPass != nullptr, No renderpass!);
	CHECK_TRUE(attachments.size() > 0, No attachment!);
	VkFramebufferCreateInfo framebufferInfo{ VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
	framebufferInfo.renderPass = pRenderPass->vkRenderPass.value();
	framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	std::vector<VkImageView> vkAttachments;
	for (int i = 0; i < attachments.size(); ++i)
	{
		CHECK_TRUE(attachments[i]->vkImageView != VK_NULL_HANDLE, Imageview is not initialized!);
		vkAttachments.push_back(attachments[i]->vkImageView);
	}
	framebufferInfo.pAttachments = vkAttachments.data();
	framebufferInfo.width = attachments[0]->pImage->GetImageInformation().width;
	framebufferInfo.height = attachments[0]->pImage->GetImageInformation().height;
	framebufferInfo.layers = 1;

	VK_CHECK(vkCreateFramebuffer(MyDevice::GetInstance().vkDevice, &framebufferInfo, nullptr, &vkFramebuffer), Failed to create framebuffer!);
}
void Framebuffer::Uninit()
{
	if (vkFramebuffer != VK_NULL_HANDLE)
	{
		vkDestroyFramebuffer(MyDevice::GetInstance().vkDevice, vkFramebuffer, nullptr);
		vkFramebuffer = VK_NULL_HANDLE;
	}
	attachments.clear();
	pRenderPass = nullptr;
}
