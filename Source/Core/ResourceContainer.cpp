#include "ResourceContainer.h"
#include "Swapchain.h"
#include "CombinedImageSampler.h"
#include "Image.h"
#include "Sampler.h"
#include "UniformBuffer.h"
#include "Descriptor.hpp"
#include "Utility/ImageFile.h"

FT_BEGIN_NAMESPACE

// TODO: Make default texture something else.
static const std::string DefaultImagePath = GetFullPath("icon");

ResourceContainer::ResourceContainer(const Device* inDevice, const Swapchain* inSwapchain)
	: m_Device(inDevice)
	, m_Swapchain(inSwapchain) {}

ResourceContainer::~ResourceContainer()
{
	for (const auto& descriptor : m_Descriptors)
	{
		DeleteResource(descriptor.Resource);
	}
}

static ResourceType GetResourceType(const VkDescriptorType inDescriptorType)
{
	switch (inDescriptorType)
	{
	case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
		return ResourceType::CombinedImageSampler;

	case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
		return ResourceType::Image;

	case VK_DESCRIPTOR_TYPE_SAMPLER:
		return ResourceType::Sampler;

	case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
		return ResourceType::UniformBuffer;

	default:
		FT_FAIL("Unsupported VkDescriptorType.");
	}
}

void ResourceContainer::RecreateUniformBuffers()
{
	const uint32_t swapchainImageCount = m_Swapchain->GetImageCount();

	for (auto& descriptor : m_Descriptors)
	{
		Resource& resource = descriptor.Resource;
		if (resource.Type == ResourceType::UniformBuffer)
		{
			UniformBuffer*& uniformBuffer = resource.Handle.UniformBuffer;
			if (uniformBuffer->GetBufferCount() == swapchainImageCount)
			{
				continue;
			}

			const uint32_t bufferSize = uniformBuffer->GetSize();
			DeleteResource(resource);

			uniformBuffer = new UniformBuffer(m_Device, m_Swapchain, bufferSize);
		}
	}
}

static uint32_t GetUniformBufferSize(const SpvReflectDescriptorBinding inReflectDescriptorBinding)
{
	return inReflectDescriptorBinding.block.padded_size * inReflectDescriptorBinding.count;
}

static ResourceHandle CreateResource(const Device* inDevice, const Swapchain* inSwapchain, const ResourceType inResourceType, const SpvReflectDescriptorBinding inReflectDescriptorBinding)
{
	ResourceHandle handle;

	switch (inResourceType)
	{
	case ResourceType::CombinedImageSampler:
	{
		const ImageFile imageFile(DefaultImagePath);
		const SamplerInfo samplerInfo{};
		handle.CombinedImageSampler = new CombinedImageSampler(inDevice, imageFile, samplerInfo);
		break;
	}

	case ResourceType::Image:
	{
		const ImageFile imageFile(DefaultImagePath);
		handle.Image = new Image(inDevice, imageFile);
		break;
	}

	case ResourceType::Sampler:
	{
		const SamplerInfo samplerInfo{};
		handle.Sampler = new Sampler(inDevice, samplerInfo);
		break;
	}

	case ResourceType::UniformBuffer:
	{
		const uint32_t bufferSize = GetUniformBufferSize(inReflectDescriptorBinding);
		handle.UniformBuffer = new UniformBuffer(inDevice, inSwapchain, bufferSize);
		break;
	}

	default:
		FT_FAIL("Unsupported ResourceType.");
	}

	return handle;
}

static void MergeBindings(std::vector<Binding>& inOutBindings)
{
	for (auto iterator = inOutBindings.begin(); iterator != inOutBindings.end(); ++iterator)
	{
		const ResourceType resourceType = GetResourceType(iterator->DescriptorSetBinding.descriptorType);
		if (resourceType != ResourceType::Image)
		{
			continue;
		}

		for (auto samplerIterator = inOutBindings.begin(); samplerIterator != inOutBindings.end(); ++samplerIterator)
		{
			const ResourceType possibleSamplerType = GetResourceType(samplerIterator->DescriptorSetBinding.descriptorType);
			if (possibleSamplerType == ResourceType::Sampler &&
				iterator->DescriptorSetBinding.binding == samplerIterator->DescriptorSetBinding.binding)
			{
				inOutBindings.erase(samplerIterator);
				iterator->DescriptorSetBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				break;
			}
		}
	}
}

