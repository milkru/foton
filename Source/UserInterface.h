#pragma once

FT_BEGIN_NAMESPACE

class Application;
class Window;
class Renderer;
struct Binding;
struct SamplerInfo;

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
	void DisplayErrorMarkers(const std::string& message);
	void ClearErrorMarkers();
	void ToggleEnabled();

public:
	std::string GetEditorText() const;

private:
	void ApplyImGuiStyle();
	void ImguiMenuBar();
	void ImguiDockSpace();
	void ImguiBindingsWindow();
	void DrawTextBackground();
	void DrawVectorInput(const SpvReflectTypeDescription* inReflectTypeDescription, const char* inName);
	void DrawStruct(const SpvReflectBlockVariable* inReflectBlock, const char* inName);
	void DrawImage(const Binding& inBinding);
	void DrawSampler(const SamplerInfo& inSamplerInfo, const Binding& inBinding);
	void DrawUniformBufferInput(const SpvReflectBlockVariable* inReflectBlock, const uint32_t inArrayDimension = 0, const char* inArrayNameSuffix = "");

private:
	Application* m_Application;
	Renderer* m_Renderer;
	TextEditor m_Editor;
	VkDescriptorPool imguiDescPool;
	// TODO: Move this to config. Make foton.ini
	float codeFontSize = 1.5f;
	bool m_Enable;
};

FT_END_NAMESPACE
