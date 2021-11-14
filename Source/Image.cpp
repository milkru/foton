#include "Image.h"
#include "Device.h"
#include "Buffer.h"
#include "ImageFile.h"

namespace FT
{
	void TransitionImageLayout(const Device* inDevice, const VkImage inImage, const VkFormat inFormat, const VkImageLayout inOldLayout, const VkImageLayout inNewLayout)
	{
		VkCommandBuffer commandBuffer = inDevice->BeginSingleTimeCommands();

		VkImageMemoryBarrier imageBarrier{};
		imageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		imageBarrier.oldLayout = inOldLayout;
		imageBarrier.newLayout = inNewLayout;
		imageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imageBarrier.image = inImage;
		imageBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageBarrier.subresourceRange.baseMipLevel = 0;
		imageBarrier.subresourceRange.levelCount = 1;
		imageBarrier.subresourceRange.baseArrayLayer = 0;
		imageBarrier.subresourceRange.layerCount = 1;

		VkPipelineStageFlags sourceStage;
		VkPipelineStageFlags destinationStage;

		if (inOldLayout == VK_IMAGE_LAYOUT_UNDEFINED && inNewLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
		{
			imageBarrier.srcAccessMask = 0;
			imageBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}
		else if (inOldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && inNewLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
		{
			imageBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			imageBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		}
		else
		{
			FT_FAIL("Unsupported layout transition.");
		}

		vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &imageBarrier);

		inDevice->EndSingleTimeCommands(commandBuffer);
	}

	void CopyBufferToImage(const Device* inDevice, const VkBuffer inBuffer, const VkImage inImage, const uint32_t inWwidth, const uint32_t inHeight)
	{
		const VkCommandBuffer commandBuffer = inDevice->BeginSingleTimeCommands();

		VkBufferImageCopy bufferImageRegion{};
		bufferImageRegion.bufferOffset = 0;
		bufferImageRegion.bufferRowLength = 0;
		bufferImageRegion.bufferImageHeight = 0;
		bufferImageRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		bufferImageRegion.imageSubresource.mipLevel = 0;
		bufferImageRegion.imageSubresource.baseArrayLayer = 0;
		bufferImageRegion.imageSubresource.layerCount = 1;
		bufferImageRegion.imageOffset = { 0, 0, 0 };
		bufferImageRegion.imageExtent = { inWwidth, inHeight, 1 };

		vkCmdCopyBufferToImage(commandBuffer, inBuffer, inImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &bufferImageRegion);

		inDevice->EndSingleTimeCommands(commandBuffer);
	}

	void CreateImage(const Device* inDevice, const ImageFile& inImageFile, VkImage& outImage, VkDeviceMemory& outMemory)
	{
		const int width = inImageFile.GetWidth();
		const int height = inImageFile.GetHeight();

		const VkDeviceSize imageSize = width * height * 4;

		Buffer stagingBuffer(inDevice, imageSize, BufferUsageFlags::TransferSrc);

		void* data = stagingBuffer.Map();
		memcpy(data, inImageFile.GetPixels(), static_cast<size_t>(imageSize));
		stagingBuffer.Unmap();

		VkImageCreateInfo imageCreateInfo{};
		imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
		imageCreateInfo.extent.width = width;
		imageCreateInfo.extent.height = height;
		imageCreateInfo.extent.depth = 1;
		imageCreateInfo.mipLevels = 1;
		imageCreateInfo.arrayLayers = 1;
		imageCreateInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
		imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		FT_VK_CALL(vkCreateImage(inDevice->GetDevice(), &imageCreateInfo, nullptr, &outImage));

		VkMemoryRequirements memRequirements;
		vkGetImageMemoryRequirements(inDevice->GetDevice(), outImage, &memRequirements);

		VkMemoryAllocateInfo allocateInfo{};
		allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocateInfo.allocationSize = memRequirements.size;
		allocateInfo.memoryTypeIndex = inDevice->FindMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		FT_VK_CALL(vkAllocateMemory(inDevice->GetDevice(), &allocateInfo, nullptr, &outMemory));

		vkBindImageMemory(inDevice->GetDevice(), outImage, outMemory, 0);

		TransitionImageLayout(inDevice, outImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		CopyBufferToImage(inDevice, stagingBuffer.GetBuffer(), outImage, static_cast<uint32_t>(width), static_cast<uint32_t>(height));
		TransitionImageLayout(inDevice, outImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}

	void CreateImageView(const VkDevice inDevice, const VkImage inImage, const VkFormat inFormat, VkImageView& outImageView)
	{
		VkImageViewCreateInfo imageViewCreateInfo{};
		imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		imageViewCreateInfo.image = inImage;
		imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		imageViewCreateInfo.format = inFormat;
		imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
		imageViewCreateInfo.subresourceRange.levelCount = 1;
		imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
		imageViewCreateInfo.subresourceRange.layerCount = 1;

		FT_VK_CALL(vkCreateImageView(inDevice, &imageViewCreateInfo, nullptr, &outImageView));
	}

	void CreateSampler(const VkPhysicalDevice inPhysicalDevice, const VkDevice inDevice, VkSampler& outSampler)
	{
		VkPhysicalDeviceProperties physicalDeviceProperties{};
		vkGetPhysicalDeviceProperties(inPhysicalDevice, &physicalDeviceProperties);

		VkSamplerCreateInfo samplerCreateInfo{};
		samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
		samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
		samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerCreateInfo.anisotropyEnable = VK_TRUE;
		samplerCreateInfo.maxAnisotropy = physicalDeviceProperties.limits.maxSamplerAnisotropy;
		samplerCreateInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		samplerCreateInfo.unnormalizedCoordinates = VK_FALSE;
		samplerCreateInfo.compareEnable = VK_FALSE;
		samplerCreateInfo.compareOp = VK_COMPARE_OP_ALWAYS;
		samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

		FT_VK_CALL(vkCreateSampler(inDevice, &samplerCreateInfo, nullptr, &outSampler));
	}

	Image::Image(const Device* inDevice, const std::string inPath)
		: m_Device(inDevice)
	{
		ImageFile imageFile(inPath);
		CreateImage(m_Device, imageFile, m_Image, m_Memory);
		CreateImageView(inDevice->GetDevice(), m_Image, VK_FORMAT_R8G8B8A8_UNORM, m_ImageView);
		CreateSampler(inDevice->GetPhysicalDevice(), inDevice->GetDevice(), m_Sampler);
	}

	Image::~Image()
	{
		vkDestroySampler(m_Device->GetDevice(), m_Sampler, nullptr);
		vkDestroyImageView(m_Device->GetDevice(), m_ImageView, nullptr);
		vkDestroyImage(m_Device->GetDevice(), m_Image, nullptr);
		vkFreeMemory(m_Device->GetDevice(), m_Memory, nullptr);
	}
}
