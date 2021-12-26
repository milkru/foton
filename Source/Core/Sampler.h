#pragma once

FT_BEGIN_NAMESPACE

enum class SamplerFilter
{
	Nearest,
	Linear,

	Count
};

enum class SamplerAddressMode
{
	Repeat,
	MirroredRepeat,
	ClampToEdge,
	ClampToBorder,
	MirrorClampToEdge,

	Count
};

enum class SamplerBorderColor
{
	TransparentBlack,
	OpaqueBlack,
	OpaqueWhite,

	Count
};

struct SamplerInfo
{
	SamplerFilter MagFilter = SamplerFilter::Linear;
	SamplerFilter MinFilter = SamplerFilter::Linear;
	SamplerAddressMode AddressModeU = SamplerAddressMode::Repeat;
	SamplerAddressMode AddressModeV = SamplerAddressMode::Repeat;
	SamplerAddressMode AddressModeW = SamplerAddressMode::Repeat;
	SamplerBorderColor BorderColor = SamplerBorderColor::TransparentBlack;
};

inline bool operator==(const SamplerInfo& inLeft, const SamplerInfo& inRight) {
	return
		inLeft.MagFilter == inRight.MagFilter &&
		inLeft.MinFilter == inRight.MinFilter &&
		inLeft.AddressModeU == inRight.AddressModeU &&
		inLeft.AddressModeV == inRight.AddressModeV &&
		inLeft.AddressModeW == inRight.AddressModeW &&
		inLeft.BorderColor == inRight.BorderColor;
}

inline bool operator!=(const SamplerInfo& inLeft, const SamplerInfo& inRight) { return !(inLeft == inRight); }

class Device;

class Sampler
{
public:
	Sampler(const Device* inDevice, const SamplerInfo& inInfo);
	~Sampler();
	FT_DELETE_COPY_AND_MOVE(Sampler)

public:
	VkSampler GetSampler() const { return m_Sampler; }
	const VkDescriptorImageInfo* GetDescriptorInfo() const { return &m_DescriptorInfo; }
	SamplerInfo GetInfo() const { return m_Info; }

private:
	const Device* m_Device;
	VkSampler m_Sampler;
	VkDescriptorImageInfo m_DescriptorInfo;
	SamplerInfo m_Info;
};

FT_END_NAMESPACE
