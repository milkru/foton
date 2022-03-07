#include "UserInterface.h"
#include "Application.h"
#include "Window.h"
#include "Core/Renderer.h"
#include "Core/Device.h"
#include "Core/Swapchain.h"
#include "Core/CombinedImageSampler.h"
#include "Core/Image.h"
#include "Core/Sampler.h"
#include "Core/UniformBuffer.h"
#include "Utility/ShaderFile.h"
#include "Utility/FileExplorer.h"
#include "Utility/FilePath.h"
#include <imgui_internal.h>

FT_BEGIN_NAMESPACE

UserInterface::UserInterface(Application* inApplication)
	: m_Application(inApplication)
	, m_Renderer(inApplication->GetRenderer())
	, m_CodeFontSize(1.5f)
	, m_Enable(true)
	, m_ShowBindings(true)
	, m_ShowOutput(true)
	, m_ShowWhiteSpaces(false)
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

	ImGui_ImplGlfw_InitForVulkan(inApplication->GetWindow()->GetWindow(), true);

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

	const static float defaultFontSize = 20.0f;
	const static std::string fontFileFullPath = GetAbsolutePath("/External/src/imgui/misc/fonts/Cousine-Regular.ttf");
	io.Fonts->AddFontFromFileTTF(fontFileFullPath.c_str(), defaultFontSize);

	VkCommandBuffer commandBuffer = device->BeginSingleTimeCommands();
	ImGui_ImplVulkan_CreateFontsTexture(commandBuffer);
	device->EndSingleTimeCommands(commandBuffer);

	ImGui_ImplVulkan_DestroyFontUploadObjects();

	ApplyImGuiStyle();

	m_Editor.SetPalette(TextEditor::GetColorPalette());
	m_Editor.SetShowWhitespaces(m_ShowWhiteSpaces);

	ImGuiStyle& style = ImGui::GetStyle();
	style.FrameRounding = 10.0f;
	style.FramePadding = ImVec2(4.0f, 5.0f);
	style.ItemSpacing = ImVec2(9.0f, 6.0f);
	style.ItemInnerSpacing = ImVec2(5.0f, 4.0f);
	style.WindowMenuButtonPosition = ImGuiDir_None;
	style.ScrollbarSize = 20.0f;
	style.ScrollbarRounding = 12.0f;

	m_StartTime = std::chrono::high_resolution_clock::now();
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

	if (m_Enable)
	{
		ImGui::Begin("Editor", nullptr, ImGuiWindowFlags_HorizontalScrollbar);

		ImGui::SetNextWindowBgAlpha(0.f);
		ImGui::SetWindowFontScale(m_CodeFontSize);

		m_Editor.Render("TextEditor");

		ImGui::End();

		if (m_ShowOutput)
		{
			ImGuiLogger::Draw("Log");
		}

		if (m_ShowBindings)
		{
			ImguiBindingsWindow();
		}
	}

	ImGui::Render();
}

void UserInterface::UpdateCodeFontSize(float inOffset)
{
	const static float MinCodeFontSize = 0.5f;
	const static float MaxCodeFontSize = 3.0f;
	const static float CodeFontSizeMul = 0.1f;

	m_CodeFontSize += inOffset * CodeFontSizeMul;
	m_CodeFontSize = FT_CLAMP(m_CodeFontSize, MinCodeFontSize, MaxCodeFontSize);
}

void UserInterface::SetEditorText(const std::string& inText)
{
	return m_Editor.SetText(inText);
}

void UserInterface::SetEditorLanguage(const ShaderLanguage inLanguage)
{
	switch (inLanguage)
	{
	case ShaderLanguage::GLSL:
		m_Editor.SetLanguageDefinition(TextEditor::LanguageDefinition::GLSL());
		return;

	case ShaderLanguage::HLSL:
		m_Editor.SetLanguageDefinition(TextEditor::LanguageDefinition::HLSL());
		return;

	default:
		FT_FAIL("Unsupported ShaderLanguage.");
	}

}

void UserInterface::SetCodeFontSize(const float inCodeFontSize)
{
	m_CodeFontSize = inCodeFontSize;
}

void UserInterface::SetShowBindings(const bool inShowBindings)
{
	m_ShowBindings = inShowBindings;
}

void UserInterface::SetShowOutput(const bool inShowOutput)
{
	m_ShowOutput = inShowOutput;
}

void UserInterface::SetShowWhiteSpaces(const bool inShowWhiteSpaces)
{
	m_ShowWhiteSpaces = inShowWhiteSpaces;
	m_Editor.SetShowWhitespaces(m_ShowWhiteSpaces);
}

void UserInterface::ClearErrorMarkers()
{
	TextEditor::ErrorMarkers ems;
	m_Editor.SetErrorMarkers(ems);
}

void UserInterface::ToggleEnabled()
{
	m_Enable = !m_Enable;
}

