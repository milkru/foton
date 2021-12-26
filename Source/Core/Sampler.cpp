#include "Sampler.h"
#include "Device.h"

FT_BEGIN_NAMESPACE

VkFilter GetFilter(const SamplerFilter inSamplerFilter)
{
	switch (inSamplerFilter)
	{
	case SamplerFilter::Nearest:
		return VK_FILTER_NEAREST;

	case SamplerFilter::Linear:
		return VK_FILTER_LINEAR;

	default:
		FT_FAIL("Unsupported SamplerFilter.");
	}
}

VkSamplerAddressMode GetSamplerAddressMode(const SamplerAddressMode inSamplerAddressMode)
{
	switch (inSamplerAddressMode)
	{
	case SamplerAddressMode::Repeat:
		return VK_SAMPLER_ADDRESS_MODE_REPEAT;

	case SamplerAddressMode::MirroredRepeat:
		return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;

	case SamplerAddressMode::ClampToEdge:
		return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

	case SamplerAddressMode::ClampToBorder:
		return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;

	case SamplerAddressMode::MirrorClampToEdge:
		return VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE;

	default:
		FT_FAIL("Unsupported SamplerAddressMode.");
	}
}

VkBorderColor GetBorderColor(const SamplerBorderColor inSamplerBorderColor)
{
	switch (inSamplerBorderColor)
	{
	case SamplerBorderColor::TransparentBlack:
		return VK_BORDER_COLOR_INT_TRANSPARENT_BLACK;

	case SamplerBorderColor::OpaqueBlack:
		return VK_BORDER_COLOR_INT_OPAQUE_BLACK;

	case SamplerBorderColor::OpaqueWhite:
		return VK_BORDER_COLOR_INT_OPAQUE_WHITE;

	default:
		FT_FAIL("Unsupported SamplerBorderColor.");
	}
}

void CreateSampler(const VkPhysicalDevice inPhysicalDevice, const VkDevice inDevice, const SamplerInfo& inInfo, VkSampler& outSampler)
{
	VkPhysicalDeviceProperties physicalDeviceProperties{};
	vkGetPhysicalDeviceProperties(inPhysicalDevice, &physicalDeviceProperties);

	VkSamplerCreateInfo samplerCreateInfo{};
	samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerCreateInfo.magFilter = GetFilter(inInfo.MagFilter);
	samplerCreateInfo.minFilter = GetFilter(inInfo.MinFilter);
	samplerCreateInfo.addressModeU = GetSamplerAddressMode(inInfo.AddressModeU);
	samplerCreateInfo.addressModeV = GetSamplerAddressMode(inInfo.AddressModeV);
	samplerCreateInfo.addressModeW = GetSamplerAddressMode(inInfo.AddressModeW);
	samplerCreateInfo.anisotropyEnable = VK_TRUE;
	samplerCreateInfo.maxAnisotropy = physicalDeviceProperties.limits.maxSamplerAnisotropy;
	samplerCreateInfo.borderColor = GetBorderColor(inInfo.BorderColor);
	samplerCreateInfo.unnormalizedCoordinates = VK_FALSE;
	samplerCreateInfo.compareEnable = VK_FALSE;
	samplerCreateInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

	FT_VK_CALL(vkCreateSampler(inDevice, &samplerCreateInfo, nullptr, &outSampler));
}

void CreateDescriptorInfo(const VkSampler inSampler, VkDescriptorImageInfo& outDescriptorInfo)
{
	outDescriptorInfo = {};
	outDescriptorInfo.sampler = inSampler;
}

Sampler::Sampler(const Device* inDevice, const SamplerInfo& inInfo)
	: m_Device(inDevice)
	, m_Info(inInfo)
{
	CreateSampler(inDevice->GetPhysicalDevice(), inDevice->GetDevice(), m_Info, m_Sampler);
	CreateDescriptorInfo(m_Sampler, m_DescriptorInfo);
}

Sampler::~Sampler()
{
	vkDestroySampler(m_Device->GetDevice(), m_Sampler, nullptr);
}

FT_END_NAMESPACE
