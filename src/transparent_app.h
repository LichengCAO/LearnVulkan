#pragma once
#include "common.h"
#include "pipeline.h"
#include "pipeline_io.h"
#include "image.h"
#include "buffer.h"
#include "camera.h"
#include "transform.h"
#include "geometry.h"
struct CameraBuffer
{
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 proj;
	alignas(16) glm::vec4 eye;
};

struct ViewportInformation
{
	glm::ivec4 extent{};
};
struct CameraViewInformation
{
	alignas(16) glm::mat4 normalView; // inverse transpose of view matrix
	alignas(4)  float invTanHalfFOV;
	alignas(4)  float screenRatioXY;
};
struct ModelTransform
{
	alignas(16) glm::mat4 model;
	alignas(16) glm::mat4 modelInvTranspose;
};

struct Model
{
	std::string objFilePath;
	std::string texturePath;
	Transform   transform{};
};
struct SimpleMaterial
{
	float roughness = 0;
};
struct GaussianBlurInformation
{
	uint32_t blurRad = 0;
};
struct GaussianKernels
{
	std::array<float, 16> kernels{};
};

class TransparentApp
{
private:
	double lastTime = 0.0;
	float frameTime = 0.0f;
	uint32_t m_mipLevel = 0;
	uint32_t m_blurLayers = 5;
	uint32_t m_currentFrame = 0;
	uint32_t m_blurRadius = 7;
	PersCamera m_camera{400, 300, glm::vec3(2,2,2), glm::vec3(0,0,0), glm::vec3(0,0,1)};
	std::vector<Model> m_models;
	std::vector<Model> m_transModels;
	std::vector<SimpleMaterial> m_transMaterials;

	// Descriptor sets
	DescriptorSetLayout m_oitSampleDSetLayout;
	std::vector<DescriptorSet> m_oitDSets;
	std::vector<Buffer> m_oitSampleTexelBuffers;
	std::vector<BufferView> m_oitSampleTexelBufferViews;
	std::vector<Image> m_oitSampleCountImages;
	std::vector<ImageView> m_oitSampleCountImageViews;
	std::vector<Image> m_oitInUseImages;
	std::vector<ImageView> m_oitInUseImageViews;
	Buffer             m_oitViewportBuffer;

	DescriptorSetLayout m_oitColorDSetLayout;
	std::vector<DescriptorSet> m_oitColorDSets;
	std::vector<Image> m_oitColorImages;
	std::vector<ImageView> m_oitColorImageViews;
	
	DescriptorSetLayout m_modelDSetLayout;
	std::vector<std::vector<DescriptorSet>> m_vecModelDSets;
	std::vector<std::vector<Buffer>> m_vecModelBuffers;
	std::vector<std::vector<DescriptorSet>> m_vecTransModelDSets;
	std::vector<std::vector<Buffer>> m_vecTransModelBuffers;
	std::vector<Texture> m_modelTextures;

	DescriptorSetLayout m_materialDSetLayout;
	std::vector<std::vector<DescriptorSet>> m_vecMaterialDSets;
	std::vector<std::vector<Buffer>> m_vecMaterialBuffers;
	
	DescriptorSetLayout m_cameraDSetLayout;
	std::vector<DescriptorSet> m_cameraDSets;
	std::vector<Buffer> m_cameraBuffers;

	DescriptorSetLayout m_distortDSetLayout;
	std::vector<DescriptorSet> m_distortDSets;
	std::vector<Buffer> m_distortBuffers;

	DescriptorSetLayout m_gbufferDSetLayout;
	std::vector<DescriptorSet> m_gbufferDSets;

	DescriptorSetLayout m_transOutputDSetLayout;
	std::vector<DescriptorSet> m_transOutputDSets;

	DescriptorSetLayout m_blurDSetLayout;
	std::vector<std::vector<DescriptorSet>> m_blurLayeredDSetsX;
	std::vector<std::vector<DescriptorSet>> m_blurLayeredDSetsY;
	std::vector<Buffer> m_blurBuffers;   // store GaussianBlufInformation
	std::vector<Buffer> m_kernelBuffers; // kernels[]

	DescriptorSetLayout m_blurOutputDSetLayout;
	std::vector<DescriptorSet> m_blurOutputDSets;