std::string UserInterface::GetEditorText() const
{
	std::string text = m_Editor.GetText();
	if (text.length() > 1)
	{
		// Editor returns additional new line character for unknown reason.
		return text.substr(0, text.length() - 1);
	}

	return text;
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
	colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.10f, 0.10f, 0.10f, 0.20f);
	colors[ImGuiCol_MenuBarBg] = ImVec4(0.10f, 0.10f, 0.10f, 0.5f);
	colors[ImGuiCol_ScrollbarBg] = ImVec4(0.20f, 0.20f, 0.20f, 0.1f);
	colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.10f, 0.10f, 0.10f, 0.2f);
	colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.10f, 0.10f, 0.10f, 0.3f);
	colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.39f, 0.39f, 0.39f, 0.4f);
	colors[ImGuiCol_CheckMark] = ImVec4(0.90f, 0.90f, 0.90f, 0.50f);
	colors[ImGuiCol_SliderGrab] = ImVec4(1.00f, 1.00f, 1.00f, 0.30f);
	colors[ImGuiCol_SliderGrabActive] = ImVec4(0.39f, 0.39f, 0.39f, 0.60f);
	colors[ImGuiCol_Button] = ImVec4(0.1f, 0.1f, 0.1f, 0.5f);
	colors[ImGuiCol_ButtonHovered] = ImVec4(0.10f, 0.10f, 0.10f, 0.79f);
	colors[ImGuiCol_ButtonActive] = ImVec4(0.46f, 0.46f, 0.46f, 1.00f);
	colors[ImGuiCol_Header] = ImVec4(0.10f, 0.10f, 0.10f, 0.55f);
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
	colors[ImGuiCol_DockingPreview] = ImVec4(0.10f, 0.10f, 0.10f, 0.3f);
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

void UserInterface::ImguiShowInfo()
{
	static std::chrono::steady_clock::time_point previousTime;
	const std::chrono::steady_clock::time_point currentTime = std::chrono::high_resolution_clock::now();
	const float deltaTime = std::chrono::duration<float, std::chrono::milliseconds::period>(currentTime - previousTime).count();
	const float elapsedTime = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - m_StartTime).count();
	previousTime = currentTime;

	static float deltaTimeDisplay = 0.0f;
	static float displayElapsedDeltaTime = FLT_MAX;
	displayElapsedDeltaTime += deltaTime;

	const static float displayPeriodMilliseconds = 500.0f;
	if (displayElapsedDeltaTime > displayPeriodMilliseconds)
	{
		deltaTimeDisplay = deltaTime;
		displayElapsedDeltaTime = 0.0f;
	}

	const ShaderFile* fragmentShaderFile = m_Renderer->GetFragmentShaderFile();
	const float frameRate = 1000.0f / deltaTimeDisplay; // TODO: Do weighted average.

	const char* shaderFileName = fragmentShaderFile->GetName().c_str();
	float indent = ImGui::GetFont()->CalcTextSizeA(ImGui::GetFontSize(), FLT_MAX, -1.0f, shaderFileName, nullptr, nullptr).x;

	char elapsedTimeText[32];
	sprintf(elapsedTimeText, "  %.2f s", elapsedTime);
	indent += ImGui::GetFont()->CalcTextSizeA(ImGui::GetFontSize(), FLT_MAX, -1.0f, elapsedTimeText, nullptr, nullptr).x;

	char frameRateText[32];
	sprintf(frameRateText, "  %.2f fps", frameRate);
	indent += ImGui::GetFont()->CalcTextSizeA(ImGui::GetFontSize(), FLT_MAX, -1.0f, frameRateText, nullptr, nullptr).x;

	char deltaTimeText[32];
	sprintf(deltaTimeText, "  %.4f ms", deltaTimeDisplay);
	indent += ImGui::GetFont()->CalcTextSizeA(ImGui::GetFontSize(), FLT_MAX, -1.0f, deltaTimeText, nullptr, nullptr).x;

	const static float additionalIndentOffset = 50;
	ImGui::SameLine(ImGui::GetWindowWidth() - indent - additionalIndentOffset);

	ImGui::Text("%s", shaderFileName);
	ImGui::Text("%s", elapsedTimeText);
	ImGui::Text("%s", frameRateText);
	ImGui::Text("%s", deltaTimeText);
}

