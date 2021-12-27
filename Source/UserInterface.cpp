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

FT_BEGIN_NAMESPACE

UserInterface::UserInterface(Application* inApplication)
	: m_Application(inApplication)
	, m_Renderer(inApplication->GetRenderer())
	, m_Enable(true)
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

	VkCommandBuffer commandBuffer = device->BeginSingleTimeCommands();
	ImGui_ImplVulkan_CreateFontsTexture(commandBuffer);
	device->EndSingleTimeCommands(commandBuffer);

	ImGui_ImplVulkan_DestroyFontUploadObjects();

	ApplyImGuiStyle();

	ShaderFile* fragmentShaderFile = m_Renderer->GetFragmentShaderFile();
	m_Editor.SetText(fragmentShaderFile->GetSourceCode());
	m_Editor.SetPalette(TextEditor::GetDarkPalette());
}

UserInterface::~UserInterface()
{
	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	Device* device = m_Renderer->GetDevice();
	vkDestroyDescriptorPool(device->GetDevice(), imguiDescPool, nullptr);
}

// https://en.wikipedia.org/wiki/UTF-8
static int UTF8CharLength(const TextEditor::Char c)
{
	if ((c & 0xFE) == 0xFC)
	{
		return 6;
	}

	if ((c & 0xFC) == 0xF8)
	{
		return 5;
	}

	if ((c & 0xF8) == 0xF0)
	{
		return 4;
	}
	else if ((c & 0xF0) == 0xE0)
	{
		return 3;
	}
	else if ((c & 0xE0) == 0xC0)
	{
		return 2;
	}

	return 1;
}

static int GetCharacterIndex(const TextEditor::Coordinates& aCoordinates, const std::vector<std::string>& mLines, const int mTabSize)
{
	if (aCoordinates.mLine >= mLines.size())
	{
		return -1;
	}

	auto& line = mLines[aCoordinates.mLine];
	int c = 0;
	int i = 0;
	for (; i < line.size() && c < aCoordinates.mColumn;)
	{
		if (line[i] == '\t')
		{
			c = (c / mTabSize) * mTabSize + mTabSize;
		}
		else
		{
			++c;
		}

		i += UTF8CharLength(line[i]);
	}

	return i;
}

static float TextDistanceToLineStart(const TextEditor::Coordinates& aFrom, const std::vector<std::string>& mLines, const int mTabSize)
{
	auto& line = mLines[aFrom.mLine];
	float distance = 0.0f;
	float spaceSize = ImGui::GetFont()->CalcTextSizeA(ImGui::GetFontSize(), FLT_MAX, -1.0f, " ", nullptr, nullptr).x;
	int colIndex = GetCharacterIndex(aFrom, mLines, mTabSize);
	for (size_t it = 0u; it < line.size() && it < colIndex; )
	{
		if (line[it] == '\t')
		{
			distance = (1.0f + std::floor((1.0f + distance) / (float(mTabSize) * spaceSize))) * (float(mTabSize) * spaceSize);
			++it;
		}
		else
		{
			auto d = UTF8CharLength(line[it]);
			char tempCString[7];
			int i = 0;
			for (; i < 6 && d-- > 0 && it < (int)line.size(); i++, it++)
			{
				tempCString[i] = line[it];
			}

			tempCString[i] = '\0';
			distance += ImGui::GetFont()->CalcTextSizeA(ImGui::GetFontSize(), FLT_MAX, -1.0f, tempCString, nullptr, nullptr).x;
		}
	}

	return distance;
}

static int GetLineMaxColumn(int aLine, const std::vector<std::string>& mLines, const int mTabSize)
{
	if (aLine >= mLines.size())
	{
		return 0;
	}

	auto& line = mLines[aLine];
	int col = 0;
	for (unsigned i = 0; i < line.size(); )
	{
		auto c = line[i];
		if (c == '\t')
		{
			col = (col / mTabSize) * mTabSize + mTabSize;
		}
		else
		{
			col++;
		}

		i += UTF8CharLength(c);
	}

	return col;
}