	// Vertex inputs
	VertexInputLayout m_gbufferVertLayout;
	std::vector<Buffer> m_gbufferVertBuffers;
	std::vector<Buffer> m_gbufferIndexBuffers;

	VertexInputLayout m_transModelVertLayout;
	std::vector<Buffer> m_transModelVertBuffers;
	std::vector<Buffer> m_transModelIndexBuffers;
	
	VertexInputLayout m_quadVertLayout;
	Buffer m_quadVertBuffer;
	Buffer m_quadIndexBuffer;
	
	// Samplers
	VkSampler m_vkSampler = VK_NULL_HANDLE;
	VkSampler m_vkLodSampler = VK_NULL_HANDLE;
	
	// Renderpass
	RenderPass m_gbufferRenderPass;
	std::vector<Image> m_depthImages;
	std::vector<ImageView> m_depthImageViews;
	std::vector<Image> m_gbufferPosImages;
	std::vector<ImageView> m_gbufferPosImageViews;
	std::vector<Image> m_gbufferNormalImages;
	std::vector<ImageView> m_gbufferNormalImageViews;
	std::vector<Image> m_gbufferAlbedoImages;
	std::vector<ImageView> m_gbufferAlbedoImageViews;
	std::vector<ImageView> m_gbufferReadAlbedoImageViews; // to make sure the sampler can see all miplevels
	std::vector<Image> m_gbufferDepthImages;
	std::vector<ImageView> m_gbufferDepthImageViews;
	std::vector<Framebuffer> m_gbufferFramebuffers;

	RenderPass m_oitRenderPass;
	std::vector<Framebuffer> m_oitFramebuffers;

	RenderPass m_distortRenderPass;
	std::vector<Image> m_distortImages;
	std::vector<ImageView> m_distortImageViews;
	std::vector<Framebuffer> m_distortFramebuffers;

	RenderPass m_lightRenderPass;
	std::vector<Image> m_lightImages;
	//std::vector<ImageView> m_lightFramebufferView;
	std::vector<std::vector<ImageView>> m_lightImageLayerViews; // view for each layer
	std::vector<ImageView> m_lightImageBlurViews; // view for output blurred image array TODO:
	std::vector<Framebuffer> m_lightFramebuffers;

	RenderPass m_renderPass;
	std::vector<Image> m_swapchainImages;
	std::vector<ImageView> m_swapchainImageViews;
	std::vector<Framebuffer> m_framebuffers;
	//pipelines
	GraphicsPipeline m_gbufferPipeline;
	
	GraphicsPipeline m_oitPipeline;
	ComputePipeline  m_oitSortPipeline;

	GraphicsPipeline m_distortPipeline;

	GraphicsPipeline m_lightPipeline;
	ComputePipeline  m_blurPipelineX;
	ComputePipeline  m_blurPipelineY;

	GraphicsPipeline m_gPipeline;
	// semaphores
	std::vector<VkSemaphore>	   m_swapchainImageAvailabilities;
	std::vector<CommandSubmission> m_commandSubmissions;
private:
	void _Init();
	void _Uninit();

	void _InitRenderPass();
	void _UninitRenderPass();

	void _InitDescriptorSetLayouts();
	void _UninitDescriptorSetLayouts();
	
	void _InitSampler();
	void _UninitSampler();

	void _InitBuffers();
	void _UninitBuffers();

	void _InitImagesAndViews();
	void _UninitImagesAndViews();
	
	void _InitFramebuffers();
	void _UninitFramebuffers();

	void _InitDescriptorSets();
	void _UninitDescriptorSets();
	
	void _InitVertexInputs();
	void _UninitVertexInputs();
	
	void _InitPipelines();
	void _UninitPipelines();

	void _MainLoop();
	void _UpdateUniformBuffer();
	void _DrawFrame();

	void _ResizeWindow();

	void _ReadObjFile(const std::string& objFile, std::vector<Vertex>& verts, std::vector<uint32_t>& indices) const;
	VkImageLayout _GetImageLayout(ImageView* pImageView) const;
	VkImageLayout _GetImageLayout(VkImage vkImage, uint32_t baseArrayLayer, uint32_t layerCount, uint32_t baseMipLevel, uint32_t levelCount, VkImageAspectFlags aspect) const;
public:
	void Run();
};