#pragma once
#include "common.h"
#include "shader.h"
#include "pipeline_io.h"

class GraphicsPipeline
{
public:
	// For pipelines that don't bind vertex buffers, e.g. pass through
	struct PipelineInput_Draw
	{
		VkExtent2D imageSize{};
		std::vector<const DescriptorSet*> pDescriptorSets;

		uint32_t vertexCount = 0u;
	};

	// For general graphics pipelines that use vertex buffers
	struct PipelineInput_Vertex
	{
		VkExtent2D imageSize{};
		std::vector<const DescriptorSet*> pDescriptorSets;

		const VertexIndexInput* pVertexIndexInput = nullptr;
		std::vector<const VertexInput*> pVertexInputs;
	};

	// For mesh shader pipelines
	struct PipelineInput_Mesh
	{
		VkExtent2D imageSize{};
		std::vector<const DescriptorSet*> pDescriptorSets;

		uint32_t groupCountX = 1;
		uint32_t groupCountY = 1;
		uint32_t groupCountZ = 1;
	};

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

public:
	VkPipelineLayout vkPipelineLayout = VK_NULL_HANDLE;
	VkPipeline vkPipeline = VK_NULL_HANDLE;
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateInfo{};
	VkPipelineRasterizationStateCreateInfo rasterizerStateInfo{};
	VkPipelineMultisampleStateCreateInfo multisampleStateInfo{};

private:
	void _InitCreateInfos();
	void _DoCommon(VkCommandBuffer cmd, const VkExtent2D& imageSize, const std::vector<const DescriptorSet*>& pSets);

public:
	GraphicsPipeline();
	~GraphicsPipeline();

	void AddShader(const VkPipelineShaderStageCreateInfo& shaderInfo);
	void AddVertexInputLayout(const VertexInputLayout* pVertLayout);
	void AddDescriptorSetLayout(const DescriptorSetLayout* pSetLayout);
	void AddDescriptorSetLayout(VkDescriptorSetLayout vkDSetLayout);
	void BindToSubpass(const RenderPass* pRenderPass, uint32_t subpass);
	void SetColorAttachmentAsAdd(int idx);
	void Init();
	void Uninit();

	void Do(VkCommandBuffer commandBuffer, const PipelineInput_Vertex& input);
	void Do(VkCommandBuffer commandBuffer, const PipelineInput_Mesh& input);
	void Do(VkCommandBuffer commandBuffer, const PipelineInput_Draw& input);
};

class ComputePipeline
{
private:
	VkPipelineShaderStageCreateInfo m_shaderStageInfo{};
	std::vector<VkDescriptorSetLayout> m_descriptorSetLayouts;

public:
	struct PipelineInput
	{
		std::vector<const DescriptorSet*> pDescriptorSets;

		uint32_t groupCountX = 1;
		uint32_t groupCountY = 1;
		uint32_t groupCountZ = 1;
	};

public:
	VkPipelineLayout vkPipelineLayout = VK_NULL_HANDLE;
	VkPipeline vkPipeline = VK_NULL_HANDLE;

	~ComputePipeline();

	void AddShader(const VkPipelineShaderStageCreateInfo& shaderInfo);
	void AddDescriptorSetLayout(const DescriptorSetLayout* pSetLayout);

	void Init();
	void Uninit();

	void Do(VkCommandBuffer commandBuffer, const PipelineInput& input);
};

class RayTracingPipeline
{
private:
	class ShaderBindingTable
	{
	private:
		std::unique_ptr<Buffer> m_uptrBuffer;
		uint32_t              m_uRayGenerationIndex = ~0;
		std::vector<uint32_t> m_uMissIndices;
		std::vector<uint32_t> m_uHitIndices;
		std::vector<uint32_t> m_uCallableIndices;

	private:
		ShaderBindingTable(const ShaderBindingTable& _other) = delete;
		ShaderBindingTable& operator=(const ShaderBindingTable& _other) = delete;

	public:
		ShaderBindingTable() {};

		void SetRayGenRecord(uint32_t index);
		void AddMissRecord(uint32_t index);
		void AddHitRecord(uint32_t index);
		void AddCallableRecord(uint32_t index);
		
		void Init(const RayTracingPipeline* pRayTracingPipeline);
		void Uninit();

		VkStridedDeviceAddressRegionKHR vkRayGenRegion{};
		VkStridedDeviceAddressRegionKHR vkMissRegion{};
		VkStridedDeviceAddressRegionKHR vkHitRegion{};
		VkStridedDeviceAddressRegionKHR vkCallRegion{};
	};

public:
	struct PipelineInput
	{
		std::vector<const DescriptorSet*> pDescriptorSets;
		// for simplicity, i bind SBT in RT pipeline
		uint32_t uWidth = 0u;
		uint32_t uHeight = 0u;
		uint32_t uDepth = 1u;
	};

private:
	std::vector<VkPipelineShaderStageCreateInfo> m_shaderStageInfos;
	std::vector<VkRayTracingShaderGroupCreateInfoKHR> m_shaderRecords;
	std::vector<VkDescriptorSetLayout> m_descriptorSetLayouts;
	ShaderBindingTable m_SBT{};
	uint32_t m_maxRayRecursionDepth = 1u;

public:
	VkPipelineLayout vkPipelineLayout = VK_NULL_HANDLE;
	VkPipeline vkPipeline = VK_NULL_HANDLE;

public:
	RayTracingPipeline();
	~RayTracingPipeline();

	uint32_t AddShader(const VkPipelineShaderStageCreateInfo& shaderInfo);
	
	void AddDescriptorSetLayout(VkDescriptorSetLayout vkDSetLayout);

	// https://www.willusher.io/graphics/2019/11/20/the-sbt-three-ways/
	
	// Set Ray Generation shader record in self shader binding table,
	// return the index of this record in all ShaderRecords,
	// i.e. index of element in pGroups of VkRayTracingPipelineCreateInfoKHR
	uint32_t SetRayGenerationShaderRecord(uint32_t rayGen); 
	
	// Add Hit shader record in self shader binding table,
	// return the index of this record in all ShaderRecords,
	// i.e. index of element in pGroups of VkRayTracingPipelineCreateInfoKHR
	uint32_t AddHitShaderRecord(uint32_t closestHit, uint32_t anyHit = VK_SHADER_UNUSED_KHR, uint32_t intersection = VK_SHADER_UNUSED_KHR);
	
	// Add Miss shader record in self shader binding table,
	// return the index of this record in all ShaderRecords,
	// i.e. index of element in pGroups of VkRayTracingPipelineCreateInfoKHR
	uint32_t AddMissShaderRecord(uint32_t miss);

	void SetMaxRecursion(uint32_t maxRecur);

	void Init();

	void Uninit();

	void Do(VkCommandBuffer commandBuffer, const PipelineInput& input);
};