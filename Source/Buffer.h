#pragma once

namespace FT
{
	class Buffer
	{
	public:
		Buffer(const class Device* inDevice, const size_t inSize, const VkBufferUsageFlags inUsage);
		~Buffer();

	private:
		Buffer(Buffer const&) = delete;
		Buffer& operator=(Buffer const&) = delete;

	public:
		void* Map();
		void Unmap();

	public:
		VkBuffer GetBuffer() const { return m_Buffer; }

	private:
		const Device* m_Device;
		VkBuffer m_Buffer;
		VkDeviceMemory m_Memory;
		size_t m_Size;
		void* m_HostVisibleData;
	};
}
