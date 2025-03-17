#pragma once
#include "common.h"
#include "shader.h"
#include "pipeline_io.h"
struct GraphicsPipelineInput
{
	std::vector<const DescriptorSet*> pDescriptorSets;

	VkExtent2D imageSize{};
	const VertexIndexInput* pVertexIndexInput = nullptr;
	std::vector<const VertexInput*> pVertexInputs;
};

class GraphicsPipeline
{
private:
	std::vector<VkPipelineShaderStageCreateInfo> m_shaderStageInfos;
	std::vector<VkVertexInputBindingDescription> m_vertBindingDescriptions;
	std::vector<VkVertexInputAttributeDescription> m_vertAttributeDescriptions;
	std::vector<VkDynamicState> m_dynamicStates;
	VkPipelineViewportStateCreateInfo m_viewportStateInfo{};
	std::vector<VkPipelineColorBlendAttachmentState> m_colorBlendAttachmentStates;
	VkPipelineColorBlendStateCreateInfo m_colorBlendStateInfo{};
	std::vector<VkDescriptorSetLayout> m_descriptorSetLayouts;
	VkPipelineDepthStencilStateCreateInfo m_depthStencilInfo{};
	const RenderPass* m_pRenderPass = nullptr;
	uint32_t m_subpass = 0;
	void _InitCreateInfos();
public:
	VkPipelineLayout vkPipelineLayout = VK_NULL_HANDLE;
	VkPipeline vkPipeline = VK_NULL_HANDLE;
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateInfo{};
	VkPipelineRasterizationStateCreateInfo rasterizerStateInfo{};
	VkPipelineMultisampleStateCreateInfo multisampleStateInfo{};
	
	GraphicsPipeline();
	~GraphicsPipeline();

	void AddShader(const VkPipelineShaderStageCreateInfo& shaderInfo);
	void AddVertexInputLayout(const VertexInputLayout* pVertLayout);
	void AddDescriptorSetLayout(const DescriptorSetLayout* pSetLayout);
	void BindToSubpass(const RenderPass* pRenderPass, uint32_t subpass);
	void SetColorAttachmentAsAdd(int idx);
	void Init();
	void Uninit();

	void Do(VkCommandBuffer commandBuffer, const GraphicsPipelineInput& input);
};

struct ComputePipelineInput
{
	std::vector<const DescriptorSet*> pDescriptorSets;

	uint32_t groupCountX = 1;
	uint32_t groupCountY = 1;
	uint32_t groupCountZ = 1;
};
class ComputePipeline
{
private:
	VkPipelineShaderStageCreateInfo m_shaderStageInfo{};
	std::vector<VkDescriptorSetLayout> m_descriptorSetLayouts;
public:
	VkPipelineLayout vkPipelineLayout = VK_NULL_HANDLE;
	VkPipeline vkPipeline = VK_NULL_HANDLE;

	~ComputePipeline();

	void AddShader(const VkPipelineShaderStageCreateInfo& shaderInfo);
	void AddDescriptorSetLayout(const DescriptorSetLayout* pSetLayout);

	void Init();
	void Uninit();

	void Do(VkCommandBuffer commandBuffer, const ComputePipelineInput& input);
};