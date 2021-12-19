#pragma once

FT_BEGIN_NAMESPACE

class Device;
class ImageFile;

class Image
{
public:
	Image(const Device* inDevice, const ImageFile& inFile);
	~Image();
	FT_DELETE_COPY_AND_MOVE(Image)

public:
	VkImage GetImage() const { return m_Image; }
	VkImageView GetImageView() const { return m_ImageView; }
	VkSampler GetSampler() const { return m_Sampler; }
	const VkDescriptorImageInfo* GetDescriptorInfo() const { return &m_DescriptorInfo; }
	uint32_t GetWidth() const { return m_Width; }
	uint32_t GetHeight() const { return m_Height; }

private:
	const Device* m_Device;
	VkImage m_Image;
	VkDeviceMemory m_Memory;
	VkImageView m_ImageView;
	VkSampler m_Sampler;
	VkDescriptorImageInfo m_DescriptorInfo;
	uint32_t m_Width;
	uint32_t m_Height;
};

FT_END_NAMESPACE
