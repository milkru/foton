#pragma once

FT_BEGIN_NAMESPACE

// TODO: Implement support for 3D, Cube and Array images.

rapidjson::Value SerializeImage(const std::string& inImagePath, rapidjson::Document::AllocatorType& inAllocator);
bool DeserializeImage(const rapidjson::Value& inImageJson, std::string& outImagePath);

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
	const VkDescriptorImageInfo* GetDescriptorInfo() const { return &m_DescriptorInfo; }
	uint32_t GetWidth() const { return m_Width; }
	uint32_t GetHeight() const { return m_Height; }
	const std::string& GetPath() const { return m_Path; }

private:
	const Device* m_Device;
	VkImage m_Image;
	VkDeviceMemory m_Memory;
	VkImageView m_ImageView;
	VkDescriptorImageInfo m_DescriptorInfo;
	uint32_t m_Width;
	uint32_t m_Height;
	std::string m_Path;
};

FT_END_NAMESPACE
