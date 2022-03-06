#include "UniformBuffer.h"
#include "Device.h"
#include "Swapchain.h"
#include "Buffer.h"

FT_BEGIN_NAMESPACE

rapidjson::Value SerializeUniformBuffer(const size_t inSize, const unsigned char* inProxyMemory,
	const unsigned char* inVectorState, rapidjson::Document::AllocatorType& inAllocator)
{
	rapidjson::Value json(rapidjson::kObjectType);

	json.AddMember("Size", inSize, inAllocator);

	rapidjson::Value proxyMemoryJson(rapidjson::kArrayType);
	for (uint32_t index = 0; index < inSize; ++index)
	{
		proxyMemoryJson.PushBack(rapidjson::Value().SetUint(uint32_t(inProxyMemory[index])), inAllocator);
	}
	json.AddMember("ProxyMemory", proxyMemoryJson, inAllocator);

	rapidjson::Value vectorStateJson(rapidjson::kArrayType);
	for (uint32_t index = 0; index < inSize; ++index)
	{
		vectorStateJson.PushBack(rapidjson::Value().SetUint(uint32_t(inVectorState[index])), inAllocator);
	}
	json.AddMember("VectorState", vectorStateJson, inAllocator);

	return json;
}

bool DeserializeUniformBuffer(const rapidjson::Value& inUniformBufferJson, size_t& outSize,
	unsigned char*& outProxyMemory, unsigned char*& outVectorState)
{
	if (!inUniformBufferJson["Size"].IsInt64())
	{
		FT_LOG("Failed UniformBuffer Size deserialization.\n");
		return false;
	}
	outSize = size_t(inUniformBufferJson["Size"].GetInt64());

	outProxyMemory = new unsigned char[outSize];
	if (!inUniformBufferJson["ProxyMemory"].IsArray() || inUniformBufferJson["ProxyMemory"].Capacity() != outSize)
	{
		FT_LOG("Failed UniformBuffer ProxyMemory deserialization.\n");
		return false;
	}

	for (uint32_t index = 0; index < outSize; ++index)
	{
		outProxyMemory[index] = unsigned char(inUniformBufferJson["ProxyMemory"].GetArray()[index].GetUint());
	}

	outVectorState = new unsigned char[outSize];
	if (!inUniformBufferJson["VectorState"].IsArray() || inUniformBufferJson["VectorState"].Capacity() != outSize)
	{
		FT_LOG("Failed UniformBuffer VectorState deserialization.\n");
		return false;
	}

	for (uint32_t index = 0; index < outSize; ++index)
	{
		outVectorState[index] = unsigned char(inUniformBufferJson["VectorState"].GetArray()[index].GetUint());
	}

	return true;
}

UniformBuffer::UniformBuffer(const Device* inDevice, const Swapchain* inSwapchain, const size_t inSize,
	unsigned char* inProxyMemory, unsigned char* inVectorState)
	: m_Size(inSize)
	, m_ProxyMemory(inProxyMemory != nullptr ? inProxyMemory : new unsigned char[inSize]())
	, m_VectorState(inVectorState != nullptr ? inVectorState : new unsigned char[inSize]())
{
	m_Buffers.resize(inSwapchain->GetImageCount());
	for (uint32_t imageIndex = 0; imageIndex < inSwapchain->GetImageCount(); ++imageIndex)
	{
		m_Buffers[imageIndex] = new Buffer(inDevice, m_Size, BufferUsageFlags::Uniform);
		m_Buffers[imageIndex]->Map();
	}
}

UniformBuffer::~UniformBuffer()
{
	for (uint32_t bufferIndex = 0; bufferIndex < m_Buffers.size(); ++bufferIndex)
	{
		delete(m_Buffers[bufferIndex]);
	}

	m_Buffers.clear();
	delete[](m_VectorState);
	delete[](m_ProxyMemory);
}

void UniformBuffer::UpdateDeviceMemory(uint32_t inCurrentImage)
{
	// Map/Unmap each frame should be slower on Vulkan, but rather mapping the memory only once in constructor.
	// This is called persistently mapped memory.
	// Faster for the driver to deal with and easier to use. This is not tested.
	memcpy(m_Buffers[inCurrentImage]->GetHostVisibleData(), m_ProxyMemory, m_Size);
}

FT_END_NAMESPACE
