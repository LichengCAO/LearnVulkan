#include "transparent_app.h"
#include "device.h"
#include "tiny_obj_loader.h"
#define MAX_FRAME_COUNT 3
void TransparentApp::_Init()
{
	MyDevice::GetInstance().Init();

	_InitDescriptorSets();
	_InitRenderPass();
	_InitImageViewsAndFramebuffers();
	_InitVertexInputs();
	_InitPipelines();
	// init semaphores and command buffers
	VkSemaphoreCreateInfo semaphoreInfo{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
	m_swapchainImageAvailabilities.resize(MAX_FRAME_COUNT);
	for (int i = 0; i < MAX_FRAME_COUNT; ++i)
	{
		VK_CHECK(vkCreateSemaphore(MyDevice::GetInstance().vkDevice, &semaphoreInfo, nullptr, &m_swapchainImageAvailabilities[i]), "Failed to create semaphore!");
	}
	for (int i = 0; i < MAX_FRAME_COUNT; ++i)
	{
		m_commandSubmissions.push_back(CommandSubmission{});
		m_commandSubmissions.back().Init();
	}
}

void TransparentApp::_InitDescriptorSets()
{
	// Setup descriptor set layout
	DescriptorSetEntry binding0;
	binding0.descriptorCount = 1;
	binding0.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	binding0.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	m_dSetLayout.AddBinding(binding0);
	m_dSetLayout.Init();

	// init uniform buffers, storage buffers
	BufferInformation bufferInfo;
	bufferInfo.size = sizeof(UBO);
	bufferInfo.usage = VkBufferUsageFlagBits::VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VkBufferUsageFlagBits::VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	bufferInfo.memoryProperty = VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
	for (int i = 0; i < MAX_FRAME_COUNT; ++i)
	{
		m_uniformBuffers.push_back(Buffer{});
		m_uniformBuffers.back().Init(bufferInfo);
	}

	// Setup descriptor sets 
	for (int i = 0; i < MAX_FRAME_COUNT; ++i)
	{
		m_dSets.push_back(DescriptorSet{});
		m_dSets.back().SetLayout(&m_dSetLayout);
		m_dSets.back().Init();
		m_dSets.back().StartDescriptorSetUpdate();
		m_dSets.back().DescriptorSetUpdate_WriteBinding(0, &m_uniformBuffers[i]);
		m_dSets.back().FinishDescriptorSetUpdate();
	}
}

void TransparentApp::_InitRenderPass()
{
	// Setup render pass
	AttachmentInformation attInfo;
	AttachmentInformation depthInfo;
	depthInfo.format = MyDevice::GetInstance().FindSupportFormat(
		{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
		VK_IMAGE_TILING_OPTIMAL,
		VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
	);
	depthInfo.clearValue.depthStencil = { 0.0f, 1 };
	depthInfo.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	m_renderPass.AddAttachment(attInfo);
	m_renderPass.AddAttachment(depthInfo);
	SubpassInformation subpassInfo;
	subpassInfo.AddColorAttachment(0);
	subpassInfo.SetDepthStencilAttachment(1);
	m_renderPass.AddSubpass(subpassInfo);
	m_renderPass.Init();
}

void TransparentApp::_InitImageViewsAndFramebuffers()
{
	// Create images and views for frame buffers
	m_swapchainImages = MyDevice::GetInstance().GetSwapchainImages(); // no need to call Init() here, swapchain images are special
	ImageInformation depthImageInfo;
	depthImageInfo.width = MyDevice::GetInstance().GetSwapchainExtent().width;
	depthImageInfo.height = MyDevice::GetInstance().GetSwapchainExtent().height;
	depthImageInfo.usage = VkImageUsageFlagBits::VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	depthImageInfo.format = MyDevice::GetInstance().FindSupportFormat( 
		{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT }, 
		VK_IMAGE_TILING_OPTIMAL, 
		VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
	);
	depthImageInfo.memoryProperty = VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	ImageViewInformation depthImageViewInfo;
	depthImageViewInfo.aspectMask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_DEPTH_BIT;
	ImageViewInformation swapchainImageViewInfo;
	swapchainImageViewInfo.aspectMask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT;
	for (int i = 0; i < m_swapchainImages.size(); ++i)
	{
		m_depthImages.push_back(Image{});
		Image& depthImage = m_depthImages.back();
		depthImage.SetImageInformation(depthImageInfo);
		depthImage.Init();

		m_depthImageViews.push_back(depthImage.NewImageView(depthImageViewInfo));
		m_depthImageViews.back().Init();

		m_swapchainImageViews.push_back(m_swapchainImages[i].NewImageView(swapchainImageViewInfo));
		m_swapchainImageViews.back().Init();
	}
	// Setup frame buffers
	for (int i = 0; i < m_swapchainImages.size(); ++i)
	{
		std::vector<const ImageView*> imageviews = { &m_swapchainImageViews[i], &m_depthImageViews[i] };
		m_framebuffers.push_back(m_renderPass.NewFramebuffer(imageviews));
		m_framebuffers.back().Init();
	}
}

void TransparentApp::_InitPipelines()
{
	SimpleShader vertShader;
	SimpleShader fragShader;
	vertShader.SetSPVFile("E:/GitStorage/LearnVulkan/bin/shaders/particle.vert.spv");
	fragShader.SetSPVFile("E:/GitStorage/LearnVulkan/bin/shaders/particle.frag.spv");
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
	std::vector<std::string> models = {};
	for (int i = 0; i < models.size(); ++i)
	{
		std::vector<Vertex> vertices;
		std::vector<uint32_t> indices;
		tinyobj::attrib_t attrib;
		std::vector<tinyobj::shape_t> shapes;
		std::vector<tinyobj::material_t> materials;
		std::string warn, err;
		CHECK_TRUE(tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, "models/viking_room.obj"), warn + err);
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
					attrib.texcoords[2 * index.texcoord_index + 1]
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
			stagingBuffer.Init(localBufferInfo);
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

void TransparentApp::_MainLoop()
{
	while (!glfwWindowShouldClose(MyDevice::GetInstance().pWindow))
	{
		glfwPollEvents();
		_DrawFrame();
	}

	vkDeviceWaitIdle(MyDevice::GetInstance().vkDevice);
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

	cmd.StartCommands({ m_swapchainImageAvailabilities[m_currentFrame] });
	cmd.StartRenderPass(&m_renderPass, &m_framebuffers[imageIndex.value()]);
	for (int i = 0; i < m_vertBuffers.size(); ++i)
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
		
		m_gPipeline.Do(cmd.vkCommandBuffer, input);
	}
	// m_gPipeline.Do(cmd.vkCommandBuffer, input); // the draw will be done unordered
	cmd.EndRenderPass();

	// use a memory barrier here to synchronize the order of the 2 render passes
	// start another render pass if we want to do deferred shading or post process shader, loadOp = LOAD_OP_LOAD 
	// from my understanding now, we can safely read the neighboring pixels in the next render pass, since the store operations will happen after a render pass end unlike subpass

	VkSemaphore renderpassFinish = cmd.SubmitCommands();

	MyDevice::GetInstance().PresentSwapchainImage({ renderpassFinish }, imageIndex.value());
	
	m_currentFrame = (m_currentFrame + 1) % MAX_FRAME_COUNT;
}

void TransparentApp::_UninitImageViewsAndFramebuffers()
{
	vkDeviceWaitIdle(MyDevice::GetInstance().vkDevice);
	for (auto& view : m_depthImageViews)
	{
		view.Uninit();
	}
	m_depthImageViews.clear();
	for (auto& image : m_depthImages)
	{
		image.Uninit();
	}
	m_depthImages.clear();
	for (auto& view : m_swapchainImageViews)
	{
		view.Uninit();
	}
	m_swapchainImageViews.clear();
	for (auto& framebuffer : m_framebuffers)
	{
		framebuffer.Uninit();
	}
	m_framebuffers.clear();
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
	_UninitImageViewsAndFramebuffers();
	m_gPipeline.Uninit();
	m_dSetLayout.Uninit();
	m_renderPass.Uninit();
	MyDevice::GetInstance().Uninit();
}

void TransparentApp::Run()
{
	_Init();
	_MainLoop();
	_Uninit();
}
