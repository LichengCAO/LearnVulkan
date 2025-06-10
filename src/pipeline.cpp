#include "pipeline.h"
#include "device.h"
#include "buffer.h"
void GraphicsPipeline::_InitCreateInfos()
{
	m_dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	
	m_viewportStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	m_viewportStateInfo.viewportCount = 1;
	m_viewportStateInfo.scissorCount = 1;

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
	rasterizerStateInfo.depthBiasEnable = VK_FALSE;//use for shadow mapping
	rasterizerStateInfo.depthBiasConstantFactor = 0.0f;
	rasterizerStateInfo.depthBiasClamp = 0.0f;
	rasterizerStateInfo.depthBiasSlopeFactor = 0.0f;
	rasterizerStateInfo.lineWidth = 1.0f; //use values bigger than 1.0, enable wideLines GPU features, extensions
	rasterizerStateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizerStateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;// TODO: otherwise the image won't be drawn if we apply the transform matrix

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

void GraphicsPipeline::_DoCommon(VkCommandBuffer cmd, const VkExtent2D& imageSize, const std::vector<const DescriptorSet*>& pSets)
{
	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, vkPipeline);

	VkViewport viewport;
	viewport.x = 0.f;
	viewport.y = 0.f;
	viewport.width = static_cast<float>(imageSize.width);
	viewport.height = static_cast<float>(imageSize.height);
	viewport.minDepth = 0.f;
	viewport.maxDepth = 1.f;
	VkRect2D scissor;
	scissor.offset = { 0, 0 };
	scissor.extent = imageSize;
	vkCmdSetViewport(cmd, 0, 1, &viewport);
	vkCmdSetScissor(cmd, 0, 1, &scissor);

	if (pSets.size() > 0)
	{
		std::vector<VkDescriptorSet> descriptorSets;
		for (int i = 0; i < pSets.size(); ++i)
		{
			descriptorSets.push_back(pSets[i]->vkDescriptorSet);
		}
		vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, vkPipelineLayout, 0, static_cast<uint32_t>(descriptorSets.size()), descriptorSets.data(), 0, nullptr);
	}
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

