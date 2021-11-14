#pragma once

namespace FT
{
	class Device
	{
	public:
		Device(GLFWwindow* inWindow);
		~Device();

	public:
		void AllocateCommandBuffers(const uint32_t inSwapchainImageCount);
		void FreeCommandBuffers();
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
		VkCommandBuffer GetCommandBuffer(const uint32_t inIndex) const { return m_CommandBuffers[inIndex]; }

	private:
		VkInstance m_Instance;
		VkDebugUtilsMessengerEXT m_DebugMessenger;
		VkSurfaceKHR m_Surface;
		VkPhysicalDevice m_PhysicalDevice;
		VkDevice m_Device;
		VkQueue m_GraphicsQueue;
		uint32_t m_GraphicsQueueFamilyIndex;
		VkCommandPool m_CommandPool;
		std::vector<VkCommandBuffer> m_CommandBuffers;
	};
}
