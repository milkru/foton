#include "Buffer.h"
#include "Device.h"

FT_BEGIN_NAMESPACE

VkBufferUsageFlags GetVkBufferUsageFlags(const BufferUsageFlags usageFlags)
{
	VkBufferUsageFlags bufferUsageFlags = 0;
	if (IsFlagSet(usageFlags & BufferUsageFlags::TransferSrc))
	{
		bufferUsageFlags |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	}

	if (IsFlagSet(usageFlags & BufferUsageFlags::TransferDst))
	{
		bufferUsageFlags |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	}

	if (IsFlagSet(usageFlags & BufferUsageFlags::Uniform))
	{
		bufferUsageFlags |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
	}

	return bufferUsageFlags;
}

void CreateBuffer(const Device* inDevice, const VkDeviceSize inSize, const VkBufferUsageFlags inUsage, const VkMemoryPropertyFlags inProperties, VkBuffer& outBuffer, VkDeviceMemory& outBufferMemory)
{
	VkBufferCreateInfo bufferCreateInfo{};
	bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferCreateInfo.size = inSize;
	bufferCreateInfo.usage = inUsage;
	bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	FT_VK_CALL(vkCreateBuffer(inDevice->GetDevice(), &bufferCreateInfo, nullptr, &outBuffer));

	VkMemoryRequirements memoryRequirements;
	vkGetBufferMemoryRequirements(inDevice->GetDevice(), outBuffer, &memoryRequirements);

	VkMemoryAllocateInfo allocateInfo{};
	allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocateInfo.allocationSize = memoryRequirements.size;
	allocateInfo.memoryTypeIndex = inDevice->FindMemoryType(memoryRequirements.memoryTypeBits, inProperties);

	FT_VK_CALL(vkAllocateMemory(inDevice->GetDevice(), &allocateInfo, nullptr, &outBufferMemory));

	vkBindBufferMemory(inDevice->GetDevice(), outBuffer, outBufferMemory, 0);
}

Buffer::Buffer(const Device* inDevice, const size_t inSize, const BufferUsageFlags inUsageFlags)
	: m_Device(inDevice)
	, m_Size(inSize)
	, m_HostVisibleData(nullptr)
{
	CreateBuffer(inDevice, inSize, GetVkBufferUsageFlags(inUsageFlags), VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_Buffer, m_Memory);
}

Buffer::~Buffer()
{
	vkDestroyBuffer(m_Device->GetDevice(), m_Buffer, nullptr);
	vkFreeMemory(m_Device->GetDevice(), m_Memory, nullptr);
}

void* Buffer::Map()
{
	FT_CHECK(m_HostVisibleData == nullptr, "Buffer is still unmapped.");

	FT_VK_CALL(vkMapMemory(m_Device->GetDevice(), m_Memory, 0, m_Size, 0, &m_HostVisibleData));

	return m_HostVisibleData;
}

void Buffer::Unmap()
{
	if (m_HostVisibleData)
	{
		vkUnmapMemory(m_Device->GetDevice(), m_Memory);
		m_HostVisibleData = nullptr;
	}
}

FT_END_NAMESPACE
