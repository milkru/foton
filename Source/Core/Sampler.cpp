#include "Sampler.h"
#include "Device.h"

FT_BEGIN_NAMESPACE

rapidjson::Value SerializeSampler(const SamplerInfo& inSamplerInfo, rapidjson::Document::AllocatorType& inAllocator)
{
	rapidjson::Value json(rapidjson::kObjectType);

	json.AddMember("MagFilter", int(inSamplerInfo.MagFilter), inAllocator);
	json.AddMember("MinFilter", int(inSamplerInfo.MinFilter), inAllocator);
	json.AddMember("AddressModeU", int(inSamplerInfo.AddressModeU), inAllocator);
	json.AddMember("AddressModeV", int(inSamplerInfo.AddressModeV), inAllocator);
	json.AddMember("AddressModeW", int(inSamplerInfo.AddressModeW), inAllocator);
	json.AddMember("BorderColor", int(inSamplerInfo.BorderColor), inAllocator);

	return json;
}

bool DeserializeSampler(const rapidjson::Value& inSamplerJson, SamplerInfo& outSamplerInfo)
{
	if (!inSamplerJson["MagFilter"].IsInt())
	{
		FT_LOG("Failed Sampler MagFilter deserialization.\n");
		return false;
	}
	outSamplerInfo.MagFilter = SamplerFilter(inSamplerJson["MagFilter"].GetInt());

	if (!inSamplerJson["MinFilter"].IsInt())
	{
		FT_LOG("Failed Sampler MinFilter deserialization.\n");
		return false;
	}
	outSamplerInfo.MinFilter = SamplerFilter(inSamplerJson["MinFilter"].GetInt());

	if (!inSamplerJson["AddressModeU"].IsInt())
	{
		FT_LOG("Failed Sampler AddressModeU deserialization.\n");
		return false;
	}
	outSamplerInfo.AddressModeU = SamplerAddressMode(inSamplerJson["AddressModeU"].GetInt());

	if (!inSamplerJson["AddressModeV"].IsInt())
	{
		FT_LOG("Failed Sampler AddressModeV deserialization.\n");
		return false;
	}
	outSamplerInfo.AddressModeV = SamplerAddressMode(inSamplerJson["AddressModeV"].GetInt());

	if (!inSamplerJson["AddressModeW"].IsInt())
	{
		FT_LOG("Failed Sampler AddressModeW deserialization.\n");
		return false;
	}
	outSamplerInfo.AddressModeW = SamplerAddressMode(inSamplerJson["AddressModeW"].GetInt());

	if (!inSamplerJson["BorderColor"].IsInt())
	{
		FT_LOG("Failed Sampler BorderColor deserialization.\n");
		return false;
	}
	outSamplerInfo.BorderColor = SamplerBorderColor(inSamplerJson["BorderColor"].GetInt());

	return true;
}

static VkFilter GetFilter(const SamplerFilter inSamplerFilter)
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

static VkSamplerAddressMode GetSamplerAddressMode(const SamplerAddressMode inSamplerAddressMode)
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

#if 0 // Currently disabled since it requires VK_KHR_sampler_mirror_clamp_to_edge which is not supported on some devices.
	case SamplerAddressMode::MirrorClampToEdge:
		return VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE;
#endif

	default:
		FT_FAIL("Unsupported SamplerAddressMode.");
	}
}

static VkBorderColor GetBorderColor(const SamplerBorderColor inSamplerBorderColor)
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

static void CreateSampler(const VkPhysicalDevice inPhysicalDevice, const VkDevice inDevice, const SamplerInfo& inInfo, VkSampler& outSampler)
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

static void CreateDescriptorInfo(const VkSampler inSampler, VkDescriptorImageInfo& outDescriptorInfo)
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
