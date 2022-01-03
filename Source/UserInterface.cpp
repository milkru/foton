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

// TODO: Customize.
static const TextEditor::Palette& GetEditorColorPalette()
{
	const static TextEditor::Palette pallete =
	{
		{
			0xff7f7f7f,	// Default
			0xffd69c56,	// Keyword	
			0xff00ff00,	// Number
			0xff7070e0,	// String
			0xff70a0e0, // Char literal
			0xffffffff, // Punctuation
			0xff408080,	// Preprocessor
			0xffaaaaaa, // Identifier
			0xff9bc64d, // Known identifier
			0xffc040a0, // Preproc identifier
			0xff206020, // Comment (single line)
			0xff406020, // Comment (multi line)
			0xff101010, // Background
			0xffe0e0e0, // Cursor
			0x80a06020, // Selection
			0x800020ff, // ErrorMarker
			0x40f08000, // Breakpoint
			0xff707000, // Line number
			0x40000000, // Current line fill
			0x40808080, // Current line fill (inactive)
			0x40a0a0a0, // Current line edge
		}
	};

	return pallete;
}

UserInterface::UserInterface(Application* inApplication)
	: m_Application(inApplication)
	, m_Renderer(inApplication->GetRenderer())
	, m_CodeFontSize(1.5f)
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

	m_Editor.SetPalette(GetEditorColorPalette());
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
	for (size_t lineIndex = 0u; lineIndex < line.size() && lineIndex < colIndex; )
	{
		if (line[lineIndex] == '\t')
		{
			distance = (1.0f + std::floor((1.0f + distance) / (float(mTabSize) * spaceSize))) * (float(mTabSize) * spaceSize);
			++lineIndex;
		}
		else
		{
			auto d = UTF8CharLength(line[lineIndex]);
			char tempCString[7];
			int i = 0;
			for (; i < 6 && d-- > 0 && lineIndex < (int)line.size(); i++, lineIndex++)
			{
				tempCString[i] = line[lineIndex];
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
	for (uint32_t lineIndex = 0; lineIndex < line.size(); )
	{
		auto c = line[lineIndex];
		if (c == '\t')
		{
			col = (col / mTabSize) * mTabSize + mTabSize;
		}
		else
		{
			++col;
		}

		lineIndex += UTF8CharLength(c);
	}

	return col;
}

void UserInterface::DrawTextBackground()
{
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));

	const std::vector<std::string> mLines = m_Editor.GetTextLines();
	const int tabSize = m_Editor.GetTabSize();

	// TODO: This value doesn't get offset if the main editor text is scrolled.
	ImVec2 cursorScreenPos = ImGui::GetCursorScreenPos();
	const float scrollY = ImGui::GetScrollY();

	const float lineSpacing = 1.0f;
	const int leftMargin = 10;

	const float fontSize = ImGui::GetFont()->CalcTextSizeA(ImGui::GetFontSize(), FLT_MAX, -1.0f, "#", nullptr, nullptr).x;
	const ImVec2 charAdvance = ImVec2(fontSize, ImGui::GetTextLineHeightWithSpacing() * lineSpacing);

	char lineMaxBuf[16];
	const size_t globalLineMax = (int)mLines.size();
	snprintf(lineMaxBuf, 16, " %zd ", globalLineMax);
	const float textStart = ImGui::GetFont()->CalcTextSizeA(ImGui::GetFontSize(), FLT_MAX, -1.0f, lineMaxBuf, nullptr, nullptr).x + leftMargin;
	const ImVec2 contentSize = ImGui::GetWindowContentRegionMax();

	int lineNumber = static_cast<int>(floor(scrollY / charAdvance.y));
	const int lineMax = std::max(0, std::min((int)mLines.size() - 1, lineNumber + static_cast<int>(floor((scrollY + contentSize.y) / charAdvance.y))));
	while (lineNumber <= lineMax)
	{
		const ImVec2 lineStartScreenPos = ImVec2(cursorScreenPos.x, cursorScreenPos.y + lineNumber * charAdvance.y);
		const ImVec2 textScreenPos = ImVec2(lineStartScreenPos.x + textStart, lineStartScreenPos.y);
		TextEditor::Coordinates lineEndCoord(lineNumber, GetLineMaxColumn(lineNumber, mLines, tabSize));

		const ImVec2 start(lineStartScreenPos.x + textStart, lineStartScreenPos.y);
		const ImVec2 end(lineStartScreenPos.x + textStart + TextDistanceToLineStart(lineEndCoord, mLines, tabSize), lineStartScreenPos.y + charAdvance.y);
		ImDrawList* drawList = ImGui::GetWindowDrawList();

		const ImU32 textBackgroundColor = 0x80000000;
		drawList->AddRectFilled(start, end, textBackgroundColor);

		++lineNumber;
	}

	ImGui::PopStyleVar();
}

