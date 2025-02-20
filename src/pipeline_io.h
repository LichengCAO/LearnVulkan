#pragma once
#include "common.h"
class Buffer;
class ImageView;
// Pipeline can have multiple DescriptorSetLayout, when execute the pipeline we need to provide DescriptorSets for each of the DescriptorSetLayout
struct DescriptorSetEntry // binding
{
	uint32_t descriptorCount = 1;
	VkDescriptorType descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	VkShaderStageFlags stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
};
class DescriptorSetLayout
{
public:
	~DescriptorSetLayout();
	VkDescriptorSetLayout vkDescriptorSetLayout = VK_NULL_HANDLE;
	std::vector<DescriptorSetEntry> bindings;
	void AddBinding(const DescriptorSetEntry& _binding);
	void Init();
	void Uninit();
};
class DescriptorSet
{
private:
	struct DescriptorSetUpdate
	{
		VkDescriptorBufferInfo bufferInfo{};
		VkDescriptorImageInfo  imageInfo{};
		VkWriteDescriptorSet   writeDescriptorSet{};
	};

private:
	std::vector<DescriptorSetUpdate> m_updates;
	const DescriptorSetLayout* m_pLayout = nullptr;

public:
	// ~DescriptorSet(); No worry, allocator will handle this
	VkDescriptorSet vkDescriptorSet = VK_NULL_HANDLE;
	void SetLayout(const DescriptorSetLayout* _layout);
	void StartDescriptorSetUpdate();
	void DescriptorSetUpdate_WriteBinding(int bindingId, const Buffer* pBuffer);
	void DescriptorSetUpdate_WriteBinding(int bindingId, const VkDescriptorImageInfo& dImageInfo);
	void FinishDescriptorSetUpdate();
	void Init();
};
class DescriptorAllocator
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
	VkDescriptorPool _CreatePool();
	VkDescriptorPool _GrabPool();
	VkDescriptorPool m_currentPool = VK_NULL_HANDLE;
	PoolSizes m_poolSizes{};
	std::vector<VkDescriptorPool> m_usedPools;
	std::vector<VkDescriptorPool> m_freePools;

public:
	void ResetPools();

	bool Allocate(VkDescriptorSet* _vkSet, VkDescriptorSetLayout layout);

	void Init();
	void Uninit();
};

// Pipeline can have multiple VertexLayouts, when execute the pipeline we need to provide VertexInputs for each of the VertexLayout
struct VertexInputEntry // location
{
	VkFormat format = VK_FORMAT_R32_SFLOAT;
	uint32_t offset = 0;
};
class VertexInputLayout
{
public:
	uint32_t stride = 0;
	VkVertexInputRate inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	std::vector<VertexInputEntry> locations;
	void AddLocation(const VertexInputEntry& _location);
	VkVertexInputBindingDescription					GetVertexInputBindingDescription(uint32_t _binding = 0) const;
	std::vector<VkVertexInputAttributeDescription>	GetVertexInputAttributeDescriptions(uint32_t _binding = 0) const;
};
class VertexInput
{
public:
	const VertexInputLayout* pVertexInputLayout = nullptr;
	const Buffer* pBuffer = nullptr;
	VkDeviceSize offset = 0;
};
class VertexIndexInput
{
public:
	const Buffer* pBuffer = nullptr;
	VkIndexType indexType = VK_INDEX_TYPE_UINT32;
};

// Pipeline only has one subpass to describe the output attachments
struct AttachmentInformation
{
	VkFormat format = VK_FORMAT_R8G8B8A8_SRGB;
	VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT;
	VkImageLayout finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	VkClearValue clearValue = { .color = {0.0f, 0.0f, 1.0f, 1.0f}/*, .depthStencil = {0.0f, 1}*/};
};
struct SubpassInformation
{
	std::vector<VkAttachmentReference> colorAttachments;
	std::optional<VkAttachmentReference> optDepthStencilAttachment;
	std::vector<VkAttachmentReference> resolveAttachments;

	void AddColorAttachment(uint32_t _binding);
	void SetDepthStencilAttachment(uint32_t _binding);
	void AddResolveAttachment(uint32_t _binding);
};
class RenderPass;
class Framebuffer
{
public:
	const RenderPass* pRenderPass = nullptr;
	VkFramebuffer vkFramebuffer = VK_NULL_HANDLE;
	std::vector<const ImageView*> attachments;
	~Framebuffer();
	
	void Init();
	void Uninit();

	VkExtent2D GetImageSize() const;
};
class RenderPass
{
private:
	std::vector<VkSubpassDependency> m_vkSubpassDependencies;
public:
	~RenderPass();
	VkRenderPass vkRenderPass = VK_NULL_HANDLE;
	std::vector<AttachmentInformation> attachments;
	std::vector<SubpassInformation> subpasses;
	uint32_t AddAttachment(AttachmentInformation _info);
	uint32_t AddSubpass(SubpassInformation _subpass);
	void Init();
	void Uninit();

	Framebuffer NewFramebuffer(const std::vector<const ImageView*> _imageViews) const;
};