#include "ResourceContainer.h"
#include "Swapchain.h"
#include "Image.h"
#include "UniformBuffer.h"
#include "Utility/ImageFile.h"

namespace FT
{
	ResourceContainer::ResourceContainer(const Device* inDevice, const Swapchain* inSwapchain)
		: m_Device(inDevice)
		, m_Swapchain(inSwapchain)
	{
		// TODO: Make default texture something else.
		const static std::string defaultImagePath = GetFullPath("icon");
		const ImageFile imageFile(defaultImagePath);
		m_DefaultImage = new Image(m_Device, imageFile);
	}

	ResourceContainer::~ResourceContainer()
	{
		for (const auto& descriptor : m_Descriptors)
		{
			DeleteResource(descriptor.Resource);
		}

		delete(m_DefaultImage);
	}

	ResourceType GetResourceType(const VkDescriptorType inDescriptorType)
	{
		switch (inDescriptorType)
		{
		case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
			return ResourceType::Image;

		case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
			return ResourceType::UniformBuffer;

		default:
			// TODO: How to handle VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE and VK_DESCRIPTOR_TYPE_SAMPLER.
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

	void ResourceContainer::UpdateBindings(const std::vector<Binding> inBindings)
	{
		for (uint32_t i = 0; i < m_Descriptors.size(); ++i)
		{
			Descriptor& descriptor = m_Descriptors[i];
			Resource& resource = descriptor.Resource;

			if (i >= inBindings.size())
			{
				DeleteResource(resource);
				break;
			}

			const Binding& newBinding = inBindings[i];
			descriptor.Binding = newBinding;

			const ResourceType newResourceType = GetResourceType(newBinding.DescriptorSetBinding.descriptorType);

			switch (resource.Type)
			{
			case ResourceType::Image:
			{
				if (newResourceType == ResourceType::UniformBuffer)
				{
					DeleteResource(resource);

					const uint32_t bufferSize = newBinding.ReflectDescriptorBinding.block.size;
					resource.Handle.UniformBuffer = new UniformBuffer(m_Device, m_Swapchain, bufferSize);
				}

				break;
			}

			case ResourceType::UniformBuffer:
			{
				const uint32_t bufferSize = newBinding.ReflectDescriptorBinding.block.size;
				if (newResourceType == ResourceType::Image)
				{
					DeleteResource(resource);

					resource.Handle.Image = m_DefaultImage;
				}
				else if (newResourceType == ResourceType::UniformBuffer && resource.Handle.UniformBuffer->GetSize() != bufferSize)
				{
					DeleteResource(resource);

					resource.Handle.UniformBuffer = new UniformBuffer(m_Device, m_Swapchain, bufferSize);
				}

				break;
			}

			default:
				FT_FAIL("Unsupported ResourceType.");
			}

			resource.Type = newResourceType;
		}

		const size_t oldDescriptorCount = m_Descriptors.size();
		m_Descriptors.resize(inBindings.size());

		for (uint32_t i = oldDescriptorCount; i < m_Descriptors.size(); ++i)
		{
			Descriptor& descriptor = m_Descriptors[i];
			Resource& resource = descriptor.Resource;

			const Binding& newBinding = inBindings[i];
			descriptor.Binding = newBinding;

			const ResourceType newResourceType = GetResourceType(newBinding.DescriptorSetBinding.descriptorType);
			resource.Type = newResourceType;

			switch (resource.Type)
			{
			case ResourceType::Image:
			{
				resource.Handle.Image = m_DefaultImage;

				break;
			}

			case ResourceType::UniformBuffer:
			{
				const uint32_t bufferSize = newBinding.ReflectDescriptorBinding.block.size;
				resource.Handle.UniformBuffer = new UniformBuffer(m_Device, m_Swapchain, bufferSize);

				break;
			}

			default:
				FT_FAIL("Unsupported ResourceType.");
			}
		}
	}

	void ResourceContainer::UpdateImage(const uint32_t inBindingIndex, const std::string& inPath)
	{
		FT_CHECK(inBindingIndex < m_Descriptors.size(), "BindingIndex is out of bounds.");

		Resource& resource = m_Descriptors[inBindingIndex].Resource;
		FT_CHECK(resource.Type == ResourceType::Image, "Bound resource needs to be Image type.");

		DeleteResource(resource);

		const ImageFile imageFile(inPath);
		resource.Handle.Image = new Image(m_Device, imageFile);
	}

	void ResourceContainer::DeleteResource(const Resource& inResource)
	{
		const ResourceHandle Handle = inResource.Handle;

		switch (inResource.Type)
		{
		case ResourceType::Image:
		{
			if (Handle.Image != m_DefaultImage)
			{
				delete(Handle.Image);
			}

			break;
		}

		case ResourceType::UniformBuffer:
			delete(Handle.UniformBuffer);
			break;

		default:
			FT_FAIL("Unsupported ResourceType.");
		}
	}
}