void UserInterface::ImguiNewFrame()
{
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	ImguiDockSpace();

	if (m_Enable)
	{
		ImGui::ShowDemoWindow();

		const static uint32_t maxShaderFileName = 128;
		char buf[maxShaderFileName];
		ShaderFile* fragmentShaderFile = m_Renderer->GetFragmentShaderFile();
		sprintf(buf, "%s###ShaderTitle", fragmentShaderFile->GetName().c_str());

		ImGui::Begin(buf, nullptr, ImGuiWindowFlags_HorizontalScrollbar);

		ImGui::SetWindowSize(ImVec2(FT_DEFAULT_WINDOW_WIDTH, FT_DEFAULT_WINDOW_HEIGHT), ImGuiCond_FirstUseEver);

		DrawTextBackground();

		ImGui::SetNextWindowBgAlpha(0.f);
		ImGui::SetWindowFontScale(m_CodeFontSize);

		m_Editor.SetShowWhitespaces(true);
		m_Editor.Render("TextEditor");

		ImGui::End();

		ImGuiLogger::Draw("Log");

		ImguiBindingsWindow();
	}

	ImGui::Render();
}

void UserInterface::UpdateCodeFontSize(float offset)
{
	const static float MinCodeFontSize = 1.0f;
	const static float MaxCodeFontSize = 3.0f;
	const static float CodeFontSizeMul = 0.1f;

	m_CodeFontSize += offset * CodeFontSizeMul;
	m_CodeFontSize = FT_CLAMP(m_CodeFontSize, MinCodeFontSize, MaxCodeFontSize);
}

void UserInterface::SetEditorText(const std::string& inText)
{
	return m_Editor.SetText(inText);
}