void UserInterface::ImguiMenuBar()
{
	if (ImGui::BeginMenuBar())
	{
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("New", "Ctrl-N"))
			{
				m_Application->NewShaderMenuItem();
			}

			if (ImGui::MenuItem("Open", "Ctrl-O"))
			{
				m_Application->OpenShaderMenuItem();
			}

			if (ImGui::MenuItem("Save", "Ctrl-S"))
			{
				m_Application->SaveShaderMenuItem();
			}

			if (ImGui::MenuItem("Save As", "Ctrl-Shift-S"))
			{
				m_Application->SaveAsShaderMenuItem();
			}

			if (ImGui::MenuItem("Quit", "Alt-F4"))
			{
				m_Application->QuitMenuItem();
			}

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Edit"))
		{
			if (ImGui::MenuItem("Undo", "Ctrl-Z", nullptr, m_Editor.CanUndo()))
			{
				m_Editor.Undo();
			}

			if (ImGui::MenuItem("Redo", "Ctrl-Y", nullptr, m_Editor.CanRedo()))
			{
				m_Editor.Redo();
			}

			ImGui::Separator();

			if (ImGui::MenuItem("Copy", "Ctrl-C", nullptr, m_Editor.HasSelection()))
			{
				m_Editor.Copy();
			}

			if (ImGui::MenuItem("Cut", "Ctrl-X", nullptr, m_Editor.HasSelection()))
			{
				m_Editor.Cut();
			}

			if (ImGui::MenuItem("Delete", "Del", nullptr, m_Editor.HasSelection()))
			{
				m_Editor.Delete();
			}

			if (ImGui::MenuItem("Paste", "Ctrl-V", nullptr, ImGui::GetClipboardText() != nullptr))
			{
				m_Editor.Paste();
			}

			ImGui::Separator();

			if (ImGui::MenuItem("Compile", "Ctrl-R", nullptr))
			{
				m_Application->RecompileFragmentShader();
			}

			ImGui::Separator();

			if (ImGui::MenuItem("Select all", "Ctrl-A", nullptr))
			{
				m_Editor.SetSelection(TextEditor::Coordinates(), TextEditor::Coordinates(m_Editor.GetTotalLines(), 0));
			}

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("View"))
		{
			ImGui::MenuItem("Show Interface", "Ctrl-F", &m_Enable);

			if (!m_Enable)
			{
				ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
				ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
			}

			ImGui::MenuItem("Show Bindings", NULL, &m_ShowBindings);
			ImGui::MenuItem("Show Output", NULL, &m_ShowOutput);

			if (!m_Enable)
			{
				ImGui::PopItemFlag();
				ImGui::PopStyleVar();
			}

			if (ImGui::MenuItem("Show Whitespaces", NULL, &m_ShowWhiteSpaces))
			{
				m_Editor.SetShowWhitespaces(m_ShowWhiteSpaces);
			}

			ImGui::EndMenu();
		}

		ImguiShowInfo();

		ImGui::EndMenuBar();
	}
}

void UserInterface::ImguiDockSpace()
{
	static ImGuiDockNodeFlags dockspaceFlags = ImGuiDockNodeFlags_PassthruCentralNode | ImGuiDockNodeFlags_NoTabBar;

	ImGuiWindowFlags windowFlags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
	const ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(viewport->WorkPos);
	ImGui::SetNextWindowSize(viewport->WorkSize);
	ImGui::SetNextWindowViewport(viewport->ID);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	windowFlags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
	windowFlags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

	static bool open = true;
	ImGui::Begin("DockSpace", &open, windowFlags);
	ImGui::PopStyleVar();

	ImGui::PopStyleVar(2);

	ImGuiIO& io = ImGui::GetIO();
	{
		ImGuiID dockspaceId = ImGui::GetID("MyDockSpace");

		if (!ImGui::DockBuilderGetNode(dockspaceId))
		{
			ImGui::DockBuilderRemoveNode(dockspaceId);
			ImGui::DockBuilderAddNode(dockspaceId, ImGuiDockNodeFlags_None);

			ImGuiID dockMainId = dockspaceId;
			ImGuiID bindingsId = ImGui::DockBuilderSplitNode(dockMainId, ImGuiDir_Left, 0.175f, nullptr, &dockMainId);
			ImGuiID editorId = ImGui::DockBuilderSplitNode(dockMainId, ImGuiDir_Up, 0.75f, nullptr, &dockMainId);
			ImGuiID logId = ImGui::DockBuilderSplitNode(dockMainId, ImGuiDir_Down, 1.0f, nullptr, &dockMainId);

			ImGui::DockBuilderDockWindow("BindingsMenu", bindingsId);
			ImGui::DockBuilderDockWindow("Editor", editorId);
			ImGui::DockBuilderDockWindow("Log", logId);

			ImGui::DockBuilderFinish(dockMainId);
		}

		ImGui::DockSpace(dockspaceId, ImVec2(0.0f, 0.0f), dockspaceFlags);
	}

	ImguiMenuBar();

	ImGui::End();
}

ImGuiDataType GetComponentDataType(const SpvReflectTypeDescription* inReflectTypeDescription)
{
	if (inReflectTypeDescription->type_flags & SPV_REFLECT_TYPE_FLAG_BOOL)
	{
		return ImGuiDataType_U32;
	}
	else if (inReflectTypeDescription->type_flags & SPV_REFLECT_TYPE_FLAG_INT)
	{
		return inReflectTypeDescription->traits.numeric.scalar.signedness ? ImGuiDataType_S32 : ImGuiDataType_U32;
	}
	else if (inReflectTypeDescription->type_flags & SPV_REFLECT_TYPE_FLAG_FLOAT)
	{
		uint32_t typeSize = inReflectTypeDescription->traits.numeric.scalar.width;
		if (typeSize == 32)
		{
			return ImGuiDataType_Float;
		}
		else if (typeSize == 64)
		{
			return ImGuiDataType_Double;
		}
		else
		{
			FT_FAIL("Unsupported floating point type.");
		}
	}
	else
	{
		FT_FAIL("Unsupported basic type type.");
	}
}

