#include "Core/Device.h"
#include "Core/Swapchain.h"
#include "Core/Buffer.h"
#include "Core/Image.h"
#include "Core/Shader.h"
#include "Core/Pipeline.h"
#include "Compiler/ShaderCompiler.h"
#include "Utility/ShaderFile.h"
#include "Utility/ImageFile.h"
#include "Utility/FileExplorer.h"

// TODO: Next to do ImageResource and BufferResource.

// TODO: Lightweight light fast tool (foton is small and fast :))

// TODO: Find out if we can make background for all text.
// TODO: Separate Runtime and Editor?

// TODO: When starting application sometimes new row is added at the end on current file.
// TOOD: Shader printf?

// TOOD: How resource loading with paths is going to work if we only run exe files? It's relative to project root, not the exe.

// TOOD: Use more high resolution font file for code editor.

// TOOD: Smart pointers for transient objects (pipeline, shader, shader file...)

namespace FT
{
	// TODO: This shouldn't be here. FT_ROOT_DIR is probably wrong for exe only???
	std::string GetFullPath(const std::string inRelativePath) { return std::string(FT_ROOT_DIR) + inRelativePath; }

	// TOOD: Move somewhere else.
	const uint32_t WIDTH = 1280;
	const uint32_t HEIGHT = 720;

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

	// TOOD: Move to separate file.
	class Application
	{
	public:
		void Run()
		{
			InitializeShaderCompiler();
			InitializeWindow();
			InitializeVulkan();
			InitializeImGui();
			MainLoop();
			Cleanup();
		}

	private:
		GLFWwindow* window;
		FileExplorer m_FileExplorer;

		Device* m_Device = nullptr;
		Swapchain* m_Swapchain = nullptr;

		Shader* m_VertexShader = nullptr;
		Shader* m_FragmentShader = nullptr;
		ShaderFile* m_FragmentShaderFile = nullptr;

		Pipeline* m_Pipeline = nullptr;

		Image* m_Image;
		std::vector<Buffer*> m_UniformBuffers; // TODO: Move double buffering to Buffer implementation, since it's always host visible.

		VkDescriptorSetLayout descriptorSetLayout;
		VkDescriptorPool descriptorPool;
		std::vector<VkDescriptorSet> descriptorSets;

		VkDescriptorPool imguiDescPool;

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

			glfwSetFramebufferSizeCallback(window, Swapchain::FramebufferResized);
			glfwSetKeyCallback(window, KeyCallback);
			glfwSetScrollCallback(window, ScrollCallback);

			const ImageFile iconImage(GetFullPath("icon"));

			GLFWimage icon;
			icon.width = iconImage.GetWidth();
			icon.height = iconImage.GetHeight();
			icon.pixels = iconImage.GetPixels();

			glfwSetWindowIcon(window, 1, &icon);
		}

