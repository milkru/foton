#pragma once

FT_BEGIN_NAMESPACE

class Window;

class Device
{
public:
	Device(const Window* inWindow);
	~Device();
	FT_DELETE_COPY_AND_MOVE(Device)

public:
	uint32_t FindMemoryType(const uint32_t inTypeFilter, const VkMemoryPropertyFlags inProperties) const;
	VkCommandBuffer BeginSingleTimeCommands() const;
	void EndSingleTimeCommands(VkCommandBuffer commandBuffer) const;

public:
	VkInstance GetInstance() const { return m_Instance; }
	VkSurfaceKHR GetSurface() const { return m_Surface; }
	VkPhysicalDevice GetPhysicalDevice() const { return m_PhysicalDevice; }
	VkDevice GetDevice() const { return m_Device; }
	VkQueue GetGraphicsQueue() const { return m_GraphicsQueue; }
	uint32_t GetGraphicsQueueFamilyIndex() const { return m_GraphicsQueueFamilyIndex; }
	VkCommandPool GetCommandPool() const { return m_CommandPool; }

private:
	VkInstance m_Instance;
	VkDebugUtilsMessengerEXT m_DebugMessenger;
	VkSurfaceKHR m_Surface;
	VkPhysicalDevice m_PhysicalDevice;
	VkDevice m_Device;
	VkQueue m_GraphicsQueue;
	uint32_t m_GraphicsQueueFamilyIndex;
	VkCommandPool m_CommandPool;
};

FT_END_NAMESPACE
