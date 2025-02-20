#include "pipeline.h"
#include "device.h"
#include "buffer.h"
void GraphicsPipeline::_InitCreateInfos()
{
	m_dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	
	m_viewportStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	m_viewportStateInfo.viewportCount = 1;
	m_viewportStateInfo.scissorCount = 1;

	VkPipelineColorBlendAttachmentState colorBlendAttachmentState
	{
		.blendEnable = VK_TRUE,
		.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
		.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
		.colorBlendOp = VK_BLEND_OP_ADD,
		.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
		.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
		.alphaBlendOp = VK_BLEND_OP_ADD,
		.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
	};
	m_colorBlendAttachmentStates.push_back(colorBlendAttachmentState);

	m_colorBlendStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	m_colorBlendStateInfo.logicOpEnable = VK_FALSE;
	m_colorBlendStateInfo.logicOp = VK_LOGIC_OP_COPY;
	m_colorBlendStateInfo.blendConstants[0] = 0.0f;
	m_colorBlendStateInfo.blendConstants[1] = 0.0f;
	m_colorBlendStateInfo.blendConstants[2] = 0.0f;
	m_colorBlendStateInfo.blendConstants[3] = 0.0f;

	inputAssemblyStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssemblyStateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssemblyStateInfo.primitiveRestartEnable = VK_FALSE;

	rasterizerStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizerStateInfo.depthClampEnable = VK_FALSE;
	rasterizerStateInfo.rasterizerDiscardEnable = VK_FALSE;
	rasterizerStateInfo.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizerStateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizerStateInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
	rasterizerStateInfo.depthBiasEnable = VK_FALSE;//use for shadow mapping
	rasterizerStateInfo.depthBiasConstantFactor = 0.0f;
	rasterizerStateInfo.depthBiasClamp = 0.0f;
	rasterizerStateInfo.depthBiasSlopeFactor = 0.0f;
	rasterizerStateInfo.lineWidth = 1.0f; //use values bigger than 1.0, enable wideLines GPU features, extensions
	rasterizerStateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
	//TODO: if application won't draw, try to change this 
	rasterizerStateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;// otherwise the image won't be drawn 

	multisampleStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampleStateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampleStateInfo.sampleShadingEnable = VK_FALSE;
	multisampleStateInfo.minSampleShading = 1.0f;
	multisampleStateInfo.pSampleMask = nullptr;
	multisampleStateInfo.alphaToCoverageEnable = VK_FALSE;
	multisampleStateInfo.alphaToOneEnable = VK_FALSE;
	multisampleStateInfo.sampleShadingEnable = VK_TRUE; // enable sample shading in the pipeline
	multisampleStateInfo.minSampleShading = .2f; // min fraction for sample shading; closer to one is smoother
}

GraphicsPipeline::GraphicsPipeline()
{
	_InitCreateInfos();
}

GraphicsPipeline::~GraphicsPipeline()
{
	assert(vkPipeline == VK_NULL_HANDLE);
	assert(vkPipelineLayout == VK_NULL_HANDLE);
}

void GraphicsPipeline::AddShader(const SimpleShader* simpleShader)
{
	assert(vkPipeline == VK_NULL_HANDLE);
	auto stageInfo = simpleShader->GetShaderStageInfo();
	m_shaderStageInfos.push_back(stageInfo);
}

void GraphicsPipeline::AddVertexInputLayout(const VertexInputLayout* pVertLayout)
{
	assert(vkPipeline == VK_NULL_HANDLE);
	uint32_t binding = static_cast<uint32_t>(m_vertBindingDescriptions.size());
	m_vertBindingDescriptions.push_back(pVertLayout->GetVertexInputBindingDescription(binding));
	auto attributeDescriptions = pVertLayout->GetVertexInputAttributeDescriptions(binding);
	for (int i = 0; i < attributeDescriptions.size(); ++i)
	{
		m_vertAttributeDescriptions.push_back(attributeDescriptions[i]);
	}
}

void GraphicsPipeline::AddDescriptorSetLayout(const DescriptorSetLayout* pSetLayout)
{
	assert(vkPipeline == VK_NULL_HANDLE);
	m_descriptorSetLayouts.push_back(pSetLayout->vkDescriptorSetLayout);
}

void GraphicsPipeline::BindToSubpass(const RenderPass* pRenderPass, uint32_t subpass)
{
	assert(vkPipeline == VK_NULL_HANDLE);
	m_pRenderPass = pRenderPass;
	m_subpass = subpass;
	if (pRenderPass->subpasses[subpass].colorAttachments.size() > 0)
	{
		uint32_t attachmentId = pRenderPass->subpasses[subpass].colorAttachments[0].attachment;
		multisampleStateInfo.rasterizationSamples = pRenderPass->attachments[attachmentId].attachmentDescription.samples;
	}
}

