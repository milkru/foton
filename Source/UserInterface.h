#pragma once

FT_BEGIN_NAMESPACE

class Application;
class Window;
class Renderer;
struct Descriptor;
struct SamplerInfo;
enum class ShaderLanguage : uint8_t;

class UserInterface
{
public:
	UserInterface(Application* inApplication);
	~UserInterface();
	FT_DELETE_COPY_AND_MOVE(UserInterface)

public:
	void ImguiNewFrame();
	void UpdateCodeFontSize(float offset);
	void SetEditorText(const std::string& inText);
	void SetEditorLanguage(const ShaderLanguage inLanguage);
	void SetCodeFontSize(const float inCodeFontSize);
	void SetShowBindings(const bool inShowBindings);
	void SetShowOutput(const bool inShowOutput);
	void SetShowWhiteSpaces(const bool inShowWhiteSpaces);
	void DisplayErrorMarkers(const std::string& message);
	void ClearErrorMarkers();
	void ToggleEnabled();

public:
	std::string GetEditorText() const;
	float GetCodeFontSize() const { return m_CodeFontSize; }
	bool IsShowBindings() const { return m_ShowBindings; }
	bool IsShowOutput() const { return m_ShowOutput; }
	bool IsShowWhiteSpaces() const { return m_ShowWhiteSpaces; }

private:
	void ApplyImGuiStyle();
	void ImguiShowInfo();
	void ImguiMenuBar();
	void ImguiDockSpace();
	void ImguiBindingsWindow();
	void DrawVectorInput(const SpvReflectTypeDescription* inReflectTypeDescription, unsigned char* inProxyMemory, unsigned char* inVectorState, const char* inName, bool inDraw);
	void DrawStruct(const SpvReflectBlockVariable* inReflectBlock, unsigned char* inProxyMemory, unsigned char* inVectorState, const char* inName, bool inDraw);
	void DrawMatrix(const SpvReflectBlockVariable* inReflectBlock, unsigned char* inProxyMemory, unsigned char* inVectorState, const char* inName, bool inDraw);
	void DrawImage(const Descriptor& inDescriptor, bool inDraw);
	void DrawSampler(const SamplerInfo& inSamplerInfo, const Descriptor& inDescriptor, bool inDraw);
	void DrawUniformBufferInput(const SpvReflectBlockVariable* inReflectBlock, unsigned char* inProxyMemory, unsigned char* inVectorState, bool inDraw, const uint32_t inArrayDimension = 0, const char* inArrayNameSuffix = "");

private:
	Application* m_Application;
	Renderer* m_Renderer;
	TextEditor m_Editor;
	VkDescriptorPool imguiDescPool;
	// TODO: Move this to config. Make foton.ini
	float m_CodeFontSize;
	bool m_Enable;
	bool m_ShowBindings;
	bool m_ShowOutput;
	bool m_ShowWhiteSpaces;
	std::chrono::steady_clock::time_point m_StartTime;
};

FT_END_NAMESPACE
