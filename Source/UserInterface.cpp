#include "UserInterface.h"
#include "Application.h"
#include "Window.h"
#include "Core/Renderer.h"
#include "Core/Device.h"
#include "Core/Swapchain.h"
#include "Utility/ShaderFile.h"

FT_BEGIN_NAMESPACE

UserInterface::UserInterface(Application* inApplication, Window* inWindow, Renderer* inRenderer)
	: m_Application(inApplication)
	, m_Renderer(inRenderer)
	, m_Window(inWindow)
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

	Device* device = m_Renderer->GetDevice();
	Swapchain* swapchain = m_Renderer->GetSwapchain();

	FT_VK_CALL(vkCreateDescriptorPool(device->GetDevice(), &descriptorPoolInfo, nullptr, &imguiDescPool));

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

	ImGui_ImplGlfw_InitForVulkan(m_Window->GetWindow(), true);

	ImGui_ImplVulkan_InitInfo vulkanImplementationInitInfo{};
	vulkanImplementationInitInfo.Instance = device->GetInstance();
	vulkanImplementationInitInfo.PhysicalDevice = device->GetPhysicalDevice();
	vulkanImplementationInitInfo.Device = device->GetDevice();
	vulkanImplementationInitInfo.QueueFamily = device->GetGraphicsQueueFamilyIndex();
	vulkanImplementationInitInfo.Queue = device->GetGraphicsQueue();
	vulkanImplementationInitInfo.PipelineCache = VK_NULL_HANDLE;
	vulkanImplementationInitInfo.DescriptorPool = imguiDescPool;
	vulkanImplementationInitInfo.Allocator = nullptr;
	vulkanImplementationInitInfo.MinImageCount = swapchain->GetImageCount();
	vulkanImplementationInitInfo.ImageCount = swapchain->GetImageCount();
	vulkanImplementationInitInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

	ImGui_ImplVulkan_Init(&vulkanImplementationInitInfo, swapchain->GetRenderPass());

	VkCommandBuffer commandBuffer = device->BeginSingleTimeCommands();
	ImGui_ImplVulkan_CreateFontsTexture(commandBuffer);
	device->EndSingleTimeCommands(commandBuffer);

	ImGui_ImplVulkan_DestroyFontUploadObjects();

	ApplyImGuiStyle();

	ShaderFile* fragmentShaderFile = m_Renderer->GetFragmentShaderFile();
	m_Editor.SetText(fragmentShaderFile->GetSourceCode());
}

UserInterface::~UserInterface()
{
	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	Device* device = m_Renderer->GetDevice();
	vkDestroyDescriptorPool(device->GetDevice(), imguiDescPool, nullptr);
}

void UserInterface::ImguiNewFrame()
{
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	ImguiDockSpace();

	ImGui::ShowDemoWindow();

	// TODO: Refactor a bit.
	char buf[128];
	ShaderFile* fragmentShaderFile = m_Renderer->GetFragmentShaderFile();
	sprintf_s(buf, "%s###ShaderTitle", fragmentShaderFile->GetName().c_str());

	ImGui::Begin(buf, nullptr, ImGuiWindowFlags_HorizontalScrollbar);
	ImGui::SetWindowSize(ImVec2(FT_DEFAULT_WINDOW_WIDTH, FT_DEFAULT_WINDOW_HEIGHT), ImGuiCond_FirstUseEver); // TODO: Change this!!!
	ImGui::SetNextWindowBgAlpha(0.f);
	ImGui::SetWindowFontScale(codeFontSize);
	m_Editor.SetShowWhitespaces(false); // TODO: Settings.
	m_Editor.Render("TextEditor");
	ImGui::End();

	logger.Draw("Log");

	ImGui::Render();
}

void UserInterface::UpdateCodeFontSize(float offset)
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

void UserInterface::SetEditorText(const std::string& inText)
{
	return m_Editor.SetText(inText);
}

void UserInterface::ClearErrorMarkers()
{
	TextEditor::ErrorMarkers ems;
	m_Editor.SetErrorMarkers(ems);
}

void UserInterface::ApplyImGuiStyle()
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

void UserInterface::ImguiMenuBar()
{
	if (ImGui::BeginMenuBar())
	{
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("New", "Ctrl-N"))
			{
				std::string shaderFilePath;
				if (m_Application->SaveShaderDialog(shaderFilePath))
				{
					// TODO: Make new empty/default file and keep it open.
				}
			}

			if (ImGui::MenuItem("Open", "Ctrl-O"))
			{
				std::string shaderFilePath;
				if (m_Application->OpenShaderDialog(shaderFilePath))
				{
					m_Application->LoadShader(shaderFilePath);
					m_Application->RecompileFragmentShader();
				}
			}

			if (ImGui::MenuItem("Save", "Ctrl-S"))
			{
				if (m_Application->RecompileFragmentShader())
				{
					m_Application->SaveFragmentShader();
				}
			}

			if (ImGui::MenuItem("Save As", "Ctrl-Shift-S"))
			{
				std::string shaderFilePath;
				if (m_Application->SaveShaderDialog(shaderFilePath))
				{
					const std::string& textToSave = m_Editor.GetText();
					// TODO: Make new file with current contents and keep it open.
				}
			}

			if (ImGui::MenuItem("Quit", "Alt-F4"))
			{
				m_Window->Close();
			}

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Edit"))
		{
			bool readOnly = m_Editor.IsReadOnly();
			if (ImGui::MenuItem("Read-only mode", nullptr, &readOnly))
			{
				m_Editor.SetReadOnly(readOnly);
			}

			ImGui::Separator();

			if (ImGui::MenuItem("Undo", "Alt-Backspace", nullptr, !readOnly && m_Editor.CanUndo()))
			{
				m_Editor.Undo();
			}

			if (ImGui::MenuItem("Redo", "Ctrl-Y", nullptr, !readOnly && m_Editor.CanRedo()))
			{
				m_Editor.Redo();
			}

			ImGui::Separator();

			if (ImGui::MenuItem("Copy", "Ctrl-C", nullptr, m_Editor.HasSelection()))
			{
				m_Editor.Copy();
			}

			if (ImGui::MenuItem("Cut", "Ctrl-X", nullptr, !readOnly && m_Editor.HasSelection()))
			{
				m_Editor.Cut();
			}

			if (ImGui::MenuItem("Delete", "Del", nullptr, !readOnly && m_Editor.HasSelection()))
			{
				m_Editor.Delete();
			}

			if (ImGui::MenuItem("Paste", "Ctrl-V", nullptr, !readOnly && ImGui::GetClipboardText() != nullptr))
			{
				m_Editor.Paste();
			}

			ImGui::Separator();

			if (ImGui::MenuItem("Compile", "Ctrl-R", nullptr, !readOnly))
			{
				m_Application->RecompileFragmentShader();
			}

			ImGui::Separator();

			if (ImGui::MenuItem("Select all", nullptr, nullptr))
			{
				m_Editor.SetSelection(TextEditor::Coordinates(), TextEditor::Coordinates(m_Editor.GetTotalLines(), 0));
			}

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("View"))
		{
			if (ImGui::MenuItem("Show UI", "Ctrl-F"))
			{
				m_Renderer->ToggleUserInterface();
			}

			ImGui::EndMenu();
		}

		ImGui::EndMenuBar();
	}
}

void UserInterface::ImguiDockSpace()
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

void UserInterface::DisplayErrorMarkers(const char* message)
{
	// TOOD: BROKEN.
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
	m_Editor.SetErrorMarkers(ems);
}

FT_END_NAMESPACE
