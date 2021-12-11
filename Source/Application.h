#pragma once

FT_BEGIN_NAMESPACE

class Window;
class FileExplorer;
class Renderer;
class UserInterface;

class Application
{
public:
	Application() = default;
	FT_DELETE_COPY_AND_MOVE(Application)

public:
	void Run();

public:
	void UpdateCodeFontSize(float offset);
	void SaveFragmentShader();
	bool RecompileFragmentShader();
	void LoadShader(const std::string& inPath);
	void ToggleUserInterface();
	bool OpenShaderDialog(std::string& outFilePath) const;
	bool SaveShaderDialog(std::string& outFilePath) const;

private:
	void MainLoop();
	void Cleanup();

private:
	Window* m_Window;
	FileExplorer* m_FileExplorer;
	Renderer* m_Renderer;
	UserInterface* m_UserInterface;
};

FT_END_NAMESPACE
