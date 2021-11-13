#include "Shader/Shader.h"
#include "Shader/ShaderCompiler.h"
#include "ImageFile.h"
#include "FileExplorer.h"
#include "Device.h"

// TODO: Next to do ImageResource and BufferResource.

// TODO: Lightweight light fast tool (foton is small and fast :))

// TODO: Find out if we can make background for all text.
// TODO: Separate Runtime and Editor?

// TODO: When starting application sometimes new row is added at the end on current file.
// TOOD: Shader printf?

// TOOD: How resource loading with paths is going to work if we only run exe files? It's relative to project root, not the exe.

// TOOD: Use more high resolution font file for code editor.

namespace FT
{
	struct SwapChainSupportDetails
	{
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};

	// TODO: This shouldn't be here.
	std::string GetFullPath(const std::string inRelativePath) { return std::string(FT_ROOT_DIR) + inRelativePath; }

	// TOOD: Move somewhere else.
	const uint32_t WIDTH = 1280;
	const uint32_t HEIGHT = 720;

	const int MAX_FRAMES_IN_FLIGHT = 2;

	const char* VertexShaderCodeEntry = "main";
	const char* FragmentShaderCodeEntry = "main"; // TOOD: Allow user to change this one.

	// TODO: Won't need this after SPIRVReflect is introduced hopefully.
	struct UniformBufferObject
	{
		alignas(16) glm::mat4 model;
		alignas(16) glm::mat4 view;
		alignas(16) glm::mat4 proj;
	};

