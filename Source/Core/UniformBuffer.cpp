#include "UniformBuffer.h"
#include "Device.h"
#include "Swapchain.h"
#include "Buffer.h"

namespace FT
{
	UniformBuffer::UniformBuffer(const Device* inDevice, const Swapchain* inSwapchain, const size_t inSize)
		: m_Size(inSize)
	{
		m_Buffers.resize(inSwapchain->GetImageCount());
		for (uint32_t i = 0; i < inSwapchain->GetImageCount(); ++i)
		{
			m_Buffers[i] = new Buffer(inDevice, m_Size, BufferUsageFlags::Uniform);
		}
	}

	UniformBuffer::~UniformBuffer()
	{
		for (uint32_t i = 0; i < m_Buffers.size(); ++i)
		{
			delete(m_Buffers[i]);
		}

		m_Buffers.clear();
	}

	void* UniformBuffer::Map(const uint32_t inCurrentSwapchainImageIndex)
	{
		return m_Buffers[inCurrentSwapchainImageIndex]->Map();
	}

	void UniformBuffer::Unmap(const uint32_t inCurrentSwapchainImageIndex)
	{
		m_Buffers[inCurrentSwapchainImageIndex]->Unmap();
	}
}
