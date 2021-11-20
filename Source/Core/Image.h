#pragma once

namespace FT
{
	class Device;
	class ImageFile;

	class Image
	{
	public:
		Image(const Device* inDevice, const ImageFile& inFile);
		~Image();

	private:
		Image(Image const&) = delete;
		Image& operator=(Image const&) = delete;

	public:
		VkImage GetImage() const { return m_Image; }
		VkImageView GetImageView() const { return m_ImageView; }
		VkSampler GetSampler() const { return m_Sampler; }

	private:
		const Device* m_Device;
		VkImage m_Image;
		VkDeviceMemory m_Memory;
		VkImageView m_ImageView;
		VkSampler m_Sampler;
	};
}