struct VectorDataType
{
	uint32_t ComponentCount;
	ImGuiDataType ComponentDataType;
};

VectorDataType GetVectorDataType(const SpvReflectTypeDescription* inReflectTypeDescription)
{
	// SPV_REFLECT_TYPE_FLAG_MATRIX uses this flag as well.
	const uint32_t componentCount = inReflectTypeDescription->type_flags & SPV_REFLECT_TYPE_FLAG_VECTOR ?	
		inReflectTypeDescription->traits.numeric.vector.component_count : 1;

	const ImGuiDataType componentType = GetComponentDataType(inReflectTypeDescription);
	return { componentCount , componentType };
}

float GetInputDragSpeed(const ImGuiDataType inDataType)
{
	switch (inDataType)
	{
		case ImGuiDataType_S8:
		case ImGuiDataType_U8:
		case ImGuiDataType_S16:
		case ImGuiDataType_U16:
		case ImGuiDataType_S32:
		case ImGuiDataType_U32:
		case ImGuiDataType_S64:
		case ImGuiDataType_U64:
			return 1.0f;

		case ImGuiDataType_Float:
		case ImGuiDataType_Double:
			return 0.005f;

		default:
			FT_FAIL("Unsupported ImGuiDataType.");
	}
}

void ComboWithoutPreview(int* currentItemIndex, const char** data, int items_count)
{
	if (ImGui::BeginCombo("##label", "", ImGuiComboFlags_NoPreview | ImGuiComboFlags_PopupAlignLeft))
	{
		for (int itemIndex = 0; itemIndex < items_count; ++itemIndex)
		{
			const bool isSelected = (*currentItemIndex == itemIndex);
			if (ImGui::Selectable(data[itemIndex], isSelected))
			{
				*currentItemIndex = itemIndex;
			}

			if (isSelected)
			{
				ImGui::SetItemDefaultFocus();
			}
		}

		ImGui::EndCombo();
	}
}

