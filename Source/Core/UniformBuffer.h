#pragma once

namespace FT
{
	class Device;
	class Swapchain;
	class Buffer;

	class UniformBuffer
	{
	public:
		UniformBuffer(const Device* inDevice, const Swapchain* inSwapchain, const size_t inSize);
		~UniformBuffer();

	private:
		UniformBuffer(UniformBuffer const&) = delete;
		UniformBuffer& operator=(UniformBuffer const&) = delete;

	public:
		void* Map(const uint32_t inCurrentSwapchainImageIndex);
		void Unmap(const uint32_t inCurrentSwapchainImageIndex);

	public:
		Buffer* GetBuffer(const uint32_t inBUfferIndex) const { return m_Buffers[inBUfferIndex]; }

	private:
		std::vector<Buffer*> m_Buffers;
	};
}