	// TOOD: These callbacks are used for polling actually. Can we make callbacks per action, in order to make cleaner code?
	void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
	void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);

	// TODO: Change the name to vulkancore and move it to another file.
	class Renderer
	{
	public:
		void Run()
		{
			FT_CHECK(InitializeShaderCompiler(), "Glslang not initialized properly."); // TODO: Move this check in constructor, like others.
			InitializeWindow();
			InitializeVulkan();
			InitializeImGui();
			MainLoop();
			Cleanup();
		}

	private:
		GLFWwindow* window;
		FileExplorer m_FileExplorer;

		Device* m_Device;

		VkSwapchainKHR swapChain;
		std::vector<VkImage> swapChainImages;
		VkFormat swapChainImageFormat;
		VkExtent2D swapChainExtent;
		std::vector<VkImageView> swapChainImageViews;
		std::vector<VkFramebuffer> swapChainFramebuffers;

		Shader* m_VertexShader = nullptr;
		Shader* m_FragmentShader = nullptr;

		VkRenderPass renderPass;
		VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
		VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
		VkPipeline graphicsPipeline = VK_NULL_HANDLE;

		VkCommandPool commandPool;

		VkImage textureImage;
		VkDeviceMemory textureImageMemory;
		VkImageView textureImageView;
		VkSampler textureSampler;

		std::vector<VkBuffer> uniformBuffers;
		std::vector<VkDeviceMemory> uniformBuffersMemory;

		VkDescriptorPool descriptorPool;
		std::vector<VkDescriptorSet> descriptorSets;
		std::vector<VkDescriptorSetLayoutBinding> descriptorSetLayoutBindings;

		std::vector<VkCommandBuffer> commandBuffers;

		std::vector<VkSemaphore> imageAvailableSemaphores;
		std::vector<VkSemaphore> renderFinishedSemaphores;
		std::vector<VkFence> inFlightFences;
		std::vector<VkFence> imagesInFlight;
		size_t currentFrame = 0;

		bool framebufferResized = false;

		VkDescriptorPool imguiDescPool;
		uint32_t swapchainImageCount;
		uint32_t graphicsQueueFamilyIndex;

		// TODO: Move this to config. Make foton.ini
		float codeFontSize = 1.5f;
		bool showImGui = true;
		TextEditor editor;
		ImGuiLogger logger;
		ShaderLanguage currentFragmentShaderLanguage;

		void InitializeWindow()
		{
			glfwInit();

			glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
			glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);
			glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

			window = glfwCreateWindow(WIDTH, HEIGHT, "Foton", nullptr, nullptr);
			glfwSetWindowUserPointer(window, this);

			glfwSetFramebufferSizeCallback(window, FramebufferResizeCallback);
			glfwSetKeyCallback(window, KeyCallback);
			glfwSetScrollCallback(window, ScrollCallback);

			{
				const ImageFile iconImage(GetFullPath("icon"));

				GLFWimage icon;
				icon.width = iconImage.GetWidth();
				icon.height = iconImage.GetHeight();
				icon.pixels = iconImage.GetPixels();

				glfwSetWindowIcon(window, 1, &icon);
			}
		}

		static void FramebufferResizeCallback(GLFWwindow* window, int width, int height)
		{
			Renderer* renderer = reinterpret_cast<Renderer*>(glfwGetWindowUserPointer(window));
			renderer->framebufferResized = true;
		}

		void InitializeVulkan()
		{
			m_Device = new Device(window);

			CreateSwapChain();
			CreateImageViews();
			CreateRenderPass();
			CreateShaders();
			CreateGraphicsPipeline();
			CreateFramebuffers();
			CreateCommandPool();
			CreateTextureImage();
			CreateTextureImageView();
			CreateTextureSampler();
			CreateUniformBuffers();
			CreateDescriptorPool();
			CreateDescriptorSets(descriptorSetLayoutBindings);
			InitializeCommandBuffers();
			CreateSyncObjects();
		}

		void ApplyImGuiStyle()
		{
			ImGuiStyle* style = &ImGui::GetStyle();
			ImVec4* colors = style->Colors;

			// TODO: Make a config file if someone wants to tweak these values.
			colors[ImGuiCol_Text] = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
			colors[ImGuiCol_TextDisabled] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
			colors[ImGuiCol_WindowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.f);
			colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
			colors[ImGuiCol_PopupBg] = ImVec4(0.11f, 0.11f, 0.14f, 0.92f);
			colors[ImGuiCol_Border] = ImVec4(0.50f, 0.50f, 0.50f, 0.50f);
			colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
			colors[ImGuiCol_FrameBg] = ImVec4(0.43f, 0.43f, 0.43f, 0.39f);
			colors[ImGuiCol_FrameBgHovered] = ImVec4(0.47f, 0.47f, 0.47f, 0.40f);
			colors[ImGuiCol_FrameBgActive] = ImVec4(0.41f, 0.41f, 0.41f, 0.69f);
			colors[ImGuiCol_TitleBg] = ImVec4(0.27f, 0.27f, 0.27f, 0.3f);
			colors[ImGuiCol_TitleBgActive] = ImVec4(0.32f, 0.32f, 0.32f, 0.2f);
			colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.40f, 0.40f, 0.40f, 0.20f);
			colors[ImGuiCol_MenuBarBg] = ImVec4(0.40f, 0.40f, 0.40f, 0.5f);
			colors[ImGuiCol_ScrollbarBg] = ImVec4(0.20f, 0.20f, 0.20f, 0.1f);
			colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.40f, 0.40f, 0.40f, 0.2f);
			colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.40f, 0.40f, 0.40f, 0.3f);
			colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.39f, 0.39f, 0.39f, 0.4f);
			colors[ImGuiCol_CheckMark] = ImVec4(0.90f, 0.90f, 0.90f, 0.50f);
			colors[ImGuiCol_SliderGrab] = ImVec4(1.00f, 1.00f, 1.00f, 0.30f);
			colors[ImGuiCol_SliderGrabActive] = ImVec4(0.39f, 0.39f, 0.39f, 0.60f);
			colors[ImGuiCol_Button] = ImVec4(0.35f, 0.35f, 0.35f, 0.62f);
			colors[ImGuiCol_ButtonHovered] = ImVec4(0.40f, 0.40f, 0.40f, 0.79f);
			colors[ImGuiCol_ButtonActive] = ImVec4(0.46f, 0.46f, 0.46f, 1.00f);
			colors[ImGuiCol_Header] = ImVec4(0.40f, 0.40f, 0.40f, 0.45f);
			colors[ImGuiCol_HeaderHovered] = ImVec4(0.45f, 0.45f, 0.45f, 0.80f);
			colors[ImGuiCol_HeaderActive] = ImVec4(0.53f, 0.53f, 0.53f, 0.80f);
			colors[ImGuiCol_Separator] = ImVec4(0.50f, 0.50f, 0.50f, 0.60f);
			colors[ImGuiCol_SeparatorHovered] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
			colors[ImGuiCol_SeparatorActive] = ImVec4(0.70f, 0.70f, 0.70f, 1.00f);
			colors[ImGuiCol_ResizeGrip] = ImVec4(1.00f, 1.00f, 1.00f, 0.10f);
			colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.78f, 0.78f, 0.78f, 0.60f);
			colors[ImGuiCol_ResizeGripActive] = ImVec4(0.78f, 0.78f, 0.78f, 0.90f);
			colors[ImGuiCol_Tab] = ImVec4(0.3f, 0.3f, 0.3f, 0.3f);
			colors[ImGuiCol_TabHovered] = ImVec4(0.3f, 0.3f, 0.3f, 0.3f);
			colors[ImGuiCol_TabActive] = ImVec4(0.3f, 0.3f, 0.3f, 0.2f);
			colors[ImGuiCol_TabUnfocused] = ImVec4(0.3f, 0.3f, 0.3f, 0.2f);
			colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.3f, 0.3f, 0.3f, 0.4f);
			colors[ImGuiCol_DockingPreview] = ImVec4(0.40f, 0.40f, 0.40f, 0.3f);
			colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
			colors[ImGuiCol_PlotLines] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
			colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
			colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
			colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
			colors[ImGuiCol_TableHeaderBg] = ImVec4(0.27f, 0.27f, 0.27f, 1.00f);
			colors[ImGuiCol_TableBorderStrong] = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
			colors[ImGuiCol_TableBorderLight] = ImVec4(0.26f, 0.26f, 0.26f, 1.00f);
			colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
			colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 0.07f);
			colors[ImGuiCol_TextSelectedBg] = ImVec4(0.00f, 0.00f, 1.00f, 0.35f);
			colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
			colors[ImGuiCol_NavHighlight] = ImVec4(0.45f, 0.45f, 0.45f, 0.80f);
			colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
			colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
			colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.20f, 0.20f, 0.20f, 0.35f);
		}

		void InitializeImGui()
		{
			const static uint32_t resourceCount = 512;
			VkDescriptorPoolSize descriptorPoolSIzes[] =
			{
				{ VK_DESCRIPTOR_TYPE_SAMPLER, resourceCount },
				{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, resourceCount },
				{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, resourceCount },
				{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, resourceCount },
				{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, resourceCount },
				{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, resourceCount },
				{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, resourceCount },
				{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, resourceCount },
				{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, resourceCount },
				{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, resourceCount },
				{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, resourceCount }
			};

			VkDescriptorPoolCreateInfo descriptorPoolInfo{};
			descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
			descriptorPoolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
			descriptorPoolInfo.maxSets = resourceCount;
			descriptorPoolInfo.poolSizeCount = static_cast<uint32_t>(std::size(descriptorPoolSIzes));
			descriptorPoolInfo.pPoolSizes = descriptorPoolSIzes;

			FT_VK_CALL(vkCreateDescriptorPool(m_Device->GetDevice(), &descriptorPoolInfo, nullptr, &imguiDescPool));

			IMGUI_CHECKVERSION();
			ImGui::CreateContext();

			ImGuiIO& io = ImGui::GetIO();
			io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

			ImGui_ImplGlfw_InitForVulkan(window, true);

			ImGui_ImplVulkan_InitInfo vulkanImplementationInitInfo{};
			vulkanImplementationInitInfo.Instance = m_Device->GetInstance();
			vulkanImplementationInitInfo.PhysicalDevice = m_Device->GetPhysicalDevice();
			vulkanImplementationInitInfo.Device = m_Device->GetDevice();
			vulkanImplementationInitInfo.QueueFamily = graphicsQueueFamilyIndex;
			vulkanImplementationInitInfo.Queue = m_Device->GetGraphicsQueue();
			vulkanImplementationInitInfo.PipelineCache = VK_NULL_HANDLE;
			vulkanImplementationInitInfo.DescriptorPool = imguiDescPool;
			vulkanImplementationInitInfo.Allocator = nullptr;
			vulkanImplementationInitInfo.MinImageCount = swapchainImageCount;
			vulkanImplementationInitInfo.ImageCount = swapchainImageCount;
			vulkanImplementationInitInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

			ImGui_ImplVulkan_Init(&vulkanImplementationInitInfo, renderPass);

			VkCommandBuffer commandBuffer = BeginSingleTimeCommands();
			ImGui_ImplVulkan_CreateFontsTexture(commandBuffer);
			EndSingleTimeCommands(commandBuffer);

			ImGui_ImplVulkan_DestroyFontUploadObjects();

			ApplyImGuiStyle();
		}

		void MainLoop()
		{
			glfwShowWindow(window);

			while (!glfwWindowShouldClose(window))
			{
				glfwPollEvents();

				if (showImGui)
				{
					ImguiNewFrame();
				}

				DrawFrame();
			}

			vkDeviceWaitIdle(m_Device->GetDevice());
		}

		void CleanupSwapChain()
		{
			for (const auto& framebuffer : swapChainFramebuffers)
			{
				vkDestroyFramebuffer(m_Device->GetDevice(), framebuffer, nullptr);
			}

			vkFreeCommandBuffers(m_Device->GetDevice(), commandPool, static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());

			vkDestroyPipeline(m_Device->GetDevice(), graphicsPipeline, nullptr);
			vkDestroyPipelineLayout(m_Device->GetDevice(), pipelineLayout, nullptr);
			vkDestroyRenderPass(m_Device->GetDevice(), renderPass, nullptr);

			for (const auto& imageView : swapChainImageViews)
			{
				vkDestroyImageView(m_Device->GetDevice(), imageView, nullptr);
			}

			vkDestroySwapchainKHR(m_Device->GetDevice(), swapChain, nullptr);

			for (size_t i = 0; i < swapChainImages.size(); ++i)
			{
				vkDestroyBuffer(m_Device->GetDevice(), uniformBuffers[i], nullptr);
				vkFreeMemory(m_Device->GetDevice(), uniformBuffersMemory[i], nullptr);
			}

			vkDestroyDescriptorPool(m_Device->GetDevice(), descriptorPool, nullptr);
		}

		void Cleanup()
		{
			delete(m_FragmentShader);
			delete(m_VertexShader);

			CleanupSwapChain();

			ImGui_ImplVulkan_Shutdown();
			ImGui_ImplGlfw_Shutdown();
			ImGui::DestroyContext();
			vkDestroyDescriptorPool(m_Device->GetDevice(), imguiDescPool, nullptr);

			vkDestroySampler(m_Device->GetDevice(), textureSampler, nullptr);
			vkDestroyImageView(m_Device->GetDevice(), textureImageView, nullptr);

			vkDestroyImage(m_Device->GetDevice(), textureImage, nullptr);
			vkFreeMemory(m_Device->GetDevice(), textureImageMemory, nullptr);

			vkDestroyDescriptorSetLayout(m_Device->GetDevice(), descriptorSetLayout, nullptr);

			for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
			{
				vkDestroySemaphore(m_Device->GetDevice(), renderFinishedSemaphores[i], nullptr);
				vkDestroySemaphore(m_Device->GetDevice(), imageAvailableSemaphores[i], nullptr);
				vkDestroyFence(m_Device->GetDevice(), inFlightFences[i], nullptr);
			}

			vkDestroyCommandPool(m_Device->GetDevice(), commandPool, nullptr);

			glfwDestroyWindow(window);

			glfwTerminate();

			FinalizeShaderCompiler();
		}

		void RecreateSwapChain()
		{
			int width = 0, height = 0;
			glfwGetFramebufferSize(window, &width, &height);
			while (width == 0 || height == 0)
			{
				glfwGetFramebufferSize(window, &width, &height);
				glfwWaitEvents();
			}

			vkDeviceWaitIdle(m_Device->GetDevice());

			CleanupSwapChain();

			CreateSwapChain();
			CreateImageViews();
			CreateRenderPass();
			CreateGraphicsPipeline();
			CreateFramebuffers();
			CreateUniformBuffers();
			CreateDescriptorPool();
			CreateDescriptorSets(descriptorSetLayoutBindings);
			InitializeCommandBuffers();

			imagesInFlight.resize(swapChainImages.size(), VK_NULL_HANDLE);

			ImGui_ImplVulkan_SetMinImageCount(swapchainImageCount);
		}

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

		void CreateSwapChain()
		{
			SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(m_Device->GetPhysicalDevice(), m_Device->GetSurface());
			VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(swapChainSupport.formats);
			VkPresentModeKHR presentMode = ChooseSwapPresentMode(swapChainSupport.presentModes);
			VkExtent2D extent = ChooseSwapExtent(swapChainSupport.capabilities);

			swapchainImageCount = swapChainSupport.capabilities.minImageCount + 1;
			if (swapChainSupport.capabilities.maxImageCount > 0 && swapchainImageCount > swapChainSupport.capabilities.maxImageCount)
			{
				swapchainImageCount = swapChainSupport.capabilities.maxImageCount;
			}

			VkSwapchainCreateInfoKHR createInfo{};
			createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
			createInfo.surface = m_Device->GetSurface();
			createInfo.minImageCount = swapchainImageCount;
			createInfo.imageFormat = surfaceFormat.format;
			createInfo.imageColorSpace = surfaceFormat.colorSpace;
			createInfo.imageExtent = extent;
			createInfo.imageArrayLayers = 1;
			createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
			createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
			createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
			createInfo.presentMode = presentMode;
			createInfo.clipped = VK_TRUE;

			FT_VK_CALL(vkCreateSwapchainKHR(m_Device->GetDevice(), &createInfo, nullptr, &swapChain));

			vkGetSwapchainImagesKHR(m_Device->GetDevice(), swapChain, &swapchainImageCount, nullptr);
			swapChainImages.resize(swapchainImageCount);
			vkGetSwapchainImagesKHR(m_Device->GetDevice(), swapChain, &swapchainImageCount, swapChainImages.data());

			swapChainImageFormat = surfaceFormat.format;
			swapChainExtent = extent;
		}

		void CreateImageViews()
		{
			swapChainImageViews.resize(swapChainImages.size());

			for (size_t i = 0; i < swapChainImages.size(); i++)
			{
				swapChainImageViews[i] = CreateImageView(swapChainImages[i], swapChainImageFormat);
			}
		}

		void CreateRenderPass()
		{
			VkAttachmentDescription colorAttachment{};
			colorAttachment.format = swapChainImageFormat;
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

			VkRenderPassCreateInfo renderPassInfo{};
			renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
			renderPassInfo.attachmentCount = 1;
			renderPassInfo.pAttachments = &colorAttachment;
			renderPassInfo.subpassCount = 1;
			renderPassInfo.pSubpasses = &subpass;
			renderPassInfo.dependencyCount = 1;
			renderPassInfo.pDependencies = &dependency;

			FT_VK_CALL(vkCreateRenderPass(m_Device->GetDevice(), &renderPassInfo, nullptr, &renderPass));
		}

		void CreateDescriptorSetLayout(const std::vector<VkDescriptorSetLayoutBinding>& bindings)
		{
			if (descriptorSetLayout != VK_NULL_HANDLE)
			{
				vkDestroyDescriptorSetLayout(m_Device->GetDevice(), descriptorSetLayout, nullptr);
			}

			VkDescriptorSetLayoutCreateInfo layoutInfo{};
			layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
			layoutInfo.pBindings = bindings.data();

			FT_VK_CALL(vkCreateDescriptorSetLayout(m_Device->GetDevice(), &layoutInfo, nullptr, &descriptorSetLayout));
		}

		void CreateShaders()
		{
			m_VertexShader = new Shader(m_Device->GetDevice(), GetFullPath("Shaders/Internal/FullScreen.vert.glsl"), ShaderStage::Vertex, VertexShaderCodeEntry);
			m_FragmentShader = new Shader(m_Device->GetDevice(), GetFullPath("Shaders/Internal/Default.frag.glsl"), ShaderStage::Fragment, FragmentShaderCodeEntry);

			editor.SetText(m_FragmentShader->GetSourceCode());

			CreateDescriptorSetLayout(m_FragmentShader->GetBindings());
		}

		void CreateGraphicsPipeline()
		{
			VkPipelineShaderStageCreateInfo shaderStages[] = { m_VertexShader->GetPipelineStageInfo(), m_FragmentShader->GetPipelineStageInfo() };

			VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
			vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

			VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
			inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
			inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
			inputAssembly.primitiveRestartEnable = VK_FALSE;

			VkViewport viewport{};
			viewport.x = 0.0f;
			viewport.y = 0.0f;
			viewport.width = static_cast<float>(swapChainExtent.width);
			viewport.height = static_cast<float>(swapChainExtent.height);
			viewport.minDepth = 0.0f;
			viewport.maxDepth = 1.0f;

			VkRect2D scissor{};
			scissor.offset = { 0, 0 };
			scissor.extent = swapChainExtent;

			VkPipelineViewportStateCreateInfo viewportState{};
			viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
			viewportState.viewportCount = 1;
			viewportState.pViewports = &viewport;
			viewportState.scissorCount = 1;
			viewportState.pScissors = &scissor;

			VkPipelineRasterizationStateCreateInfo rasterizer{};
			rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
			rasterizer.depthClampEnable = VK_FALSE;
			rasterizer.rasterizerDiscardEnable = VK_FALSE;
			rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
			rasterizer.lineWidth = 1.0f;
			rasterizer.cullMode = VK_CULL_MODE_FRONT_BIT;
			rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
			rasterizer.depthBiasEnable = VK_FALSE;

			VkPipelineMultisampleStateCreateInfo multisampling{};
			multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
			multisampling.sampleShadingEnable = VK_FALSE;
			multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

			VkPipelineColorBlendAttachmentState colorBlendAttachment{};
			colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
			colorBlendAttachment.blendEnable = VK_FALSE;

			VkPipelineColorBlendStateCreateInfo colorBlending{};
			colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
			colorBlending.logicOpEnable = VK_FALSE;
			colorBlending.logicOp = VK_LOGIC_OP_COPY;
			colorBlending.attachmentCount = 1;
			colorBlending.pAttachments = &colorBlendAttachment;
			colorBlending.blendConstants[0] = 0.0f;
			colorBlending.blendConstants[1] = 0.0f;
			colorBlending.blendConstants[2] = 0.0f;
			colorBlending.blendConstants[3] = 0.0f;

			VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
			pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			pipelineLayoutInfo.setLayoutCount = 1;
			pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;

			FT_VK_CALL(vkCreatePipelineLayout(m_Device->GetDevice(), &pipelineLayoutInfo, nullptr, &pipelineLayout));

			VkGraphicsPipelineCreateInfo pipelineInfo{};
			pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
			pipelineInfo.stageCount = 2;
			pipelineInfo.pStages = shaderStages;
			pipelineInfo.pVertexInputState = &vertexInputInfo;
			pipelineInfo.pInputAssemblyState = &inputAssembly;
			pipelineInfo.pViewportState = &viewportState;
			pipelineInfo.pRasterizationState = &rasterizer;
			pipelineInfo.pMultisampleState = &multisampling;
			pipelineInfo.pColorBlendState = &colorBlending;
			pipelineInfo.layout = pipelineLayout;
			pipelineInfo.renderPass = renderPass;
			pipelineInfo.subpass = 0;
			pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

			FT_VK_CALL(vkCreateGraphicsPipelines(m_Device->GetDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline));
		}

		void CreateFramebuffers()
		{
			swapChainFramebuffers.resize(swapChainImageViews.size());

			for (size_t i = 0; i < swapChainImageViews.size(); i++) {
				VkImageView attachments[] = {
					swapChainImageViews[i]
				};

				VkFramebufferCreateInfo framebufferInfo{};
				framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
				framebufferInfo.renderPass = renderPass;
				framebufferInfo.attachmentCount = 1;
				framebufferInfo.pAttachments = attachments;
				framebufferInfo.width = swapChainExtent.width;
				framebufferInfo.height = swapChainExtent.height;
				framebufferInfo.layers = 1;

				FT_VK_CALL(vkCreateFramebuffer(m_Device->GetDevice(), &framebufferInfo, nullptr, &swapChainFramebuffers[i]));
			}
		}

		void CreateCommandPool()
		{
			VkCommandPoolCreateInfo poolInfo{};
			poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
			poolInfo.queueFamilyIndex = graphicsQueueFamilyIndex;
			poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

			FT_VK_CALL(vkCreateCommandPool(m_Device->GetDevice(), &poolInfo, nullptr, &commandPool));
		}

		void CreateTextureImage()
		{
			const ImageFile textureData(GetFullPath("icon"));
			const int texWidth = textureData.GetWidth();
			const int texHeight = textureData.GetHeight();

			VkDeviceSize imageSize = texWidth * texHeight * 4;

			VkBuffer stagingBuffer;
			VkDeviceMemory stagingBufferMemory;
			CreateBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

			void* data;
			vkMapMemory(m_Device->GetDevice(), stagingBufferMemory, 0, imageSize, 0, &data);
			memcpy(data, textureData.GetPixels(), static_cast<size_t>(imageSize));
			vkUnmapMemory(m_Device->GetDevice(), stagingBufferMemory);

			CreateImage(texWidth, texHeight, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureImage, textureImageMemory);

			TransitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
			CopyBufferToImage(stagingBuffer, textureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
			TransitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

			vkDestroyBuffer(m_Device->GetDevice(), stagingBuffer, nullptr);
			vkFreeMemory(m_Device->GetDevice(), stagingBufferMemory, nullptr);
		}

		void CreateTextureImageView()
		{
			textureImageView = CreateImageView(textureImage, VK_FORMAT_R8G8B8A8_UNORM);
		}

		void CreateTextureSampler()
		{
			VkPhysicalDeviceProperties properties{};
			vkGetPhysicalDeviceProperties(m_Device->GetPhysicalDevice(), &properties);

			VkSamplerCreateInfo samplerInfo{};
			samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
			samplerInfo.magFilter = VK_FILTER_LINEAR;
			samplerInfo.minFilter = VK_FILTER_LINEAR;
			samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			samplerInfo.anisotropyEnable = VK_TRUE;
			samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
			samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
			samplerInfo.unnormalizedCoordinates = VK_FALSE;
			samplerInfo.compareEnable = VK_FALSE;
			samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
			samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

			FT_VK_CALL(vkCreateSampler(m_Device->GetDevice(), &samplerInfo, nullptr, &textureSampler));
		}

		VkImageView CreateImageView(VkImage image, VkFormat format)
		{
			VkImageViewCreateInfo viewInfo{};
			viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			viewInfo.image = image;
			viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			viewInfo.format = format;
			viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			viewInfo.subresourceRange.baseMipLevel = 0;
			viewInfo.subresourceRange.levelCount = 1;
			viewInfo.subresourceRange.baseArrayLayer = 0;
			viewInfo.subresourceRange.layerCount = 1;

			VkImageView imageView;
			FT_VK_CALL(vkCreateImageView(m_Device->GetDevice(), &viewInfo, nullptr, &imageView));

			return imageView;
		}

		void CreateImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory)
		{
			VkImageCreateInfo imageInfo{};
			imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
			imageInfo.imageType = VK_IMAGE_TYPE_2D;
			imageInfo.extent.width = width;
			imageInfo.extent.height = height;
			imageInfo.extent.depth = 1;
			imageInfo.mipLevels = 1;
			imageInfo.arrayLayers = 1;
			imageInfo.format = format;
			imageInfo.tiling = tiling;
			imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			imageInfo.usage = usage;
			imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
			imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

			FT_VK_CALL(vkCreateImage(m_Device->GetDevice(), &imageInfo, nullptr, &image));

			VkMemoryRequirements memRequirements;
			vkGetImageMemoryRequirements(m_Device->GetDevice(), image, &memRequirements);

			VkMemoryAllocateInfo allocInfo{};
			allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			allocInfo.allocationSize = memRequirements.size;
			allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, properties);

			FT_VK_CALL(vkAllocateMemory(m_Device->GetDevice(), &allocInfo, nullptr, &imageMemory));

			vkBindImageMemory(m_Device->GetDevice(), image, imageMemory, 0);
		}

		void TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout)
		{
			VkCommandBuffer commandBuffer = BeginSingleTimeCommands();

			VkImageMemoryBarrier barrier{};
			barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			barrier.oldLayout = oldLayout;
			barrier.newLayout = newLayout;
			barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.image = image;
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			barrier.subresourceRange.baseMipLevel = 0;
			barrier.subresourceRange.levelCount = 1;
			barrier.subresourceRange.baseArrayLayer = 0;
			barrier.subresourceRange.layerCount = 1;

			VkPipelineStageFlags sourceStage;
			VkPipelineStageFlags destinationStage;

			if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
			{
				barrier.srcAccessMask = 0;
				barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

				sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
				destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			}
			else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
			{
				barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

				sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
				destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			}
			else
			{
				FT_FAIL("Unsupported layout transition.");
			}

			vkCmdPipelineBarrier(
				commandBuffer,
				sourceStage, destinationStage,
				0,
				0, nullptr,
				0, nullptr,
				1, &barrier
			);

			EndSingleTimeCommands(commandBuffer);
		}

		void CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
		{
			VkCommandBuffer commandBuffer = BeginSingleTimeCommands();

			VkBufferImageCopy region{};
			region.bufferOffset = 0;
			region.bufferRowLength = 0;
			region.bufferImageHeight = 0;
			region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			region.imageSubresource.mipLevel = 0;
			region.imageSubresource.baseArrayLayer = 0;
			region.imageSubresource.layerCount = 1;
			region.imageOffset = { 0, 0, 0 };
			region.imageExtent = {
				width,
				height,
				1
			};

			vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

			EndSingleTimeCommands(commandBuffer);
		}

		void CreateUniformBuffers()
		{
			VkDeviceSize bufferSize = sizeof(UniformBufferObject);

			uniformBuffers.resize(swapChainImages.size());
			uniformBuffersMemory.resize(swapChainImages.size());

			for (size_t i = 0; i < swapChainImages.size(); ++i)
			{
				CreateBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformBuffers[i], uniformBuffersMemory[i]);
			}
		}

		void CreateDescriptorPool()
		{
			std::array<VkDescriptorPoolSize, 2> poolSizes{};
			poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			poolSizes[0].descriptorCount = static_cast<uint32_t>(swapChainImages.size());
			poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			poolSizes[1].descriptorCount = static_cast<uint32_t>(swapChainImages.size());

			VkDescriptorPoolCreateInfo poolInfo{};
			poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
			poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
			poolInfo.pPoolSizes = poolSizes.data();
			poolInfo.maxSets = static_cast<uint32_t>(swapChainImages.size());

			FT_VK_CALL(vkCreateDescriptorPool(m_Device->GetDevice(), &poolInfo, nullptr, &descriptorPool));
		}

		void CreateDescriptorSets(const std::vector<VkDescriptorSetLayoutBinding>& bindings)
		{
			std::vector<VkDescriptorSetLayout> layouts(swapChainImages.size(), descriptorSetLayout);
			VkDescriptorSetAllocateInfo allocInfo{};
			allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			allocInfo.descriptorPool = descriptorPool;
			allocInfo.descriptorSetCount = static_cast<uint32_t>(swapChainImages.size());
			allocInfo.pSetLayouts = layouts.data();

			descriptorSets.resize(swapChainImages.size());
			FT_VK_CALL(vkAllocateDescriptorSets(m_Device->GetDevice(), &allocInfo, descriptorSets.data()));

			// TODO: Recreate descriptor sets each time descriptor layout gets recreated.
			for (size_t i = 0; i < swapChainImages.size(); ++i)
			{
				std::vector<VkWriteDescriptorSet> descriptorWrites(bindings.size());

				// TODO: Temporary, until imgui support for shader properties gets implemented.
				VkDescriptorBufferInfo bufferInfo{};
				bufferInfo.buffer = uniformBuffers[i];
				bufferInfo.offset = 0;
				bufferInfo.range = sizeof(UniformBufferObject);

				VkDescriptorImageInfo imageInfo{};
				imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				imageInfo.imageView = textureImageView;
				imageInfo.sampler = textureSampler;

				for (uint32_t j = 0; j < bindings.size(); ++j)
				{
					descriptorWrites[j].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
					descriptorWrites[j].dstSet = descriptorSets[i];
					descriptorWrites[j].dstBinding = bindings[j].binding;
					descriptorWrites[j].dstArrayElement = 0;
					descriptorWrites[j].descriptorType = bindings[j].descriptorType;
					descriptorWrites[j].descriptorCount = 1;
					descriptorWrites[j].pBufferInfo = &bufferInfo;
					descriptorWrites[j].pImageInfo = &imageInfo;
				}

				vkUpdateDescriptorSets(m_Device->GetDevice(), static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
			}
		}

		void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory)
		{
			VkBufferCreateInfo bufferInfo{};
			bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			bufferInfo.size = size;
			bufferInfo.usage = usage;
			bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

			FT_VK_CALL(vkCreateBuffer(m_Device->GetDevice(), &bufferInfo, nullptr, &buffer));

			VkMemoryRequirements memRequirements;
			vkGetBufferMemoryRequirements(m_Device->GetDevice(), buffer, &memRequirements);

			VkMemoryAllocateInfo allocInfo{};
			allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			allocInfo.allocationSize = memRequirements.size;
			allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, properties);

			FT_VK_CALL(vkAllocateMemory(m_Device->GetDevice(), &allocInfo, nullptr, &bufferMemory));

			vkBindBufferMemory(m_Device->GetDevice(), buffer, bufferMemory, 0);
		}

		VkCommandBuffer BeginSingleTimeCommands()
		{
			VkCommandBufferAllocateInfo allocInfo{};
			allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			allocInfo.commandPool = commandPool;
			allocInfo.commandBufferCount = 1;

			VkCommandBuffer commandBuffer;
			vkAllocateCommandBuffers(m_Device->GetDevice(), &allocInfo, &commandBuffer);

			VkCommandBufferBeginInfo beginInfo{};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

			vkBeginCommandBuffer(commandBuffer, &beginInfo);

			return commandBuffer;
		}

		void EndSingleTimeCommands(VkCommandBuffer commandBuffer)
		{
			vkEndCommandBuffer(commandBuffer);

			VkSubmitInfo submitInfo{};
			submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submitInfo.commandBufferCount = 1;
			submitInfo.pCommandBuffers = &commandBuffer;

			vkQueueSubmit(m_Device->GetGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE);
			vkQueueWaitIdle(m_Device->GetGraphicsQueue());

			vkFreeCommandBuffers(m_Device->GetDevice(), commandPool, 1, &commandBuffer);
		}

		void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
		{
			VkCommandBuffer commandBuffer = BeginSingleTimeCommands();

			VkBufferCopy copyRegion{};
			copyRegion.size = size;
			vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

			EndSingleTimeCommands(commandBuffer);
		}

		uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
		{
			VkPhysicalDeviceMemoryProperties memProperties;
			vkGetPhysicalDeviceMemoryProperties(m_Device->GetPhysicalDevice(), &memProperties);

			for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i)
			{
				if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
				{
					return i;
				}
			}

			FT_FAIL("Failed to find suitable memory type.");
		}

		void InitializeCommandBuffers()
		{
			commandBuffers.resize(swapChainFramebuffers.size());

			VkCommandBufferAllocateInfo allocInfo{};
			allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			allocInfo.commandPool = commandPool;
			allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			allocInfo.commandBufferCount = (uint32_t)commandBuffers.size();

			FT_VK_CALL(vkAllocateCommandBuffers(m_Device->GetDevice(), &allocInfo, commandBuffers.data()));
		}

		void CreateSyncObjects()
		{
			imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
			renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
			inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
			imagesInFlight.resize(swapChainImages.size(), VK_NULL_HANDLE);

			VkSemaphoreCreateInfo semaphoreInfo{};
			semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

			VkFenceCreateInfo fenceInfo{};
			fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
			fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

			for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
			{
				FT_VK_CALL(vkCreateSemaphore(m_Device->GetDevice(), &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]));
				FT_VK_CALL(vkCreateSemaphore(m_Device->GetDevice(), &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]));
				FT_VK_CALL(vkCreateFence(m_Device->GetDevice(), &fenceInfo, nullptr, &inFlightFences[i]));
			}
		}

		void UpdateUniformBuffer(uint32_t currentImage)
		{
			static auto startTime = std::chrono::high_resolution_clock::now();

			auto currentTime = std::chrono::high_resolution_clock::now();
			float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

			UniformBufferObject ubo{};
			ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
			ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
			ubo.proj = glm::perspective(glm::radians(45.0f), swapChainExtent.width / static_cast<float>(swapChainExtent.height), 0.1f, 10.0f);
			ubo.proj[1][1] *= -1;

			void* data;
			vkMapMemory(m_Device->GetDevice(), uniformBuffersMemory[currentImage], 0, sizeof(ubo), 0, &data);
			memcpy(data, &ubo, sizeof(ubo));
			vkUnmapMemory(m_Device->GetDevice(), uniformBuffersMemory[currentImage]);
		}

		void ImguiMenuBar()
		{
			if (ImGui::BeginMenuBar())
			{
				if (ImGui::BeginMenu("File"))
				{
					if (ImGui::MenuItem("New", "Ctrl-N"))
					{
						std::string shaderFilePath;
						if (m_FileExplorer.SaveShaderDialog(shaderFilePath))
						{
							// TODO: Make new empty/default file and keep it open.
						}
					}

					if (ImGui::MenuItem("Open", "Ctrl-O"))
					{
						std::string shaderFilePath;
						if (m_FileExplorer.OpenShaderDialog(shaderFilePath))
						{
							LoadShader(shaderFilePath);
							RecompileFragmentShader();
						}
					}

					if (ImGui::MenuItem("Save", "Ctrl-S"))
					{
						RecompileFragmentShader();
					}

					if (ImGui::MenuItem("Save As", "Ctrl-Shift-S"))
					{
						std::string shaderFilePath;
						if (m_FileExplorer.SaveShaderDialog(shaderFilePath))
						{
							const std::string& textToSave = editor.GetText();
							// TODO: Make new file with current contents and keep it open.
						}
					}

					if (ImGui::MenuItem("Quit", "Alt-F4"))
					{
						glfwSetWindowShouldClose(window, GLFW_TRUE);
					}

					ImGui::EndMenu();
				}

				if (ImGui::BeginMenu("Edit"))
				{
					bool readOnly = editor.IsReadOnly();
					if (ImGui::MenuItem("Read-only mode", nullptr, &readOnly))
					{
						editor.SetReadOnly(readOnly);
					}

					ImGui::Separator();

					if (ImGui::MenuItem("Undo", "Alt-Backspace", nullptr, !readOnly && editor.CanUndo()))
					{
						editor.Undo();
					}

					if (ImGui::MenuItem("Redo", "Ctrl-Y", nullptr, !readOnly && editor.CanRedo()))
					{
						editor.Redo();
					}

					ImGui::Separator();

					if (ImGui::MenuItem("Copy", "Ctrl-C", nullptr, editor.HasSelection()))
					{
						editor.Copy();
					}

					if (ImGui::MenuItem("Cut", "Ctrl-X", nullptr, !readOnly && editor.HasSelection()))
					{
						editor.Cut();
					}

					if (ImGui::MenuItem("Delete", "Del", nullptr, !readOnly && editor.HasSelection()))
					{
						editor.Delete();
					}

					if (ImGui::MenuItem("Paste", "Ctrl-V", nullptr, !readOnly && ImGui::GetClipboardText() != nullptr))
					{
						editor.Paste();
					}

					ImGui::Separator();

					if (ImGui::MenuItem("Compile", "Ctrl-R", nullptr, !readOnly))
					{
						RecompileFragmentShader();
					}

					ImGui::Separator();

					if (ImGui::MenuItem("Select all", nullptr, nullptr))
					{
						editor.SetSelection(TextEditor::Coordinates(), TextEditor::Coordinates(editor.GetTotalLines(), 0));
					}

					ImGui::EndMenu();
				}

				if (ImGui::BeginMenu("View"))
				{
					if (ImGui::MenuItem("Show UI", "Ctrl-F"))
					{
						ToggleImGui();
					}

					ImGui::EndMenu();
				}

				ImGui::EndMenuBar();
			}
		}

		void ImguiDockSpace()
		{
			static ImGuiDockNodeFlags dockspaceFlags = ImGuiDockNodeFlags_PassthruCentralNode;

			ImGuiWindowFlags windowFlags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
			const ImGuiViewport* viewport = ImGui::GetMainViewport();
			ImGui::SetNextWindowPos(viewport->WorkPos);
			ImGui::SetNextWindowSize(viewport->WorkSize);
			ImGui::SetNextWindowViewport(viewport->ID);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
			windowFlags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
			windowFlags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

			if (dockspaceFlags & ImGuiDockNodeFlags_PassthruCentralNode)
			{
				windowFlags |= ImGuiWindowFlags_NoBackground;
			}

			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

			static bool open = true;
			ImGui::Begin("DockSpace", &open, windowFlags);
			ImGui::PopStyleVar();

			ImGui::PopStyleVar(2);

			ImGuiIO& io = ImGui::GetIO();
			{
				ImGuiID dockspaceId = ImGui::GetID("MyDockSpace");
				ImGui::DockSpace(dockspaceId, ImVec2(0.0f, 0.0f), dockspaceFlags);
			}

			ImguiMenuBar();

			ImGui::End();
		}

		void ImguiNewFrame()
		{
			ImGui_ImplVulkan_NewFrame();
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();

			ImguiDockSpace();

			ImGui::ShowDemoWindow();

			// TODO: Refactor a bit.
			char buf[128];
			sprintf_s(buf, "%s###ShaderTitle", m_FragmentShader->GetName().c_str());

			ImGui::Begin(buf, nullptr, ImGuiWindowFlags_HorizontalScrollbar);
			ImGui::SetWindowSize(ImVec2(WIDTH, HEIGHT), ImGuiCond_FirstUseEver); // TODO: Change this!!!
			ImGui::SetNextWindowBgAlpha(0.f);
			ImGui::SetWindowFontScale(codeFontSize);
			editor.SetShowWhitespaces(false); // TODO: Settings.
			editor.Render("TextEditor");
			ImGui::End();

			logger.Draw("Log");

			ImGui::Render();
		}

	public: // TOOD: HACK.
		void UpdateCodeFontSize(float offset)
		{
			const static float MinCodeFontSize = 1.f;
			const static float MaxCodeFontSize = 3.f;
			const static float CodeFontSizeMul = 0.1f;

			codeFontSize += offset * CodeFontSizeMul; // TODO: Multiplier

			// TODO: Clamp.
			if (codeFontSize < MinCodeFontSize)
			{
				codeFontSize = MinCodeFontSize;
			}

			if (codeFontSize > MaxCodeFontSize)
			{
				codeFontSize = MaxCodeFontSize;
			}
		}

		void ToggleImGui()
		{
			// TODO: Imgui Demo Window -> Style -> Rendering -> Global alpha. You can use this to fade toggle.
			showImGui = !showImGui;
		}

		void RecompileFragmentShader()
		{
			ClearErrorMarkers();

			const std::string fragmentShaderSourceCode = editor.GetText();

			{
				vkQueueWaitIdle(m_Device->GetGraphicsQueue());

				m_FragmentShader->Recompile(fragmentShaderSourceCode);

				vkDestroyPipeline(m_Device->GetDevice(), graphicsPipeline, nullptr);
				vkDestroyPipelineLayout(m_Device->GetDevice(), pipelineLayout, nullptr);
			}

			CreateDescriptorSetLayout(m_FragmentShader->GetBindings());
			CreateGraphicsPipeline();
		}
	private: // HACK:

	// TODO: Refactor. No need for multiple methods with double code.
		void LoadShader(const std::string& inFilePath)
		{
			vkQueueWaitIdle(m_Device->GetGraphicsQueue()); // TODO: This wait wait idle will be called twice (second one in RecompileFragmentShader). Make this code path more clear.

			delete(m_FragmentShader);
			m_FragmentShader = new Shader(m_Device->GetDevice(), inFilePath, ShaderStage::Fragment, FragmentShaderCodeEntry);

			editor.SetText(m_FragmentShader->GetSourceCode());
		}

		void DrawFrame()
		{
			vkWaitForFences(m_Device->GetDevice(), 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

			uint32_t imageIndex;
			VkResult result = vkAcquireNextImageKHR(m_Device->GetDevice(), swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

			if (result == VK_ERROR_OUT_OF_DATE_KHR)
			{
				RecreateSwapChain();
				return;
			}
			else
			{
				FT_CHECK(result == VK_SUCCESS || result == VK_SUBOPTIMAL_KHR, "Failed to acquire swap chain image.");
			}

			UpdateUniformBuffer(imageIndex);

			FillCommandBuffers(imageIndex);

			if (imagesInFlight[imageIndex] != VK_NULL_HANDLE)
			{
				FT_VK_CALL(vkWaitForFences(m_Device->GetDevice(), 1, &imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX));
			}

			imagesInFlight[imageIndex] = inFlightFences[currentFrame];

			VkSubmitInfo submitInfo{};
			submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

			VkSemaphore waitSemaphores[] = { imageAvailableSemaphores[currentFrame] };
			VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
			submitInfo.waitSemaphoreCount = 1;
			submitInfo.pWaitSemaphores = waitSemaphores;
			submitInfo.pWaitDstStageMask = waitStages;

			submitInfo.commandBufferCount = 1;
			submitInfo.pCommandBuffers = &commandBuffers[imageIndex];

			VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[currentFrame] };
			submitInfo.signalSemaphoreCount = 1;
			submitInfo.pSignalSemaphores = signalSemaphores;

			vkResetFences(m_Device->GetDevice(), 1, &inFlightFences[currentFrame]);

			FT_VK_CALL(vkQueueSubmit(m_Device->GetGraphicsQueue(), 1, &submitInfo, inFlightFences[currentFrame]));

			VkPresentInfoKHR presentInfo{};
			presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

			presentInfo.waitSemaphoreCount = 1;
			presentInfo.pWaitSemaphores = signalSemaphores;

			VkSwapchainKHR swapChains[] = { swapChain };
			presentInfo.swapchainCount = 1;
			presentInfo.pSwapchains = swapChains;

			presentInfo.pImageIndices = &imageIndex;

			result = vkQueuePresentKHR(m_Device->GetGraphicsQueue(), &presentInfo);

			if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized)
			{
				framebufferResized = false;
				RecreateSwapChain();
			}
			else
			{
				FT_CHECK(result == VK_SUCCESS, "Failed to present swap chain image.");
			}

			currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
		}

		VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
		{
			for (const auto& availableFormat : availableFormats)
			{
				if (availableFormat.format == VK_FORMAT_R8G8B8A8_UNORM)
				{
					return availableFormat;
				}
			}

			return availableFormats[0];
		}

		VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
		{
			for (const auto& availablePresentMode : availablePresentModes)
			{
				if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
				{
					return availablePresentMode;
				}
			}

			return VK_PRESENT_MODE_FIFO_KHR;
		}

		VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
		{
			if (capabilities.currentExtent.width != UINT32_MAX)
			{
				return capabilities.currentExtent;
			}
			else
			{
				int width, height;
				glfwGetFramebufferSize(window, &width, &height);

				VkExtent2D actualExtent =
				{
					static_cast<uint32_t>(width),
					static_cast<uint32_t>(height)
				};

				if (actualExtent.width < capabilities.minImageExtent.width)
				{
					actualExtent.width = capabilities.minImageExtent.width;
				}

				if (actualExtent.height < capabilities.minImageExtent.height)
				{
					actualExtent.height = capabilities.minImageExtent.height;
				}

				if (actualExtent.width > capabilities.maxImageExtent.width)
				{
					actualExtent.width = capabilities.maxImageExtent.width;
				}

				if (actualExtent.height > capabilities.maxImageExtent.height)
				{
					actualExtent.height = capabilities.maxImageExtent.height;
				}

				return actualExtent;
			}
		}

		// TODO: Don't use auto.
		void FillCommandBuffers(uint32_t i)
		{
			VkCommandBufferBeginInfo beginInfo{};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

			FT_VK_CALL(vkBeginCommandBuffer(commandBuffers[i], &beginInfo));

			VkRenderPassBeginInfo renderPassInfo{};
			renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderPassInfo.renderPass = renderPass;
			renderPassInfo.framebuffer = swapChainFramebuffers[i];
			renderPassInfo.renderArea.offset = { 0, 0 };
			renderPassInfo.renderArea.extent = swapChainExtent;

			VkClearValue clearColor = { {{0.0f, 0.0f, 0.0f, 1.0f}} };
			renderPassInfo.clearValueCount = 1;
			renderPassInfo.pClearValues = &clearColor;

			vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

			vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

			vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[i], 0, nullptr);

			vkCmdDraw(commandBuffers[i], 3, 1, 0, 0);

			if (showImGui)
			{
				ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffers[i]);
			}

			vkCmdEndRenderPass(commandBuffers[i]);

			FT_VK_CALL(vkEndCommandBuffer(commandBuffers[i]));
		}

		void ClearErrorMarkers()
		{
			TextEditor::ErrorMarkers ems;
			editor.SetErrorMarkers(ems);
		}

		// TOOD: BROKEN.
		void DisplayErrorMarkers(const char* message)
		{
			// TODO: HACK!!!
			char line[10];
			int j = 0;
			for (int i = 9; message[i] != ':'; ++i)
			{
				line[j++] = message[i];
			}
			line[j] = '\0';

			int lineInt = atoi(line);

			TextEditor::ErrorMarkers ems;
			ems[lineInt] = message;
			editor.SetErrorMarkers(ems);
		}
	};
}