void UserInterface::DrawVectorInput(const SpvReflectTypeDescription* inReflectTypeDescription, unsigned char* inProxyMemory, unsigned char* inVectorState, const char* inName)
{
	ImGui::PushID(inName);

	const VectorDataType vectorDataType = GetVectorDataType(inReflectTypeDescription);
	const float dragSpeed = GetInputDragSpeed(vectorDataType.ComponentDataType);

	int* currentItemIndex = (int*)inVectorState;

	if (vectorDataType.ComponentCount == 1 && vectorDataType.ComponentDataType == ImGuiDataType_Float)
	{
		static const char* items[] = { "Constant", "Time in Seconds" };
		if (*currentItemIndex == 0)
		{
			ImGui::DragScalarN(inName, vectorDataType.ComponentDataType, inProxyMemory, vectorDataType.ComponentCount, dragSpeed);
		}
		else if (*currentItemIndex == 1)
		{
			const std::chrono::steady_clock::time_point currentTime = std::chrono::high_resolution_clock::now();
			const float elapsedTime = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - m_StartTime).count();
			float* elapsedTimeMemory = (float*) inProxyMemory;
			*elapsedTimeMemory = elapsedTime;

			ImGui::Text("Time in Seconds (%.2f s)", elapsedTime);
		}
		else
		{
			*currentItemIndex = 0;
		}

		ImGui::SameLine();
		ComboWithoutPreview(currentItemIndex, items, IM_ARRAYSIZE(items));
	}
	else if (vectorDataType.ComponentCount == 1 && (vectorDataType.ComponentDataType == ImGuiDataType_S32 || vectorDataType.ComponentDataType == ImGuiDataType_U32))
	{
		static const char* items[] =
		{
			"Constant",
			"Ctrl Key",
			"Shift Key",
			"Alt Key",
			"Left Mouse Button",
			"Right Mouse Button",
			"A Key",
			"B Key",
			"C Key",
			"D Key",
			"E Key",
			"F Key",
			"G Key",
			"H Key",
			"I Key",
			"J Key",
			"K Key",
			"L Key",
			"M Key",
			"N Key",
			"O Key",
			"P Key",
			"Q Key",
			"R Key",
			"S Key",
			"T Key",
			"U Key",
			"V Key",
			"W Key",
			"X Key",
			"Y Key",
			"Z Key"
		};

		if (*currentItemIndex == 0)
		{
			ImGui::DragScalarN(inName, vectorDataType.ComponentDataType, inProxyMemory, vectorDataType.ComponentCount, dragSpeed);
		}
		else
		{
			ImGui::Text(items[*currentItemIndex]);
			uint32_t* keyMemory = (uint32_t*)inProxyMemory;

			ImGuiIO& io = ImGui::GetIO();
			// Modifier keys.
			if (*currentItemIndex == 1)
			{
				*keyMemory = io.KeyCtrl;
			}
			else if (*currentItemIndex == 2)
			{
				*keyMemory = io.KeyShift;
			}
			else if (*currentItemIndex == 3)
			{
				*keyMemory = io.KeyAlt;
			}
			// Mouse buttons.
			else if (*currentItemIndex == 4)
			{
				*keyMemory = io.MouseDown[0];
			}
			else if (*currentItemIndex == 5)
			{
				*keyMemory = io.MouseDown[1];
			}
			// Keyboard buttons.
			else if (*currentItemIndex >= 6 && *currentItemIndex <= 31)
			{
				*keyMemory = io.KeysDown[*currentItemIndex - 6 + 'A'];
			}
			else
			{
				*currentItemIndex = 0;
			}
		}

		ImGui::SameLine();

		ComboWithoutPreview(currentItemIndex, items, IM_ARRAYSIZE(items));
	}
	else if (vectorDataType.ComponentCount == 2 && vectorDataType.ComponentDataType == ImGuiDataType_Float)
	{
		static const char* items[] = { "Constant", "Mouse Position", "Mouse Delta"};

		if (*currentItemIndex == 0)
		{
			ImGui::DragScalarN(inName, vectorDataType.ComponentDataType, inProxyMemory, vectorDataType.ComponentCount, dragSpeed);
		}
		else if (*currentItemIndex == 1)
		{
			const ImVec2 mousePosition = ImGui::GetIO().MousePos;
			ImVec2* mousePositionMemory = (ImVec2*)inProxyMemory;
			*mousePositionMemory = mousePosition;

			ImGui::Text("Mouse Position (%.1f, %.1f)", mousePosition.x, mousePosition.y);
		}
		else if (*currentItemIndex == 2)
		{
			const ImVec2 mouseDelta = ImGui::GetIO().MouseDelta;
			ImVec2* mouseDeltaMemory = (ImVec2*)inProxyMemory;
			*mouseDeltaMemory = mouseDelta;

			ImGui::Text("Mouse Delta (%.4f, %.4f)", mouseDelta.x, mouseDelta.y);
		}
		else
		{
			*currentItemIndex = 0;
		}

		ImGui::SameLine();
		ComboWithoutPreview(currentItemIndex, items, IM_ARRAYSIZE(items));
	}
	else if (vectorDataType.ComponentCount == 2 && (vectorDataType.ComponentDataType == ImGuiDataType_S32 || vectorDataType.ComponentDataType == ImGuiDataType_U32))
	{
		static const char* items[] = { "Constant", "Screen Resolution" };

		if (*currentItemIndex == 0)
		{
			ImGui::DragScalarN(inName, vectorDataType.ComponentDataType, inProxyMemory, vectorDataType.ComponentCount, dragSpeed);
		}
		else if (*currentItemIndex == 1)
		{
			int width;
			int height;

			glfwGetWindowSize(m_Application->GetWindow()->GetWindow(), &width, &height);

			uint32_t* widthMemory = (uint32_t*)inProxyMemory;
			uint32_t* heightMemory = (uint32_t*)(inProxyMemory + sizeof(uint32_t));

			*widthMemory = width;
			*heightMemory = height;

			ImGui::Text("Screen Resolution (%d, %d)", width, height);
		}
		else
		{
			*currentItemIndex = 0;
		}

		ImGui::SameLine();
		ComboWithoutPreview(currentItemIndex, items, IM_ARRAYSIZE(items));
	}
	else if (vectorDataType.ComponentCount == 3 && vectorDataType.ComponentDataType == ImGuiDataType_Float && inReflectTypeDescription->op == SpvOpTypeVector)
	{
		ImGui::ColorEdit3(inName, (float*)inProxyMemory,
			ImGuiColorEditFlags_Float |
			ImGuiColorEditFlags_HDR |
			ImGuiColorEditFlags_DisplayRGB |
			ImGuiColorEditFlags_AlphaPreview);
	}
	else if (vectorDataType.ComponentCount == 4 && vectorDataType.ComponentDataType == ImGuiDataType_Float && inReflectTypeDescription->op == SpvOpTypeVector)
	{
		ImGui::ColorEdit4(inName, (float*)inProxyMemory,
			ImGuiColorEditFlags_Float |
			ImGuiColorEditFlags_HDR |
			ImGuiColorEditFlags_DisplayRGB |
			ImGuiColorEditFlags_AlphaPreview);
	}
	else
	{
		ImGui::DragScalarN(inName, vectorDataType.ComponentDataType, inProxyMemory, vectorDataType.ComponentCount, dragSpeed);
	}

	ImGui::PopID();
}

