#pragma once

FT_BEGIN_NAMESPACE

class Device;
class Swapchain;
struct Descriptor;

class DescriptorSet
{
public:
	DescriptorSet(const Device* inDevice, const Swapchain* inSwapchain, const std::vector<Descriptor> inDescriptors);
	~DescriptorSet();
	FT_DELETE_COPY_AND_MOVE(DescriptorSet)

public:
	VkDescriptorSetLayout GetDescriptorSetLayout() const { return m_DescriptorSetLayout; }
	VkDescriptorSet GetDescriptorSet(const uint32_t inDescriptorSetIndex) const { return m_DescriptorSets[inDescriptorSetIndex]; }

private:
	const Device* m_Device;
	VkDescriptorSetLayout m_DescriptorSetLayout;
	VkDescriptorPool m_DescriptorPool;
	std::vector<VkDescriptorSet> m_DescriptorSets;
};

FT_END_NAMESPACE
