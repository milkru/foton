#pragma once

namespace FT
{
	// TODO: Move?
#	define FT_FLAG_TYPE_SETUP(flagType) \
		inline flagType operator&(flagType a, flagType b) \
		{ \
			return static_cast<flagType>(static_cast<std::underlying_type<flagType>::type>(a) & \
				static_cast<std::underlying_type<flagType>::type>(b) ); \
		} \
		\
		inline flagType operator|(flagType a, flagType b) \
		{ \
			return static_cast<flagType>( static_cast<std::underlying_type<flagType>::type>(a) | \
				static_cast<std::underlying_type<flagType>::type>(b)); \
		} \
		\
		inline bool IsFlagSet(flagType x) \
		{ \
			return (static_cast<uint32_t>(x) != 0); \
		}

	enum class BufferUsageFlags
	{
		None = 0x0 << 0,
		TransferSrc = 0x1 << 0,
		TransferDst = 0x1 << 1,
		Uniform = 0x1 << 2,
	};
	FT_FLAG_TYPE_SETUP(BufferUsageFlags)
	
	class Device;

	class Buffer
	{
	public:
		Buffer(const Device* inDevice, const size_t inSize, const BufferUsageFlags inUsageFlags);
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