// TODO: Update using https://www.khronos.org/registry/OpenGL/specs/gl/GLSLangSpec.4.60.pdf
static const TextEditor::LanguageDefinition& GetLanguageDefinitionVkGLSL()
{
	static bool initialized = false;
	static TextEditor::LanguageDefinition languageDefinition;

	if (!initialized)
	{
		static const char* const keywords[] =
		{
			"auto", "break", "case", "char", "const", "continue", "default", "do", "double", "else", "enum", "extern", "float", "for", "goto", "if", "inline", "int", "long", "register", "restrict", "return", "short",
			"signed", "sizeof", "static", "struct", "switch", "typedef", "union", "unsigned", "void", "volatile", "while", "_Alignas", "_Alignof", "_Atomic", "_Bool", "_Complex", "_Generic", "_Imaginary",
			"_Noreturn", "_Static_assert", "_Thread_local", "attribute", "uniform", "varying", "layout", "centroid", "flat", "smooth", "noperspective", "patch", "sample", "subroutine", "in", "out", "inout",
			"bool", "true", "false", "invariant", "mat2", "mat3", "mat4", "dmat2", "dmat3", "dmat4", "mat2x2", "mat2x3", "mat2x4", "dmat2x2", "dmat2x3", "dmat2x4", "mat3x2", "mat3x3", "mat3x4", "dmat3x2", "dmat3x3", "dmat3x4",
			"mat4x2", "mat4x3", "mat4x4", "dmat4x2", "dmat4x3", "dmat4x4", "vec2", "vec3", "vec4", "ivec2", "ivec3", "ivec4", "bvec2", "bvec3", "bvec4", "dvec2", "dvec3", "dvec4", "uint", "uvec2", "uvec3", "uvec4",
			"lowp", "mediump", "highp", "precision", "sampler1D", "sampler2D", "sampler3D", "samplerCube", "sampler1DShadow", "sampler2DShadow", "samplerCubeShadow", "sampler1DArray", "sampler2DArray", "sampler1DArrayShadow",
			"sampler2DArrayShadow", "isampler1D", "isampler2D", "isampler3D", "isamplerCube", "isampler1DArray", "isampler2DArray", "usampler1D", "usampler2D", "usampler3D", "usamplerCube", "usampler1DArray", "usampler2DArray",
			"sampler2DRect", "sampler2DRectShadow", "isampler2DRect", "usampler2DRect", "samplerBuffer", "isamplerBuffer", "usamplerBuffer", "sampler2DMS", "isampler2DMS", "usampler2DMS", "sampler2DMSArray", "isampler2DMSArray",
			"usampler2DMSArray", "samplerCubeArray", "samplerCubeArrayShadow", "isamplerCubeArray", "usamplerCubeArray", "shared", "writeonly", "readonly", "image2D", "image1D", "image3D"
		};

		for (const auto& keyword : keywords)
		{
			languageDefinition.mKeywords.insert(keyword);
		}

		static const char* const identifiers[] =
		{
			"abort", "abs", "acos", "asin", "atan", "atexit", "atof", "atoi", "atol", "ceil", "clock", "cosh", "ctime", "div", "exit", "fabs", "floor", "fmod", "getchar", "getenv", "isalnum", "isalpha", "isdigit", "isgraph",
			"ispunct", "isspace", "isupper", "kbhit", "log10", "log2", "log", "memcmp", "modf", "pow", "putchar", "putenv", "puts", "rand", "remove", "rename", "sinh", "sqrt", "srand", "strcat", "strcmp", "strerror", "time", "tolower", "toupper"
		};

		for (const auto& identifier : identifiers)
		{
			TextEditor::Identifier id;
			id.mDeclaration = "Built-in function";
			languageDefinition.mIdentifiers.insert(std::make_pair(std::string(identifier), id));
		}

		languageDefinition.mTokenRegexStrings.push_back(std::make_pair<std::string, TextEditor::PaletteIndex>("[ \\t]*#[ \\t]*[a-zA-Z_]+", TextEditor::PaletteIndex::Preprocessor));
		languageDefinition.mTokenRegexStrings.push_back(std::make_pair<std::string, TextEditor::PaletteIndex>("L?\\\"(\\\\.|[^\\\"])*\\\"", TextEditor::PaletteIndex::String));
		languageDefinition.mTokenRegexStrings.push_back(std::make_pair<std::string, TextEditor::PaletteIndex>("\\'\\\\?[^\\']\\'", TextEditor::PaletteIndex::CharLiteral));
		languageDefinition.mTokenRegexStrings.push_back(std::make_pair<std::string, TextEditor::PaletteIndex>("[+-]?([0-9]+([.][0-9]*)?|[.][0-9]+)([eE][+-]?[0-9]+)?[fF]?", TextEditor::PaletteIndex::Number));
		languageDefinition.mTokenRegexStrings.push_back(std::make_pair<std::string, TextEditor::PaletteIndex>("[+-]?[0-9]+[Uu]?[lL]?[lL]?", TextEditor::PaletteIndex::Number));
		languageDefinition.mTokenRegexStrings.push_back(std::make_pair<std::string, TextEditor::PaletteIndex>("0[0-7]+[Uu]?[lL]?[lL]?", TextEditor::PaletteIndex::Number));
		languageDefinition.mTokenRegexStrings.push_back(std::make_pair<std::string, TextEditor::PaletteIndex>("0[xX][0-9a-fA-F]+[uU]?[lL]?[lL]?", TextEditor::PaletteIndex::Number));
		languageDefinition.mTokenRegexStrings.push_back(std::make_pair<std::string, TextEditor::PaletteIndex>("[a-zA-Z_][a-zA-Z0-9_]*", TextEditor::PaletteIndex::Identifier));
		languageDefinition.mTokenRegexStrings.push_back(std::make_pair<std::string, TextEditor::PaletteIndex>("[\\[\\]\\{\\}\\!\\%\\^\\&\\*\\(\\)\\-\\+\\=\\~\\|\\<\\>\\?\\/\\;\\,\\.]", TextEditor::PaletteIndex::Punctuation));

		languageDefinition.mCommentStart = "/*";
		languageDefinition.mCommentEnd = "*/";
		languageDefinition.mSingleLineComment = "//";

		languageDefinition.mCaseSensitive = true;
		languageDefinition.mAutoIndentation = true;

		languageDefinition.mName = "GLSL";

		initialized = true;
	}

	return languageDefinition;
}

