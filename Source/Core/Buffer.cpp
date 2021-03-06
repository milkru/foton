#include "Buffer.h"
#include "Device.h"

FT_BEGIN_NAMESPACE

static VkBufferUsageFlags GetVkBufferUsageFlags(const BufferUsageFlags usageFlags)
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

static void CreateBuffer(const Device* inDevice, const VkDeviceSize inSize, const VkBufferUsageFlags inUsage, const VkMemoryPropertyFlags inProperties, VkBuffer& outBuffer, VkDeviceMemory& outBufferMemory)
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

static void CreateDescriptorInfo(const VkBuffer inBuffer, const size_t inSize, VkDescriptorBufferInfo& outDescriptorInfo)
{
	outDescriptorInfo = {};
	outDescriptorInfo.buffer = inBuffer;
	outDescriptorInfo.offset = 0;
	outDescriptorInfo.range = inSize;
}

Buffer::Buffer(const Device* inDevice, const size_t inSize, const BufferUsageFlags inUsageFlags)
	: m_Device(inDevice)
	, m_Size(inSize)
	, m_HostVisibleData(nullptr)
{
	// VK_MEMORY_PROPERTY_HOST_COHERENT_BIT means that if we update this memory on the CPU, in the next command we use it on the GPU it will be guarantied that this memory is updated (so it's coherent).
	CreateBuffer(inDevice, inSize, GetVkBufferUsageFlags(inUsageFlags), VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_Buffer, m_Memory);
	CreateDescriptorInfo(m_Buffer, m_Size, m_DescriptorInfo);
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
