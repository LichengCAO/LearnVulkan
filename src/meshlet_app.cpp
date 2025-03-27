#include "meshlet_app.h"

#define MAX_FRAME_COUNT 3

void MeshletApp::_MainLoop()
{
	lastTime = glfwGetTime();
	while (!glfwWindowShouldClose(pDevice->pWindow))
	{
		glfwPollEvents();
		_DrawFrame();
		double currentTime = glfwGetTime();
		frameTime = (currentTime - lastTime) * 1000.0;
		lastTime = currentTime;
	}

	vkDeviceWaitIdle(pDevice->vkDevice);
}

void MeshletApp::_UpdateUniformBuffer()
{
	static std::optional<float> lastX;
	static std::optional<float> lastY;
	UserInput userInput = pDevice->GetUserInput();
	if (!userInput.RMB)
	{
		lastX = userInput.xPos;
		lastY = userInput.yPos;
	}
	float sensitivity = 0.3f;
	float xoffset = lastX.has_value() ? static_cast<float>(userInput.xPos) - lastX.value() : 0.0f;
	float yoffset = lastY.has_value() ? lastY.value() - static_cast<float>(userInput.yPos) : 0.0f;
	lastX = userInput.xPos;
	lastY = userInput.yPos;
	xoffset *= sensitivity;
	yoffset *= sensitivity;
	m_camera.RotateAboutWorldUp(glm::radians(-xoffset));
	m_camera.RotateAboutRight(glm::radians(yoffset));
	if (userInput.RMB)
	{
		glm::vec3 fwd = glm::normalize(glm::cross(m_camera.world_up, m_camera.right));
		float speed = 0.005 * frameTime;
		glm::vec3 mov = m_camera.eye;
		if (userInput.W) mov += (speed * fwd);
		if (userInput.S) mov += (-speed * fwd);
		if (userInput.Q) mov += (speed * m_camera.world_up);
		if (userInput.E) mov += (-speed * m_camera.world_up);
		if (userInput.A) mov += (-speed * m_camera.right);
		if (userInput.D) mov += (speed * m_camera.right);
		m_camera.MoveTo(mov);
	}
}

void MeshletApp::_DrawFrame()
{
	if (MyDevice::GetInstance().NeedRecreateSwapchain())
	{
		_ResizeWindow();
	}

	auto imageIndex = pDevice->AquireAvailableSwapchainImageIndex(m_swapchainImageAvailabilities[m_currentFrame]);
	if (!imageIndex.has_value()) return;

	auto& cmd = m_commandSubmissions[m_currentFrame];
	cmd.WaitTillAvailable();
	_UpdateUniformBuffer();
	WaitInformation waitInfo{};
	waitInfo.waitSamaphore = m_swapchainImageAvailabilities[m_currentFrame];
	waitInfo.waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	cmd.StartCommands({ waitInfo });


	VkSemaphore renderpassFinish = cmd.SubmitCommands();
	MyDevice::GetInstance().PresentSwapchainImage({ renderpassFinish }, imageIndex.value());
	m_currentFrame = (m_currentFrame + 1) % MAX_FRAME_COUNT;
}

VkImageLayout MeshletApp::_GetImageLayout(ImageView* pImageView) const
{
	auto info = pImageView->GetImageViewInformation();
	auto itr = pDevice->imageLayouts.find(pImageView->pImage->vkImage);
	CHECK_TRUE(itr != pDevice->imageLayouts.end(), "Layout is not recorded!");
	return itr->second.GetLayout(info.baseArrayLayer, info.layerCount, info.baseMipLevel, info.levelCount, info.aspectMask);
}

VkImageLayout MeshletApp::_GetImageLayout(VkImage vkImage, uint32_t baseArrayLayer, uint32_t layerCount, uint32_t baseMipLevel, uint32_t levelCount, VkImageAspectFlags aspect) const
{
	auto itr = pDevice->imageLayouts.find(vkImage);
	CHECK_TRUE(itr != pDevice->imageLayouts.end(), "Layout is not recorded!");
	return itr->second.GetLayout(baseArrayLayer, layerCount, baseMipLevel, levelCount, aspect);
}

void MeshletApp::Run()
{
	_Init();
	_MainLoop();
	_Uninit();
}

void MeshletApp::_Init()
{
	pDevice = &MyDevice::GetInstance();
	pDevice->Init();
	
	_InitRenderPass();
	_InitDescriptorSetLayouts();

	_InitBuffers();
	_InitImagesAndViews();
	_InitFramebuffers();
	_InitSampler();

	_InitDescriptorSets();
	_InitVertexInputs();
	_InitPipelines();
	
	// init semaphores 
	VkSemaphoreCreateInfo semaphoreInfo{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
	m_swapchainImageAvailabilities.resize(MAX_FRAME_COUNT);
	for (int i = 0; i < MAX_FRAME_COUNT; ++i)
	{
		VK_CHECK(vkCreateSemaphore(pDevice->vkDevice, &semaphoreInfo, nullptr, &m_swapchainImageAvailabilities[i]), "Failed to create semaphore!");
	}
	
	// init command buffers
	for (int i = 0; i < MAX_FRAME_COUNT; ++i)
	{
		m_commandSubmissions.push_back(std::make_unique<CommandSubmission>());
		m_commandSubmissions.back()->Init();
	}
}

void MeshletApp::_Uninit()
{
	for (auto& cmd : m_commandSubmissions)
	{
		cmd->Uninit();
	}
	for (auto& semaphore : m_swapchainImageAvailabilities)
	{
		vkDestroySemaphore(pDevice->vkDevice, semaphore, nullptr);
	}
	
	_UninitPipelines();
	_UninitVertexInputs();
	_UninitDescriptorSets();

	_UninitFramebuffers();
	_UninitImagesAndViews();
	_UninitBuffers();
	_UninitSampler();

	_UninitDescriptorSetLayouts();
	_UninitRenderPass();

	pDevice->Uninit();
}

void MeshletApp::_InitRenderPass()
{
	m_renderPass = RenderPass();

	AttachmentInformation swapchainInfo = AttachmentInformation::GetPresetInformation(AttachmentPreset::SWAPCHAIN);
	AttachmentInformation depthInfo = AttachmentInformation::GetPresetInformation(AttachmentPreset::DEPTH);
	m_renderPass.AddAttachment(swapchainInfo);
	m_renderPass.AddAttachment(depthInfo);

	SubpassInformation subpassInfo{};
	subpassInfo.AddColorAttachment(0);
	subpassInfo.SetDepthStencilAttachment(1);
	
	m_renderPass.AddSubpass(subpassInfo);
	m_renderPass.Init();
}

void MeshletApp::_UninitRenderPass()
{
	m_renderPass.Uninit();
}

void MeshletApp::_InitDescriptorSetLayouts()
{
}
