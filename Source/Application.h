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
	void SaveFragmentShader();
	bool RecompileFragmentShader();
	void LoadShader(const std::string& inPath);
	void UpdateCodeFontSize(float inOffset) const;
	void ToggleUserInterface() const;

private:
	void MainLoop();
	void Cleanup();

private:
	Window* m_Window;
	Renderer* m_Renderer;
	UserInterface* m_UserInterface;
};

FT_END_NAMESPACE
