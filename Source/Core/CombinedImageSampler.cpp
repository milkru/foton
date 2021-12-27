#include "CombinedImageSampler.h"
#include "Device.h"
#include "Utility/ImageFile.h"
#include "Image.h"
#include "Sampler.h"

FT_BEGIN_NAMESPACE

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
