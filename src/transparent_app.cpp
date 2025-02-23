#include "transparent_app.h"
#include "device.h"
#include "tiny_obj_loader.h"
#define MAX_FRAME_COUNT 3
void TransparentApp::_Init()
{
	MyDevice::GetInstance().Init();

	_InitRenderPass();
	_InitImageViewsAndFramebuffers();
	_InitDescriptorSets();
	_InitVertexInputs();
	_InitPipelines();
	// init semaphores 
	VkSemaphoreCreateInfo semaphoreInfo{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
	m_swapchainImageAvailabilities.resize(MAX_FRAME_COUNT);
	for (int i = 0; i < MAX_FRAME_COUNT; ++i)
	{
		VK_CHECK(vkCreateSemaphore(MyDevice::GetInstance().vkDevice, &semaphoreInfo, nullptr, &m_swapchainImageAvailabilities[i]), "Failed to create semaphore!");
	}
	// init command buffers
	for (int i = 0; i < MAX_FRAME_COUNT; ++i)
	{
		m_commandSubmissions.push_back(CommandSubmission{});
		m_commandSubmissions.back().Init();
	}
}
void TransparentApp::_Uninit()
{
	for (auto& cmd : m_commandSubmissions)
	{
		cmd.Uninit();
	}
	for (auto& semaphore : m_swapchainImageAvailabilities)
	{
		vkDestroySemaphore(MyDevice::GetInstance().vkDevice, semaphore, nullptr);
	}
	_UninitPipelines();
	_UninitVertexInputs();
	_UninitDescriptorSets();
	_UninitImageViewsAndFramebuffers();
	_UninitRenderPass();
	
	MyDevice::GetInstance().Uninit();
}

//TODO:
void TransparentApp::_InitDescriptorSets()
{
	m_uniformBuffers.reserve(MAX_FRAME_COUNT);

	// Setup descriptor set layout
	{
		DescriptorSetEntry binding0;
		binding0.descriptorCount = 1;
		binding0.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		binding0.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;// | VK_SHADER_STAGE_FRAGMENT_BIT;
		
		m_gbufferDSetLayout.AddBinding(binding0);
		m_gbufferDSetLayout.Init();
	}
	
	{
		DescriptorSetEntry binding0;
		binding0.descriptorCount = 1;
		binding0.descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		binding0.stageFlags = VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT;

		DescriptorSetEntry binding1;
		binding1.descriptorCount = 1;
		binding1.descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		binding1.stageFlags = VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT;

		DescriptorSetEntry binding2;
		binding2.descriptorCount = 1;
		binding2.descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		binding2.stageFlags = VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT;
		
		m_dSetLayout.AddBinding(binding0);
		m_dSetLayout.AddBinding(binding1);
		m_dSetLayout.AddBinding(binding2);
		m_dSetLayout.Init();
	}

	// init uniform buffers, storage buffers
	BufferInformation bufferInfo;
	bufferInfo.size = sizeof(UBO);
	bufferInfo.usage = VkBufferUsageFlagBits::VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;// | VkBufferUsageFlagBits::VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	bufferInfo.memoryProperty = VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
	for (int i = 0; i < MAX_FRAME_COUNT; ++i)
	{
		m_uniformBuffers.push_back(Buffer{});
		m_uniformBuffers.back().Init(bufferInfo);
	}

	// init texture
	m_texture.SetFilePath("E:/GitStorage/LearnVulkan/res/models/viking_room/viking_room.png");
	m_texture.Init();

	// Setup descriptor sets 
	VkDescriptorImageInfo imageInfo = m_texture.GetVkDescriptorImageInfo();
	for (int i = 0; i < MAX_FRAME_COUNT; ++i)
	{
		m_dSets.push_back(DescriptorSet{});
		m_dSets.back().SetLayout(&m_dSetLayout);
		m_dSets.back().Init();
		m_dSets.back().StartDescriptorSetUpdate();
		m_dSets.back().DescriptorSetUpdate_WriteBinding(0, &m_uniformBuffers[i]);
		m_dSets.back().DescriptorSetUpdate_WriteBinding(1, imageInfo);
		m_dSets.back().FinishDescriptorSetUpdate();
	}
}
void TransparentApp::_UninitDescriptorSets()
{
	m_dSets.clear();
	m_dSetLayout.Uninit();
	for (auto& buffer : m_uniformBuffers)
	{
		buffer.Uninit();
	}
	m_uniformBuffers.clear();
	m_texture.Uninit();
}

void TransparentApp::_InitRenderPass()
{
	// Setup render pass
	AttachmentInformation albedoInfo = AttachmentInformation::GetPresetInformation(AttachmentPreset::GBUFFER_ALBEDO);
	AttachmentInformation posInfo = AttachmentInformation::GetPresetInformation(AttachmentPreset::GBUFFER_POSITION);
	AttachmentInformation normalInfo = AttachmentInformation::GetPresetInformation(AttachmentPreset::GBUFFER_NORMAL);
	AttachmentInformation depthInfo = AttachmentInformation::GetPresetInformation(AttachmentPreset::DEPTH);
	m_gbufferRenderPass.AddAttachment(albedoInfo);
	m_gbufferRenderPass.AddAttachment(posInfo);
	m_gbufferRenderPass.AddAttachment(normalInfo);
	m_gbufferRenderPass.AddAttachment(depthInfo);
	SubpassInformation gSubpassInfo{};
	gSubpassInfo.AddColorAttachment(0);
	gSubpassInfo.AddColorAttachment(1);
	gSubpassInfo.AddColorAttachment(2);
	gSubpassInfo.SetDepthStencilAttachment(3);
	m_gbufferRenderPass.AddSubpass(gSubpassInfo);
	m_gbufferRenderPass.Init();

	AttachmentInformation swapchainInfo = AttachmentInformation::GetPresetInformation(AttachmentPreset::SWAPCHAIN);
	m_renderPass.AddAttachment(swapchainInfo);
	// m_renderPass.AddAttachment(depthInfo);
	SubpassInformation subpassInfo{};
	subpassInfo.AddColorAttachment(0);
	// subpassInfo.SetDepthStencilAttachment(1);
	m_renderPass.AddSubpass(subpassInfo);
	m_renderPass.Init();
}
void TransparentApp::_UninitRenderPass()
{
	m_renderPass.Uninit();
	m_gbufferRenderPass.Uninit();
}

void TransparentApp::_InitImageViewsAndFramebuffers()
{
	// prevent implicit copy
	m_swapchainImages = MyDevice::GetInstance().GetSwapchainImages(); // no need to call Init() here, swapchain images are special
	auto n = m_swapchainImages.size();
	uint32_t width = MyDevice::GetInstance().GetSwapchainExtent().width;
	uint32_t height = MyDevice::GetInstance().GetSwapchainExtent().height;
	m_swapchainImageViews.reserve(n);
	m_depthImages.reserve(n);
	m_depthImageViews.reserve(n);
	m_gbufferPosImages.reserve(n);
	m_gbufferPosImageViews.reserve(n);
	m_gbufferNormalImages.reserve(n);
	m_gbufferNormalImageViews.reserve(n);
	m_gbufferAlbedoImages.reserve(n);
	m_gbufferAlbedoImageViews.reserve(n);
	m_gbufferFramebuffers.reserve(n);
	m_framebuffers.reserve(n);

	// Create depth image and view
	ImageInformation depthImageInfo;
	depthImageInfo.width = width;
	depthImageInfo.height = height;
	depthImageInfo.usage = VkImageUsageFlagBits::VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	depthImageInfo.format = MyDevice::GetInstance().GetDepthFormat();
	depthImageInfo.memoryProperty = VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	ImageViewInformation depthImageViewInfo;
	depthImageViewInfo.aspectMask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_DEPTH_BIT;
	for (int i = 0; i < n; ++i)
	{
		m_depthImages.push_back(Image{});
		Image& depthImage = m_depthImages.back();
		depthImage.SetImageInformation(depthImageInfo);
		depthImage.Init();
		m_depthImageViews.push_back(depthImage.NewImageView(depthImageViewInfo));
		m_depthImageViews.back().Init();
	}

	// Create gbuffer images and views
	ImageInformation gbufferAlbedoImageInfo;
	gbufferAlbedoImageInfo.width = width;
	gbufferAlbedoImageInfo.height = height;
	gbufferAlbedoImageInfo.format = VkFormat::VK_FORMAT_R8G8B8_SRGB;
	gbufferAlbedoImageInfo.memoryProperty = VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	ImageViewInformation gbufferAlbedoImageViewInfo;
	gbufferAlbedoImageViewInfo.aspectMask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT;

	ImageInformation gbufferPosImageInfo;
	gbufferPosImageInfo.width = width;
	gbufferPosImageInfo.height = height;
	gbufferPosImageInfo.format = VkFormat::VK_FORMAT_R32G32B32_SFLOAT;
	gbufferPosImageInfo.memoryProperty = VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	ImageViewInformation gbufferPosImageViewInfo;
	gbufferPosImageViewInfo.aspectMask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT;

	ImageInformation gbufferNormalImageInfo;
	gbufferNormalImageInfo.width = width;
	gbufferNormalImageInfo.height = height;
	gbufferNormalImageInfo.format = VkFormat::VK_FORMAT_R16G16B16_SFLOAT;
	gbufferNormalImageInfo.memoryProperty = VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	ImageViewInformation gbufferNormalImageViewInfo;
	gbufferNormalImageViewInfo.aspectMask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT;

	std::vector<Image>* arrGbufferImages[] = {&m_gbufferAlbedoImages, &m_gbufferPosImages, &m_gbufferNormalImages};
	std::vector<ImageView>* arrGbufferImageViews[] = { &m_gbufferAlbedoImageViews, &m_gbufferPosImageViews, &m_gbufferNormalImageViews };
	ImageInformation* gbufferImageInfos[] = { &gbufferAlbedoImageInfo, &gbufferPosImageInfo, &gbufferNormalImageInfo };
	ImageViewInformation* gbufferViewInfos[] = { &gbufferAlbedoImageViewInfo, &gbufferPosImageViewInfo, &gbufferNormalImageViewInfo };
	for (int i = 0; i < n; ++i)
	{
		for (int j = 0; j < 3; ++j)
		{
			std::vector<Image>& gbufferImages = *arrGbufferImages[j];
			std::vector<ImageView>& gbufferViews = *arrGbufferImageViews[j];
			ImageInformation& imageInfo = *gbufferImageInfos[j];
			ImageViewInformation& viewInfo = *gbufferViewInfos[j];
			gbufferImages.push_back(Image{});
			Image& gbufferImage = gbufferImages.back();
			gbufferImage.SetImageInformation(imageInfo);
			gbufferImage.Init();
			gbufferViews.push_back(gbufferImage.NewImageView(viewInfo));
			gbufferViews.back().Init();
		}
	}

	// create swapchain view
	ImageViewInformation swapchainImageViewInfo;
	swapchainImageViewInfo.aspectMask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT;
	for (int i = 0; i < n; ++i)
	{
		m_swapchainImageViews.push_back(m_swapchainImages[i].NewImageView(swapchainImageViewInfo));
		m_swapchainImageViews.back().Init();
	}

	// Setup frame buffers
	for (int i = 0; i < n; ++i)
	{
		std::vector<const ImageView*> gbufferViews =
		{
			&m_gbufferAlbedoImageViews[i],
			&m_gbufferPosImageViews[i],
			&m_gbufferNormalImageViews[i],
		};
		m_gbufferFramebuffers.push_back(m_gbufferRenderPass.NewFramebuffer(gbufferViews));
		m_gbufferFramebuffers.back().Init();
	}
	for (int i = 0; i < n; ++i)
	{
		std::vector<const ImageView*> imageviews = 
		{ 
			&m_swapchainImageViews[i],
		};
		m_framebuffers.push_back(m_renderPass.NewFramebuffer(imageviews));
		m_framebuffers.back().Init();
	}

}
void TransparentApp::_UninitImageViewsAndFramebuffers()
{
	vkDeviceWaitIdle(MyDevice::GetInstance().vkDevice);

	std::vector<std::vector<Framebuffer>*> pFramebufferVecsToUninit =
	{
		&m_framebuffers,
		&m_gbufferFramebuffers,
	};
	for (auto pFramebufferVec : pFramebufferVecsToUninit)
	{
		std::vector<Framebuffer>& framebuffers = *pFramebufferVec;
		for (auto& framebuffer : framebuffers)
		{
			framebuffer.Uninit();
		}
		framebuffers.clear();
	}

	std::vector<std::vector<ImageView>*> pViewVecsToUninit =
	{
		&m_swapchainImageViews,
		&m_depthImageViews,
		&m_gbufferAlbedoImageViews,
		&m_gbufferPosImageViews,
		&m_gbufferNormalImageViews
	};
	for (auto pViewVec : pViewVecsToUninit)
	{
		std::vector<ImageView>& viewVec = *pViewVec;
		for (auto& view : viewVec)
		{
			view.Uninit();
		}
		viewVec.clear();
	}
	
	std::vector<std::vector<Image>*> pImageVecsToUninit = 
	{ 
		&m_depthImages,
		&m_gbufferAlbedoImages,
		&m_gbufferPosImages,
		&m_gbufferNormalImages 
	};
	for (auto pImageVec : pImageVecsToUninit)
	{
		std::vector<Image>& imageVec = *pImageVec;
		for (auto& image : imageVec)
		{
			image.Uninit();
		}
		imageVec.clear();
	}
}

void TransparentApp::_InitPipelines()
{
	SimpleShader gbufferVertShader;
	SimpleShader gbufferFragShader;
	gbufferVertShader.SetSPVFile("E:/GitStorage/LearnVulkan/bin/shaders/gbuffer.vert.spv");
	gbufferFragShader.SetSPVFile("E:/GitStorage/LearnVulkan/bin/shaders/gbuffer.frag.spv");
	gbufferVertShader.stage = VK_SHADER_STAGE_VERTEX_BIT;
	gbufferFragShader.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	gbufferVertShader.Init();
	gbufferFragShader.Init();

	m_gbufferPipeline.AddDescriptorSetLayout(&m_gbufferDSetLayout);
	m_gbufferPipeline.AddShader(&gbufferVertShader);
	m_gbufferPipeline.AddShader(&gbufferFragShader);
	m_gbufferPipeline.BindToSubpass(&m_gbufferRenderPass, 0);
	m_gbufferPipeline.AddVertexInputLayout(&m_gbufferVertLayout);
	m_gbufferPipeline.Init();

	gbufferVertShader.Uninit();
	gbufferFragShader.Uninit();

	SimpleShader vertShader;
	SimpleShader fragShader;
	vertShader.SetSPVFile("E:/GitStorage/LearnVulkan/bin/shaders/room.vert.spv");
	fragShader.SetSPVFile("E:/GitStorage/LearnVulkan/bin/shaders/room.frag.spv");
	vertShader.stage = VK_SHADER_STAGE_VERTEX_BIT;
	fragShader.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	vertShader.Init();
	fragShader.Init();

	m_gPipeline.AddDescriptorSetLayout(&m_dSetLayout);
	m_gPipeline.AddShader(&vertShader);
	m_gPipeline.AddShader(&fragShader);
	m_gPipeline.BindToSubpass(&m_renderPass, 0);
	m_gPipeline.AddVertexInputLayout(&m_vertLayout);
	m_gPipeline.Init();

	vertShader.Uninit();
	fragShader.Uninit();
}
void TransparentApp::_UninitPipelines()
{
	m_gPipeline.Uninit();
	m_gbufferPipeline.Uninit();
}

//TODO:
void TransparentApp::_InitVertexInputs()
{
	// Setup vertex input layout
	VertexInputEntry location0;
	location0.format = VkFormat::VK_FORMAT_R32G32B32_SFLOAT;
	location0.offset = offsetof(Vertex, pos);
	VertexInputEntry location1;
	location1.format = VkFormat::VK_FORMAT_R32G32_SFLOAT;
	location1.offset = offsetof(Vertex, uv);
	m_vertLayout.AddLocation(location0);
	m_vertLayout.AddLocation(location1);
	m_vertLayout.stride = sizeof(Vertex);
	m_vertLayout.inputRate = VkVertexInputRate::VK_VERTEX_INPUT_RATE_VERTEX;

	// Load models
	std::vector<std::string> models = {"E:/GitStorage/LearnVulkan/res/models/viking_room/viking_room.obj"};
	for (int i = 0; i < models.size(); ++i)
	{
		std::vector<Vertex> vertices;
		std::vector<uint32_t> indices;
		tinyobj::attrib_t attrib;
		std::vector<tinyobj::shape_t> shapes;
		std::vector<tinyobj::material_t> materials;
		std::string warn, err;
		CHECK_TRUE(tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, models[i].c_str()), warn + err);
		std::unordered_map<Vertex, uint32_t> uniqueVertices{};
		for (const auto& shape : shapes)
		{
			for (const auto& index : shape.mesh.indices)
			{
				Vertex vertex{};
				vertex.pos = {
					attrib.vertices[3 * index.vertex_index + 0],
					attrib.vertices[3 * index.vertex_index + 1],
					attrib.vertices[3 * index.vertex_index + 2]
				};
				vertex.uv = {
					attrib.texcoords[2 * index.texcoord_index + 0],
					1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
				};
				if (uniqueVertices.count(vertex) == 0)
				{
					uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
					vertices.push_back(vertex);
				}
				indices.push_back(uniqueVertices[vertex]);
			}

			// Upload to device
			m_vertBuffers.push_back(Buffer{});
			m_indexBuffers.push_back(Buffer{});
			BufferInformation localBufferInfo;
			BufferInformation stagingBufferInfo;
			Buffer stagingBuffer;
			Buffer& vertBuffer = m_vertBuffers.back();
			Buffer& indexBuffer = m_indexBuffers.back();
			stagingBufferInfo.size = sizeof(Vertex) * vertices.size();
			stagingBufferInfo.usage = VkBufferUsageFlagBits::VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
			stagingBufferInfo.memoryProperty = VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
			stagingBuffer.Init(stagingBufferInfo);
			stagingBuffer.CopyFromHost(vertices.data());

			localBufferInfo.size = sizeof(Vertex) * vertices.size();
			localBufferInfo.usage = VkBufferUsageFlagBits::VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VkBufferUsageFlagBits::VK_BUFFER_USAGE_TRANSFER_DST_BIT;
			localBufferInfo.memoryProperty = VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
			vertBuffer.Init(localBufferInfo);
			vertBuffer.CopyFromBuffer(stagingBuffer);

			stagingBuffer.Uninit();
			stagingBufferInfo.size = sizeof(uint32_t) * indices.size();
			stagingBuffer.Init(stagingBufferInfo);
			stagingBuffer.CopyFromHost(indices.data());

			localBufferInfo.size = sizeof(uint32_t) * indices.size();;
			localBufferInfo.usage = VkBufferUsageFlagBits::VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VkBufferUsageFlagBits::VK_BUFFER_USAGE_TRANSFER_DST_BIT;
			localBufferInfo.memoryProperty = VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
			indexBuffer.Init(localBufferInfo);
			indexBuffer.CopyFromBuffer(stagingBuffer);

			stagingBuffer.Uninit();
		}
	}
}
void TransparentApp::_UninitVertexInputs()
{
	for (auto& vertBuffer : m_vertBuffers)
	{
		vertBuffer.Uninit();
	}
	m_vertBuffers.clear();
	for (auto& indexBuffer : m_indexBuffers)
	{
		indexBuffer.Uninit();
	}
	m_indexBuffers.clear();
}

