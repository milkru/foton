#pragma once

// TODO: How?
#include "Descriptor.hpp"

FT_BEGIN_NAMESPACE

class Device;
class Swapchain;

// TODO: Rename?
class ResourceContainer
{
public:
	ResourceContainer(const Device* inDevice, const Swapchain* inSwapchain);
	~ResourceContainer();
	FT_DELETE_COPY_AND_MOVE(ResourceContainer)

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

FT_END_NAMESPACE