void UserInterface::DrawStruct(const SpvReflectBlockVariable* inReflectBlock, unsigned char* inProxyMemory, unsigned char* inVectorState, const char* inName)
{
	if (ImGui::TreeNode(inName))
	{
		for (uint32_t memberIndex = 0; memberIndex < inReflectBlock->member_count; ++memberIndex)
		{
			const SpvReflectBlockVariable* memberReflectBlock = &(inReflectBlock->members[memberIndex]);
			DrawUniformBufferInput(memberReflectBlock, inProxyMemory, inVectorState);
			inProxyMemory += memberReflectBlock->padded_size;
			inVectorState += memberReflectBlock->padded_size;
		}

		ImGui::TreePop();
	}
}

void UserInterface::DrawMatrix(const SpvReflectBlockVariable* inReflectBlock, unsigned char* inProxyMemory, unsigned char* inVectorState, const char* inName)
{
	if (ImGui::TreeNode(inName))
	{
		const SpvReflectNumericTraits numericTraits = inReflectBlock->type_description->traits.numeric;

		uint32_t stride = 0;
		switch (inReflectBlock->type_description->op)
		{
		case SpvOpTypeArray:
			stride = inReflectBlock->type_description->traits.array.stride;
			break;

		case SpvOpTypeMatrix:
			stride = inReflectBlock->padded_size;
			break;

		default:
			FT_FAIL("Unsupported matrix SpvOp.");
		}

		for (uint32_t matrixRowIndex = 0; matrixRowIndex < numericTraits.matrix.row_count; ++matrixRowIndex)
		{
			std::string rowName = inName;
			rowName += "[";
			rowName += std::to_string(matrixRowIndex);;
			rowName += "]";

			DrawVectorInput(inReflectBlock->type_description, inProxyMemory, inVectorState, rowName.c_str());

			inProxyMemory += stride / numericTraits.matrix.row_count;
			inVectorState += stride / numericTraits.matrix.row_count;
		}

		ImGui::TreePop();
	}
}

void UserInterface::DrawImage(const Binding& inBinding)
{
	ImGui::PushID(inBinding.DescriptorSetBinding.binding);

	if (ImGui::Button(" Load Image "))
	{
		std::string imagePath;
		if (FileExplorer::OpenImageDialog(imagePath))
		{
			m_Renderer->WaitQueueToFinish();
			m_Renderer->UpdateImageDescriptor(inBinding.DescriptorSetBinding.binding, imagePath);
			m_Renderer->RecreateDescriptorSet();
		}
	}

	ImGui::PopID();
}

