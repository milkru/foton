#include "Application.h"
#include "Window.h"
#include "Core/Renderer.h"
#include "UserInterface.h"
#include "Utility/ImageFile.h"
#include "Utility/FileExplorer.h"
#include "Compiler/ShaderCompiler.h"
#include "Core/Device.h"
#include "Core/Swapchain.h"
#include "Core/Shader.h"
#include "Utility/ShaderFile.h"

// TODO: Next to do ImageResource and BufferResource.
// TODO: Lightweight light fast tool (foton is small and fast :))
// TODO: Find out if we can make background for all text.
// TODO: Separate Runtime and Editor?
// TODO: When starting application sometimes new row is added at the end on current file.
// TOOD: Shader printf?
// TOOD: How resource loading with paths is going to work if we only run exe files? It's relative to project root, not the exe.
// TOOD: Use more high resolution font file for code editor.
// TOOD: Smart pointers for transient objects (pipeline, shader, shader file...)
// TOOD: Allow user to change shader entry in settings.

FT_BEGIN_NAMESPACE

void Application::Run()
{
	m_Window = new Window(this);
	m_Renderer = new Renderer(m_Window);
	m_UserInterface = new UserInterface(this, m_Window, m_Renderer);
	m_FileExplorer = new FileExplorer();
	MainLoop();
	Cleanup();
}

void Application::SaveFragmentShader()
{
	ShaderFile* fragmentShaderFile = m_Renderer->GetFragmentShaderFile();
	fragmentShaderFile->UpdateSourceCode(m_UserInterface->GetEditorText());
}

bool Application::RecompileFragmentShader()
{
	ShaderFile* fragmentShaderFile = m_Renderer->GetFragmentShaderFile();
	const std::string& fragmentShaderSourceCode = m_UserInterface->GetEditorText();

	if (fragmentShaderSourceCode.compare(fragmentShaderFile->GetSourceCode()) == 0)
	{
		return false;
	}

	const ShaderCompileResult compileResult = CompileShader(fragmentShaderFile->GetLanguage(), ShaderStage::Fragment, fragmentShaderSourceCode);

	if (compileResult.Status != ShaderCompileStatus::Success)
	{
		FT_LOG("Failed %s shader %s.\n", ConvertCompilationStatusToText(compileResult.Status), fragmentShaderFile->GetName().c_str());
		return false;
	}

	m_UserInterface->ClearErrorMarkers();

	m_Renderer->OnFragmentShaderRecompiled(compileResult.SpvCode);

	return true;
}

void Application::LoadShader(const std::string& inPath)
{
	ShaderFile* loadedShaderFile = new ShaderFile(inPath);
	const ShaderCompileResult compileResult = CompileShader(loadedShaderFile->GetLanguage(), ShaderStage::Fragment, loadedShaderFile->GetSourceCode());

	if (compileResult.Status != ShaderCompileStatus::Success)
	{
		FT_LOG("Failed %s for loaded shader %s.\n", ConvertCompilationStatusToText(compileResult.Status), loadedShaderFile->GetName().c_str());
		return;
	}

	m_Renderer->UpdateFragmentShaderFile(loadedShaderFile);
	m_UserInterface->SetEditorText(loadedShaderFile->GetSourceCode());
}

bool Application::OpenShaderDialog(std::string& outFilePath) const
{
	return m_FileExplorer->OpenShaderDialog(outFilePath);
}

bool Application::SaveShaderDialog(std::string& outFilePath) const
{
	return m_FileExplorer->SaveShaderDialog(outFilePath);
}

void Application::UpdateCodeFontSize(float inOffset) const
{
	m_UserInterface->UpdateCodeFontSize(inOffset);
}

void Application::ToggleUserInterface() const
{
	m_Renderer->ToggleUserInterface();
}

void Application::MainLoop()
{
	m_Window->Show();

	while (!m_Window->ShouldClose())
	{
		glfwPollEvents();

		if (m_Renderer->IsUserInterfaceEnabled())
		{
			m_UserInterface->ImguiNewFrame();
		}

		m_Renderer->DrawFrame();
	}

	m_Renderer->WaitDeviceToFinish();
}

void Application::Cleanup()
{
	delete(m_FileExplorer);
	delete(m_UserInterface);
	delete(m_Renderer);
	delete(m_Window);
}

FT_END_NAMESPACE