void GraphicsPipeline::Init()
{
	VkPipelineVertexInputStateCreateInfo vertexInputInfo{ VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
	vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(m_vertBindingDescriptions.size());
	vertexInputInfo.pVertexBindingDescriptions = m_vertBindingDescriptions.data();
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(m_vertAttributeDescriptions.size());
	vertexInputInfo.pVertexAttributeDescriptions = m_vertAttributeDescriptions.data();

	VkPipelineDynamicStateCreateInfo dynamicStateInfo{ VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
	dynamicStateInfo.dynamicStateCount = static_cast<uint32_t>(m_dynamicStates.size());
	dynamicStateInfo.pDynamicStates = m_dynamicStates.data();

	VkPipelineViewportStateCreateInfo viewportStateInfo{ VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
	viewportStateInfo.viewportCount = 1;
	viewportStateInfo.scissorCount = 1;

	m_colorBlendStateInfo.attachmentCount = static_cast<uint32_t>(m_colorBlendAttachmentStates.size());
	m_colorBlendStateInfo.pAttachments = m_colorBlendAttachmentStates.data();

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
	pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(m_descriptorSetLayouts.size());
	pipelineLayoutInfo.pSetLayouts = m_descriptorSetLayouts.data();
	pipelineLayoutInfo.pushConstantRangeCount = 0;
	pipelineLayoutInfo.pPushConstantRanges = nullptr;
	VK_CHECK(vkCreatePipelineLayout(MyDevice::GetInstance().vkDevice, &pipelineLayoutInfo, nullptr, &vkPipelineLayout), "Failed to create pipeline layout!");

	VkGraphicsPipelineCreateInfo pipelineInfo{ VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
	pipelineInfo.stageCount = static_cast<uint32_t>(m_shaderStageInfos.size());
	pipelineInfo.pStages = m_shaderStageInfos.data();
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssemblyStateInfo;
	pipelineInfo.pViewportState = &viewportStateInfo;
	pipelineInfo.pRasterizationState = &rasterizerStateInfo;
	pipelineInfo.pMultisampleState = &multisampleStateInfo;
	pipelineInfo.pColorBlendState = &m_colorBlendStateInfo;
	pipelineInfo.pDynamicState = &dynamicStateInfo;
	pipelineInfo.layout = vkPipelineLayout;
	pipelineInfo.renderPass = m_pRenderPass->vkRenderPass;
	pipelineInfo.subpass = m_subpass;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;//only when in graphics pipleininfo VK_PIPELINE_CREATE_DERIVATIVE_BIT flag is set
	pipelineInfo.basePipelineIndex = -1;
	VK_CHECK(vkCreateGraphicsPipelines(MyDevice::GetInstance().vkDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &vkPipeline), "Failed to create graphics pipeline!");
}

void GraphicsPipeline::Uninit()
{
	if (vkPipeline != VK_NULL_HANDLE)
	{
		vkDestroyPipeline(MyDevice::GetInstance().vkDevice, vkPipeline, nullptr);
		vkPipeline = VK_NULL_HANDLE;
	}
	if (vkPipelineLayout != VK_NULL_HANDLE)
	{
		vkDestroyPipelineLayout(MyDevice::GetInstance().vkDevice, vkPipelineLayout, nullptr);
		vkPipelineLayout = VK_NULL_HANDLE;
	}
	m_pRenderPass = nullptr;
}

void GraphicsPipeline::Do(VkCommandBuffer commandBuffer, const PipelineInput& input)
{
	// TODO: check m_subpass should match number of vkCmdNextSubpass calls after vkCmdBeginRenderPass
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vkPipeline);

	VkViewport viewport;
	viewport.x = 0.f;
	viewport.y = 0.f;
	viewport.width = static_cast<float>(input.imageSize.width);
	viewport.height = static_cast<float>(input.imageSize.height);
	viewport.minDepth = 0.f;
	viewport.maxDepth = 1.f;
	VkRect2D scissor;
	scissor.offset = { 0, 0 };
	scissor.extent = input.imageSize;
	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
	
	if (input.pDescriptorSets.size() > 0)
	{
		std::vector<VkDescriptorSet> descriptorSets;
		for (int i = 0; i < input.pDescriptorSets.size(); ++i)
		{
			descriptorSets.push_back(input.pDescriptorSets[i]->vkDescriptorSet);
		}
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vkPipelineLayout, 0, static_cast<uint32_t>(descriptorSets.size()), descriptorSets.data(), 0, nullptr);
	}
	if (input.pVertexInputs.size() > 0)
	{
		std::vector<VkBuffer> vertBuffers;
		std::vector<VkDeviceSize> offsets;
		for (int i = 0; i < input.pVertexInputs.size(); ++i)
		{
			vertBuffers.push_back(input.pVertexInputs[i]->pBuffer->vkBuffer);
			offsets.push_back(input.pVertexInputs[i]->offset);
		}
		vkCmdBindVertexBuffers(commandBuffer, 0, static_cast<uint32_t>(vertBuffers.size()), vertBuffers.data(), offsets.data());
		if (input.pVertexIndexInput != nullptr)
		{
			VkIndexType indexType = input.pVertexIndexInput->indexType;
			vkCmdBindIndexBuffer(commandBuffer, input.pVertexIndexInput->pBuffer->vkBuffer, 0, indexType);
			uint32_t bufferSize = static_cast<uint32_t>(input.pVertexIndexInput->pBuffer->GetBufferInformation().size);
			uint32_t stride = 0;
			switch (indexType)
			{
			case VK_INDEX_TYPE_UINT32:
				stride = sizeof(uint32_t);
				break;
			case VK_INDEX_TYPE_UINT16:
				stride = sizeof(uint16_t);
				break;
			}
			CHECK_TRUE(stride != 0, "Size of index is unset!");
			uint32_t indexCount = bufferSize / stride;
			vkCmdDrawIndexed(commandBuffer, indexCount, 1, 0, 0, 0);
		}
		else
		{
			CHECK_TRUE(vertBuffers.size() > 0, "No vertex input!");
			uint32_t bufferSize = static_cast<uint32_t>(input.pVertexInputs[0]->pBuffer->GetBufferInformation().size);
			uint32_t stride = static_cast<uint32_t>(input.pVertexInputs[0]->pVertexInputLayout->stride);
			uint32_t vertCount = bufferSize / stride;
			vkCmdDraw(commandBuffer, vertCount, 1, 0, 0);
		}
	}

}

ComputePipeline::~ComputePipeline()
{
	assert(vkPipeline == VK_NULL_HANDLE);
	assert(vkPipelineLayout == VK_NULL_HANDLE);
}

void ComputePipeline::AddShader(const SimpleShader* simpleShader)
{
	assert(vkPipeline == VK_NULL_HANDLE);
	m_shaderStageInfo = simpleShader->GetShaderStageInfo();
}

void ComputePipeline::AddDescriptorSetLayout(const DescriptorSetLayout* pSetLayout)
{
	assert(vkPipeline == VK_NULL_HANDLE);
	m_descriptorSetLayouts.push_back(pSetLayout->vkDescriptorSetLayout);
}

void ComputePipeline::Init()
{
	VkPipelineLayoutCreateInfo pipelineLayoutInfo{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
	pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(m_descriptorSetLayouts.size());
	pipelineLayoutInfo.pSetLayouts = m_descriptorSetLayouts.data();
	VK_CHECK(vkCreatePipelineLayout(MyDevice::GetInstance().vkDevice, &pipelineLayoutInfo, nullptr, &vkPipelineLayout), "Failed to create pipeline layout!");

	VkComputePipelineCreateInfo pipelineInfo{ VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO };
	pipelineInfo.stage = m_shaderStageInfo;
	pipelineInfo.layout = vkPipelineLayout;
	VK_CHECK(vkCreateComputePipelines(MyDevice::GetInstance().vkDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &vkPipeline), "Failed to create compute pipeline!");
}

void ComputePipeline::Uninit()
{
	if (vkPipeline != VK_NULL_HANDLE)
	{
		vkDestroyPipeline(MyDevice::GetInstance().vkDevice, vkPipeline, nullptr);
		vkPipeline = VK_NULL_HANDLE;
	}
	if (vkPipelineLayout != VK_NULL_HANDLE)
	{
		vkDestroyPipelineLayout(MyDevice::GetInstance().vkDevice, vkPipelineLayout, nullptr);
		vkPipelineLayout = VK_NULL_HANDLE;
	}
}

void ComputePipeline::Do(VkCommandBuffer commandBuffer, const PipelineInput& input)
{
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, vkPipeline);
	std::vector<VkDescriptorSet> descriptorSets;
	for (int i = 0; i < input.pDescriptorSets.size(); ++i)
	{
		descriptorSets.push_back(input.pDescriptorSets[i]->vkDescriptorSet);
	}
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, vkPipelineLayout, 0, static_cast<uint32_t>(descriptorSets.size()), descriptorSets.data(), 0, nullptr);

	vkCmdDispatch(commandBuffer, input.groupCountX, input.groupCountY, input.groupCountZ);
}