// TODO: HACK.
FT::Renderer renderer;

namespace FT
{
	void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
	{
		if (key == GLFW_KEY_S && action == GLFW_PRESS && mods == GLFW_MOD_CONTROL)
		{
			renderer.RecompileFragmentShader();
		}

		if (key == GLFW_KEY_S && action == GLFW_PRESS && mods & GLFW_MOD_CONTROL && mods & GLFW_MOD_SHIFT)
		{
			// TODO: Make new file with current editor contents.
		}

		if (key == GLFW_KEY_R && action == GLFW_PRESS && mods == GLFW_MOD_CONTROL)
		{
			renderer.RecompileFragmentShader();
		}

		if (key == GLFW_KEY_F && action == GLFW_PRESS && mods == GLFW_MOD_CONTROL)
		{
			renderer.ToggleImGui();
		}

		if (key == GLFW_KEY_N && action == GLFW_PRESS && mods == GLFW_MOD_CONTROL)
		{
			// TODO: New file.
		}

		if (key == GLFW_KEY_O && action == GLFW_PRESS && mods == GLFW_MOD_CONTROL)
		{
			// TOOD: Open file.
		}
	}

	void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
	{
		// TODO: Maybe add a condition if the code window is in focus????????
		if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) || glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL))
		{
			renderer.UpdateCodeFontSize(static_cast<float>(yoffset));
		}
	}
}

int main()
{
	renderer.Run();
	return EXIT_SUCCESS;
}
