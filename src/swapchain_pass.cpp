#include "swapchain_pass.h"
#include "device.h"
#include "image.h"
#include "pipeline_io.h"
#include "pipeline.h"
#include "shader.h"

void SwapchainPass::_InitViews()
{
	std::vector<Image*> pSwapchainImages;
	_UninitViews();
	MyDevice::GetInstance().GetSwapchainImagePointers(pSwapchainImages);
	m_swapchainViews.reserve(pSwapchainImages.size());
	for (size_t i = 0; i < pSwapchainImages.size(); ++i)
	{
		std::unique_ptr<ImageView> uptrView = std::make_unique<ImageView>(pSwapchainImages[i]->NewImageView(VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1));
		uptrView->Init();
		m_swapchainViews.push_back(std::move(uptrView));
	}
}

void SwapchainPass::_UninitViews()
{
	for (auto& uptrView : m_swapchainViews)
	{
		uptrView->Uninit();
		uptrView.reset();
	}
	m_swapchainViews.clear();
}

void SwapchainPass::_InitDSetLayout()
{
	m_DSetLayout = std::make_unique<DescriptorSetLayout>();

	m_DSetLayout->AddBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);

	m_DSetLayout->Init();
}

void SwapchainPass::_UninitDSetLayout()
{
	if (m_DSetLayout.get() != nullptr)
	{
		m_DSetLayout->Uninit();
	}
	m_DSetLayout.reset();
}

void SwapchainPass::_InitDSets()
{
	int n = static_cast<int>(m_uMaxFrameCount);
	_UninitDSets();
	m_DSets.reserve(n);

	for (int i = 0; i < n; ++i)
	{
		std::unique_ptr<DescriptorSet> uptrDSet = std::make_unique<DescriptorSet>(m_DSetLayout->NewDescriptorSet());

		uptrDSet->Init();
		uptrDSet->StartUpdate();
		uptrDSet->UpdateBinding(0, { m_textureInfos[i] });
		uptrDSet->FinishUpdate();

		m_DSets.push_back(std::move(uptrDSet));
	}
}

void SwapchainPass::_UninitDSets()
{
	for (auto& uptrDSet : m_DSets)
	{
		uptrDSet.reset();
	}
	m_DSets.clear();
}

void SwapchainPass::_InitRenderPass()
{
	RenderPass::Subpass subpass{};

	m_renderPass = std::make_unique<RenderPass>();
	m_renderPass->AddAttachment(RenderPass::AttachmentPreset::SWAPCHAIN);
	subpass.AddColorAttachment(0);
	m_renderPass->AddSubpass(subpass);

	m_renderPass->Init();
}

void SwapchainPass::_UninitRenderPass()
{
	if (m_renderPass.get() != nullptr)
	{
		m_renderPass->Uninit();
	}
	m_renderPass.reset();
}

void SwapchainPass::_InitPipeline()
{
	SimpleShader vertShader{};
	SimpleShader fragShader{};

	vertShader.SetSPVFile("E:/GitStorage/LearnVulkan/bin/shaders/swapchain.vert.spv");
	fragShader.SetSPVFile("E:/GitStorage/LearnVulkan/bin/shaders/swapchain.frag.spv");

	vertShader.Init();
	fragShader.Init();

	m_pipeline = std::make_unique<GraphicsPipeline>();

	m_pipeline->AddDescriptorSetLayout(m_DSetLayout->vkDescriptorSetLayout);
	m_pipeline->AddShader(vertShader.GetShaderStageInfo());
	m_pipeline->AddShader(fragShader.GetShaderStageInfo());
	m_pipeline->BindToSubpass(m_renderPass.get(), 0);

	m_pipeline->Init();
	fragShader.Uninit();
	vertShader.Uninit();
}

void SwapchainPass::_UninitPipeline()
{
	if (m_pipeline.get() != nullptr)
	{
		m_pipeline->Uninit();
	}
	m_pipeline.reset();
}

void SwapchainPass::_InitFramebuffer()
{
	int n = m_swapchainViews.size();
	_UninitFramebuffer();
	m_framebuffers.reserve(n);

	for (int i = 0; i < n; ++i)
	{
		std::unique_ptr<Framebuffer> uptrFramebuffer = std::make_unique<Framebuffer>(m_renderPass->NewFramebuffer({ m_swapchainViews[i].get() }));
		
		uptrFramebuffer->Init();

		m_framebuffers.push_back(std::move(uptrFramebuffer));
	}
}