void GraphicsPipeline::AddShader(const VkPipelineShaderStageCreateInfo& shaderInfo)
{
	assert(vkPipeline == VK_NULL_HANDLE);
	m_shaderStageInfos.push_back(shaderInfo);
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

void GraphicsPipeline::AddDescriptorSetLayout(VkDescriptorSetLayout vkDSetLayout)
{
	assert(vkPipeline == VK_NULL_HANDLE);
	m_descriptorSetLayouts.push_back(vkDSetLayout);
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
	if (pRenderPass->subpasses[subpass].optDepthStencilAttachment.has_value())
	{
		VkPipelineDepthStencilStateCreateInfo depthStencil{ VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
		VkAttachmentReference depthAtt = pRenderPass->subpasses[subpass].optDepthStencilAttachment.value();
		bool readOnlyDepth = (depthAtt.layout == VkImageLayout::VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL);
		depthStencil.depthTestEnable = VK_TRUE;
		depthStencil.depthWriteEnable = readOnlyDepth ? VK_FALSE : VK_TRUE;
		depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
		depthStencil.depthBoundsTestEnable = VK_FALSE;
		depthStencil.minDepthBounds = 0.0f; // Optional
		depthStencil.maxDepthBounds = 1.0f; // Optional
		depthStencil.stencilTestEnable = VK_FALSE;
		depthStencil.front = {}; // Optional
		depthStencil.back = {}; // Optional
		m_depthStencilInfo = depthStencil;
	}
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
	int colorAttachmentCount = m_pRenderPass->subpasses[m_subpass].colorAttachments.size();
	m_colorBlendAttachmentStates.reserve(colorAttachmentCount);
	for (int i = 0; i < colorAttachmentCount; ++i)
	{
		m_colorBlendAttachmentStates.push_back(colorBlendAttachmentState);
	}
}

void GraphicsPipeline::SetColorAttachmentAsAdd(int idx)
{
	m_colorBlendAttachmentStates[idx].dstAlphaBlendFactor = VkBlendFactor::VK_BLEND_FACTOR_ONE;
	m_colorBlendAttachmentStates[idx].srcAlphaBlendFactor = VkBlendFactor::VK_BLEND_FACTOR_ONE;
	m_colorBlendAttachmentStates[idx].dstColorBlendFactor = VkBlendFactor::VK_BLEND_FACTOR_ONE;
	m_colorBlendAttachmentStates[idx].srcColorBlendFactor = VkBlendFactor::VK_BLEND_FACTOR_ONE;
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
	CHECK_TRUE(vkPipelineLayout == VK_NULL_HANDLE, "VkPipelineLayout is already created!");
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
	pipelineInfo.pDepthStencilState = &m_depthStencilInfo;
	CHECK_TRUE(vkPipeline == VK_NULL_HANDLE, "VkPipeline is already created!");
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

void GraphicsPipeline::Do(VkCommandBuffer commandBuffer, const GraphicsVertexPipelineInput& input)
{
	// TODO: check m_subpass should match number of vkCmdNextSubpass calls after vkCmdBeginRenderPass

	_DoCommon(commandBuffer, input.imageSize, input.pDescriptorSets);

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

void GraphicsPipeline::Do(VkCommandBuffer commandBuffer, const GraphicsMeshPipelineInput& input)
{
	_DoCommon(commandBuffer, input.imageSize, input.pDescriptorSets);
	vkCmdDrawMeshTasksEXT(commandBuffer, input.groupCountX, input.groupCountY, input.groupCountZ);
}

ComputePipeline::~ComputePipeline()
{
	assert(vkPipeline == VK_NULL_HANDLE);
	assert(vkPipelineLayout == VK_NULL_HANDLE);
}

void ComputePipeline::AddShader(const VkPipelineShaderStageCreateInfo& shaderInfo)
{
	assert(vkPipeline == VK_NULL_HANDLE);
	m_shaderStageInfo = shaderInfo;
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
	CHECK_TRUE(vkPipelineLayout == VK_NULL_HANDLE, "VkPipelineLayout is already created!");
	VK_CHECK(vkCreatePipelineLayout(MyDevice::GetInstance().vkDevice, &pipelineLayoutInfo, nullptr, &vkPipelineLayout), "Failed to create pipeline layout!");

	VkComputePipelineCreateInfo pipelineInfo{ VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO };
	pipelineInfo.stage = m_shaderStageInfo;
	pipelineInfo.layout = vkPipelineLayout;
	CHECK_TRUE(vkPipeline == VK_NULL_HANDLE, "VkPipeline is already created!");
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

void ComputePipeline::Do(VkCommandBuffer commandBuffer, const ComputePipelineInput& input)
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

RayTracingPipeline::RayTracingPipeline()
{
}

RayTracingPipeline::~RayTracingPipeline()
{
	assert(vkPipeline == VK_NULL_HANDLE);
	assert(vkPipelineLayout == VK_NULL_HANDLE);
}

uint32_t RayTracingPipeline::AddShader(const VkPipelineShaderStageCreateInfo& shaderInfo)
{
	uint32_t uIndex = ~0;
	assert(vkPipeline == VK_NULL_HANDLE);
	uIndex = static_cast<uint32_t>(m_shaderStageInfos.size());
	m_shaderStageInfos.push_back(shaderInfo);
	return uIndex;
}

void RayTracingPipeline::AddDescriptorSetLayout(const DescriptorSetLayout* pSetLayout)
{
	AddDescriptorSetLayout(pSetLayout->vkDescriptorSetLayout);
}

void RayTracingPipeline::AddDescriptorSetLayout(VkDescriptorSetLayout vkDSetLayout)
{
	m_descriptorSetLayouts.push_back(vkDSetLayout);
}

uint32_t RayTracingPipeline::SetRayGenerationShaderRecord(uint32_t rayGen)
{
	VkRayTracingShaderGroupCreateInfoKHR shaderRecord{ VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR };
	uint32_t uRet = static_cast<uint32_t>(m_shaderRecords.size());
	
	shaderRecord.pNext = nullptr;
	shaderRecord.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
	shaderRecord.generalShader = rayGen;
	shaderRecord.closestHitShader = VK_SHADER_UNUSED_KHR;
	shaderRecord.anyHitShader = VK_SHADER_UNUSED_KHR;
	shaderRecord.intersectionShader = VK_SHADER_UNUSED_KHR;
	shaderRecord.pShaderGroupCaptureReplayHandle = nullptr;

	m_shaderRecords.push_back(shaderRecord);
	m_SBT.SetRayGenRecord(uRet);
	return uRet;
}

uint32_t RayTracingPipeline::AddHitShaderRecord(uint32_t closestHit, uint32_t anyHit, uint32_t intersection)
{
	VkRayTracingShaderGroupCreateInfoKHR shaderRecord{ VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR };
	uint32_t uRet = static_cast<uint32_t>(m_shaderRecords.size());

	shaderRecord.pNext = nullptr;
	shaderRecord.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
	shaderRecord.generalShader = VK_SHADER_UNUSED_KHR;
	shaderRecord.closestHitShader = closestHit;
	shaderRecord.anyHitShader = anyHit;
	shaderRecord.intersectionShader = intersection;
	shaderRecord.pShaderGroupCaptureReplayHandle = nullptr;

	m_shaderRecords.push_back(shaderRecord);
	m_SBT.AddHitRecord(uRet);
	return uRet;
}

uint32_t RayTracingPipeline::AddMissShaderRecord(uint32_t miss)
{
	VkRayTracingShaderGroupCreateInfoKHR shaderRecord{ VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR };
	uint32_t uRet = static_cast<uint32_t>(m_shaderRecords.size());

	shaderRecord.pNext = nullptr;
	shaderRecord.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
	shaderRecord.generalShader = miss;
	shaderRecord.closestHitShader = VK_SHADER_UNUSED_KHR;
	shaderRecord.anyHitShader = VK_SHADER_UNUSED_KHR;
	shaderRecord.intersectionShader = VK_SHADER_UNUSED_KHR;
	shaderRecord.pShaderGroupCaptureReplayHandle = nullptr;

	m_shaderRecords.push_back(shaderRecord);
	m_SBT.AddMissRecord(uRet);
	return uRet;
}

void RayTracingPipeline::SetMaxRecursion(uint32_t maxRecur)
{
	m_maxRayRecursionDepth = maxRecur;
}

void RayTracingPipeline::Init()
{
	VkPipelineLayoutCreateInfo pipelineLayoutInfo{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
	pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(m_descriptorSetLayouts.size());
	pipelineLayoutInfo.pSetLayouts = m_descriptorSetLayouts.data();
	pipelineLayoutInfo.pushConstantRangeCount = 0;
	pipelineLayoutInfo.pPushConstantRanges = nullptr;
	CHECK_TRUE(vkPipelineLayout == VK_NULL_HANDLE, "VkPipelineLayout is already created!");
	VK_CHECK(vkCreatePipelineLayout(MyDevice::GetInstance().vkDevice, &pipelineLayoutInfo, nullptr, &vkPipelineLayout), "Failed to create pipeline layout!");

	VkRayTracingPipelineCreateInfoKHR pipelineInfo{VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR};
	pipelineInfo.pNext = nullptr;
	pipelineInfo.flags = 0;
	pipelineInfo.stageCount = static_cast<uint32_t>(m_shaderStageInfos.size());
	pipelineInfo.pStages = m_shaderStageInfos.data();
	pipelineInfo.groupCount = static_cast<uint32_t>(m_shaderRecords.size());
	pipelineInfo.pGroups = m_shaderRecords.data();
	pipelineInfo.maxPipelineRayRecursionDepth = m_maxRayRecursionDepth;
	pipelineInfo.pLibraryInfo = nullptr;
	pipelineInfo.pLibraryInterface = nullptr;
	pipelineInfo.pDynamicState = nullptr;
	pipelineInfo.layout = vkPipelineLayout;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipelineInfo.basePipelineIndex = -1;

	CHECK_TRUE(vkPipeline == VK_NULL_HANDLE, "VkPipeline is already created!");
	VK_CHECK(vkCreateRayTracingPipelinesKHR(MyDevice::GetInstance().vkDevice, {}, {}, 1, &pipelineInfo, nullptr, &vkPipeline), "Failed to create ray tracing pipeline!");

	m_SBT.Init(this);
}

void RayTracingPipeline::Uninit()
{
	m_SBT.Uninit();
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

void RayTracingPipeline::Do(VkCommandBuffer commandBuffer, const RayTracingPipelineInput& input)
{
	const auto& pSets = input.pDescriptorSets;
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, vkPipeline);

	if (pSets.size() > 0)
	{
		std::vector<VkDescriptorSet> descriptorSets;
		for (int i = 0; i < pSets.size(); ++i)
		{
			descriptorSets.push_back(pSets[i]->vkDescriptorSet);
		}
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, vkPipelineLayout, 0, static_cast<uint32_t>(descriptorSets.size()), descriptorSets.data(), 0, nullptr);
	}

	vkCmdTraceRaysKHR(commandBuffer, &m_SBT.vkRayGenRegion, &m_SBT.vkMissRegion, &m_SBT.vkHitRegion, &m_SBT.vkCallRegion, input.uWidth, input.uHeight, input.uDepth);
}

void RayTracingPipeline::ShaderBindingTable::SetRayGenRecord(uint32_t index)
{
	m_uRayGenerationIndex = index;
}

void RayTracingPipeline::ShaderBindingTable::AddMissRecord(uint32_t index)
{
	m_uMissIndices.push_back(index);
}

void RayTracingPipeline::ShaderBindingTable::AddHitRecord(uint32_t index)
{
	m_uHitIndices.push_back(index);
}

void RayTracingPipeline::ShaderBindingTable::AddCallableRecord(uint32_t index)
{
	m_uCallableIndices.push_back(index);
}

void RayTracingPipeline::ShaderBindingTable::Init(const RayTracingPipeline* pRayTracingPipeline)
{
	VkPhysicalDeviceRayTracingPipelinePropertiesKHR rayTracingProperties{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR };
	VkPhysicalDeviceProperties2 prop2{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};
	uint32_t uRecordCount = static_cast<uint32_t>(pRayTracingPipeline->m_shaderRecords.size()); // This is because the inner (nested) class A is considered part of B and has access to all private, protected, and public members of B
	uint32_t uHandleSizeHost = 0u;
	uint32_t uHandleSizeDevice = 0u;
	size_t uGroupDataSize = 0u;
	std::vector<uint8_t> groupData;
	static auto funcAlignUp = [](uint32_t uToAlign, uint32_t uAlignRef)
	{
		return (uToAlign + uAlignRef - 1) & ~(uAlignRef - 1);
	};
	
	prop2.pNext = &rayTracingProperties;
	vkGetPhysicalDeviceProperties2(MyDevice::GetInstance().vkPhysicalDevice, &prop2);
	
	uHandleSizeHost = rayTracingProperties.shaderGroupHandleSize;
	uHandleSizeDevice = funcAlignUp(uHandleSizeHost, rayTracingProperties.shaderGroupHandleAlignment);

	vkRayGenRegion.size = funcAlignUp(uHandleSizeDevice * 1u, rayTracingProperties.shaderGroupBaseAlignment);
	vkMissRegion.size = funcAlignUp(uHandleSizeDevice * static_cast<uint32_t>(m_uMissIndices.size()), rayTracingProperties.shaderGroupBaseAlignment);
	vkHitRegion.size = funcAlignUp(uHandleSizeDevice * static_cast<uint32_t>(m_uHitIndices.size()), rayTracingProperties.shaderGroupBaseAlignment);
	vkCallRegion.size = funcAlignUp(uHandleSizeDevice * static_cast<uint32_t>(m_uCallableIndices.size()), rayTracingProperties.shaderGroupBaseAlignment);

	vkRayGenRegion.stride = vkRayGenRegion.size; // The size member of pRayGenShaderBindingTable must be equal to its stride member
	vkMissRegion.stride = uHandleSizeDevice;
	vkHitRegion.stride = uHandleSizeDevice;
	vkCallRegion.stride = uHandleSizeDevice;

	uGroupDataSize = static_cast<size_t>(uRecordCount * uHandleSizeHost);
	groupData.resize(uGroupDataSize);
	VK_CHECK(vkGetRayTracingShaderGroupHandlesKHR(MyDevice::GetInstance().vkDevice, pRayTracingPipeline->vkPipeline, 0, uRecordCount, uGroupDataSize, groupData.data())
		, "Failed to get group handle data!");

	// bind shader group(shader record) handle to table on device
	{
		Buffer stagingBuffer{};
		BufferInformation stagBufferInfo{};
		BufferInformation bufferInfo{};
		VkDeviceAddress SBTAddress = 0;
		std::vector<uint8_t> dataToDevice;
		auto funcGetGroupHandle = [&](int i) { return groupData.data() + i * static_cast<int>(uHandleSizeHost); };
		m_uptrBuffer = std::make_unique<Buffer>();

		stagBufferInfo.memoryProperty = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
		stagBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		stagBufferInfo.size = vkRayGenRegion.size + vkMissRegion.size + vkHitRegion.size + vkCallRegion.size;

		bufferInfo.memoryProperty = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR;
		bufferInfo.size = stagBufferInfo.size;

		stagingBuffer.Init(stagBufferInfo);
		m_uptrBuffer->Init(bufferInfo);

		SBTAddress = m_uptrBuffer->GetDeviceAddress();

		dataToDevice.resize(stagBufferInfo.size, 0u);
		// set rayGen
		uint8_t* pEntryPoint = dataToDevice.data();
		vkRayGenRegion.deviceAddress = SBTAddress;
		memcpy(pEntryPoint, funcGetGroupHandle(m_uRayGenerationIndex), static_cast<size_t>(uHandleSizeHost));

		// set Miss
		vkMissRegion.deviceAddress = SBTAddress + vkRayGenRegion.size;
		pEntryPoint = dataToDevice.data() + vkRayGenRegion.size;
		for (const auto index : m_uMissIndices)
		{
			memcpy(pEntryPoint, funcGetGroupHandle(index), static_cast<size_t>(uHandleSizeHost));
			pEntryPoint += static_cast<int>(uHandleSizeDevice);
		}

		// set Hit
		vkHitRegion.deviceAddress = SBTAddress + vkRayGenRegion.size + vkMissRegion.size;
		pEntryPoint = dataToDevice.data() + vkRayGenRegion.size + vkMissRegion.size;
		for (const auto index : m_uHitIndices)
		{
			memcpy(pEntryPoint, funcGetGroupHandle(index), static_cast<size_t>(uHandleSizeHost));
			pEntryPoint += static_cast<int>(uHandleSizeDevice);
		}

		// set Callable
		vkCallRegion.deviceAddress = SBTAddress + vkRayGenRegion.size + vkMissRegion.size + vkHitRegion.size;
		pEntryPoint = dataToDevice.data() + vkRayGenRegion.size + vkMissRegion.size + vkHitRegion.size;
		for (const auto index : m_uCallableIndices)
		{
			memcpy(pEntryPoint, funcGetGroupHandle(index), static_cast<size_t>(uHandleSizeHost));
			pEntryPoint += static_cast<int>(uHandleSizeDevice);
		}

		stagingBuffer.CopyFromHost(dataToDevice.data());
		m_uptrBuffer->CopyFromBuffer(stagingBuffer);
		stagingBuffer.Uninit();
	}
}

void RayTracingPipeline::ShaderBindingTable::Uninit()
{
	m_uptrBuffer->Uninit();
}
