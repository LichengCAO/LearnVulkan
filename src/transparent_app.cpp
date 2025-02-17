#include "transparent_app.h"
#include "device.h"
#include "tiny_obj_loader.h"
#define MAX_FRAME_COUNT 3
VkFormat TransparentApp::_FindSupportFormat(const std::vector<VkFormat>& candidates, VkImageTiling tilling, VkFormatFeatureFlags features) const
{
	for (auto format : candidates)
	{
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(MyDevice::GetInstance().vkPhysicalDevice, format, &props);
		if (tilling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
		{
			return format;
		}
		else if (tilling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
		{
			return format;
		}
	}
	throw std::runtime_error("failed to find supported format!");
}
void TransparentApp::_Init()
{
	MyDevice::GetInstance().Init();

	_InitDescriptorSets();
	_InitRenderPass();
	_InitVertexInputs();
	_InitPipelines();

	for (int i = 0; i < MAX_FRAME_COUNT; ++i)
	{
		CommandSubmission newCmd;
		newCmd.Init();
		m_commandSubmissions.push_back(newCmd);
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
		Buffer newUniformBuffer;
		newUniformBuffer.Init(bufferInfo);
		m_uniformBuffers.push_back(newUniformBuffer);
	}

	// Setup descriptor sets 
	for (int i = 0; i < MAX_FRAME_COUNT; ++i)
	{
		DescriptorSet newDSet;
		newDSet.SetLayout(&m_dSetLayout);
		newDSet.Init();
		newDSet.StartDescriptorSetUpdate();
		newDSet.DescriptorSetUpdate_WriteBinding(0, &m_uniformBuffers[i]);
		newDSet.FinishDescriptorSetUpdate();
		m_dSets.push_back(newDSet);
	}
}

void TransparentApp::_InitRenderPass()
{
	// Setup render pass
	AttachmentInformation attInfo;
	AttachmentInformation depthInfo;
	depthInfo.format = _FindSupportFormat(
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
	
	// Create images and views for frame buffers
	m_swapchainImages = MyDevice::GetInstance().GetSwapchainImages(); // no need to call Init() here, swapchain images are special
	ImageInformation depthImageInfo;
	depthImageInfo.width = MyDevice::GetInstance().GetCurrentSwapchainExtent().width;
	depthImageInfo.height = MyDevice::GetInstance().GetCurrentSwapchainExtent().height;
	depthImageInfo.usage = VkImageUsageFlagBits::VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	depthImageInfo.format = depthInfo.format;
	depthImageInfo.memoryProperty = VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	ImageViewInformation depthImageViewInfo;
	depthImageViewInfo.aspectMask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_DEPTH_BIT;
	ImageViewInformation swapchainImageViewInfo;
	swapchainImageViewInfo.aspectMask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT;
	for (int i = 0; i < MAX_FRAME_COUNT; ++i)
	{
		Image depthImage;
		depthImage.SetImageInformation(depthImageInfo);
		depthImage.Init();
		m_depthImages.push_back(depthImage);
		ImageView depthImageView = depthImage.NewImageView(depthImageViewInfo);
		depthImageView.Init();
		ImageView swapchainImageView = m_swapchainImages[i].NewImageView(swapchainImageViewInfo);
		swapchainImageView.Init();
		m_depthImageViews.push_back(depthImageView);
		m_swapchainImageViews.push_back(swapchainImageView);
	}

	// Setup frame buffers
	for (int i = 0; i < MAX_FRAME_COUNT; ++i)
	{
		std::vector<const ImageView*> imageviews = { &m_swapchainImageViews[i], &m_depthImageViews[i] };
		Framebuffer framebuffer = m_renderPass.NewFramebuffer(imageviews);
		framebuffer.Init();
		m_framebuffers.push_back(framebuffer);
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

	// Load model
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
	}

	// Upload to device
	BufferInformation localBufferInfo;
	BufferInformation stagingBufferInfo;
	Buffer stagingBuffer;
	stagingBufferInfo.size = sizeof(Vertex) * vertices.size();
	stagingBufferInfo.usage = VkBufferUsageFlagBits::VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	stagingBufferInfo.memoryProperty = VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
	stagingBuffer.Init(stagingBufferInfo);
	stagingBuffer.CopyFromHost(vertices.data());

	localBufferInfo.size = sizeof(Vertex) * vertices.size();
	localBufferInfo.usage = VkBufferUsageFlagBits::VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VkBufferUsageFlagBits::VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	localBufferInfo.memoryProperty = VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	m_vertBuffer.Init(localBufferInfo);
	m_vertBuffer.CopyFromBuffer(stagingBuffer);

	stagingBuffer.Uninit();
	stagingBufferInfo.size = sizeof(uint32_t) * indices.size();
	stagingBuffer.Init(localBufferInfo);
	stagingBuffer.CopyFromHost(indices.data());

	localBufferInfo.size = sizeof(uint32_t) * indices.size();;
	localBufferInfo.usage = VkBufferUsageFlagBits::VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VkBufferUsageFlagBits::VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	localBufferInfo.memoryProperty = VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	m_indexBuffer.Init(localBufferInfo);
	m_indexBuffer.CopyFromBuffer(stagingBuffer);

	stagingBuffer.Uninit();
	
	// Setup vertex inputs
	m_vertInput.pBuffer = &m_vertBuffer;
	m_vertInput.pVertexInputLayout = &m_vertLayout;
	m_vertIndexInput.pBuffer = &m_indexBuffer;
	m_vertIndexInput.indexType = VkIndexType::VK_INDEX_TYPE_UINT32;
}

void TransparentApp::_MainLoop()
{
	while (!glfwWindowShouldClose(MyDevice::GetInstance().pWindow))
	{
		glfwPollEvents();
	}

	vkDeviceWaitIdle(MyDevice::GetInstance().vkDevice);
}

void TransparentApp::_DrawFrame()
{
	auto& cmd = m_commandSubmissions[m_currentFrame];
	auto& uniformBuffer = m_uniformBuffers[m_currentFrame];
	cmd.WaitTillAvailable();
	cmd.StartCommands({});
	cmd.StartRenderPass(&m_renderPass, &m_framebuffers[m_currentFrame]);
	PipelineInput input;
	input.pDescriptorSets = { &m_dSets[m_currentFrame] };
	input.imageSize = MyDevice::GetInstance().GetCurrentSwapchainExtent();
	input.pVertexIndexInput = &m_vertIndexInput;
	input.pVertexInputs = { &m_vertInput };
	m_gPipeline.Do(cmd.vkCommandBuffer, input);
	//TODO: present swapchain, resize
	cmd.EndRenderPass();
	cmd.SubmitCommands();
}

void TransparentApp::_Uninit()
{
	for (auto& cmd : m_commandSubmissions)
	{
		cmd.Uninit();
	}
	for (auto& framebuffer : m_framebuffers)
	{
		framebuffer.Uninit();
	}
	m_gPipeline.Uninit();
	m_dSetLayout.Uninit();
	m_renderPass.Uninit();
	for (auto& view : m_depthImageViews)
	{
		view.Uninit();
	}
	for (auto& image : m_depthImages)
	{
		image.Uninit();
	}
	for (auto& view : m_swapchainImageViews)
	{
		view.Uninit();
	}
	for (auto& image : m_swapchainImages)
	{
		image.Uninit();
	}
	MyDevice::GetInstance().Uninit();
}