void SwapchainPass::_UninitFramebuffer()
{
	for (auto& uptrFramebuffer : m_framebuffers)
	{
		if (uptrFramebuffer.get() != nullptr)
		{
			uptrFramebuffer->Uninit();
		}
		uptrFramebuffer.reset();
	}
	m_framebuffers.clear();
}

void SwapchainPass::_InitCommandBuffer()
{
	m_cmd = std::make_unique<CommandSubmission>();
	m_cmd->Init();
}

void SwapchainPass::_UninitCommandBuffer()
{
	m_cmd->Uninit();
}

void SwapchainPass::_InitSemaphores()
{
	VkSemaphoreCreateInfo semaphoreInfo{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };

	_UninitSemaphores();

	m_aquireImages.resize(m_uMaxFrameCount);

	for (int i = 0; i < m_uMaxFrameCount; ++i)
	{
		VK_CHECK(vkCreateSemaphore(MyDevice::GetInstance().vkDevice, &semaphoreInfo, nullptr, &m_aquireImages[i]), "Failed to create semaphore!");
	}
}

void SwapchainPass::_UninitSemaphores()
{
	for (auto& semaphore : m_aquireImages)
	{
		vkDestroySemaphore(MyDevice::GetInstance().vkDevice, semaphore, nullptr);
	}
	m_aquireImages.clear();
}

void SwapchainPass::PreSetPassThroughImages(const std::vector<VkDescriptorImageInfo>& textureInfos)
{
	m_textureInfos = textureInfos;
	m_uMaxFrameCount = static_cast<uint32_t>(textureInfos.size());
	m_uCurrentFrame = 0u;
}

void SwapchainPass::Init()
{
	_InitViews();
	_InitDSetLayout();
	_InitDSets();
	_InitRenderPass();
	_InitPipeline();
	_InitFramebuffer();
	_InitCommandBuffer();
	_InitSemaphores();
}

void SwapchainPass::Uninit()
{
	_UninitSemaphores();
	_UninitCommandBuffer();
	_UninitFramebuffer();
	_UninitPipeline();
	_UninitRenderPass();
	_UninitDSets();
	_UninitDSetLayout();
	_UninitViews();
}

void SwapchainPass::RecreateSwapchain(const std::vector<VkDescriptorImageInfo>& textureInfos)
{
	PreSetPassThroughImages(textureInfos);
	_UninitSemaphores();
	_UninitDSets();
	_UninitFramebuffer();
	_UninitViews();
	_InitViews();
	_InitFramebuffer();
	_InitDSets();
	_InitSemaphores();
}

void SwapchainPass::Do(const std::vector<CommandSubmission::WaitInformation>& renderFinish)
{
	auto& device = MyDevice::GetInstance();
	auto index = device.AquireAvailableSwapchainImageIndex(m_aquireImages[m_uCurrentFrame]);
	VkSemaphore semaphoreRenderFinish = VK_NULL_HANDLE;
	CommandSubmission::WaitInformation waitFor{ m_aquireImages[m_uCurrentFrame], VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	GraphicsPipeline::PipelineInput_Draw input{};
	std::vector<CommandSubmission::WaitInformation> waitInfos{};
	
	if (!index.has_value()) return;

	waitInfos = renderFinish;
	waitInfos.push_back(waitFor);
	m_cmd->StartCommands(waitInfos);
	m_cmd->StartRenderPass(m_renderPass.get(), m_framebuffers[m_uCurrentFrame].get());

	input.imageSize = device.GetSwapchainExtent();
	input.vertexCount = 6;
	input.vkDescriptorSets = { m_DSets[m_uCurrentFrame]->vkDescriptorSet };
	m_pipeline->Do(m_cmd->vkCommandBuffer, input);

	m_cmd->EndRenderPass();
	semaphoreRenderFinish = m_cmd->SubmitCommands();

	device.PresentSwapchainImage({ semaphoreRenderFinish }, index.value());

	m_uCurrentFrame = (m_uCurrentFrame + 1) % m_uMaxFrameCount;
}