// TODO: Find VK HLSL Specification and update this function.
static const TextEditor::LanguageDefinition& GetLanguageDefinitionVkHLSL()
{
	static bool initialized = false;
	static TextEditor::LanguageDefinition languageDefinition;

	if (!initialized)
	{
		static const char* const keywords[] =
		{
			"AppendStructuredBuffer", "asm", "asm_fragment", "BlendState", "bool", "break", "Buffer", "ByteAddressBuffer", "case", "cbuffer", "centroid", "class", "column_major", "compile", "compile_fragment",
			"CompileShader", "const", "continue", "ComputeShader", "ConsumeStructuredBuffer", "default", "DepthStencilState", "DepthStencilView", "discard", "do", "double", "DomainShader", "dword", "else",
			"export", "extern", "false", "float", "for", "fxgroup", "GeometryShader", "groupshared", "half", "Hullshader", "if", "in", "inline", "inout", "InputPatch", "int", "interface", "line", "lineadj",
			"linear", "LineStream", "matrix", "min16float", "min10float", "min16int", "min12int", "min16uint", "namespace", "nointerpolation", "noperspective", "NULL", "out", "OutputPatch", "packoffset",
			"pass", "pixelfragment", "PixelShader", "point", "PointStream", "precise", "RasterizerState", "RenderTargetView", "return", "register", "row_major", "RWBuffer", "RWByteAddressBuffer", "RWStructuredBuffer",
			"RWTexture1D", "RWTexture1DArray", "RWTexture2D", "RWTexture2DArray", "RWTexture3D", "sample", "sampler", "SamplerState", "SamplerComparisonState", "shared", "snorm", "stateblock", "stateblock_state",
			"static", "string", "struct", "switch", "StructuredBuffer", "tbuffer", "technique", "technique10", "technique11", "texture", "Texture1D", "Texture1DArray", "Texture2D", "Texture2DArray", "Texture2DMS",
			"Texture2DMSArray", "Texture3D", "TextureCube", "TextureCubeArray", "true", "typedef", "triangle", "triangleadj", "TriangleStream", "uint", "uniform", "unorm", "unsigned", "vector", "vertexfragment",
			"VertexShader", "void", "volatile", "while",
			"bool1","bool2","bool3","bool4","double1","double2","double3","double4", "float1", "float2", "float3", "float4", "int1", "int2", "int3", "int4", "in", "out", "inout",
			"uint1", "uint2", "uint3", "uint4", "dword1", "dword2", "dword3", "dword4", "half1", "half2", "half3", "half4",
			"float1x1","float2x1","float3x1","float4x1","float1x2","float2x2","float3x2","float4x2",
			"float1x3","float2x3","float3x3","float4x3","float1x4","float2x4","float3x4","float4x4",
			"half1x1","half2x1","half3x1","half4x1","half1x2","half2x2","half3x2","half4x2",
			"half1x3","half2x3","half3x3","half4x3","half1x4","half2x4","half3x4","half4x4",
		};

		for (const auto& keyword : keywords)
		{
			languageDefinition.mKeywords.insert(keyword);
		}

		static const char* const identifiers[] =
		{
			"abort", "abs", "acos", "all", "AllMemoryBarrier", "AllMemoryBarrierWithGroupSync", "any", "asdouble", "asfloat", "asin", "asint", "asint", "asuint",
			"asuint", "atan", "atan2", "ceil", "CheckAccessFullyMapped", "clamp", "clip", "cos", "cosh", "countbits", "cross", "D3DCOLORtoUBYTE4", "ddx",
			"ddx_coarse", "ddx_fine", "ddy", "ddy_coarse", "ddy_fine", "degrees", "determinant", "DeviceMemoryBarrier", "DeviceMemoryBarrierWithGroupSync",
			"distance", "dot", "dst", "errorf", "EvaluateAttributeAtCentroid", "EvaluateAttributeAtSample", "EvaluateAttributeSnapped", "exp", "exp2",
			"f16tof32", "f32tof16", "faceforward", "firstbithigh", "firstbitlow", "floor", "fma", "fmod", "frac", "frexp", "fwidth", "GetRenderTargetSampleCount",
			"GetRenderTargetSamplePosition", "GroupMemoryBarrier", "GroupMemoryBarrierWithGroupSync", "InterlockedAdd", "InterlockedAnd", "InterlockedCompareExchange",
			"InterlockedCompareStore", "InterlockedExchange", "InterlockedMax", "InterlockedMin", "InterlockedOr", "InterlockedXor", "isfinite", "isinf", "isnan",
			"ldexp", "length", "lerp", "lit", "log", "log10", "log2", "mad", "max", "min", "modf", "msad4", "mul", "noise", "normalize", "pow", "printf",
			"Process2DQuadTessFactorsAvg", "Process2DQuadTessFactorsMax", "Process2DQuadTessFactorsMin", "ProcessIsolineTessFactors", "ProcessQuadTessFactorsAvg",
			"ProcessQuadTessFactorsMax", "ProcessQuadTessFactorsMin", "ProcessTriTessFactorsAvg", "ProcessTriTessFactorsMax", "ProcessTriTessFactorsMin",
			"radians", "rcp", "reflect", "refract", "reversebits", "round", "rsqrt", "saturate", "sign", "sin", "sincos", "sinh", "smoothstep", "sqrt", "step",
			"tan", "tanh", "tex1D", "tex1D", "tex1Dbias", "tex1Dgrad", "tex1Dlod", "tex1Dproj", "tex2D", "tex2D", "tex2Dbias", "tex2Dgrad", "tex2Dlod", "tex2Dproj",
			"tex3D", "tex3D", "tex3Dbias", "tex3Dgrad", "tex3Dlod", "tex3Dproj", "texCUBE", "texCUBE", "texCUBEbias", "texCUBEgrad", "texCUBElod", "texCUBEproj", "transpose", "trunc"
		};

		for (const auto& identifier : identifiers)
		{
			TextEditor::Identifier id;
			id.mDeclaration = "Built-in function";
			languageDefinition.mIdentifiers.insert(std::make_pair(std::string(identifier), id));
		}

		languageDefinition.mTokenRegexStrings.push_back(std::make_pair<std::string, TextEditor::PaletteIndex>("[ \\t]*#[ \\t]*[a-zA-Z_]+", TextEditor::PaletteIndex::Preprocessor));
		languageDefinition.mTokenRegexStrings.push_back(std::make_pair<std::string, TextEditor::PaletteIndex>("L?\\\"(\\\\.|[^\\\"])*\\\"", TextEditor::PaletteIndex::String));
		languageDefinition.mTokenRegexStrings.push_back(std::make_pair<std::string, TextEditor::PaletteIndex>("\\'\\\\?[^\\']\\'", TextEditor::PaletteIndex::CharLiteral));
		languageDefinition.mTokenRegexStrings.push_back(std::make_pair<std::string, TextEditor::PaletteIndex>("[+-]?([0-9]+([.][0-9]*)?|[.][0-9]+)([eE][+-]?[0-9]+)?[fF]?", TextEditor::PaletteIndex::Number));
		languageDefinition.mTokenRegexStrings.push_back(std::make_pair<std::string, TextEditor::PaletteIndex>("[+-]?[0-9]+[Uu]?[lL]?[lL]?", TextEditor::PaletteIndex::Number));
		languageDefinition.mTokenRegexStrings.push_back(std::make_pair<std::string, TextEditor::PaletteIndex>("0[0-7]+[Uu]?[lL]?[lL]?", TextEditor::PaletteIndex::Number));
		languageDefinition.mTokenRegexStrings.push_back(std::make_pair<std::string, TextEditor::PaletteIndex>("0[xX][0-9a-fA-F]+[uU]?[lL]?[lL]?", TextEditor::PaletteIndex::Number));
		languageDefinition.mTokenRegexStrings.push_back(std::make_pair<std::string, TextEditor::PaletteIndex>("[a-zA-Z_][a-zA-Z0-9_]*", TextEditor::PaletteIndex::Identifier));
		languageDefinition.mTokenRegexStrings.push_back(std::make_pair<std::string, TextEditor::PaletteIndex>("[\\[\\]\\{\\}\\!\\%\\^\\&\\*\\(\\)\\-\\+\\=\\~\\|\\<\\>\\?\\/\\;\\,\\.]", TextEditor::PaletteIndex::Punctuation));

		languageDefinition.mCommentStart = "/*";
		languageDefinition.mCommentEnd = "*/";
		languageDefinition.mSingleLineComment = "//";

		languageDefinition.mCaseSensitive = true;
		languageDefinition.mAutoIndentation = true;

		languageDefinition.mName = "HLSL";

		initialized = true;
	}

	return languageDefinition;
}

