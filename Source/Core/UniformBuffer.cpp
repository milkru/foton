#include "UniformBuffer.h"
#include "Device.h"
#include "Swapchain.h"
#include "Buffer.h"

FT_BEGIN_NAMESPACE

UniformBuffer::UniformBuffer(const Device* inDevice, const Swapchain* inSwapchain, const size_t inSize)
	: m_Size(inSize)
	, m_ProxyMemory(new unsigned char[inSize]())
{
	m_Buffers.resize(inSwapchain->GetImageCount());
	for (uint32_t imageIndex = 0; imageIndex < inSwapchain->GetImageCount(); ++imageIndex)
	{
		m_Buffers[imageIndex] = new Buffer(inDevice, m_Size, BufferUsageFlags::Uniform);
	}
}

UniformBuffer::~UniformBuffer()
{
	for (uint32_t bufferIndex = 0; bufferIndex < m_Buffers.size(); ++bufferIndex)
	{
		delete(m_Buffers[bufferIndex]);
	}

	m_Buffers.clear();
	delete[](m_ProxyMemory);
}

void* UniformBuffer::Map(const uint32_t inCurrentSwapchainImageIndex)
{
	return m_Buffers[inCurrentSwapchainImageIndex]->Map();
}

void UniformBuffer::Unmap(const uint32_t inCurrentSwapchainImageIndex)
{
	m_Buffers[inCurrentSwapchainImageIndex]->Unmap();
}

void UniformBuffer::UpdateDeviceMemory(uint32_t inCurrentImage)
{
	void* deviceMappedMemory = m_Buffers[inCurrentImage]->Map();
	memcpy(deviceMappedMemory, m_ProxyMemory, m_Size);
	m_Buffers[inCurrentImage]->Unmap();
}

FT_END_NAMESPACE
