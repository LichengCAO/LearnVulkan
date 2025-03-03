#include "pipeline_io.h"
#include "device.h"
#include "buffer.h"
#include "image.h"
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
	DescriptorSetUpdate newUpdate{};
	newUpdate.binding = bindingId;
	newUpdate.descriptorInfo.bufferInfo.buffer = pBuffer->vkBuffer;
	newUpdate.descriptorInfo.bufferInfo.offset = 0;
	newUpdate.descriptorInfo.bufferInfo.range = pBuffer->GetBufferInformation().size;
	newUpdate.descriptorType = DescriptorType::BUFFER;
	m_updates.push_back(newUpdate);
}
void DescriptorSet::DescriptorSetUpdate_WriteBinding(int bindingId, const VkDescriptorImageInfo& dImageInfo)
{
	DescriptorSetUpdate newUpdate{};
	newUpdate.binding = bindingId;
	newUpdate.descriptorInfo.imageInfo = dImageInfo;
	newUpdate.descriptorType = DescriptorType::IMAGE;
	m_updates.push_back(newUpdate);
}
void DescriptorSet::DescriptorSetUpdate_WriteBinding(int bindingId, const BufferView* pBufferView)
{
	DescriptorSetUpdate newUpdate{};
	newUpdate.binding = bindingId;
	newUpdate.descriptorInfo.bufferView = pBufferView->vkBufferView;
	newUpdate.descriptorType = DescriptorType::TEXEL_BUFFER;
	m_updates.push_back(newUpdate);
}
void DescriptorSet::FinishDescriptorSetUpdate()
{
	std::vector<VkWriteDescriptorSet> writes;
	for (auto& eUpdate : m_updates)
	{
		VkWriteDescriptorSet wds{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
		wds.dstSet = vkDescriptorSet;
		wds.dstBinding = eUpdate.binding;
		wds.dstArrayElement = 0;
		wds.descriptorCount = m_pLayout->bindings[eUpdate.binding].descriptorCount;
		wds.descriptorType = m_pLayout->bindings[eUpdate.binding].descriptorType;
		switch (eUpdate.descriptorType)
		{
		case DescriptorType::BUFFER:
		{
			wds.pBufferInfo = reinterpret_cast<VkDescriptorBufferInfo*>(&eUpdate.descriptorInfo);
			break;
		}
		case DescriptorType::IMAGE:
		{
			wds.pImageInfo = reinterpret_cast<VkDescriptorImageInfo*>(&eUpdate.descriptorInfo);
			break;
		}
		case DescriptorType::TEXEL_BUFFER:
		{
			wds.pTexelBufferView = reinterpret_cast<VkBufferView*>(&eUpdate.descriptorInfo);
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
void DescriptorSet::Init()
{
	if (m_pLayout == nullptr)
	{
		throw std::runtime_error("No descriptor layout set!");
	}
	CHECK_TRUE(vkDescriptorSet == VK_NULL_HANDLE, "VkDescriptorSet is already created!");
	MyDevice::GetInstance().descriptorAllocator.Allocate(&vkDescriptorSet, m_pLayout->vkDescriptorSetLayout);
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
		layoutBindings.push_back(layoutBinding);
	}

	VkDescriptorSetLayoutCreateInfo createInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.bindingCount = static_cast<uint32_t>(layoutBindings.size()),
		.pBindings = layoutBindings.data(),
	};
	CHECK_TRUE(vkDescriptorSetLayout == VK_NULL_HANDLE, "VkDescriptorSetLayout is already created!");
	VK_CHECK(vkCreateDescriptorSetLayout(MyDevice::GetInstance().vkDevice, &createInfo, nullptr, &vkDescriptorSetLayout), "Failed to create descriptor set layout!");
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
	VkVertexInputBindingDescription ret{};
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
		VkVertexInputAttributeDescription attr{};
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
	VkSubpassDependency subpassDependency{};
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
		vkAttachments.push_back(attachments[i].attachmentDescription);
	}
	std::vector<VkSubpassDescription> vkSubpasses;
	for (int i = 0; i < subpasses.size(); ++i)
	{
		VkSubpassDescription vkSubpass{};
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
	CHECK_TRUE(vkRenderPass == VK_NULL_HANDLE, "VkRenderPass is already created!");
	VK_CHECK(vkCreateRenderPass(MyDevice::GetInstance().vkDevice, &renderPassInfo, nullptr, &vkRenderPass), "Failed to create render pass!");
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
	CHECK_TRUE(vkRenderPass != VK_NULL_HANDLE, "Render pass is not initialized!");
	CHECK_TRUE(_imageViews.size() == attachments.size(), "Number of image views is not the same as number of attachments");
	for (int i = 0; i < _imageViews.size(); ++i)
	{
		CHECK_TRUE(attachments[i].attachmentDescription.format == _imageViews[i]->pImage->GetImageInformation().format, "Format of the imageview is not the same as the format of attachment");
	}
	Framebuffer framebuffer{};
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
	CHECK_TRUE(pRenderPass != nullptr, "No render pass!");
	CHECK_TRUE(attachments.size() > 0, "No attachment!");
	VkFramebufferCreateInfo framebufferInfo{ VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
	framebufferInfo.renderPass = pRenderPass->vkRenderPass;
	framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	std::vector<VkImageView> vkAttachments;
	for (int i = 0; i < attachments.size(); ++i)
	{
		CHECK_TRUE(attachments[i]->vkImageView != VK_NULL_HANDLE, "Image view is not initialized!");
		vkAttachments.push_back(attachments[i]->vkImageView);
	}
	framebufferInfo.pAttachments = vkAttachments.data();
	framebufferInfo.width = attachments[0]->pImage->GetImageInformation().width;
	framebufferInfo.height = attachments[0]->pImage->GetImageInformation().height;
	framebufferInfo.layers = 1;
	CHECK_TRUE(vkFramebuffer == VK_NULL_HANDLE, "VkFramebuffer is already created!");
	VK_CHECK(vkCreateFramebuffer(MyDevice::GetInstance().vkDevice, &framebufferInfo, nullptr, &vkFramebuffer), "Failed to create framebuffer!");
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
VkExtent2D Framebuffer::GetImageSize() const
{
	CHECK_TRUE(attachments.size() > 0, "No image in this framebuffer!");
	VkExtent2D ret{};
	ImageInformation imageInfo = attachments[0]->pImage->GetImageInformation();
	ret.width = imageInfo.width;
	ret.height = imageInfo.height;
	return ret;
}

VkDescriptorPool DescriptorAllocator::_CreatePool()
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
VkDescriptorPool DescriptorAllocator::_GrabPool()
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
void DescriptorAllocator::ResetPools()
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
bool DescriptorAllocator::Allocate(VkDescriptorSet* _vkSet, VkDescriptorSetLayout layout)
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
void DescriptorAllocator::Init()
{
}
void DescriptorAllocator::Uninit()
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

void SubpassInformation::AddColorAttachment(uint32_t _binding)
{
	colorAttachments.push_back({ _binding, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });
}
void SubpassInformation::SetDepthStencilAttachment(uint32_t _binding, bool _readOnly)
{
	// ATTACHMENT here means writable
	// VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL -> depth is readonly, stencil is writable
	// VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL -> depth is writable, stencil is readonly
	if (_readOnly)
	{
		optDepthStencilAttachment = { _binding, VkImageLayout::VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL };
	}
	else
	{
		optDepthStencilAttachment = { _binding, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };
	}
}
void SubpassInformation::AddResolveAttachment(uint32_t _binding)
{
	resolveAttachments.push_back({ _binding, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });
}

AttachmentInformation AttachmentInformation::GetPresetInformation(AttachmentPreset _preset)
{
	AttachmentInformation info{};
	VkAttachmentDescription& vkAttachment = info.attachmentDescription;
	switch(_preset)
	{
	case AttachmentPreset::SWAPCHAIN:
	{
		vkAttachment.format = MyDevice::GetInstance().GetSwapchainFormat();
		vkAttachment.samples = VkSampleCountFlagBits::VK_SAMPLE_COUNT_1_BIT;
		vkAttachment.loadOp = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_CLEAR;
		vkAttachment.storeOp = VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_STORE; // https://www.reddit.com/r/vulkan/comments/d8meej/gridlike_pattern_over_a_basic_clearcolor_vulkan/?rdt=33712
		vkAttachment.stencilLoadOp = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		vkAttachment.stencilStoreOp = VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_DONT_CARE;
		vkAttachment.initialLayout = VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED;
		vkAttachment.finalLayout = VkImageLayout::VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		info.clearValue = VkClearValue{};
		info.clearValue.color = { 0.0f, 0.0f, 0.0f, 1.0f };
		break;
	}
	case AttachmentPreset::DEPTH:
	{
		vkAttachment.format = MyDevice::GetInstance().GetDepthFormat();
		vkAttachment.samples = VkSampleCountFlagBits::VK_SAMPLE_COUNT_1_BIT;
		vkAttachment.loadOp = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_CLEAR;
		vkAttachment.storeOp = VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_DONT_CARE;
		vkAttachment.stencilLoadOp = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		vkAttachment.stencilStoreOp = VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_DONT_CARE;
		vkAttachment.initialLayout = VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED;
		vkAttachment.finalLayout = VkImageLayout::VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		info.clearValue = VkClearValue{};
		info.clearValue.depthStencil = { 1.0f, 0 };
		break;
	}
	case AttachmentPreset::GBUFFER_NORMAL:
	{
		vkAttachment.format = VkFormat::VK_FORMAT_R32G32B32A32_SFLOAT;
		vkAttachment.samples = VkSampleCountFlagBits::VK_SAMPLE_COUNT_1_BIT;
		vkAttachment.loadOp = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_CLEAR;
		vkAttachment.storeOp = VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_STORE; // https://www.reddit.com/r/vulkan/comments/d8meej/gridlike_pattern_over_a_basic_clearcolor_vulkan/?rdt=33712
		vkAttachment.stencilLoadOp = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		vkAttachment.stencilStoreOp = VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_DONT_CARE;
		vkAttachment.initialLayout = VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED;
		vkAttachment.finalLayout = VkImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		info.clearValue = VkClearValue{};
		info.clearValue.color = { 0.0f, 0.0f, 0.0f, 1.0f };
		break;
	}
	case AttachmentPreset::GBUFFER_POSITION:
	{
		vkAttachment.format = VkFormat::VK_FORMAT_R32G32B32A32_SFLOAT;
		vkAttachment.samples = VkSampleCountFlagBits::VK_SAMPLE_COUNT_1_BIT;
		vkAttachment.loadOp = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_CLEAR;
		vkAttachment.storeOp = VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_STORE; // https://www.reddit.com/r/vulkan/comments/d8meej/gridlike_pattern_over_a_basic_clearcolor_vulkan/?rdt=33712
		vkAttachment.stencilLoadOp = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		vkAttachment.stencilStoreOp = VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_DONT_CARE;
		vkAttachment.initialLayout = VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED;
		vkAttachment.finalLayout = VkImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		info.clearValue = VkClearValue{};
		info.clearValue.color = { 0.0f, 0.0f, 0.0f, 1.0f };
		break;
	}
	case AttachmentPreset::GBUFFER_UV:
	{
		vkAttachment.format = VkFormat::VK_FORMAT_R32G32B32A32_SFLOAT;
		vkAttachment.samples = VkSampleCountFlagBits::VK_SAMPLE_COUNT_1_BIT;
		vkAttachment.loadOp = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_CLEAR;
		vkAttachment.storeOp = VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_STORE; // https://www.reddit.com/r/vulkan/comments/d8meej/gridlike_pattern_over_a_basic_clearcolor_vulkan/?rdt=33712
		vkAttachment.stencilLoadOp = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		vkAttachment.stencilStoreOp = VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_DONT_CARE;
		vkAttachment.initialLayout = VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED;
		vkAttachment.finalLayout = VkImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		info.clearValue = VkClearValue{};
		info.clearValue.color = { 0.0f, 0.0f, 0.0f, 1.0f };
		break;
	}
	case AttachmentPreset::GBUFFER_ALBEDO:
	{
		vkAttachment.format = VkFormat::VK_FORMAT_R32G32B32A32_SFLOAT;
		vkAttachment.samples = VkSampleCountFlagBits::VK_SAMPLE_COUNT_1_BIT;
		vkAttachment.loadOp = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_CLEAR;
		vkAttachment.storeOp = VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_STORE;
		vkAttachment.stencilLoadOp = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		vkAttachment.stencilStoreOp = VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_DONT_CARE;
		vkAttachment.initialLayout = VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED;
		vkAttachment.finalLayout = VkImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		info.clearValue = VkClearValue{};
		info.clearValue.color = { 0.0f, 0.0f, 0.0f, 1.0f };
		break;
	}
	default:
	{
		CHECK_TRUE(false, "No such attachment preset!");
		break;
	}
	}
	return info;
}
