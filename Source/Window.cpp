#include "Window.h"
#include "Application.h"
#include "Core/Renderer.h"
#include "Core/Swapchain.h"
#include "Utility/ImageFile.h"

FT_BEGIN_NAMESPACE

void Window::KeyCallback(GLFWwindow* inWindow, int inKey, int inScanCode, int inAction, int inMods)
{
	Application* application = static_cast<Application*>(glfwGetWindowUserPointer(inWindow));

	if (inKey == GLFW_KEY_N && inAction == GLFW_PRESS && inMods == GLFW_MOD_CONTROL)
	{
		application->NewShaderMenuItem();
	}

	if (inKey == GLFW_KEY_O && inAction == GLFW_PRESS && inMods == GLFW_MOD_CONTROL)
	{
		application->OpenShaderMenuItem();
	}

	if (inKey == GLFW_KEY_S && inAction == GLFW_PRESS && inMods == GLFW_MOD_CONTROL)
	{
		application->SaveShaderMenuItem();
	}

	if (inKey == GLFW_KEY_S && inAction == GLFW_PRESS && inMods & GLFW_MOD_CONTROL && inMods & GLFW_MOD_SHIFT)
	{
		application->SaveAsShaderMenuItem();
	}

	if (inKey == GLFW_KEY_R && inAction == GLFW_PRESS && inMods == GLFW_MOD_CONTROL)
	{
		application->QuitMenuItem();
	}

	if (inKey == GLFW_KEY_F && inAction == GLFW_PRESS && inMods == GLFW_MOD_CONTROL)
	{
		application->ToggleUserInterface();
	}
}

void Window::ScrollCallback(GLFWwindow* inWindow, double inXOffset, double inYOffset)
{
	Application* application = static_cast<Application*>(glfwGetWindowUserPointer(inWindow));

	if (glfwGetKey(inWindow, GLFW_KEY_LEFT_CONTROL) || glfwGetKey(inWindow, GLFW_KEY_RIGHT_CONTROL))
	{
		application->UpdateCodeFontSize(static_cast<float>(inYOffset));
	}
}

Window::Window(Application* inApplication)
{
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);
	glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

	// TODO: Controversial.
	//glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);

	m_Window = glfwCreateWindow(FT_DEFAULT_WINDOW_WIDTH, FT_DEFAULT_WINDOW_HEIGHT, FT_APPLICATION_NAME, nullptr, nullptr);
	glfwSetWindowUserPointer(m_Window, inApplication);

	glfwSetFramebufferSizeCallback(m_Window, Swapchain::FramebufferResized);
	glfwSetKeyCallback(m_Window, KeyCallback);
	glfwSetScrollCallback(m_Window, ScrollCallback);

	const ImageFile iconImage(GetFullPath("icon"));

	GLFWimage icon;
	icon.width = iconImage.GetWidth();
	icon.height = iconImage.GetHeight();
	icon.pixels = iconImage.GetPixels();

	glfwSetWindowIcon(m_Window, 1, &icon);
}

Window::~Window()
{
	glfwDestroyWindow(m_Window);
	glfwTerminate();
}

void Window::Show()
{
	glfwShowWindow(m_Window);
}

bool Window::ShouldClose()
{
	return glfwWindowShouldClose(m_Window);
}

void Window::Close()
{
	glfwSetWindowShouldClose(m_Window, GLFW_TRUE);
}

FT_END_NAMESPACE
