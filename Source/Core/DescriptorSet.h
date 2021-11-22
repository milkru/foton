#pragma once

#include "Binding.hpp"
#include "Resource.hpp"

namespace FT
{
	struct Descriptor
	{
		Binding Binding;
		Resource Resource;
	};

	class Device;
	class Swapchain;

	class DescriptorSet
	{
	public:
		DescriptorSet(const Device* inDevice, const Swapchain* inSwapchain, const std::vector<Descriptor> inDescriptors);
		~DescriptorSet();

	private:
		DescriptorSet(DescriptorSet const&) = delete;
		DescriptorSet& operator=(DescriptorSet const&) = delete;

	public:
		VkDescriptorSetLayout GetDescriptorSetLayout() const { return m_DescriptorSetLayout; }
		VkDescriptorSet GetDescriptorSet(const uint32_t inDescriptorSetIndex) const { return m_DescriptorSets[inDescriptorSetIndex]; }

	private:
		const Device* m_Device;
		VkDescriptorSetLayout m_DescriptorSetLayout;
		VkDescriptorPool m_DescriptorPool;
		std::vector<VkDescriptorSet> m_DescriptorSets;
	};
}
