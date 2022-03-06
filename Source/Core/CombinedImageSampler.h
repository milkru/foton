#pragma once

FT_BEGIN_NAMESPACE

class Device;
class ImageFile;
class Image;
class Sampler;
struct SamplerInfo;

rapidjson::Value SerializeCombinedImageSampler(const std::string& inImagePath, const SamplerInfo& inSamplerInfo, rapidjson::Document::AllocatorType& inAllocator);
bool DeserializeCombinedImageSampler(const rapidjson::Value& inCombinedImageSamplerJson, std::string& outImagePath, SamplerInfo& outSamplerInfo);

class CombinedImageSampler
{
public:
	CombinedImageSampler(const Device* inDevice, const ImageFile& inFile, const SamplerInfo& inSamplerInfo);
	~CombinedImageSampler();
	FT_DELETE_COPY_AND_MOVE(CombinedImageSampler)

public:
	void UpdateImage(const ImageFile& inFile);
	void UpdateSampler(const SamplerInfo& inSamplerInfo);

public:
	const Image* GetImage() const { return m_Image; }
	const Sampler* GetSampler() const { return m_Sampler; }
	const VkDescriptorImageInfo* GetDescriptorInfo() const { return &m_DescriptorInfo; }

private:
	const Device* m_Device;
	const Image* m_Image;
	const Sampler* m_Sampler;
	VkDescriptorImageInfo m_DescriptorInfo;
};

FT_END_NAMESPACE
