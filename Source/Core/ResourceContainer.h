#pragma once

FT_BEGIN_NAMESPACE

class Device;
class Swapchain;
struct SamplerInfo;
struct Binding;
struct Resource;
struct Descriptor;

class ResourceContainer
{
public:
	ResourceContainer(const Device* inDevice, const Swapchain* inSwapchain);
	~ResourceContainer();
	FT_DELETE_COPY_AND_MOVE(ResourceContainer)

public:
	rapidjson::Value Serialize(rapidjson::Document::AllocatorType& inAllocator);
	bool Deserialize();

public:
	void RecreateUniformBuffers();
	void UpdateBindings(std::vector<Binding> inBindings);
	void UpdateImage(const uint32_t inDescriptorIndex, const std::string& inPath);
	void UpdateSampler(const uint32_t inDescriptorIndex, const SamplerInfo& inSamplerInfo);
	void UpdateUniformBuffer(const uint32_t inDescriptorIndex, const size_t inSize,
		unsigned char* inProxyMemory, unsigned char* inVectorState);

public:
	const std::vector<Descriptor>& GetDescriptors() const { return m_Descriptors; }

private:
	void DeleteResource(const Resource& inResource);

private:
	const Device* m_Device;
	const Swapchain* m_Swapchain;
	std::vector<Descriptor> m_Descriptors;
};

FT_END_NAMESPACE
