#pragma once
#include "common.h"
class Buffer;
class BufferView;
class ImageView;
class CommandSubmission;
class DescriptorSet;
class Framebuffer;

// Pipeline can have multiple DescriptorSetLayout, 
// when execute the pipeline we need to provide DescriptorSets for each of the DescriptorSetLayout
class DescriptorSetLayout
{
public:
	VkDescriptorSetLayout vkDescriptorSetLayout = VK_NULL_HANDLE;
	std::vector<VkDescriptorSetLayoutBinding> bindings;

public:
	~DescriptorSetLayout();

public:
	uint32_t AddBinding(uint32_t descriptorCount, VkDescriptorType descriptorType, VkShaderStageFlags stageFlags, const VkSampler* pImmutableSamplers = nullptr);
	
	void Init();
	
	DescriptorSet NewDescriptorSet() const;

	void Uninit();
};
class DescriptorSet
{
private:
	enum class DescriptorType { BUFFER, IMAGE, TEXEL_BUFFER, ACCELERATION_STRUCTURE };
	struct DescriptorSetUpdate
	{
		uint32_t        binding = 0;
		uint32_t        startElement = 0;
		DescriptorType  descriptorType = DescriptorType::BUFFER;
		std::vector<VkDescriptorBufferInfo> bufferInfos;
		std::vector<VkDescriptorImageInfo>  imageInfos;
		std::vector<VkBufferView>           bufferViews;
		std::vector<VkAccelerationStructureKHR> accelerationStructures;
	};

private:
	std::vector<DescriptorSetUpdate> m_updates;
	const DescriptorSetLayout* m_pLayout = nullptr;

public:
	VkDescriptorSet vkDescriptorSet = VK_NULL_HANDLE;

public:
	// ~DescriptorSet(); No worry, allocator will handle this

public:
	void SetLayout(const DescriptorSetLayout* _layout);
	
	void Init();

	void StartUpdate();
	void UpdateBinding(uint32_t bindingId, const Buffer* pBuffer);
	void UpdateBinding(uint32_t bindingId, const VkDescriptorImageInfo& dImageInfo);
	void UpdateBinding(uint32_t bindingId, const BufferView* bufferView);
	void UpdateBinding(uint32_t bindingId, const std::vector<VkDescriptorBufferInfo>& bufferInfos);
	void UpdateBinding(uint32_t bindingId, const std::vector<VkAccelerationStructureKHR>& _accelStructs);
	void FinishUpdate();
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

// Graphics pipeline can have multiple vertex bindings,
// i.e. layout(binding = 0, location = 0); layout(binding = 1, location = 0)...
// This class helps to describe vertex attribute layout that belongs to the same binding
// in a graphics pipeline, I use this to create a graphics pipeline.
// This class doesn't hold any Vulkan handle,
// and can be destroyed after the construction of the graphics pipeline.
class VertexInputLayout
{
private:
	uint32_t          m_uStride	= 0;
	VkVertexInputRate m_InputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	std::vector<VkVertexInputAttributeDescription> m_Locations;

public:
	// Set up per Vertex Description information
	void SetUpVertex(uint32_t _stride, VkVertexInputRate _inputRate = VK_VERTEX_INPUT_RATE_VERTEX);

	// Add location for this vertex binding, return the index of this location.
	// Then you can use it in layout(binding = ..., location = <return>) in the shader
	uint32_t AddLocation(VkFormat _format, uint32_t _offset);

	// Call after the SetUpVertex() and AddLocation() are called, used in graphics pipeline creation
	VkVertexInputBindingDescription	GetVertexInputBindingDescription(uint32_t _binding = 0) const;

	// Call after the SetUpVertex() and AddLocation() are called, used in graphics pipeline creation
	std::vector<VkVertexInputAttributeDescription>	GetVertexInputAttributeDescriptions(uint32_t _binding = 0) const;
};

// Render pass can be something like a blueprint of frame buffers
class RenderPass
{
public:
	// Pipeline only has one subpass to describe the output attachedViews
	enum class AttachmentPreset 
	{ 
		SWAPCHAIN,
		DEPTH,
		GBUFFER_UV,
		GBUFFER_NORMAL,
		GBUFFER_POSITION,
		GBUFFER_ALBEDO,
		GBUFFER_DEPTH 
	};
	
	struct Attachment
	{
		VkAttachmentDescription attachmentDescription{};
		VkClearValue clearValue{};
	};

	struct Subpass
	{
		std::vector<VkAttachmentReference> colorAttachments;
		std::optional<VkAttachmentReference> optDepthStencilAttachment;
		std::vector<VkAttachmentReference> resolveAttachments;

		void AddColorAttachment(uint32_t _binding);
		
		void SetDepthStencilAttachment(uint32_t _binding, bool _readOnly = false);
		
		void AddResolveAttachment(uint32_t _binding);
	};

private:
	std::vector<VkSubpassDependency> m_vkSubpassDependencies;
	std::vector<VkClearValue> m_clearValues;

private:
	// Return begin info of this render pass
	VkRenderPassBeginInfo _GetVkRenderPassBeginInfo(const Framebuffer* pFramebuffer = nullptr) const;

public:
	VkRenderPass vkRenderPass = VK_NULL_HANDLE;
	std::vector<Attachment> attachments;
	std::vector<Subpass> subpasses;

public:
	static RenderPass::Attachment GetPresetAttachment(AttachmentPreset _preset);

	~RenderPass();

	uint32_t AddAttachment(const Attachment& _info);
	
	uint32_t AddAttachment(AttachmentPreset _preset);
	
	uint32_t AddSubpass(Subpass _subpass);
	
	void Init();

	Framebuffer NewFramebuffer(const std::vector<const ImageView*> _imageViews) const;

	// record vkCmdBeginRenderPass command in command buffer, also bind callback for image layout management
	void StartRenderPass(CommandSubmission* pCmd, const Framebuffer* pFramebuffer = nullptr) const;
	
	void Uninit();
};
class Framebuffer
{
private:
	Framebuffer(const RenderPass* _pRenderPass, const std::vector<const ImageView*> _imageViews)
		: pRenderPass(_pRenderPass), attachedViews(_imageViews) {};

public:
	VkFramebuffer vkFramebuffer = VK_NULL_HANDLE;
	const RenderPass*const pRenderPass = nullptr;
	const std::vector<const ImageView*> attachedViews;

	Framebuffer() {};
	~Framebuffer();

	void Init();

	void Uninit();

	VkExtent2D GetImageSize() const;

	friend class RenderPass;
};