void UserInterface::DrawTextBackground()
{
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));

	const std::vector<std::string> mLines = m_Editor.GetTextLines();
	const int mTabSize = m_Editor.GetTabSize();

	ImVec2 cursorScreenPos = ImGui::GetCursorScreenPos();
	const float scrollX = ImGui::GetScrollX();
	const float scrollY = ImGui::GetScrollY();

	// Default unchanged values in TextEditor constructor.
	const float mLineSpacing = 1.0f;
	const int mLeftMargin = 10;

	const float fontSize = ImGui::GetFont()->CalcTextSizeA(ImGui::GetFontSize(), FLT_MAX, -1.0f, "#", nullptr, nullptr).x;
	const ImVec2 mCharAdvance = ImVec2(fontSize, ImGui::GetTextLineHeightWithSpacing() * mLineSpacing);

	char lineMaxBuf[16];
	const size_t globalLineMax = (int)mLines.size();
	snprintf(lineMaxBuf, 16, " %zd ", globalLineMax);
	const float mTextStart = ImGui::GetFont()->CalcTextSizeA(ImGui::GetFontSize(), FLT_MAX, -1.0f, lineMaxBuf, nullptr, nullptr).x + mLeftMargin;
	const ImVec2 contentSize = ImGui::GetWindowContentRegionMax();

	int lineNo = (int)floor(scrollY / mCharAdvance.y);
	const int lineMax = std::max(0, std::min((int)mLines.size() - 1, lineNo + (int)floor((scrollY + contentSize.y) / mCharAdvance.y)));
	while (lineNo <= lineMax)
	{
		ImVec2 lineStartScreenPos = ImVec2(cursorScreenPos.x, cursorScreenPos.y + lineNo * mCharAdvance.y);
		ImVec2 textScreenPos = ImVec2(lineStartScreenPos.x + mTextStart, lineStartScreenPos.y);
		TextEditor::Coordinates lineEndCoord(lineNo, GetLineMaxColumn(lineNo, mLines, mTabSize));

		ImVec2 vstart(lineStartScreenPos.x + mTextStart, lineStartScreenPos.y);
		ImVec2 vend(lineStartScreenPos.x + mTextStart + TextDistanceToLineStart(lineEndCoord, mLines, mTabSize), lineStartScreenPos.y + mCharAdvance.y);
		auto drawList = ImGui::GetWindowDrawList();

		const ImU32 textBackgroundColor = 0x80000000;
		drawList->AddRectFilled(vstart, vend, textBackgroundColor);

		++lineNo;
	}

	ImGui::PopStyleVar();
}

