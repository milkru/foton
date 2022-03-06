#include "Image.h"
#include "Sampler.h"
#include "CombinedImageSampler.h"
#include "Device.h"
#include "Utility/ImageFile.h"

FT_BEGIN_NAMESPACE

rapidjson::Value SerializeCombinedImageSampler(const std::string& inImagePath, const SamplerInfo& inSamplerInfo, rapidjson::Document::AllocatorType& inAllocator)
{
	rapidjson::Value json(rapidjson::kObjectType);

	rapidjson::Value imageJson = SerializeImage(inImagePath, inAllocator);
	rapidjson::Value samplerJson = SerializeSampler(inSamplerInfo, inAllocator);

	json.AddMember("Image", imageJson, inAllocator);
	json.AddMember("Sampler", samplerJson, inAllocator);

	return json;
}

bool DeserializeCombinedImageSampler(const rapidjson::Value& inCombinedImageSamplerJson, std::string& outImagePath, SamplerInfo& outSamplerInfo)
{
	if (!inCombinedImageSamplerJson["Image"].IsObject() ||
		!DeserializeImage(inCombinedImageSamplerJson["Image"], outImagePath))
	{
		FT_LOG("Failed Image deserialization.\n");
		return false;
	}

	if (!inCombinedImageSamplerJson["Sampler"].IsObject() ||
		!DeserializeSampler(inCombinedImageSamplerJson["Sampler"], outSamplerInfo))
	{
		FT_LOG("Failed Sampler deserialization.\n");
		return false;
	}

	return true;
}

static void CreateDescriptorInfo(const VkImageView inImageView, const VkSampler inSampler, VkDescriptorImageInfo& outDescriptorInfo)
{
	outDescriptorInfo = {};
	outDescriptorInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	outDescriptorInfo.imageView = inImageView;
	outDescriptorInfo.sampler = inSampler;
}

CombinedImageSampler::CombinedImageSampler(const Device* inDevice, const ImageFile& inFile, const SamplerInfo& inSamplerInfo)
	: m_Device(inDevice)
	, m_Image(new Image(inDevice, inFile))
	, m_Sampler(new Sampler(inDevice, inSamplerInfo))
{
	CreateDescriptorInfo(m_Image->GetImageView(), m_Sampler->GetSampler(), m_DescriptorInfo);
}

CombinedImageSampler::~CombinedImageSampler()
{
	delete(m_Sampler);
	delete(m_Image);
}

void CombinedImageSampler::UpdateImage(const ImageFile& inFile)
{
	delete(m_Image);
	m_Image = new Image(m_Device, inFile);
	CreateDescriptorInfo(m_Image->GetImageView(), m_Sampler->GetSampler(), m_DescriptorInfo);
}

void CombinedImageSampler::UpdateSampler(const SamplerInfo& inSamplerInfo)
{
	delete(m_Sampler);
	m_Sampler = new Sampler(m_Device, inSamplerInfo);
	CreateDescriptorInfo(m_Image->GetImageView(), m_Sampler->GetSampler(), m_DescriptorInfo);
}

FT_END_NAMESPACE
