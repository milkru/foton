#pragma once

FT_BEGIN_NAMESPACE

class Application;
class Window;
class Renderer;

class UserInterface
{
public:
	UserInterface(Application* inApplication, class Window* inWindow, Renderer* inRenderer);
	~UserInterface();
	FT_DELETE_COPY_AND_MOVE(UserInterface)

public:
	void ImguiNewFrame();
	void UpdateCodeFontSize(float offset);
	void SetEditorText(const std::string& inText);
	void DisplayErrorMarkers(const std::string& message);
	void ClearErrorMarkers();

public:
	std::string GetEditorText() const { return m_Editor.GetText(); }

private:
	void ApplyImGuiStyle();
	void ImguiMenuBar();
	void ImguiDockSpace();
	void ImguiBindingsWindow();
	void DrawTextBackground();
	void DrawVectorInput(const SpvReflectTypeDescription* inReflectTypeDescription, const char* inName);
	void DrawStruct(const SpvReflectBlockVariable* inReflectBlock, const char* inName);
	void DrawUniformBufferInput(const SpvReflectBlockVariable* inReflectBlock, const uint32_t inArrayDimension = 0, const char* inArrayNameSuffix = "");

private:
	Application* m_Application;
	Renderer* m_Renderer;
	Window* m_Window;
	TextEditor m_Editor;
	ImGuiLogger logger;
	VkDescriptorPool imguiDescPool;
	// TODO: Move this to config. Make foton.ini
	float codeFontSize = 1.5f;
};

FT_END_NAMESPACE
