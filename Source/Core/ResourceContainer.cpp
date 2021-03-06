#include "ResourceContainer.h"
#include "Swapchain.h"
#include "Image.h"
#include "Sampler.h"
#include "CombinedImageSampler.h"
#include "UniformBuffer.h"
#include "Descriptor.hpp"
#include "Utility/ImageFile.h"

FT_BEGIN_NAMESPACE

// TODO: Make default texture something else.
static const std::string DefaultImagePath = GetAbsolutePath("icon");

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

rapidjson::Value ResourceContainer::Serialize(rapidjson::Document::AllocatorType& inAllocator)
{
	rapidjson::Value json(rapidjson::kArrayType);

	for (const auto& descriptor : m_Descriptors)
	{
		const ResourceType resourceType = descriptor.Resource.Type;
		const ResourceHandle resourceHandle = descriptor.Resource.Handle;

		rapidjson::Value resourceJson;

		switch (resourceType)
		{
		case ResourceType::CombinedImageSampler:
		{
			const CombinedImageSampler* combinedImageSampler = resourceHandle.CombinedImageSampler;
			resourceJson = SerializeCombinedImageSampler(combinedImageSampler->GetImage()->GetPath(),
				combinedImageSampler->GetSampler()->GetInfo(), inAllocator);
			break;
		}

		case ResourceType::Image:
		{
			resourceJson = SerializeImage(resourceHandle.Image->GetPath(), inAllocator);
			break;
		}

		case ResourceType::Sampler:
		{
			resourceJson = SerializeSampler(resourceHandle.Sampler->GetInfo(), inAllocator);
			break;
		}

		case ResourceType::UniformBuffer:
		{
			const UniformBuffer* uniformBuffer = resourceHandle.UniformBuffer;
			resourceJson = SerializeUniformBuffer(uniformBuffer->GetSize(), uniformBuffer->GetProxyMemory(),
				uniformBuffer->GetVectorState(), inAllocator);
			break;
		}

		default:
			FT_FAIL("Unsupported ResourceType.");
		}

		rapidjson::Value descriptorJson(rapidjson::kObjectType);
		descriptorJson.AddMember("Type", int(resourceType), inAllocator);
		descriptorJson.AddMember("Resource", resourceJson, inAllocator);

		json.PushBack(descriptorJson, inAllocator);
	}

	return json;
}

bool ResourceContainer::Deserialize()
{
	return false;
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

		descriptor.Index = descriptorIndex;

		const Binding& newBinding = inBindings[descriptorIndex];
		descriptor.Binding = newBinding;

		const ResourceType newResourceType = GetResourceType(newBinding.DescriptorSetBinding.descriptorType);
		
		resource.Handle = CreateResource(m_Device, m_Swapchain, newResourceType, newBinding.ReflectDescriptorBinding);
		resource.Type = newResourceType;
	}
}

void ResourceContainer::UpdateImage(const uint32_t inDescriptorIndex, const std::string& inPath)
{
	FT_CHECK(inDescriptorIndex < m_Descriptors.size(), "BindingIndex is out of bounds.");

	Resource& resource = m_Descriptors[inDescriptorIndex].Resource;
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

void ResourceContainer::UpdateSampler(const uint32_t inDescriptorIndex, const SamplerInfo& inSamplerInfo)
{
	FT_CHECK(inDescriptorIndex < m_Descriptors.size(), "BindingIndex is out of bounds.");

	Resource& resource = m_Descriptors[inDescriptorIndex].Resource;

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

void ResourceContainer::UpdateUniformBuffer(const uint32_t inDescriptorIndex, const size_t inSize,
	unsigned char* inProxyMemory, unsigned char* inVectorState)
{
	FT_CHECK(inDescriptorIndex < m_Descriptors.size(), "BindingIndex is out of bounds.");

	Resource& resource = m_Descriptors[inDescriptorIndex].Resource;
	FT_CHECK(resource.Type == ResourceType::UniformBuffer, "Tried updating non UniformBuffer resource.");

	DeleteResource(resource);
	resource.Handle.UniformBuffer = new UniformBuffer(m_Device, m_Swapchain, inSize, inProxyMemory, inVectorState);
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