void UserInterface::ImguiNewFrame()
{
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	if (m_Enable)
	{
		ImguiDockSpace();

		ImGui::ShowDemoWindow();

		const static uint32_t maxShaderFileName = 128;
		char buf[maxShaderFileName];
		ShaderFile* fragmentShaderFile = m_Renderer->GetFragmentShaderFile();
		sprintf(buf, "%s###ShaderTitle", fragmentShaderFile->GetName().c_str());

		ImGui::Begin(buf, nullptr, ImGuiWindowFlags_HorizontalScrollbar);
		// TODO: Set different LanguageDefinition to editor for GLSL and HLSL.
		// TODO: Text color Palette can be changed as well.
		// TODO: More commands can be added.

		DrawTextBackground();

		ImGui::SetWindowSize(ImVec2(FT_DEFAULT_WINDOW_WIDTH, FT_DEFAULT_WINDOW_HEIGHT), ImGuiCond_FirstUseEver); // TODO: Change this!!!
		ImGui::SetNextWindowBgAlpha(0.f);
		ImGui::SetWindowFontScale(codeFontSize);
		m_Editor.SetShowWhitespaces(false); // TODO: Settings.
		m_Editor.Render("TextEditor");
		ImGui::End();

		ImGuiLogger::Draw("Log");

		ImguiBindingsWindow();
	}

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
			if (ImGui::MenuItem("Show UI", "Ctrl-F"))
			{
				ToggleEnabled();
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

// TODO: Temp.
static char* GetTempDummyMem()
{
	static char* mem = nullptr;

	if (mem == nullptr)
	{
		mem = new char[1024 * 1024]();
	}

	return mem;
}

void UserInterface::DrawVectorInput(const SpvReflectTypeDescription* inReflectTypeDescription, const char* inName)
{
	const VectorDataType vectorDataType = GetVectorDataType(inReflectTypeDescription);

	ImGui::DragScalarN(inName, vectorDataType.ComponentDataType, GetTempDummyMem(), vectorDataType.ComponentCount, GetInputDragSpeed(vectorDataType.ComponentDataType));
}

void UserInterface::DrawStruct(const SpvReflectBlockVariable* inReflectBlock, const char* inName)
{
	if (ImGui::TreeNode(inName))
	{
		for (uint32_t i = 0; i < inReflectBlock->member_count; ++i)
		{
			const SpvReflectBlockVariable* memberReflectBlock = &(inReflectBlock->members[i]);
			DrawUniformBufferInput(memberReflectBlock);
		}

		ImGui::TreePop();
	}
}

void UserInterface::DrawImage(const Binding& inBinding)
{
	ImGui::PushID(inBinding.DescriptorSetBinding.binding);

	if (ImGui::Button("Load Image"))
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
			ImGui::Combo("Magnification Filter Mode", &currentFilterMode, samplerFilters, samplerFilterSize);
			newSamplerInfo.MagFilter = static_cast<SamplerFilter>(currentFilterMode);
		}

		{
			const int previousFilterMode = static_cast<int>(inSamplerInfo.MinFilter);
			int currentFilterMode = previousFilterMode;
			ImGui::Combo("Minification Filter Mode", &currentFilterMode, samplerFilters, samplerFilterSize);
			newSamplerInfo.MinFilter = static_cast<SamplerFilter>(currentFilterMode);
		}
	}

	ImGui::Spacing();

	{
		const char* samplerAddresses[] = { "Repeat", "Mirrored Repeat", "Clamp to Edge", "Clamp to Border", "Mirror Clamp to Edge" };
		const static int samplerAddressesSize = IM_ARRAYSIZE(samplerAddresses);
		static_assert(samplerAddressesSize == static_cast<int>(SamplerAddressMode::Count), "Update SamplerAddressMode names array.");

		{
			const int previousAddressMode = static_cast<int>(inSamplerInfo.AddressModeU);
			int currentAddressMode = previousAddressMode;
			ImGui::Combo("Addressing Mode U", &currentAddressMode, samplerAddresses, samplerAddressesSize);
			newSamplerInfo.AddressModeU = static_cast<SamplerAddressMode>(currentAddressMode);
		}

		{
			const int previousAddressMode = static_cast<int>(inSamplerInfo.AddressModeV);
			int currentAddressMode = previousAddressMode;
			ImGui::Combo("Addressing Mode V", &currentAddressMode, samplerAddresses, samplerAddressesSize);
			newSamplerInfo.AddressModeV = static_cast<SamplerAddressMode>(currentAddressMode);
		}

		{
			const int previousAddressMode = static_cast<int>(inSamplerInfo.AddressModeW);
			int currentAddressMode = previousAddressMode;
			ImGui::Combo("Addressing Mode W", &currentAddressMode, samplerAddresses, samplerAddressesSize);
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

void UserInterface::DrawUniformBufferInput(const SpvReflectBlockVariable* inReflectBlock, const uint32_t inArrayDimension, const char* inArrayNameSuffix)
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
		DrawStruct(inReflectBlock, structTreeName.c_str());

		break;
	}

	case SpvOpTypeArray:
	{
		const SpvReflectArrayTraits arrayTraits = inReflectBlock->type_description->traits.array;

		std::string arrayTreeName = inReflectBlock->name;
		arrayTreeName += inArrayNameSuffix;
		if (ImGui::TreeNode(arrayTreeName.c_str()))
		{
			if (inArrayDimension == arrayTraits.dims_count - 1)
			{
				for (uint32_t arrayElementIndex = 0; arrayElementIndex < arrayTraits.dims[inArrayDimension]; ++arrayElementIndex)
				{
					std::string arrayElementName = arrayTreeName;
					arrayElementName += "[";
					arrayElementName += std::to_string(arrayElementIndex);;
					arrayElementName += "]";

					if (inReflectBlock->type_description->type_flags & SPV_REFLECT_TYPE_FLAG_STRUCT)
					{
						DrawStruct(inReflectBlock, arrayElementName.c_str());
					}
					else
					{
						DrawVectorInput(inReflectBlock->type_description, arrayElementName.c_str());
					}
				}
			}
			else
			{
				for (uint32_t arrayElementIndex = 0; arrayElementIndex < arrayTraits.dims[inArrayDimension]; ++arrayElementIndex)
				{
					std::string nameSuffix = inArrayNameSuffix;
					nameSuffix += "[";
					nameSuffix += std::to_string(arrayElementIndex);;
					nameSuffix += "]";
					DrawUniformBufferInput(inReflectBlock, inArrayDimension + 1, nameSuffix.c_str());
				}
			}

			ImGui::TreePop();
		}
		
		break;
	}

	case SpvOpTypeMatrix:
	{
		if (ImGui::TreeNode(inReflectBlock->name))
		{
			const SpvReflectNumericTraits numericTraits = inReflectBlock->type_description->traits.numeric;

			for (uint32_t matrixRowIndex = 0; matrixRowIndex < numericTraits.matrix.row_count; ++matrixRowIndex)
			{
				std::string rowName = inReflectBlock->name;
				rowName += "[";
				rowName += std::to_string(matrixRowIndex);;
				rowName += "]";
				DrawVectorInput(inReflectBlock->type_description, rowName.c_str());
			}

			ImGui::TreePop();
		}

		break;
	}

	default:
	{
		DrawVectorInput(inReflectBlock->type_description, inReflectBlock->name);
		break;
	}
	}

	ImGui::Spacing();
}