//TODO:
void TransparentApp::_MainLoop()
{
	while (!glfwWindowShouldClose(MyDevice::GetInstance().pWindow))
	{
		glfwPollEvents();
		_DrawFrame();
	}

	vkDeviceWaitIdle(MyDevice::GetInstance().vkDevice);
}
void TransparentApp::_UpdateUniformBuffer()
{
	static std::optional<float> lastX;
	static std::optional<float> lastY;
	UserInput userInput = MyDevice::GetInstance().GetUserInput();
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
		float speed = 0.0002f;
		glm::vec3 mov = m_camera.eye;
		if (userInput.W) mov += (speed * fwd);
		if (userInput.S) mov += (-speed * fwd);
		if (userInput.Q) mov += (speed * m_camera.world_up);
		if (userInput.E) mov += (-speed * m_camera.world_up);
		if (userInput.A) mov += (-speed * m_camera.right);
		if (userInput.D) mov += (speed * m_camera.right);
		m_camera.MoveTo(mov);
	}

	UBO ubo{};
	VkExtent2D swapchainExtent = MyDevice::GetInstance().GetSwapchainExtent();
	ubo.model = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	ubo.proj = m_camera.GetProjectionMatrix();
	ubo.proj[1][1] *= -1;
	ubo.view = m_camera.GetViewMatrix();

	m_uniformBuffers[m_currentFrame].CopyFromHost(&ubo);
}
void TransparentApp::_DrawFrame()
{
	if (MyDevice::GetInstance().NeedRecreateSwapchain())
	{
		_UninitImageViewsAndFramebuffers();
		MyDevice::GetInstance().RecreateSwapchain();
		_InitImageViewsAndFramebuffers();
	}

	auto& cmd = m_commandSubmissions[m_currentFrame];
	auto& uniformBuffer = m_uniformBuffers[m_currentFrame];
	cmd.WaitTillAvailable();
	auto imageIndex = MyDevice::GetInstance().AquireAvailableSwapchainImageIndex(m_swapchainImageAvailabilities[m_currentFrame]);
	if (!imageIndex.has_value()) return;
	_UpdateUniformBuffer();
	WaitInformation waitInfo{};
	waitInfo.waitSamaphore = m_swapchainImageAvailabilities[m_currentFrame];
	waitInfo.waitStage = VkPipelineStageFlagBits::VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	cmd.StartCommands({ waitInfo });
	cmd.StartRenderPass(&m_renderPass, &m_framebuffers[imageIndex.value()]);
	for (int i = 0; i < m_indexBuffers.size(); ++i)
	{
		PipelineInput input;
		VertexIndexInput indexInput;
		VertexInput vertInput;
		indexInput.pBuffer = &m_indexBuffers[i];
		vertInput.pVertexInputLayout = &m_vertLayout;
		vertInput.pBuffer = &m_vertBuffers[i];
		input.pDescriptorSets = { &m_dSets[m_currentFrame] };
		input.imageSize = MyDevice::GetInstance().GetSwapchainExtent();
		input.pVertexIndexInput = &indexInput;
		input.pVertexInputs = { &vertInput };
		
		m_gPipeline.Do(cmd.vkCommandBuffer, input); // the draw will be done unordered
	}
	cmd.EndRenderPass();

	// use a memory barrier here to synchronize the order of the 2 render passes
	// start another render pass if we want to do deferred shading or post process shader, loadOp = LOAD_OP_LOAD 
	// from my understanding now, we can safely read the neighboring pixels in the next render pass, since the store operations will happen after a render pass end unlike subpass

	VkSemaphore renderpassFinish = cmd.SubmitCommands();

	MyDevice::GetInstance().PresentSwapchainImage({ renderpassFinish }, imageIndex.value());
	
	m_currentFrame = (m_currentFrame + 1) % MAX_FRAME_COUNT;
}

void TransparentApp::Run()
{
	_Init();
	_MainLoop();
	_Uninit();
}
