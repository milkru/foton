#include "Swapchain.h"
#include "Device.h"
#include "Buffer.h"
#include "CommandBuffer.h"

FT_BEGIN_NAMESPACE

const int MaxFramesInFlight = 2;

struct SwapChainSupportDetails
{
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};

SwapChainSupportDetails QuerySwapChainSupport(const VkPhysicalDevice inDevice, const VkSurfaceKHR inSurface)
{
	SwapChainSupportDetails details;

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(inDevice, inSurface, &details.capabilities);

	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(inDevice, inSurface, &formatCount, nullptr);

	if (formatCount != 0)
	{
		details.formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(inDevice, inSurface, &formatCount, details.formats.data());
	}

	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(inDevice, inSurface, &presentModeCount, nullptr);

	if (presentModeCount != 0)
	{
		details.presentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(inDevice, inSurface, &presentModeCount, details.presentModes.data());
	}

	return details;
}

VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& inAvailableFormats)
{
	for (const auto& availableFormat : inAvailableFormats)
	{
		if (availableFormat.format == VK_FORMAT_R8G8B8A8_UNORM)
		{
			return availableFormat;
		}
	}

	return inAvailableFormats[0];
}

VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& inAvailablePresentModes)
{
	for (const auto& availablePresentMode : inAvailablePresentModes)
	{
		if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
		{
			return availablePresentMode;
		}
	}

	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& inCapabilities, GLFWwindow* inWindow)
{
	if (inCapabilities.currentExtent.width != UINT32_MAX)
	{
		return inCapabilities.currentExtent;
	}
	else
	{
		int width, height;
		glfwGetFramebufferSize(inWindow, &width, &height);

		VkExtent2D actualExtent =
		{
			static_cast<uint32_t>(width),
			static_cast<uint32_t>(height)
		};

		if (actualExtent.width < inCapabilities.minImageExtent.width)
		{
			actualExtent.width = inCapabilities.minImageExtent.width;
		}

		if (actualExtent.height < inCapabilities.minImageExtent.height)
		{
			actualExtent.height = inCapabilities.minImageExtent.height;
		}

		if (actualExtent.width > inCapabilities.maxImageExtent.width)
		{
			actualExtent.width = inCapabilities.maxImageExtent.width;
		}

		if (actualExtent.height > inCapabilities.maxImageExtent.height)
		{
			actualExtent.height = inCapabilities.maxImageExtent.height;
		}

		return actualExtent;
	}
}

