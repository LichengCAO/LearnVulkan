#pragma once
#include "common.h"

struct SamplerInfo
{
	VkSamplerCreateInfo info{};
	SamplerInfo() { memset(this, 0, sizeof(SamplerInfo)); }

	bool operator==(const SamplerInfo& other) const { return memcmp(this, &other, sizeof(SamplerInfo)) == 0; }
};

class Sampler
{
public:
	VkSampler vkSampler = VK_NULL_HANDLE;
	SamplerInfo info{};
};

class SamplerPool
{
private:
	struct SamplerEntry
	{
		VkSampler vkSampler = VK_NULL_HANDLE;
		SamplerInfo info{};
		uint32_t refCount = 0;
		uint32_t nextId = static_cast<uint32_t>(~0);
	};
	struct hash_fn
	{
		std::size_t operator() (const SamplerInfo& node) const
		{
			std::size_t h1 = std::hash<int>()(static_cast<int>(node.info.addressModeU));
			std::size_t h2 = std::hash<int>()(static_cast<int>(node.info.addressModeV));

			return h1 ^ h2;
		}
	};
	std::unordered_map<SamplerInfo, uint32_t, hash_fn> m_mapInfoToIndex;
	std::unordered_map<VkSampler, uint32_t> m_mapSamplerToIndex;
	std::vector<SamplerEntry> m_vecSamplerEntries;
	uint32_t m_currentId = ~0;
public:
	VkSampler GetSampler(const SamplerInfo _info);
	void ReturnSampler(VkSampler* _sampler);
};