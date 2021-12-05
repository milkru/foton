#pragma once

// TODO: How?
#include "Descriptor.hpp"

namespace FT
{
	class Device;
	class Swapchain;

	class ResourceContainer
	{
	public:
		ResourceContainer(const Device* inDevice, const Swapchain* inSwapchain);
		~ResourceContainer();

	private:
		ResourceContainer(ResourceContainer const&) = delete;
		ResourceContainer& operator=(ResourceContainer const&) = delete;

	public:
		void RecreateUniformBuffers();
		void UpdateBindings(const std::vector<Binding> inBindings);
		void UpdateImage(const uint32_t inBindingIndex, const std::string& inPath);

	public:
		std::vector<Descriptor> GetDescriptors() const { return m_Descriptors; }

	private:
		void DeleteResource(const Resource& inResource);

	private:
		const Device* m_Device;
		const Swapchain* m_Swapchain;
		Image* m_DefaultImage;
		std::vector<Descriptor> m_Descriptors;
	};
}