void ResourceContainer::UpdateBindings(std::vector<Binding> inBindings)
{
	MergeBindings(inBindings);

	// TODO: Implement NullResource for all non implemented resources in order to prevent crashes.
	for (uint32_t descriptorIndex = 0; descriptorIndex < m_Descriptors.size(); ++descriptorIndex)
	{
		Descriptor& descriptor = m_Descriptors[descriptorIndex];
		Resource& resource = descriptor.Resource;

		if (descriptorIndex >= inBindings.size())
		{
			DeleteResource(resource);
			break;
		}

		const Binding& newBinding = inBindings[descriptorIndex];
		const ResourceType newResourceType = GetResourceType(newBinding.DescriptorSetBinding.descriptorType);

		if (newResourceType != resource.Type ||
			(resource.Type == ResourceType::UniformBuffer &&
				resource.Handle.UniformBuffer->GetSize() != GetUniformBufferSize(newBinding.ReflectDescriptorBinding)))
		{
			DeleteResource(resource);
			resource.Handle = CreateResource(m_Device, m_Swapchain, newResourceType, newBinding.ReflectDescriptorBinding);
			resource.Type = newResourceType;
		}

		descriptor.Binding = newBinding;
	}

	const size_t oldDescriptorCount = m_Descriptors.size();
	m_Descriptors.resize(inBindings.size());

	for (uint32_t descriptorIndex = oldDescriptorCount; descriptorIndex < m_Descriptors.size(); ++descriptorIndex)
	{
		Descriptor& descriptor = m_Descriptors[descriptorIndex];
		Resource& resource = descriptor.Resource;

		const Binding& newBinding = inBindings[descriptorIndex];
		descriptor.Binding = newBinding;

		const ResourceType newResourceType = GetResourceType(newBinding.DescriptorSetBinding.descriptorType);
		
		resource.Handle = CreateResource(m_Device, m_Swapchain, newResourceType, newBinding.ReflectDescriptorBinding);
		resource.Type = newResourceType;
	}
}

void ResourceContainer::UpdateImage(const uint32_t inBindingIndex, const std::string& inPath)
{
	FT_CHECK(inBindingIndex < m_Descriptors.size(), "BindingIndex is out of bounds.");

	Resource& resource = m_Descriptors[inBindingIndex].Resource;
	const ImageFile imageFile(inPath);

	switch (resource.Type)
	{
	case ResourceType::CombinedImageSampler:
	{
		resource.Handle.CombinedImageSampler->UpdateImage(imageFile);
		break;
	}

	case ResourceType::Image:
	{
		DeleteResource(resource);
		resource.Handle.Image = new Image(m_Device, imageFile);

		break;
	}

	default:
		FT_FAIL("Unsupported ResourceType.");
	}
}

void ResourceContainer::UpdateSampler(const uint32_t inBindingIndex, const SamplerInfo& inSamplerInfo)
{
	FT_CHECK(inBindingIndex < m_Descriptors.size(), "BindingIndex is out of bounds.");

	Resource& resource = m_Descriptors[inBindingIndex].Resource;

	switch (resource.Type)
	{
	case ResourceType::CombinedImageSampler:
	{
		resource.Handle.CombinedImageSampler->UpdateSampler(inSamplerInfo);
		break;
	}

	case ResourceType::Image:
	{
		DeleteResource(resource);
		resource.Handle.Sampler = new Sampler(m_Device, inSamplerInfo);

		break;
	}

	default:
		FT_FAIL("Unsupported ResourceType.");
	}
}

void ResourceContainer::DeleteResource(const Resource& inResource)
{
	const ResourceHandle Handle = inResource.Handle;

	switch (inResource.Type)
	{
	case ResourceType::CombinedImageSampler:
	{
		delete(Handle.CombinedImageSampler);
		break;
	}

	case ResourceType::Image:
	{
		delete(Handle.Image);
		break;
	}

	case ResourceType::Sampler:
	{
		delete(Handle.Sampler);
		break;
	}

	case ResourceType::UniformBuffer:
		delete(Handle.UniformBuffer);
		break;

	default:
		FT_FAIL("Unsupported ResourceType.");
	}
}

FT_END_NAMESPACE