void UserInterface::ImguiBindingsWindow()
{
	static const ImVec2 DefaultWindowSize = ImVec2(400, 400);

	// TODO: Rename it to Shader Input? Or something more general? In that case we can change the shader entry point function name as well.
	ImGui::Begin("Bindings");

	// TODO: Generalize and use for each window.
	ImGui::SetWindowFontScale(1.25f);

	const auto& descriptors = m_Renderer->GetDescriptors();

	for (uint32_t descriptorIndex = 0; descriptorIndex < descriptors.size(); ++descriptorIndex)
	{
		const auto& descriptor = descriptors[descriptorIndex];
		const SpvReflectDescriptorBinding& reflectDescriptorBinding = descriptor.Binding.ReflectDescriptorBinding;

		// HLSL wraps uniform buffers additionally. TODO: Check if this is always the case.

		const char* headerName = reflectDescriptorBinding.name;
		if (descriptor.Resource.Type == ResourceType::UniformBuffer &&
			m_Renderer->GetFragmentShaderFile()->GetLanguage() == ShaderLanguage::HLSL &&
			reflectDescriptorBinding.block.member_count > 0)
		{
			headerName = reflectDescriptorBinding.block.members[0].name;
		}

		if (ImGui::CollapsingHeader(headerName))
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
				// TODO: Reset data after field data type switch for example.
				// TODO: Window options: No move and No close flags should be enabled for all docked windows. see demo.
				// TODO: IMGUI PROBABLY HAS SOME SORT OF ID WHICH COULD BE USE FOR MEMORY MAPPING OF UBO LEAF NODES!
				// TODO: Implement different types of data passed to uniform buffer elements: time, keyboard inputs etc. So you can make a game in this as well.

				const SpvReflectBlockVariable* reflectBlockVariable = &reflectDescriptorBinding.block;
				if (m_Renderer->GetFragmentShaderFile()->GetLanguage() == ShaderLanguage::HLSL)
				{
					reflectBlockVariable = reflectBlockVariable->members;
				}

				ImGui::PushID(descriptor.Binding.DescriptorSetBinding.binding);
				DrawUniformBufferInput(reflectBlockVariable);
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