void CreateSwapChain(const Device* inDevice, GLFWwindow* inWindow, VkSwapchainKHR& outSwapchain, std::vector<VkImage>& outSwapchainImages, VkFormat& outSwapchainImageFormat, VkExtent2D& outSwapchainExtent)
{
	SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(inDevice->GetPhysicalDevice(), inDevice->GetSurface());
	VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(swapChainSupport.formats);
	VkPresentModeKHR presentMode = ChooseSwapPresentMode(swapChainSupport.presentModes);
	VkExtent2D extent = ChooseSwapExtent(swapChainSupport.capabilities, inWindow);

	uint32_t swapchainImageCount = swapChainSupport.capabilities.minImageCount + 1;
	if (swapChainSupport.capabilities.maxImageCount > 0 && swapchainImageCount > swapChainSupport.capabilities.maxImageCount)
	{
		swapchainImageCount = swapChainSupport.capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR swapchainCreateInfo{};
	swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchainCreateInfo.surface = inDevice->GetSurface();
	swapchainCreateInfo.minImageCount = swapchainImageCount;
	swapchainCreateInfo.imageFormat = surfaceFormat.format;
	swapchainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;
	swapchainCreateInfo.imageExtent = extent;
	swapchainCreateInfo.imageArrayLayers = 1;
	swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	swapchainCreateInfo.preTransform = swapChainSupport.capabilities.currentTransform;
	swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapchainCreateInfo.presentMode = presentMode;
	swapchainCreateInfo.clipped = VK_TRUE;

	FT_VK_CALL(vkCreateSwapchainKHR(inDevice->GetDevice(), &swapchainCreateInfo, nullptr, &outSwapchain));

	vkGetSwapchainImagesKHR(inDevice->GetDevice(), outSwapchain, &swapchainImageCount, nullptr);
	outSwapchainImages.resize(swapchainImageCount);
	vkGetSwapchainImagesKHR(inDevice->GetDevice(), outSwapchain, &swapchainImageCount, outSwapchainImages.data());

	outSwapchainImageFormat = surfaceFormat.format;
	outSwapchainExtent = extent;
}

void CreateImageViews(const VkDevice inDevice, const std::vector<VkImage> inSwapchainImages, const VkFormat inSwapchainImageFormat, std::vector<VkImageView>& outSwapchainImageViews)
{
	outSwapchainImageViews.resize(inSwapchainImages.size());

	for (size_t i = 0; i < inSwapchainImages.size(); ++i)
	{
		VkImageViewCreateInfo imageViewCreateInfo{};
		imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		imageViewCreateInfo.image = inSwapchainImages[i];
		imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		imageViewCreateInfo.format = inSwapchainImageFormat;
		imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
		imageViewCreateInfo.subresourceRange.levelCount = 1;
		imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
		imageViewCreateInfo.subresourceRange.layerCount = 1;

		FT_VK_CALL(vkCreateImageView(inDevice, &imageViewCreateInfo, nullptr, &outSwapchainImageViews[i]));
	}
}

void CreateRenderPass(const VkDevice inDevice, const VkFormat inSwapchainImageFormat, VkRenderPass& outRenderPass)
{
	VkAttachmentDescription colorAttachment{};
	colorAttachment.format = inSwapchainImageFormat;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference colorAttachmentRef{};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;

	VkSubpassDependency dependency{};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	VkRenderPassCreateInfo renderPassCreateInfo{};
	renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassCreateInfo.attachmentCount = 1;
	renderPassCreateInfo.pAttachments = &colorAttachment;
	renderPassCreateInfo.subpassCount = 1;
	renderPassCreateInfo.pSubpasses = &subpass;
	renderPassCreateInfo.dependencyCount = 1;
	renderPassCreateInfo.pDependencies = &dependency;

	FT_VK_CALL(vkCreateRenderPass(inDevice, &renderPassCreateInfo, nullptr, &outRenderPass));
}

void CreateFramebuffers(const VkDevice inDevice, const VkRenderPass inRenderPass, const std::vector<VkImageView> inSwapchainImageViews, const VkExtent2D inSwapchainExtent, std::vector<VkFramebuffer>& outSwapchainFramebuffers)
{
	outSwapchainFramebuffers.resize(inSwapchainImageViews.size());

	for (size_t i = 0; i < inSwapchainImageViews.size(); ++i)
	{
		VkImageView attachments[] =
		{
			inSwapchainImageViews[i]
		};

		VkFramebufferCreateInfo framebufferCreateInfo{};
		framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferCreateInfo.renderPass = inRenderPass;
		framebufferCreateInfo.attachmentCount = 1;
		framebufferCreateInfo.pAttachments = attachments;
		framebufferCreateInfo.width = inSwapchainExtent.width;
		framebufferCreateInfo.height = inSwapchainExtent.height;
		framebufferCreateInfo.layers = 1;

		FT_VK_CALL(vkCreateFramebuffer(inDevice, &framebufferCreateInfo, nullptr, &outSwapchainFramebuffers[i]));
	}
}

void CreateSemaphores(const VkDevice inDevice, std::vector<VkSemaphore>& outImageAvailableSemaphores, std::vector<VkSemaphore>& outRenderFinishedSemaphores)
{
	outImageAvailableSemaphores.resize(MaxFramesInFlight);
	outRenderFinishedSemaphores.resize(MaxFramesInFlight);

	VkSemaphoreCreateInfo semaphoreCreateInfo{};
	semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	for (size_t i = 0; i < MaxFramesInFlight; ++i)
	{
		FT_VK_CALL(vkCreateSemaphore(inDevice, &semaphoreCreateInfo, nullptr, &outImageAvailableSemaphores[i]));
		FT_VK_CALL(vkCreateSemaphore(inDevice, &semaphoreCreateInfo, nullptr, &outRenderFinishedSemaphores[i]));
	}
}

void CreateFences(const VkDevice inDevice, std::vector<VkFence>& outInFlightFences, std::vector<VkFence>& outImagesInFlight)
{
	outInFlightFences.resize(MaxFramesInFlight);

	VkFenceCreateInfo fenceCreateInfo{};
	fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (size_t i = 0; i < MaxFramesInFlight; ++i)
	{
		FT_VK_CALL(vkCreateFence(inDevice, &fenceCreateInfo, nullptr, &outInFlightFences[i]));
	}
}

void Swapchain::FramebufferResized(GLFWwindow* inWindow, int inWidth, int inHeight)
{
	s_FramebufferResized = true;
}

bool Swapchain::s_FramebufferResized = false;

Swapchain::Swapchain(const Device* inDevice, GLFWwindow* inWindow)
	: m_Device(inDevice)
	, m_Window(inWindow)
{
	Recreate();
	CreateSemaphores(inDevice->GetDevice(), m_ImageAvailableSemaphores, m_RenderFinishedSemaphores);
	CreateFences(inDevice->GetDevice(), m_InFlightFences, m_ImagesInFlight);
}

Swapchain::~Swapchain()
{
	for (size_t i = 0; i < MaxFramesInFlight; ++i)
	{
		vkDestroySemaphore(m_Device->GetDevice(), m_RenderFinishedSemaphores[i], nullptr);
		vkDestroySemaphore(m_Device->GetDevice(), m_ImageAvailableSemaphores[i], nullptr);
		vkDestroyFence(m_Device->GetDevice(), m_InFlightFences[i], nullptr);
	}
}

void Swapchain::Recreate()
{
	CreateSwapChain(m_Device, m_Window, m_Swapchain, m_Images, m_Format, m_Extent);
	CreateImageViews(m_Device->GetDevice(), m_Images, m_Format, m_ImageViews);
	CreateRenderPass(m_Device->GetDevice(), m_Format, m_RenderPass);
	CreateFramebuffers(m_Device->GetDevice(), m_RenderPass, m_ImageViews, m_Extent, m_Framebuffers);

	m_ImagesInFlight.resize(GetImageCount(), VK_NULL_HANDLE);
}

void Swapchain::Cleanup()
{
	for (const auto& framebuffer : m_Framebuffers)
	{
		vkDestroyFramebuffer(m_Device->GetDevice(), framebuffer, nullptr);
	}

	vkDestroyRenderPass(m_Device->GetDevice(), m_RenderPass, nullptr);

	for (const auto& imageView : m_ImageViews)
	{
		vkDestroyImageView(m_Device->GetDevice(), imageView, nullptr);
	}

	vkDestroySwapchainKHR(m_Device->GetDevice(), m_Swapchain, nullptr);
}

SwapchainImageAcquireResult Swapchain::AcquireNextImage()
{
	vkWaitForFences(m_Device->GetDevice(), 1, &m_InFlightFences[m_CurrentFrame], VK_TRUE, UINT64_MAX);

	uint32_t imageIndex;
	VkResult result = vkAcquireNextImageKHR(m_Device->GetDevice(), m_Swapchain, UINT64_MAX, m_ImageAvailableSemaphores[m_CurrentFrame], VK_NULL_HANDLE, &imageIndex);

	SwapchainImageAcquireResult imageAcquireResult{};

	if (result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		imageAcquireResult.Status = SwapchainStatus::Recreate;
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
	{
		imageAcquireResult.Status = SwapchainStatus::Failed;
	}
	else
	{
		imageAcquireResult.ImageIndex = imageIndex;
		imageAcquireResult.Status = SwapchainStatus::Success;
	}

	return imageAcquireResult;
}

SwapchainStatus Swapchain::Present(const uint32_t inImageIndex, const CommandBuffer* inCommandBuffer)
{
	if (m_ImagesInFlight[inImageIndex] != VK_NULL_HANDLE)
	{
		FT_VK_CALL(vkWaitForFences(m_Device->GetDevice(), 1, &m_ImagesInFlight[inImageIndex], VK_TRUE, UINT64_MAX));
	}

	m_ImagesInFlight[inImageIndex] = m_InFlightFences[m_CurrentFrame];

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore waitSemaphores[] = { m_ImageAvailableSemaphores[m_CurrentFrame] };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;

	VkCommandBuffer commandBuffer = inCommandBuffer->GetCommandBuffer(inImageIndex);
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	VkSemaphore signalSemaphores[] = { m_RenderFinishedSemaphores[m_CurrentFrame] };
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	vkResetFences(m_Device->GetDevice(), 1, &m_InFlightFences[m_CurrentFrame]);

	FT_VK_CALL(vkQueueSubmit(m_Device->GetGraphicsQueue(), 1, &submitInfo, m_InFlightFences[m_CurrentFrame]));

	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;

	VkSwapchainKHR swapChains[] = { m_Swapchain };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;

	presentInfo.pImageIndices = &inImageIndex;

	VkResult result = vkQueuePresentKHR(m_Device->GetGraphicsQueue(), &presentInfo);

	m_CurrentFrame = (m_CurrentFrame + 1) % MaxFramesInFlight;

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || s_FramebufferResized)
	{
		s_FramebufferResized = false;
		return SwapchainStatus::Recreate;
	}

	return result == VK_SUCCESS ? SwapchainStatus::Success : SwapchainStatus::Failed;
}

FT_END_NAMESPACE
