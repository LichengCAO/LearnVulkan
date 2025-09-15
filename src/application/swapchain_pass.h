#pragma once
#include "common.h"
#include "image.h"
#include "commandbuffer.h"

class GraphicsPipeline;
class RenderPass;
class Framebuffer;
class DescriptorSetLayout;
class DescriptorSet;

class SwapchainPass
{
private:
	uint32_t m_uMaxFrameCount = 1u;
	uint32_t m_uCurrentFrame = 0u;

	std::vector<VkSemaphore> m_aquireImages;

	std::vector<std::unique_ptr<ImageView>> m_swapchainViews;
	std::vector<VkDescriptorImageInfo> m_textureInfos; // render results passed as textures into this pass

	std::unique_ptr<GraphicsPipeline> m_pipeline; // graphics pipeline that run pass through shaders
	std::unique_ptr<RenderPass> m_renderPass;     // render pass that write result color to swapchain
	std::vector<std::unique_ptr<Framebuffer>> m_framebuffers; // frame buffers that hold swapchain

	std::unique_ptr<DescriptorSetLayout> m_DSetLayout; 
	std::vector<std::unique_ptr<DescriptorSet>> m_DSets; // descriptor sets that hold the input render result

	std::unique_ptr<CommandSubmission> m_cmd;

private:
	void _InitViews();
	void _UninitViews();

	void _InitDSetLayout();
	void _UninitDSetLayout();

	void _InitDSets();
	void _UninitDSets();

	void _InitRenderPass();
	void _UninitRenderPass();

	void _InitPipeline();
	void _UninitPipeline();

	void _InitFramebuffer();
	void _UninitFramebuffer();

	void _InitCommandBuffer();
	void _UninitCommandBuffer();

	void _InitSemaphores();
	void _UninitSemaphores();

public:
	// Set up images that will be rendered to swapchain
	void PreSetPassThroughImages(const std::vector<VkDescriptorImageInfo>& textureInfos);

	// Init this pass
	void Init();

	// Destroy attribute this pass holds
	void Uninit();

	// When swapchain images resized, we need to get resized render result textures,
	// call this after swapchain images are recreated,
	// this pass will destroy necessary objects it holds and recreate them
	void RecreateSwapchain(const std::vector<VkDescriptorImageInfo>& textureInfos);
	
	// After the render result is rendered as VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL (based on PreSetPassThroughImages),
	// this pass will get a swapchain image, render the result to it and then present the swapchain
	void Do(const std::vector<CommandSubmission::WaitInformation>& renderFinish);
};