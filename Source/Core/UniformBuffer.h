#pragma once

FT_BEGIN_NAMESPACE

class Device;
class Swapchain;
class Buffer;

class UniformBuffer
{
public:
	UniformBuffer(const Device* inDevice, const Swapchain* inSwapchain, const size_t inSize);
	~UniformBuffer();
	FT_DELETE_COPY_AND_MOVE(UniformBuffer)

public:
	void* Map(const uint32_t inCurrentSwapchainImageIndex);
	void Unmap(const uint32_t inCurrentSwapchainImageIndex);

public:
	Buffer* GetBuffer(const uint32_t inBufferIndex) const { return m_Buffers[inBufferIndex]; }
	size_t GetBufferCount() const { return m_Buffers.size(); }
	size_t GetSize() const { return m_Size; }

private:
	std::vector<Buffer*> m_Buffers;
	size_t m_Size;
};

FT_END_NAMESPACE
