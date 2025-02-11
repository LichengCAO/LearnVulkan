#pragma once
#include "trig_app.h"
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
private:
	// std::vector<DescriptorSet*> m_pDescriptorSets;
public:
	~DescriptorSetLayout();
	std::optional<VkDescriptorSetLayout> vkDescriptorSetLayout;
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
		VkDescriptorBufferInfo bufferInfo;
		VkWriteDescriptorSet   writeDescriptorSet;
	};

private:
	std::vector<DescriptorSetUpdate> m_updates;
	DescriptorSetLayout* m_pLayout = nullptr;

public:
	~DescriptorSet();
	std::optional<VkDescriptorSet> vkDescriptorSet;
	void SetLayout(const DescriptorSetLayout* _layout);
	void StartDescriptorSetUpdate();
	void DescriptorSetUpdate_WriteBinding(int bindingId, const Buffer* pBuffer);
	void FinishDescriptorSetUpdate();
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
private:
	const VertexInputLayout* m_pVertexInputLayout = nullptr;
	const Buffer* m_pBuffer = nullptr;
	const Buffer* m_pIndexBuffer = nullptr;
public:
	void SetLayout(const VertexInputLayout* _layout);
	void SetBuffer(const Buffer* _pBuffer);
	void SetIndexBuffer(const Buffer* _pIndexBuffer);
};

// Pipeline only has one subpass to describe the output attachments
struct AttachmentInformation
{
	VkFormat format = VK_FORMAT_R8G8B8A8_SRGB;
	VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT;
	VkImageLayout finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
};
struct SubpassInformation
{
	VkPipelineBindPoint pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	std::vector<VkAttachmentReference> colorAttachments;
	std::optional<VkAttachmentReference> depthStencilAttachment;
};
class Framebuffer
{
public:
	const RenderPass* pRenderPass = nullptr;
	std::vector<const ImageView*> attachments;
	std::optional<VkFramebuffer> vkFramebuffer;
	~Framebuffer();
	void Init();
	void Uninit();
	friend class Renderpass;
};
class RenderPass
{
private:
	std::vector<VkSubpassDependency> m_vkSubpassDependencies;
public:
	~RenderPass();
	std::optional<VkRenderPass> vkRenderPass;
	std::vector<AttachmentInformation> attachments;
	std::vector<SubpassInformation> subpasses;
	void AddAttachment(AttachmentInformation _info);
	void AddSubpass(SubpassInformation _subpass);
	void Init();
	void Uninit();

	Framebuffer NewFramebuffer(const std::vector<const ImageView*> _imageViews) const;
};