void UserInterface::DrawSampler(const SamplerInfo& inSamplerInfo, const Binding& inBinding)
{
	ImGui::PushID(inBinding.DescriptorSetBinding.binding);

	SamplerInfo newSamplerInfo{};

	{
		const static char* samplerFilters[] = { "Nearest", "Linear" };
		const static int samplerFilterSize = IM_ARRAYSIZE(samplerFilters);
		static_assert(samplerFilterSize == static_cast<int>(SamplerFilter::Count), "Update SamplerFilter names array.");

		{
			const int previousFilterMode = static_cast<int>(inSamplerInfo.MagFilter);
			int currentFilterMode = previousFilterMode;
			ImGui::Combo("Mag Filter", &currentFilterMode, samplerFilters, samplerFilterSize);
			newSamplerInfo.MagFilter = static_cast<SamplerFilter>(currentFilterMode);
		}

		{
			const int previousFilterMode = static_cast<int>(inSamplerInfo.MinFilter);
			int currentFilterMode = previousFilterMode;
			ImGui::Combo("Min Filter", &currentFilterMode, samplerFilters, samplerFilterSize);
			newSamplerInfo.MinFilter = static_cast<SamplerFilter>(currentFilterMode);
		}
	}

	ImGui::Spacing();

	{
		// TODO: This wont work for Cube maps. How does Cube map and image arrays addressing even works? Test it.
		// TODO: Also handle arrays of images (not image arrays).

		const char* samplerAddresses[] = { "Repeat", "Mirrored Repeat", "Clamp to Edge", "Clamp to Border"
#if 0 // Currently disabled since it requires VK_KHR_sampler_mirror_clamp_to_edge which is not supported on some devices.
			, "Mirror Clamp to Edge"
#endif
		};
		const static int samplerAddressesSize = IM_ARRAYSIZE(samplerAddresses);
		static_assert(samplerAddressesSize == static_cast<int>(SamplerAddressMode::Count), "Update SamplerAddressMode names array.");

		const SpvDim imageDimension = inBinding.ReflectDescriptorBinding.image.dim;

		if (imageDimension == SpvDim1D || imageDimension == SpvDim2D || imageDimension == SpvDim3D)
		{
			const int previousAddressMode = static_cast<int>(inSamplerInfo.AddressModeU);
			int currentAddressMode = previousAddressMode;
			ImGui::Combo("Address U", &currentAddressMode, samplerAddresses, samplerAddressesSize);
			newSamplerInfo.AddressModeU = static_cast<SamplerAddressMode>(currentAddressMode);
		}

		if (imageDimension == SpvDim2D || imageDimension == SpvDim3D)
		{
			const int previousAddressMode = static_cast<int>(inSamplerInfo.AddressModeV);
			int currentAddressMode = previousAddressMode;
			ImGui::Combo("Address V", &currentAddressMode, samplerAddresses, samplerAddressesSize);
			newSamplerInfo.AddressModeV = static_cast<SamplerAddressMode>(currentAddressMode);
		}

		if (imageDimension == SpvDim3D)
		{
			const int previousAddressMode = static_cast<int>(inSamplerInfo.AddressModeW);
			int currentAddressMode = previousAddressMode;
			ImGui::Combo("Address W", &currentAddressMode, samplerAddresses, samplerAddressesSize);
			newSamplerInfo.AddressModeW = static_cast<SamplerAddressMode>(currentAddressMode);
		}
	}

	if (inSamplerInfo.AddressModeU == SamplerAddressMode::ClampToBorder ||
		inSamplerInfo.AddressModeV == SamplerAddressMode::ClampToBorder ||
		inSamplerInfo.AddressModeW == SamplerAddressMode::ClampToBorder)
	{
		ImGui::Spacing();

		const char* samplerBorderColors[] = { "Transparent Black", "Opaque Black", "Opaque White" };
		const static int samplerBorderColorsSize = IM_ARRAYSIZE(samplerBorderColors);
		static_assert(samplerBorderColorsSize == static_cast<int>(SamplerBorderColor::Count), "Update SamplerBorderColor names array.");

		{
			const int previousBorderColor = static_cast<int>(inSamplerInfo.BorderColor);
			int currentBorderColor = previousBorderColor;
			ImGui::Combo("Border Color", &currentBorderColor, samplerBorderColors, samplerBorderColorsSize);
			newSamplerInfo.BorderColor = static_cast<SamplerBorderColor>(currentBorderColor);
		}
	}

	ImGui::PopID();

	if (inSamplerInfo != newSamplerInfo)
	{
		m_Renderer->WaitQueueToFinish();
		m_Renderer->UpdateSamplerDescriptor(inBinding.DescriptorSetBinding.binding, newSamplerInfo);
		m_Renderer->RecreateDescriptorSet();
	}
}

void UserInterface::DrawUniformBufferInput(const SpvReflectBlockVariable* inReflectBlock, unsigned char* inProxyMemory, unsigned char* inVectorState, const uint32_t inArrayDimension, const char* inArrayNameSuffix)
{
	if (inReflectBlock == nullptr)
	{
		return;
	}

	switch (inReflectBlock->type_description->op)
	{
	case SpvOpTypeStruct:
	{
		std::string structTreeName = inReflectBlock->name;
		structTreeName += inArrayNameSuffix;
		DrawStruct(inReflectBlock, inProxyMemory, inVectorState, structTreeName.c_str());

		break;
	}

	case SpvOpTypeArray:
	{
		const SpvReflectArrayTraits arrayTraits = inReflectBlock->type_description->traits.array;

		std::string arrayTreeName = inReflectBlock->name;
		arrayTreeName += inArrayNameSuffix;
		if (ImGui::TreeNode(arrayTreeName.c_str()))
		{
			const uint32_t arraySize = arrayTraits.dims[inArrayDimension];
			if (inArrayDimension == arrayTraits.dims_count - 1)
			{
				for (uint32_t arrayElementIndex = 0; arrayElementIndex < arraySize; ++arrayElementIndex)
				{
					std::string arrayElementName = arrayTreeName;
					arrayElementName += "[";
					arrayElementName += std::to_string(arrayElementIndex);;
					arrayElementName += "]";

					if (inReflectBlock->type_description->type_flags & SPV_REFLECT_TYPE_FLAG_STRUCT)
					{
						DrawStruct(inReflectBlock, inProxyMemory, inVectorState, arrayElementName.c_str());
						// If the outer struct which is directly bound as a uniform buffer, is an array, it's stride is 0 on GLSL for some reason.
						inProxyMemory += arrayTraits.stride == 0 ? inReflectBlock->padded_size : arrayTraits.stride;
						inVectorState += arrayTraits.stride == 0 ? inReflectBlock->padded_size : arrayTraits.stride;
					}
					else if (inReflectBlock->type_description->type_flags & SPV_REFLECT_TYPE_FLAG_MATRIX)
					{
						DrawMatrix(inReflectBlock, inProxyMemory, inVectorState, arrayElementName.c_str());
						inProxyMemory += arrayTraits.stride;
						inVectorState += arrayTraits.stride;
					}
					else
					{
						DrawVectorInput(inReflectBlock->type_description, inProxyMemory, inVectorState, arrayElementName.c_str());
						inProxyMemory += arrayTraits.stride;
						inVectorState += arrayTraits.stride;
					}
				}
			}
			else
			{
				uint32_t elementCount = 1;
				for (uint32_t arrayDimension = inArrayDimension + 1; arrayDimension < arrayTraits.dims_count; ++arrayDimension)
				{
					elementCount *= arrayTraits.dims[arrayDimension];
				}

				for (uint32_t arrayElementIndex = 0; arrayElementIndex < arraySize; ++arrayElementIndex)
				{
					std::string nameSuffix = inArrayNameSuffix;
					nameSuffix += "[";
					nameSuffix += std::to_string(arrayElementIndex);;
					nameSuffix += "]";
					DrawUniformBufferInput(inReflectBlock, inProxyMemory, inVectorState, inArrayDimension + 1, nameSuffix.c_str());

					// If the outer struct which is directly bound as a uniform buffer, is an array, it's stride is 0 on GLSL for some reason.
					inProxyMemory += elementCount * (arrayTraits.stride == 0 ? inReflectBlock->padded_size : arrayTraits.stride);
					inVectorState += elementCount * (arrayTraits.stride == 0 ? inReflectBlock->padded_size : arrayTraits.stride);
				}
			}

			ImGui::TreePop();
		}
		
		break;
	}

	case SpvOpTypeMatrix:
	{
		DrawMatrix(inReflectBlock, inProxyMemory, inVectorState, inReflectBlock->name);
		break;
	}

	default:
	{
		DrawVectorInput(inReflectBlock->type_description, inProxyMemory, inVectorState, inReflectBlock->name);
		break;
	}
	}

	ImGui::Spacing();
}

