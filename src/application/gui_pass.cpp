#include "gui_pass.h"
#include "pipeline_program.h"
#include "render_pass.h"
#include "image.h"
#include "my_gui.h"
#include "device.h"

GUIPass::GUIPass()
{
}

GUIPass::~GUIPass()
{
}

void GUIPass::Init(uint32_t _frameInFlight)
{
	m_uMaxFrameCount = _frameInFlight;
	_InitImageAndViews();
	_InitRenderPass();
	_InitFramebuffers();
	_InitPipeline();
	_InitCommandBuffer();
	_InitMyGUI();
}

void GUIPass::Uninit()
{
	_UninitMyGUI();
	_UninitCommandBuffer();
	_UninitPipeline();
	_UninitFramebuffers();
	_UninitRenderPass();
	_UninitImageAndViews();
}

void GUIPass::OnSwapchainRecreated()
{
	m_uptrProgram->GetDescriptorSetManager().ClearDescriptorSets();
	_UninitFramebuffers();
	_UninitImageAndViews();
	_InitImageAndViews();
	_InitFramebuffers();
}

MyGUI& GUIPass::StartPass(const VkDescriptorImageInfo& _originalImage, const std::vector<VkSemaphore>& _waitSignals)
{
	CommandSubmission* cmd = m_uptrCmds[m_uCurrentFrame].get();
	std::vector<CommandSubmission::WaitInformation> waitSignals;
	for (const auto& semaphore : _waitSignals)
	{
		CommandSubmission::WaitInformation waitSignal{};
		waitSignal.waitSamaphore = semaphore;
		waitSignal.waitPipelineStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

		waitSignals.push_back(waitSignal);
	}
	cmd->WaitTillAvailable();
	cmd->StartCommands(waitSignals);
	m_uptrProgram->BindFramebuffer(cmd, m_uptrFramebuffers[m_uCurrentFrame].get());
	auto& binder = m_uptrProgram->GetDescriptorSetManager();
	binder.StartBind();
	binder.BindDescriptor(0, 0, { _originalImage }, DescriptorSetManager::DESCRIPTOR_BIND_SETTING::DEDICATE_DESCRIPTOR_SET_PER_FRAME);
	binder.EndBind();
	m_uptrProgram->Draw(cmd, 6);
	return *m_uptrGUI.get();
}

void GUIPass::EndPass(const std::vector<VkSemaphore>& _finishSignals)
{
	CommandSubmission* cmd = m_uptrCmds[m_uCurrentFrame].get();

	m_uptrGUI->Apply(m_uptrCmds[m_uCurrentFrame]->vkCommandBuffer);
	
	m_uptrProgram->UnbindFramebuffer(cmd);
	m_uptrProgram->EndFrame();

	cmd->SubmitCommands(_finishSignals);

	m_uCurrentFrame = (m_uCurrentFrame + 1) % m_uMaxFrameCount;
}

void GUIPass::_InitImageAndViews()
{
	auto imageSize = MyDevice::GetInstance().GetSwapchainExtent();
	Image::Information imageInfo{};

	imageInfo.format = VK_FORMAT_R32G32B32A32_SFLOAT;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.arrayLayers = 1;
	imageInfo.depth = 1;
	imageInfo.height = imageSize.height;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.inSwapchain = false;
	imageInfo.memoryProperty = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	imageInfo.mipLevels = 1;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	imageInfo.width = imageSize.width;

	for (uint32_t i = 0; i < m_uMaxFrameCount; ++i)
	{
		std::unique_ptr<Image> uptrImage = std::make_unique<Image>();
		std::unique_ptr<ImageView> uptrView;
		ImageView* pView = nullptr;

		uptrImage->SetImageInformation(imageInfo);
		uptrImage->Init();
		uptrImage->NewImageView(pView);
		uptrView.reset(pView);
		uptrView->Init();
		m_uptrImages.push_back(std::move(uptrImage));
		m_uptrViews.push_back(std::move(uptrView));
	}
}

void GUIPass::_UninitImageAndViews()
{
	for (std::unique_ptr<ImageView>& uptr : m_uptrViews)
	{
		uptr->Uninit();
	}
	m_uptrViews.clear();

	for (auto& uptr : m_uptrImages)
	{
		uptr->Uninit();
	}
	m_uptrImages.clear();
}

void GUIPass::_InitRenderPass()
{
	RenderPass::Subpass subpass{};

	m_uptrRenderPass = std::make_unique<RenderPass>();
	subpass.AddColorAttachment(0);
	m_uptrRenderPass->AddAttachment(RenderPass::AttachmentPreset::COLOR_OUTPUT);
	m_uptrRenderPass->AddSubpass(subpass);
	m_uptrRenderPass->Init();
}

void GUIPass::_UninitRenderPass()
{
	m_uptrRenderPass->Uninit();
	m_uptrRenderPass.reset();
}

void GUIPass::_InitFramebuffers()
{
	for (uint32_t i = 0; i < m_uMaxFrameCount; ++i)
	{
		std::unique_ptr<Framebuffer> uptrFramebuffer;
		Framebuffer* pFramebuffer = nullptr;

		m_uptrRenderPass->NewFramebuffer(pFramebuffer, { m_uptrViews[i].get() });

		uptrFramebuffer.reset(pFramebuffer);
		uptrFramebuffer->Init();

		m_uptrFramebuffers.push_back(std::move(uptrFramebuffer));
	}
}

void GUIPass::_UninitFramebuffers()
{
	for (auto& uptr : m_uptrFramebuffers)
	{
		uptr->Uninit();
	}
	m_uptrFramebuffers.clear();
}

void GUIPass::_InitPipeline()
{
	m_uptrProgram = std::make_unique<GraphicsProgram>();

	m_uptrProgram->SetUpRenderPass(m_uptrRenderPass.get(), 0);
	m_uptrProgram->Init({ "E:/GitStorage/LearnVulkan/bin/shaders/pass_through.vert.spv", "E:/GitStorage/LearnVulkan/bin/shaders/pass_through.frag.spv" }, m_uMaxFrameCount);
}

void GUIPass::_UninitPipeline()
{
	m_uptrProgram->Uninit();
	m_uptrProgram.reset();
}

void GUIPass::_InitCommandBuffer()
{
	for (uint32_t i = 0; i < m_uMaxFrameCount; ++i)
	{
		std::unique_ptr<CommandSubmission> uptrCmd = std::make_unique<CommandSubmission>();

		uptrCmd->Init();
		
		m_uptrCmds.push_back(std::move(uptrCmd));
	}
}

void GUIPass::_UninitCommandBuffer()
{
	for (auto& uptr : m_uptrCmds)
	{
		uptr->Uninit();
	}
	m_uptrCmds.clear();
}

void GUIPass::_InitMyGUI()
{
	m_uptrGUI = std::make_unique<MyGUI>();
	m_uptrGUI->SetUpRenderPass(m_uptrRenderPass->vkRenderPass);
	m_uptrGUI->Init();
}

void GUIPass::_UninitMyGUI()
{
	m_uptrGUI->Uninit();
	m_uptrGUI.reset();
}
