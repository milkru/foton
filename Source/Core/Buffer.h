#pragma once

FT_BEGIN_NAMESPACE

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
	FT_DELETE_COPY_AND_MOVE(Buffer)

public:
	void* Map();
	void Unmap();

public:
	VkBuffer GetBuffer() const { return m_Buffer; }
	size_t GetSize() const { return m_Size; }

private:
	const Device* m_Device;
	VkBuffer m_Buffer;
	VkDeviceMemory m_Memory;
	size_t m_Size;
	void* m_HostVisibleData;
};

FT_END_NAMESPACE