void UserInterface::ImguiBindingsWindow()
{
	static const ImVec2 DefaultWindowSize = ImVec2(400, 400);

	ImGui::Begin("BindingsMenu");

	ImGui::SetWindowFontScale(1.25f);
	ImGui::Text("Bindings");
	ImGui::SetWindowFontScale(1.0f);
	ImGui::Separator();

	auto& descriptors = m_Renderer->GetDescriptors();

	for (uint32_t descriptorIndex = 0; descriptorIndex < descriptors.size(); ++descriptorIndex)
	{
		auto& descriptor = descriptors[descriptorIndex];
		SpvReflectDescriptorBinding& reflectDescriptorBinding = descriptor.Binding.ReflectDescriptorBinding;

		// HLSL wraps uniform buffers additionally.
		if (descriptor.Resource.Type == ResourceType::UniformBuffer &&
			m_Renderer->GetFragmentShaderFile()->GetLanguage() == ShaderLanguage::HLSL &&
			reflectDescriptorBinding.block.member_count > 0)
		{
			reflectDescriptorBinding.name = reflectDescriptorBinding.type_description->type_name;
		}

		if (ImGui::CollapsingHeader(reflectDescriptorBinding.name))
		{
			ImGui::Indent();

			switch (descriptor.Resource.Type)
			{
			case ResourceType::CombinedImageSampler:
			{
				DrawImage(descriptor.Binding);

				ImGui::Spacing();
				ImGui::Separator();
				ImGui::Spacing();

				SamplerInfo samplerInfo = descriptor.Resource.Handle.CombinedImageSampler->GetSampler()->GetInfo();
				DrawSampler(samplerInfo, descriptor.Binding);

				break;
			}

			case ResourceType::Image:
			{
				DrawImage(descriptor.Binding);

				break;
			}

			case ResourceType::UniformBuffer:
			{
				SpvReflectBlockVariable* reflectBlockVariable = &reflectDescriptorBinding.block;
				if (m_Renderer->GetFragmentShaderFile()->GetLanguage() == ShaderLanguage::HLSL)
				{
					reflectBlockVariable->name = reflectBlockVariable->type_description->type_name;
				}

				ImGui::PushID(descriptor.Binding.DescriptorSetBinding.binding);
				const UniformBuffer* uniformBuffer = descriptor.Resource.Handle.UniformBuffer;
				unsigned char* proxyMemory = uniformBuffer->GetProxyMemory();
				unsigned char* vectorState = uniformBuffer->GetVectorState();
				DrawUniformBufferInput(reflectBlockVariable, proxyMemory, vectorState);
				ImGui::PopID();

				break;
			}

			case ResourceType::Sampler:
			{
				SamplerInfo samplerInfo = descriptor.Resource.Handle.Sampler->GetInfo();
				DrawSampler(samplerInfo, descriptor.Binding);

				break;
			}

			default:
				FT_FAIL("Unsupported ResourceType.");
			}

			ImGui::Unindent();
		}

		ImGui::Spacing();
	}

	ImGui::End();
}

void UserInterface::DisplayErrorMarkers(const std::string& message)
{
	char line[10];
	int j = 0;
	const uint32_t ignoreCharacterCount = 9;
	for (int i = ignoreCharacterCount; message[i] != ':'; ++i)
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
