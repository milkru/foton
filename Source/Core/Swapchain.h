#pragma once

FT_BEGIN_NAMESPACE

enum class SwapchainStatus
{
	Success,
	Recreate,
	Failed,

	Count
};

struct SwapchainImageAcquireResult
{
	SwapchainStatus Status;
	uint32_t ImageIndex;
};

class Device;
class CommandBuffer;

class Swapchain
{
public:
	static void FramebufferResized(struct GLFWwindow* inWindow, int inWidth, int inHeight);
	static bool s_FramebufferResized;

public:
	Swapchain(const Device* inDevice, GLFWwindow* inWindow);
	~Swapchain();
	FT_DELETE_COPY_AND_MOVE(Swapchain)

public:
	void Recreate();
	void Cleanup();
	SwapchainImageAcquireResult AcquireNextImage();
	SwapchainStatus Present(const uint32_t inImageIndex, const CommandBuffer* inCommandBuffer);

public:
	uint32_t GetImageCount() const { return static_cast<uint32_t>(m_Images.size()); }
	VkRenderPass GetRenderPass() const { return m_RenderPass; }
	VkFramebuffer GetFramebuffer(const uint32_t inIndex) const { return m_Framebuffers[inIndex]; }
	VkExtent2D GetExtent() const { return m_Extent; }

private:
	const Device* m_Device;
	GLFWwindow* m_Window;
	VkSwapchainKHR m_Swapchain;
	std::vector<VkImage> m_Images;
	VkFormat m_Format;
	VkExtent2D m_Extent;
	std::vector<VkImageView> m_ImageViews;
	VkRenderPass m_RenderPass;
	std::vector<VkFramebuffer> m_Framebuffers;
	std::vector<VkSemaphore> m_ImageAvailableSemaphores;
	std::vector<VkSemaphore> m_RenderFinishedSemaphores;
	std::vector<VkFence> m_InFlightFences;
	std::vector<VkFence> m_ImagesInFlight;
	size_t m_CurrentFrame = 0;
};

FT_END_NAMESPACE
