#include "DescriptorSet.h"
#include "Device.h"
#include "Swapchain.h"
#include "Buffer.h"
#include "CombinedImageSampler.h"
#include "Image.h"
#include "Sampler.h"
#include "UniformBuffer.h"

FT_BEGIN_NAMESPACE

void CreateDescriptorSetLayout(const VkDevice inDevice, const std::vector<Descriptor>& inDescriptors, VkDescriptorSetLayout& outDescriptorSetLayout)
{
	std::vector<VkDescriptorSetLayoutBinding> descriptorSetBindings;
	descriptorSetBindings.resize(inDescriptors.size());
	for (uint32_t i = 0; i < inDescriptors.size(); ++i)
	{
		descriptorSetBindings[i] = inDescriptors[i].Binding.DescriptorSetBinding;
	}

	VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo{};
	descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descriptorSetLayoutCreateInfo.bindingCount = static_cast<uint32_t>(descriptorSetBindings.size());
	descriptorSetLayoutCreateInfo.pBindings = descriptorSetBindings.data();

	FT_VK_CALL(vkCreateDescriptorSetLayout(inDevice, &descriptorSetLayoutCreateInfo, nullptr, &outDescriptorSetLayout));
}

void CreateDescriptorPool(const VkDevice inDevice, const uint32_t inSwapchainImageCount, VkDescriptorPool& outDescriptorPool)
{
	const static uint32_t MaxDescriptorCount = 128;

	std::array<VkDescriptorPoolSize, 2> poolSizes{};
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[0].descriptorCount = MaxDescriptorCount * inSwapchainImageCount;
	poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[1].descriptorCount = MaxDescriptorCount * inSwapchainImageCount;

	VkDescriptorPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	poolInfo.pPoolSizes = poolSizes.data();
	poolInfo.maxSets = inSwapchainImageCount;

	FT_VK_CALL(vkCreateDescriptorPool(inDevice, &poolInfo, nullptr, &outDescriptorPool));
}

void CreateDescriptorSets(const VkDevice inDevice, const VkDescriptorPool inDescriptorPool, const VkDescriptorSetLayout inDescriptorSetLayout, const uint32_t inSwapchainImageCount, const std::vector<Descriptor>& inDescriptors, std::vector<VkDescriptorSet>& outDescriptorSets)
{
	std::vector<VkDescriptorSetLayout> descriptorSetLayouts(inSwapchainImageCount, inDescriptorSetLayout);

	VkDescriptorSetAllocateInfo allocateInfo{};
	allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocateInfo.descriptorPool = inDescriptorPool;
	allocateInfo.descriptorSetCount = inSwapchainImageCount;
	allocateInfo.pSetLayouts = descriptorSetLayouts.data();

	outDescriptorSets.resize(inSwapchainImageCount);
	FT_VK_CALL(vkAllocateDescriptorSets(inDevice, &allocateInfo, outDescriptorSets.data()));

	for (size_t i = 0; i < inSwapchainImageCount; ++i)
	{
		std::vector<VkWriteDescriptorSet> descriptorWrites(inDescriptors.size());
		for (uint32_t j = 0; j < inDescriptors.size(); ++j)
		{
			const Binding binding = inDescriptors[j].Binding;
			const Resource resource = inDescriptors[j].Resource;

			descriptorWrites[j] = {};
			descriptorWrites[j].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[j].dstSet = outDescriptorSets[i];
			descriptorWrites[j].dstBinding = binding.DescriptorSetBinding.binding;
			descriptorWrites[j].dstArrayElement = 0;
			descriptorWrites[j].descriptorType = binding.DescriptorSetBinding.descriptorType;
			descriptorWrites[j].descriptorCount = 1;

			if (resource.Type == ResourceType::CombinedImageSampler)
			{
				const CombinedImageSampler* combinedImageSampler = resource.Handle.CombinedImageSampler;
				descriptorWrites[j].pImageInfo = combinedImageSampler->GetDescriptorInfo();
			}
			else if (resource.Type == ResourceType::Image)
			{
				const Image* image = resource.Handle.Image;
				descriptorWrites[j].pImageInfo = image->GetDescriptorInfo();
			}
			else if (resource.Type == ResourceType::Sampler)
			{
				const Sampler* sampler = resource.Handle.Sampler;
				descriptorWrites[j].pImageInfo = sampler->GetDescriptorInfo();
			}
			else if (resource.Type == ResourceType::UniformBuffer)
			{
				const Buffer* buffer = resource.Handle.UniformBuffer->GetBuffer(i);
				descriptorWrites[j].pBufferInfo = buffer->GetDescriptorInfo();
			}
			else
			{
				FT_FAIL("Descriptor type not supported.");
			}
		}

		vkUpdateDescriptorSets(inDevice, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
	}
}

DescriptorSet::DescriptorSet(const Device* inDevice, const Swapchain* inSwapchain, const std::vector<Descriptor> inDescriptors)
	: m_Device(inDevice)
{
	CreateDescriptorSetLayout(m_Device->GetDevice(), inDescriptors, m_DescriptorSetLayout);
	CreateDescriptorPool(m_Device->GetDevice(), inSwapchain->GetImageCount(), m_DescriptorPool);
	CreateDescriptorSets(m_Device->GetDevice(), m_DescriptorPool, m_DescriptorSetLayout, inSwapchain->GetImageCount(), inDescriptors, m_DescriptorSets);
}

DescriptorSet::~DescriptorSet()
{
	vkDestroyDescriptorPool(m_Device->GetDevice(), m_DescriptorPool, nullptr);
	vkDestroyDescriptorSetLayout(m_Device->GetDevice(), m_DescriptorSetLayout, nullptr);
}

FT_END_NAMESPACE