void UserInterface::SetEditorLanguage(const ShaderLanguage inLanguage)
{
	switch (inLanguage)
	{
	case ShaderLanguage::GLSL:
		m_Editor.SetLanguageDefinition(GetLanguageDefinitionVkGLSL());
		return;

	case ShaderLanguage::HLSL:
		m_Editor.SetLanguageDefinition(GetLanguageDefinitionVkHLSL());
		return;

	default:
		FT_FAIL("Unsupported ShaderLanguage.");
	}

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
			if (ImGui::MenuItem("Show Editor", "Ctrl-F"))
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

void UserInterface::DrawVectorInput(const SpvReflectTypeDescription* inReflectTypeDescription, unsigned char* inProxyMemory, const char* inName)
{
	const VectorDataType vectorDataType = GetVectorDataType(inReflectTypeDescription);
	ImGui::DragScalarN(inName, vectorDataType.ComponentDataType, inProxyMemory, vectorDataType.ComponentCount, GetInputDragSpeed(vectorDataType.ComponentDataType));
}

void UserInterface::DrawStruct(const SpvReflectBlockVariable* inReflectBlock, unsigned char* inProxyMemory, const char* inName)
{
	if (ImGui::TreeNode(inName))
	{
		for (uint32_t memberIndex = 0; memberIndex < inReflectBlock->member_count; ++memberIndex)
		{
			const SpvReflectBlockVariable* memberReflectBlock = &(inReflectBlock->members[memberIndex]);
			DrawUniformBufferInput(memberReflectBlock, inProxyMemory);
			inProxyMemory += memberReflectBlock->padded_size;
		}

		ImGui::TreePop();
	}
}

void UserInterface::DrawMatrix(const SpvReflectBlockVariable* inReflectBlock, unsigned char* inProxyMemory, const char* inName)
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

			DrawVectorInput(inReflectBlock->type_description, inProxyMemory, rowName.c_str());

			inProxyMemory += stride / numericTraits.matrix.row_count;
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

		const SpvDim imageDimension = inBinding.ReflectDescriptorBinding.image.dim;

		if (imageDimension == SpvDim1D || imageDimension == SpvDim2D || imageDimension == SpvDim3D)
		{
			const int previousAddressMode = static_cast<int>(inSamplerInfo.AddressModeU);
			int currentAddressMode = previousAddressMode;
			ImGui::Combo("Addressing Mode U", &currentAddressMode, samplerAddresses, samplerAddressesSize);
			newSamplerInfo.AddressModeU = static_cast<SamplerAddressMode>(currentAddressMode);
		}

		if (imageDimension == SpvDim2D || imageDimension == SpvDim3D)
		{
			const int previousAddressMode = static_cast<int>(inSamplerInfo.AddressModeV);
			int currentAddressMode = previousAddressMode;
			ImGui::Combo("Addressing Mode V", &currentAddressMode, samplerAddresses, samplerAddressesSize);
			newSamplerInfo.AddressModeV = static_cast<SamplerAddressMode>(currentAddressMode);
		}

		if (imageDimension == SpvDim3D)
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

void UserInterface::DrawUniformBufferInput(const SpvReflectBlockVariable* inReflectBlock, unsigned char* inProxyMemory, const uint32_t inArrayDimension, const char* inArrayNameSuffix)
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
		DrawStruct(inReflectBlock, inProxyMemory, structTreeName.c_str());

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
						DrawStruct(inReflectBlock, inProxyMemory, arrayElementName.c_str());
						// If the outer struct which is directly bound as a uniform buffer, is an array, it's stride is 0 on GLSL for some reason.
						inProxyMemory += arrayTraits.stride == 0 ? inReflectBlock->padded_size : arrayTraits.stride;
					}
					else if (inReflectBlock->type_description->type_flags & SPV_REFLECT_TYPE_FLAG_MATRIX)
					{
						DrawMatrix(inReflectBlock, inProxyMemory, arrayElementName.c_str());
						inProxyMemory += arrayTraits.stride;
					}
					else
					{
						DrawVectorInput(inReflectBlock->type_description, inProxyMemory, arrayElementName.c_str());
						inProxyMemory += arrayTraits.stride;
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
					DrawUniformBufferInput(inReflectBlock, inProxyMemory, inArrayDimension + 1, nameSuffix.c_str());

					// If the outer struct which is directly bound as a uniform buffer, is an array, it's stride is 0 on GLSL for some reason.
					inProxyMemory += elementCount * (arrayTraits.stride == 0 ? inReflectBlock->padded_size : arrayTraits.stride);
				}
			}

			ImGui::TreePop();
		}
		
		break;
	}

	case SpvOpTypeMatrix:
	{
		DrawMatrix(inReflectBlock, inProxyMemory, inReflectBlock->name);
		break;
	}

	default:
	{
		DrawVectorInput(inReflectBlock->type_description, inProxyMemory, inReflectBlock->name);
		break;
	}
	}

	ImGui::Spacing();
}

void UserInterface::ImguiBindingsWindow()
{
	static const ImVec2 DefaultWindowSize = ImVec2(400, 400);

	ImGui::Begin("Bindings");

	// TODO: Generalize and use for each window.
	ImGui::SetWindowFontScale(1.25f);

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
				// TODO: Window options: No move and No close flags should be enabled for all docked windows. see demo.
				// TODO: Implement different types of data passed to uniform buffer elements: time, keyboard inputs etc. So you can make a game in this as well.

				SpvReflectBlockVariable* reflectBlockVariable = &reflectDescriptorBinding.block;
				if (m_Renderer->GetFragmentShaderFile()->GetLanguage() == ShaderLanguage::HLSL)
				{
					reflectBlockVariable->name = reflectBlockVariable->type_description->type_name;
				}

				ImGui::PushID(descriptor.Binding.DescriptorSetBinding.binding);
				unsigned char* proxyMemory = descriptor.Resource.Handle.UniformBuffer->GetProxyMemory();
				DrawUniformBufferInput(reflectBlockVariable, proxyMemory);
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