		void InitializeVulkan()
		{
			m_Device = new Device(window);
			m_Swapchain = new Swapchain(m_Device, window);

			CreateShaders();

			m_Pipeline = new Pipeline(m_Device, m_Swapchain, descriptorSetLayout, m_VertexShader, m_FragmentShader);

			const ImageFile imageFile(GetFullPath("icon"));
			m_Image = new Image(m_Device, imageFile);

			CreateUniformBuffers();
			CreateDescriptorPool();
			CreateDescriptorSets(m_FragmentShader->GetBindings());
			m_Device->AllocateCommandBuffers(m_Swapchain->GetImageCount());
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
			vulkanImplementationInitInfo.QueueFamily = m_Device->GetGraphicsQueueFamilyIndex();
			vulkanImplementationInitInfo.Queue = m_Device->GetGraphicsQueue();
			vulkanImplementationInitInfo.PipelineCache = VK_NULL_HANDLE;
			vulkanImplementationInitInfo.DescriptorPool = imguiDescPool;
			vulkanImplementationInitInfo.Allocator = nullptr;
			vulkanImplementationInitInfo.MinImageCount = m_Swapchain->GetImageCount();
			vulkanImplementationInitInfo.ImageCount = m_Swapchain->GetImageCount();
			vulkanImplementationInitInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

			ImGui_ImplVulkan_Init(&vulkanImplementationInitInfo, m_Swapchain->GetRenderPass());

			VkCommandBuffer commandBuffer = m_Device->BeginSingleTimeCommands();
			ImGui_ImplVulkan_CreateFontsTexture(commandBuffer);
			m_Device->EndSingleTimeCommands(commandBuffer);

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

		void CleanupSwapchain()
		{
			m_Swapchain->Cleanup();

			for (uint32_t i = 0; i < m_UniformBuffers.size(); ++i)
			{
				delete(m_UniformBuffers[i]);
			}

			m_UniformBuffers.clear();

			m_Device->FreeCommandBuffers();

			delete(m_Pipeline);

			vkDestroyDescriptorPool(m_Device->GetDevice(), descriptorPool, nullptr);
		}

		void Cleanup()
		{
			delete(m_FragmentShaderFile);
			delete(m_FragmentShader);
			delete(m_VertexShader);

			CleanupSwapchain();

			ImGui_ImplVulkan_Shutdown();
			ImGui_ImplGlfw_Shutdown();
			ImGui::DestroyContext();
			vkDestroyDescriptorPool(m_Device->GetDevice(), imguiDescPool, nullptr); // TODO: Should this be here or in swapchain cleanup?

			delete(m_Image);

			vkDestroyDescriptorSetLayout(m_Device->GetDevice(), descriptorSetLayout, nullptr);

			delete(m_Swapchain);
			delete(m_Device);

			glfwDestroyWindow(window);

			glfwTerminate();

			FinalizeShaderCompiler();
		}

		void RecreateSwapchain()
		{
			int width = 0, height = 0;
			glfwGetFramebufferSize(window, &width, &height);
			while (width == 0 || height == 0)
			{
				glfwGetFramebufferSize(window, &width, &height);
				glfwWaitEvents();
			}

			vkDeviceWaitIdle(m_Device->GetDevice());

			CleanupSwapchain();

			m_Swapchain->Recreate();

			m_Pipeline = new Pipeline(m_Device, m_Swapchain, descriptorSetLayout, m_VertexShader, m_FragmentShader);

			CreateUniformBuffers();
			CreateDescriptorPool();
			CreateDescriptorSets(m_FragmentShader->GetBindings());
			m_Device->AllocateCommandBuffers(m_Swapchain->GetImageCount());

			ImGui_ImplVulkan_SetMinImageCount(m_Swapchain->GetImageCount());
		}

		void CreateDescriptorSetLayout(const std::vector<Binding>& inBindings)
		{
			std::vector<VkDescriptorSetLayoutBinding> descriptorSetBindings;
			descriptorSetBindings.resize(inBindings.size());
			for (uint32_t i = 0; i < inBindings.size(); ++i)
			{
				descriptorSetBindings[i] = inBindings[i].DescriptorSetBinding;
			}

			VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo{};
			descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			descriptorSetLayoutCreateInfo.bindingCount = static_cast<uint32_t>(descriptorSetBindings.size());
			descriptorSetLayoutCreateInfo.pBindings = descriptorSetBindings.data();

			FT_VK_CALL(vkCreateDescriptorSetLayout(m_Device->GetDevice(), &descriptorSetLayoutCreateInfo, nullptr, &descriptorSetLayout));
		}

		void CreateShaders()
		{
			// TODO: How to prevent loading uncompilable file??? Maybe make internal shader somehow uneditable or serialized and fallback to them if previous shader cannot be compiled.
			{
				const ShaderFile shaderFile(GetFullPath("Shaders/Internal/FullScreen.vert.glsl"));

				const ShaderCompileResult compileResult = CompileShader(shaderFile.GetLanguage(), ShaderStage::Vertex, shaderFile.GetSourceCode(), VertexShaderCodeEntry);
				FT_CHECK(compileResult.Status == ShaderCompileStatus::Success, "Failed %s vertex shader %s.", ConvertCompilationStatusToText(compileResult.Status), shaderFile.GetName().c_str());

				m_VertexShader = new Shader(m_Device, ShaderStage::Vertex, VertexShaderCodeEntry, compileResult.SpvCode);
			}

			{
				m_FragmentShaderFile = new ShaderFile(GetFullPath("Shaders/Internal/Default.frag.glsl"));

				const ShaderCompileResult compileResult = CompileShader(m_FragmentShaderFile->GetLanguage(), ShaderStage::Fragment, m_FragmentShaderFile->GetSourceCode(), FragmentShaderCodeEntry);
				FT_CHECK(compileResult.Status == ShaderCompileStatus::Success, "Failed %s fragment shader %s.", ConvertCompilationStatusToText(compileResult.Status), m_FragmentShaderFile->GetName().c_str());

				m_FragmentShader = new Shader(m_Device, ShaderStage::Fragment, FragmentShaderCodeEntry, compileResult.SpvCode);
			}

			editor.SetText(m_FragmentShaderFile->GetSourceCode());

			CreateDescriptorSetLayout(m_FragmentShader->GetBindings());
		}

		void CreateUniformBuffers()
		{
			VkDeviceSize bufferSize = sizeof(UniformBufferObject);
			m_UniformBuffers.resize(m_Swapchain->GetImageCount());
			for (size_t i = 0; i < m_Swapchain->GetImageCount(); ++i)
			{
				m_UniformBuffers[i] = new Buffer(m_Device, bufferSize, BufferUsageFlags::Uniform);
			}
		}

		void CreateDescriptorPool()
		{
			std::array<VkDescriptorPoolSize, 2> poolSizes{};
			poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			poolSizes[0].descriptorCount = 100 * m_Swapchain->GetImageCount();
			poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			poolSizes[1].descriptorCount = 100 * m_Swapchain->GetImageCount();

			VkDescriptorPoolCreateInfo poolInfo{};
			poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
			poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
			poolInfo.pPoolSizes = poolSizes.data();
			poolInfo.maxSets = m_Swapchain->GetImageCount();

			FT_VK_CALL(vkCreateDescriptorPool(m_Device->GetDevice(), &poolInfo, nullptr, &descriptorPool));
		}

		void CreateDescriptorSets(const std::vector<Binding>& inBindings)
		{
			std::vector<VkDescriptorSetLayout> descriptorSetLayouts(m_Swapchain->GetImageCount(), descriptorSetLayout);

			VkDescriptorSetAllocateInfo allocateInfo{};
			allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			allocateInfo.descriptorPool = descriptorPool;
			allocateInfo.descriptorSetCount = m_Swapchain->GetImageCount();
			allocateInfo.pSetLayouts = descriptorSetLayouts.data();

			descriptorSets.resize(m_Swapchain->GetImageCount());
			FT_VK_CALL(vkAllocateDescriptorSets(m_Device->GetDevice(), &allocateInfo, descriptorSets.data()));

			// TODO: Recreate descriptor sets each time descriptor layout gets recreated.
			for (size_t i = 0; i < m_Swapchain->GetImageCount(); ++i)
			{
				std::vector<VkWriteDescriptorSet> descriptorWrites(inBindings.size());

				VkDescriptorBufferInfo bufferInfo{};
				bufferInfo.buffer = m_UniformBuffers[i]->GetBuffer();
				bufferInfo.offset = 0;
				bufferInfo.range = sizeof(UniformBufferObject);

				VkDescriptorImageInfo imageInfo{};
				imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				imageInfo.imageView = m_Image->GetImageView();
				imageInfo.sampler = m_Image->GetSampler();

				for (uint32_t j = 0; j < inBindings.size(); ++j)
				{
					descriptorWrites[j].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
					descriptorWrites[j].dstSet = descriptorSets[i];
					descriptorWrites[j].dstBinding = inBindings[j].DescriptorSetBinding.binding;
					descriptorWrites[j].dstArrayElement = 0;
					descriptorWrites[j].descriptorType = inBindings[j].DescriptorSetBinding.descriptorType;
					descriptorWrites[j].descriptorCount = 1;
					
					if (inBindings[j].DescriptorSetBinding.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
					{
						descriptorWrites[j].pBufferInfo = &bufferInfo;
					}
					else if (inBindings[j].DescriptorSetBinding.descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
					{
						descriptorWrites[j].pImageInfo = &imageInfo;
					}
					else
					{
						// TOOD: FAIL? Recover? Not supported.
					}
				}

				vkUpdateDescriptorSets(m_Device->GetDevice(), static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
			}
		}

		void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
		{
			VkCommandBuffer commandBuffer = m_Device->BeginSingleTimeCommands();

			VkBufferCopy copyRegion{};
			copyRegion.size = size;
			vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

			m_Device->EndSingleTimeCommands(commandBuffer);
		}

		void UpdateUniformBuffer(uint32_t currentImage)
		{
			static auto startTime = std::chrono::high_resolution_clock::now();

			auto currentTime = std::chrono::high_resolution_clock::now();
			float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

			UniformBufferObject ubo{};
			ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
			ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
			ubo.proj = glm::perspective(glm::radians(45.0f), m_Swapchain->GetExtent().width / static_cast<float>(m_Swapchain->GetExtent().height), 0.1f, 10.0f);
			ubo.proj[1][1] *= -1;

			void* data = m_UniformBuffers[currentImage]->Map();
			memcpy(data, &ubo, sizeof(ubo));
			m_UniformBuffers[currentImage]->Unmap();
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
						if (RecompileFragmentShader())
						{
							SaveFragmentShader();
						}
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
			sprintf_s(buf, "%s###ShaderTitle", m_FragmentShaderFile->GetName().c_str());

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

		void SaveFragmentShader()
		{
			m_FragmentShaderFile->UpdateSourceCode(editor.GetText());
		}

		bool RecompileFragmentShader()
		{
			// TODO: editor.IsTextChanged() is not working. Investigate.
			if (editor.GetText().compare(m_FragmentShaderFile->GetSourceCode()) == 0)
			{
				return false;
			}

			const std::string& fragmentShaderSourceCode = editor.GetText();
			const ShaderCompileResult compileResult = CompileShader(m_FragmentShaderFile->GetLanguage(), ShaderStage::Fragment, fragmentShaderSourceCode, FragmentShaderCodeEntry);

			if (compileResult.Status != ShaderCompileStatus::Success)
			{
				FT_LOG("Failed %s shader %s.", ConvertCompilationStatusToText(compileResult.Status), m_FragmentShaderFile->GetName().c_str());
				return false;
			}

			ClearErrorMarkers();

			vkQueueWaitIdle(m_Device->GetGraphicsQueue());

			delete(m_FragmentShader);
			m_FragmentShader = new Shader(m_Device, ShaderStage::Fragment, FragmentShaderCodeEntry, compileResult.SpvCode);

			delete(m_Pipeline);

			vkDestroyDescriptorPool(m_Device->GetDevice(), descriptorPool, nullptr);
			CreateDescriptorPool();

			vkDestroyDescriptorSetLayout(m_Device->GetDevice(), descriptorSetLayout, nullptr);
			CreateDescriptorSetLayout(m_FragmentShader->GetBindings());

			CreateDescriptorSets(m_FragmentShader->GetBindings());
			
			m_Pipeline = new Pipeline(m_Device, m_Swapchain, descriptorSetLayout, m_VertexShader, m_FragmentShader);

			return true;
		}
	private: // HACK:

		void LoadShader(const std::string& inPath)
		{
			ShaderFile* loadedShaderFile = new ShaderFile(inPath);
			const ShaderCompileResult compileResult = CompileShader(loadedShaderFile->GetLanguage(), ShaderStage::Fragment, loadedShaderFile->GetSourceCode(), FragmentShaderCodeEntry);

			if (compileResult.Status != ShaderCompileStatus::Success)
			{
				FT_LOG("Failed %s for loaded shader %s.", ConvertCompilationStatusToText(compileResult.Status), loadedShaderFile->GetName().c_str());
				return;
			}

			delete(m_FragmentShaderFile);
			m_FragmentShaderFile = loadedShaderFile;

			editor.SetText(m_FragmentShaderFile->GetSourceCode());
		}

		// TODO: Move to device and call it only when shader gets compiled/recompiled.
		void FillCommandBuffers(uint32_t i)
		{
			VkCommandBuffer commandBuffer = m_Device->GetCommandBuffer(i);

			VkCommandBufferBeginInfo beginInfo{};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

			FT_VK_CALL(vkBeginCommandBuffer(commandBuffer, &beginInfo));

			VkRenderPassBeginInfo renderPassInfo{};
			renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderPassInfo.renderPass = m_Swapchain->GetRenderPass();
			renderPassInfo.framebuffer = m_Swapchain->GetFramebuffer(i);
			renderPassInfo.renderArea.offset = { 0, 0 };
			renderPassInfo.renderArea.extent = m_Swapchain->GetExtent();

			VkClearValue clearColor = { {{0.0f, 0.0f, 0.0f, 1.0f}} };
			renderPassInfo.clearValueCount = 1;
			renderPassInfo.pClearValues = &clearColor;

			vkCmdBeginRenderPass(m_Device->GetCommandBuffer(i), &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline->GetGraphicsPipeline());

			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline->GetPipelineLayout(), 0, 1, &descriptorSets[i], 0, nullptr);

			vkCmdDraw(commandBuffer, 3, 1, 0, 0);

			if (showImGui)
			{
				ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
			}

			vkCmdEndRenderPass(commandBuffer);

			FT_VK_CALL(vkEndCommandBuffer(commandBuffer));
		}

		void DrawFrame()
		{
			const SwapchainImageAcquireResult imageAcquireResult = m_Swapchain->AcquireNextImage();
			if (imageAcquireResult.Status == SwapchainStatus::Recreate)
			{
				RecreateSwapchain();
				return;
			}

			FT_CHECK(imageAcquireResult.Status == SwapchainStatus::Success, "Failed acquiring swapchain image.");

			const uint32_t imageIndex = imageAcquireResult.ImageIndex;

			UpdateUniformBuffer(imageIndex);
			FillCommandBuffers(imageIndex);

			const SwapchainStatus presentStatus = m_Swapchain->Present(imageIndex);
			if (presentStatus == SwapchainStatus::Recreate)
			{
				RecreateSwapchain();
			}
			else
			{
				FT_CHECK(presentStatus == SwapchainStatus::Success, "Swapchain present failed.");
			}
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
FT::Application application;

namespace FT
{
	void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
	{
		if (key == GLFW_KEY_S && action == GLFW_PRESS && mods == GLFW_MOD_CONTROL)
		{
			if (application.RecompileFragmentShader())
			{
				application.SaveFragmentShader();
			}
		}

		if (key == GLFW_KEY_S && action == GLFW_PRESS && mods & GLFW_MOD_CONTROL && mods & GLFW_MOD_SHIFT)
		{
			// TODO: Make new file with current editor contents.
		}

		if (key == GLFW_KEY_R && action == GLFW_PRESS && mods == GLFW_MOD_CONTROL)
		{
			application.RecompileFragmentShader();
		}

		if (key == GLFW_KEY_F && action == GLFW_PRESS && mods == GLFW_MOD_CONTROL)
		{
			application.ToggleImGui();
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
			application.UpdateCodeFontSize(static_cast<float>(yoffset));
		}
	}
}

int main()
{
	application.Run();
	return EXIT_SUCCESS;
}
