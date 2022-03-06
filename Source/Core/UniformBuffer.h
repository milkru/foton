#pragma once

FT_BEGIN_NAMESPACE

class Device;
class Swapchain;
class Buffer;

rapidjson::Value SerializeUniformBuffer(const size_t inSize, const unsigned char* inProxyMemory,
	const unsigned char* inVectorState, rapidjson::Document::AllocatorType& inAllocator);
bool DeserializeUniformBuffer(const rapidjson::Value& inImageJson, size_t& outSize,
	unsigned char*& outProxyMemory, unsigned char*& outVectorState);

class UniformBuffer
{
public:
	UniformBuffer(const Device* inDevice, const Swapchain* inSwapchain, const size_t inSize,
		unsigned char* inProxyMemory = nullptr, unsigned char* inVectorState = nullptr);
	~UniformBuffer();
	FT_DELETE_COPY_AND_MOVE(UniformBuffer)

public:
	void UpdateDeviceMemory(uint32_t inCurrentImage);

public:
	Buffer* GetBuffer(const uint32_t inBufferIndex) const { return m_Buffers[inBufferIndex]; }
	size_t GetBufferCount() const { return m_Buffers.size(); }
	size_t GetSize() const { return m_Size; }
	unsigned char* GetProxyMemory() const {	return m_ProxyMemory; }
	unsigned char* GetVectorState() const {	return m_VectorState; }

private:
	std::vector<Buffer*> m_Buffers;
	unsigned char* m_ProxyMemory;
	unsigned char* m_VectorState;
	size_t m_Size;
};

FT_END_NAMESPACE
