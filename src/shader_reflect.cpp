#include "shader_reflect.h"
#include "shader.h"
#include "pipeline.h"
#include "pipeline_io.h"
#include "utils.h"
#include <spirv_reflect.h>
#include <fstream>

namespace
{
	struct DescriptorSetLayoutData {
		std::vector<std::string> names;
		std::vector<VkDescriptorSetLayoutBinding> bindings;
	};

#define PRINT_STAGE(os, stage, bit) if (COMPARE_BITS(stage, bit))\
									{\
										os << #bit <<" ";\
									}
	void _PrintStage(std::ostream& os, VkShaderStageFlags stages)
	{
		PRINT_STAGE(os, stages, VK_SHADER_STAGE_VERTEX_BIT);
		PRINT_STAGE(os, stages, VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT);
		PRINT_STAGE(os, stages, VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT);
		PRINT_STAGE(os, stages, VK_SHADER_STAGE_GEOMETRY_BIT);
		PRINT_STAGE(os, stages, VK_SHADER_STAGE_FRAGMENT_BIT);
		PRINT_STAGE(os, stages, VK_SHADER_STAGE_COMPUTE_BIT);
		PRINT_STAGE(os, stages, VK_SHADER_STAGE_RAYGEN_BIT_KHR);
		PRINT_STAGE(os, stages, VK_SHADER_STAGE_ANY_HIT_BIT_KHR);
		PRINT_STAGE(os, stages, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);
		PRINT_STAGE(os, stages, VK_SHADER_STAGE_MISS_BIT_KHR);
		PRINT_STAGE(os, stages, VK_SHADER_STAGE_INTERSECTION_BIT_KHR);
		PRINT_STAGE(os, stages, VK_SHADER_STAGE_CALLABLE_BIT_KHR);
		PRINT_STAGE(os, stages, VK_SHADER_STAGE_TASK_BIT_EXT);
		PRINT_STAGE(os, stages, VK_SHADER_STAGE_MESH_BIT_EXT);
		PRINT_STAGE(os, stages, VK_SHADER_STAGE_SUBPASS_SHADING_BIT_HUAWEI);
		PRINT_STAGE(os, stages, VK_SHADER_STAGE_CLUSTER_CULLING_BIT_HUAWEI);
	}
#undef PRINT_STAGE(os, stage, bit)

#define PRINT_TYPE(os, vkt, vkt2) if (vkt == vkt2)\
									{\
										os << #vkt2 << " ";\
									}
	void _PrintDescriptorType(std::ostream& os, VkDescriptorType vkType)
	{
		PRINT_TYPE(os, vkType, VK_DESCRIPTOR_TYPE_SAMPLER);
		PRINT_TYPE(os, vkType, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		PRINT_TYPE(os, vkType, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE);
		PRINT_TYPE(os, vkType, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
		PRINT_TYPE(os, vkType, VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER);
		PRINT_TYPE(os, vkType, VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER);
		PRINT_TYPE(os, vkType, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
		PRINT_TYPE(os, vkType, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
		PRINT_TYPE(os, vkType, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC);
		PRINT_TYPE(os, vkType, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC);
		PRINT_TYPE(os, vkType, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT);
		PRINT_TYPE(os, vkType, VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK);
		PRINT_TYPE(os, vkType, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR);
		PRINT_TYPE(os, vkType, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV);
		PRINT_TYPE(os, vkType, VK_DESCRIPTOR_TYPE_SAMPLE_WEIGHT_IMAGE_QCOM);
		PRINT_TYPE(os, vkType, VK_DESCRIPTOR_TYPE_BLOCK_MATCH_IMAGE_QCOM);
		PRINT_TYPE(os, vkType, VK_DESCRIPTOR_TYPE_MUTABLE_EXT);
		PRINT_TYPE(os, vkType, VK_DESCRIPTOR_TYPE_PARTITIONED_ACCELERATION_STRUCTURE_NV);
	}
#undef PRINT_TYPE(os, vkt, vkt2)

	// reflect descriptor set layout based on shaders of a pipeline
	// _shaderPaths: paths of all shader files of a pipeline,
	// _mapSetBinding: output, maps descriptor name to {set, binding}
	// _uptrDescriptorSetLayouts: output, stores layout in order, i.e. set0, set1, ...
	void _ReflectDescriptorSet(
		const std::vector<std::string>& _shaderPaths,
		std::unordered_map<std::string, std::pair<uint32_t, uint32_t>>& _mapSetBinding,
		std::vector<std::unique_ptr<DescriptorSetLayout>>& _uptrDescriptorSetLayouts)
	{
		std::unordered_map<uint32_t, DescriptorSetLayoutData> mapDescriptorSets;

		// fill mapDescriptorSets
		for (auto& _shader : _shaderPaths)
		{
			SpvReflectShaderModule reflectModule{};
			std::vector<uint8_t> spirvData;
			auto eResult = SPV_REFLECT_RESULT_SUCCESS;
			uint32_t uSetCount = 0u;
			std::vector<SpvReflectDescriptorSet*> pSets;

			CommonUtils::ReadFile(_shader, spirvData);

			eResult = spvReflectCreateShaderModule(spirvData.size(), spirvData.data(), &reflectModule);
			assert(eResult == SPV_REFLECT_RESULT_SUCCESS);

			eResult = spvReflectEnumerateDescriptorSets(&reflectModule, &uSetCount, nullptr);
			assert(eResult == SPV_REFLECT_RESULT_SUCCESS);

			pSets.resize(uSetCount);
			eResult = spvReflectEnumerateDescriptorSets(&reflectModule, &uSetCount, pSets.data());
			assert(eResult == SPV_REFLECT_RESULT_SUCCESS);

			// store descriptor set of this stage into map
			for (auto _pSet : pSets)
			{
				const SpvReflectDescriptorSet& curSet = *_pSet;
				std::vector<SpvReflectDescriptorBinding*> pBindings(_pSet->bindings, _pSet->bindings + static_cast<size_t>(_pSet->binding_count));
				if (mapDescriptorSets.find(curSet.set) == mapDescriptorSets.end())
				{
					DescriptorSetLayoutData setData{};
					for (auto pBinding : pBindings)
					{
						VkDescriptorSetLayoutBinding vkBinding{};
						vkBinding.binding = pBinding->binding;
						vkBinding.descriptorType = static_cast<VkDescriptorType>(pBinding->descriptor_type);
						vkBinding.stageFlags |= static_cast<VkShaderStageFlagBits>(reflectModule.shader_stage);
						vkBinding.descriptorCount = 1;
						for (uint32_t uDim = 0u; uDim < pBinding->array.dims_count; ++uDim)
						{
							vkBinding.descriptorCount *= (pBinding->array.dims[uDim]);
						}

						setData.bindings.push_back(std::move(vkBinding));
						setData.names.emplace_back(pBinding->name);
					}

					mapDescriptorSets.insert(std::make_pair(curSet.set, setData));
				}
				else
				{
					auto& setData = mapDescriptorSets[curSet.set];
					for (auto pBinding : pBindings)
					{
						int index = -1;

						// check if we already have such binding stored
						for (int nBindingIndex = 0; nBindingIndex < setData.bindings.size(); ++nBindingIndex)
						{
							auto& vkBinding = setData.bindings[nBindingIndex];
							if (vkBinding.binding == pBinding->binding)
							{
								index = nBindingIndex;
								break;
							}
						}

						if (index != -1)
						{
							// we update the existing binding
							VkDescriptorSetLayoutBinding& vkBinding = setData.bindings[index];
							uint32_t uDescriptorCount = 1;

							for (uint32_t uDim = 0u; uDim < pBinding->array.dims_count; ++uDim)
							{
								uDescriptorCount *= (pBinding->array.dims[uDim]);
							}
							CHECK_TRUE(vkBinding.descriptorCount == uDescriptorCount, "The descriptor doesn't match!");
							CHECK_TRUE(vkBinding.descriptorType == static_cast<VkDescriptorType>(pBinding->descriptor_type), "The descriptor doesn't match!");
							vkBinding.stageFlags |= static_cast<VkShaderStageFlagBits>(reflectModule.shader_stage);
						}
						else
						{
							// we add a new binding
							VkDescriptorSetLayoutBinding vkBinding{};

							vkBinding.binding = pBinding->binding;
							vkBinding.descriptorType = static_cast<VkDescriptorType>(pBinding->descriptor_type);
							vkBinding.stageFlags |= static_cast<VkShaderStageFlagBits>(reflectModule.shader_stage);
							vkBinding.descriptorCount = 1;
							for (uint32_t uDim = 0u; uDim < pBinding->array.dims_count; ++uDim)
							{
								vkBinding.descriptorCount *= (pBinding->array.dims[uDim]);
							}

							setData.bindings.push_back(std::move(vkBinding));
							setData.names.emplace_back(pBinding->name);
						}
					}
				}
			}

			spvReflectDestroyShaderModule(&reflectModule);
		}

		// build descriptor set layout
		{
			// find out how many descriptor layouts do we have
			uint32_t uMaxSet = 0u;
			int setCount = 0;
			for (const auto& _pair : mapDescriptorSets)
			{
				uint32_t uSetId = _pair.first;
				uMaxSet = std::max(uMaxSet, uSetId);
				++setCount;
			}
			if (setCount == 0) return;

			// update _uptrDescriptorSetLayouts, _mapSetBinding
			_uptrDescriptorSetLayouts.reserve(static_cast<size_t>(uMaxSet + 1));
			for (uint32_t uSetId = 0u; uSetId <= uMaxSet; ++uSetId)
			{
				std::unique_ptr<DescriptorSetLayout> uptrDescriptorSetLayout = std::make_unique<DescriptorSetLayout>();

				// fill descriptor setting in the layout and then initialize layout
				if (mapDescriptorSets.find(uSetId) != mapDescriptorSets.end())
				{
					const auto& descriptorData = mapDescriptorSets[uSetId];
					for (size_t i = 0; i < descriptorData.bindings.size(); ++i)
					{
						const auto& vkInfo = descriptorData.bindings[i];
						const auto& strDescriptorName = descriptorData.names[i];

						uptrDescriptorSetLayout->AddBinding(vkInfo.binding, vkInfo.descriptorCount, vkInfo.descriptorType, vkInfo.stageFlags, vkInfo.pImmutableSamplers);
						_mapSetBinding[strDescriptorName] = { uSetId, vkInfo.binding };
					}
					uptrDescriptorSetLayout->Init();
				}
				_uptrDescriptorSetLayouts.push_back(std::move(uptrDescriptorSetLayout));
			}
		}
	}
}

std::vector<char> SpirvReflector::_ReadFile(const std::string& filename)
{
	std::ifstream file(filename, std::ios::ate | std::ios::binary);

	CHECK_TRUE(file.is_open(), "Failed to open file!");

	size_t fileSize = (size_t)file.tellg();
	std::vector<char> ans(fileSize);
	file.seekg(0);
	file.read(ans.data(), fileSize);
	file.close();

	return ans;
}

void SpirvReflector::ReflectPipeline(const std::vector<std::string>& _shaderStage) const
{
	std::unordered_map<uint32_t, DescriptorSetLayoutData> mapDescriptorSets;
	static const std::string& strIndent = "\t";

	for (auto& _shader : _shaderStage)
	{
		SpvReflectShaderModule reflectModule{};
		auto spirvData = _ReadFile(_shader);
		auto eResult = SPV_REFLECT_RESULT_SUCCESS; 
		uint32_t uSetCount = 0u;
		std::vector<SpvReflectDescriptorSet*> pSets;

		eResult = spvReflectCreateShaderModule(sizeof(char) * spirvData.size(), spirvData.data(), &reflectModule);
		assert(eResult == SPV_REFLECT_RESULT_SUCCESS);

		eResult = spvReflectEnumerateDescriptorSets(&reflectModule, &uSetCount, nullptr);
		assert(eResult == SPV_REFLECT_RESULT_SUCCESS);

		pSets.resize(uSetCount);
		eResult = spvReflectEnumerateDescriptorSets(&reflectModule, &uSetCount, pSets.data());
		assert(eResult == SPV_REFLECT_RESULT_SUCCESS);
		
		// store descriptor set of this stage into map
		for (auto _pSet : pSets)
		{
			const SpvReflectDescriptorSet& curSet = *_pSet;
			std::vector<SpvReflectDescriptorBinding*> pBindings(_pSet->bindings, _pSet->bindings + static_cast<size_t>(_pSet->binding_count));
			if (mapDescriptorSets.find(curSet.set) == mapDescriptorSets.end())
			{
				DescriptorSetLayoutData setData{};
				for (auto pBinding : pBindings)
				{
					VkDescriptorSetLayoutBinding vkBinding{};
					vkBinding.binding = pBinding->binding;
					vkBinding.descriptorType = static_cast<VkDescriptorType>(pBinding->descriptor_type);
					vkBinding.stageFlags |= static_cast<VkShaderStageFlagBits>(reflectModule.shader_stage);
					vkBinding.descriptorCount = 1;
					for (uint32_t uDim = 0u; uDim < pBinding->array.dims_count; ++uDim)
					{
						vkBinding.descriptorCount *= (pBinding->array.dims[uDim]);
					}

					setData.bindings.push_back(std::move(vkBinding));
					setData.names.emplace_back(pBinding->name);
				}

				mapDescriptorSets.insert(std::make_pair(curSet.set, setData));
			}
			else
			{
				auto& setData = mapDescriptorSets[curSet.set];
				for (auto pBinding : pBindings)
				{
					int index = -1;

					// check if we already have such binding stored
					for (int nBindingIndex = 0; nBindingIndex < setData.bindings.size(); ++nBindingIndex)
					{
						auto& vkBinding = setData.bindings[nBindingIndex];
						if (vkBinding.binding == pBinding->binding)
						{
							index = nBindingIndex;
							break;
						}
					}

					if (index != -1)
					{
						// we update the existing binding
						VkDescriptorSetLayoutBinding& vkBinding = setData.bindings[index];
						uint32_t uDescriptorCount = 1;
						
						for (uint32_t uDim = 0u; uDim < pBinding->array.dims_count; ++uDim)
						{
							uDescriptorCount *= (pBinding->array.dims[uDim]);
						}
						CHECK_TRUE(vkBinding.descriptorCount == uDescriptorCount, "The descriptor doesn't match!");
						CHECK_TRUE(vkBinding.descriptorType == static_cast<VkDescriptorType>(pBinding->descriptor_type), "The descriptor doesn't match!");
						vkBinding.stageFlags |= static_cast<VkShaderStageFlagBits>(reflectModule.shader_stage);
					}
					else
					{
						// we add a new binding
						VkDescriptorSetLayoutBinding vkBinding{};
						
						vkBinding.binding = pBinding->binding;
						vkBinding.descriptorType = static_cast<VkDescriptorType>(pBinding->descriptor_type);
						vkBinding.stageFlags |= static_cast<VkShaderStageFlagBits>(reflectModule.shader_stage);
						vkBinding.descriptorCount = 1;
						for (uint32_t uDim = 0u; uDim < pBinding->array.dims_count; ++uDim)
						{
							vkBinding.descriptorCount *= (pBinding->array.dims[uDim]);
						}

						setData.bindings.push_back(std::move(vkBinding));
						setData.names.emplace_back(pBinding->name);
					}
				}
			}
		}

		spvReflectDestroyShaderModule(&reflectModule);
	}

	for (const auto& mapPair : mapDescriptorSets)
	{
		uint32_t setId = mapPair.first;
		const auto& setData = mapPair.second;
		std::cout << "set: " << setId << std::endl;
		for (size_t i = 0; i < setData.bindings.size(); ++i)
		{
			const auto& vkBinding = setData.bindings[i];
			std::cout << strIndent << "binding name: " << setData.names[i] << std::endl;
			std::cout << strIndent << "location: " << vkBinding.binding << std::endl;
			std::cout << strIndent << "descriptor count: " << vkBinding.descriptorCount << std::endl;
			std::cout << strIndent << "descriptor type: ";
			_PrintDescriptorType(std::cout, vkBinding.descriptorType);
			std::cout << std::endl;
			std::cout << strIndent << "stage: ";
			_PrintStage(std::cout, vkBinding.stageFlags);
			std::cout << "\r\n\r\n";
		}
		std::cout << std::endl;
	}
}

void ComputePipelineProgram::_ReflectPipeline()
{
	std::vector<std::string> shaderPaths;
	std::vector<std::unique_ptr<SimpleShader>> uptrSimpleShaders;
	std::unique_ptr<ComputePipeline> uptrPipeline = std::make_unique<ComputePipeline>();
	std::unordered_map<std::string, std::pair<uint32_t, uint32_t>> mapSetBinding;
	std::vector<std::unique_ptr<DescriptorSetLayout>> uptrDescriptorSetLayouts;

	_ReflectDescriptorSet(shaderPaths, mapSetBinding, uptrDescriptorSetLayouts);

	// add descriptor layouts
	for (auto& uptrLayout : uptrDescriptorSetLayouts)
	{
		uptrPipeline->AddDescriptorSetLayout(uptrLayout.get());
	}

	// setup shader modules
	for (auto& shaderPath : shaderPaths)
	{
		std::unique_ptr<SimpleShader> uptrShader = std::make_unique<SimpleShader>();
		uptrShader->SetSPVFile(shaderPath);
		uptrShader->Init();
		uptrPipeline->AddShader(uptrShader->GetShaderStageInfo());
	}

	uptrPipeline->Init();

	for (auto& uptrSimpleShader : uptrSimpleShaders)
	{
		uptrSimpleShader->Uninit();
		uptrSimpleShader.reset();
	}
	uptrSimpleShaders.clear();
}
