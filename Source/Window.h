#pragma once

class GLFWwindow;

FT_BEGIN_NAMESPACE

class Application;

class Window
{
private:
	static void KeyCallback(GLFWwindow* inWindow, int inKey, int inScanCode, int inAction, int inMods);
	static void ScrollCallback(GLFWwindow* inWindow, double inXOffset, double inYOffset);

public:
	Window(Application* inApplication);
	~Window();
	FT_DELETE_COPY_AND_MOVE(Window)

public:
	void Show();
	bool ShouldClose();
	void Close();

public:
	GLFWwindow* GetWindow() const { return m_Window; }

private:
	GLFWwindow* m_Window;
};

FT_END_NAMESPACE
