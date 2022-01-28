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
#include "Utility/DefaultShader.h"

// TODO: Lightweight light fast tool (foton is small and fast :))
// TODO: Find out if we can make background for all text.
// TODO: Allow user to change shader entry in settings.
// TODO: Async file loading system.
// 
// TODO: What about vertex shader output/fragment shader input? We only have UVs. Is that enough for generalization? Tell this or allow vertex (and maybe even geometry) shader modification as well.
// Or don't. Just use reflection on fragment shader, so you can link vertex_out with fragment_in (Interpolants). Vertex shader can have #ifdefs for different outs (or just pass a macro with binding index), which you can easily control (in order to prevent OutputNotonsumed validation error (which is also maybeok to leave)).
// TODO: Printf

FT_BEGIN_NAMESPACE

void Application::Run()
{
	FileExplorer::Initialize();

	m_Window = new Window(this);
	m_Renderer = nullptr;
	m_UserInterface = nullptr;
	
	std::string fragmentShaderPath;
	if (FileExplorer::SaveShaderDialog(fragmentShaderPath))
	{
		ShaderFile* fragmentShaderFile = new ShaderFile(fragmentShaderPath);

		const char* defaultFragmentShaderCode = GetDefaultFragmentShader(fragmentShaderFile->GetLanguage());
		fragmentShaderFile->UpdateSourceCode(defaultFragmentShaderCode);

		m_Renderer = new Renderer(m_Window, fragmentShaderFile);
		
		m_UserInterface = new UserInterface(this);
		m_UserInterface->SetEditorText(defaultFragmentShaderCode);
		m_UserInterface->SetEditorLanguage(fragmentShaderFile->GetLanguage());

		MainLoop();
	}

	Cleanup();
}

void Application::SaveFragmentShader()
{
	ShaderFile* fragmentShaderFile = m_Renderer->GetFragmentShaderFile();
	fragmentShaderFile->UpdateSourceCode(m_UserInterface->GetEditorText());
	FT_LOG("Shader saved to file %s.\n", fragmentShaderFile->GetPath().c_str());
}

bool Application::RecompileFragmentShader()
{
	m_UserInterface->ClearErrorMarkers();

	ShaderFile* fragmentShaderFile = m_Renderer->GetFragmentShaderFile();
	const std::string& fragmentShaderSourceCode = m_UserInterface->GetEditorText();

	const ShaderCompileResult compileResult = ShaderCompiler::Compile(fragmentShaderFile->GetLanguage(), ShaderStage::Fragment, fragmentShaderSourceCode);

	if (compileResult.Status != ShaderCompileStatus::Success)
	{
		FT_LOG("Failed %s shader %s.\n", ShaderCompiler::GetStatusText(compileResult.Status), fragmentShaderFile->GetName().c_str());
		if (!compileResult.InfoLog.empty())
		{
			FT_LOG(compileResult.InfoLog.c_str());
			m_UserInterface->DisplayErrorMarkers(compileResult.InfoLog);
		}

		return false;
	}

	FT_LOG("Successfully compiled shader %s.\n", fragmentShaderFile->GetName().c_str());

	m_Renderer->OnFragmentShaderRecompiled(compileResult.SpvCode);

	return true;
}

void Application::NewShader(const std::string& inPath, const std::string& inCode)
{
	ShaderFile* newShaderFile = new ShaderFile(inPath);

	const char* defaultFragmentShader = GetDefaultFragmentShader(newShaderFile->GetLanguage());
	newShaderFile->UpdateSourceCode(defaultFragmentShader);

	m_Renderer->UpdateFragmentShaderFile(newShaderFile);
	m_UserInterface->SetEditorText(newShaderFile->GetSourceCode());
	m_UserInterface->SetEditorLanguage(newShaderFile->GetLanguage());

	RecompileFragmentShader();

	FT_LOG("New shader file %s created.\n", inPath.c_str());
}

void Application::LoadShader(const std::string& inPath)
{
	ShaderFile* loadedShaderFile = new ShaderFile(inPath);
	const ShaderCompileResult compileResult = ShaderCompiler::Compile(loadedShaderFile->GetLanguage(), ShaderStage::Fragment, loadedShaderFile->GetSourceCode());

	if (compileResult.Status != ShaderCompileStatus::Success)
	{
		FT_LOG("Failed %s for loaded shader %s.\n", ShaderCompiler::GetStatusText(compileResult.Status), loadedShaderFile->GetName().c_str());
		return;
	}

	m_Renderer->UpdateFragmentShaderFile(loadedShaderFile);
	m_UserInterface->SetEditorText(loadedShaderFile->GetSourceCode());
	m_UserInterface->SetEditorLanguage(loadedShaderFile->GetLanguage());

	RecompileFragmentShader();

	FT_LOG("Shader file %s loaded.\n", inPath.c_str());
}

void Application::UpdateCodeFontSize(float inOffset) const
{
	m_UserInterface->UpdateCodeFontSize(inOffset);
}

void Application::ToggleUserInterface() const
{
	m_UserInterface->ToggleEnabled();
}

void Application::NewShaderMenuItem()
{
	std::string shaderFilePath;
	if (FileExplorer::SaveShaderDialog(shaderFilePath))
	{
		NewShader(shaderFilePath, "");
	}
}

void Application::OpenShaderMenuItem()
{
	std::string shaderFilePath;
	if (FileExplorer::OpenShaderDialog(shaderFilePath))
	{
		LoadShader(shaderFilePath);
	}
}

void Application::SaveShaderMenuItem()
{
	ShaderFile* fragmentShaderFile = m_Renderer->GetFragmentShaderFile();
	const std::string& fragmentShaderSourceCode = m_UserInterface->GetEditorText();

	if (fragmentShaderSourceCode.compare(fragmentShaderFile->GetSourceCode()) == 0)
	{
		return;
	}

	RecompileFragmentShader();
	SaveFragmentShader();
}

void Application::SaveAsShaderMenuItem()
{
	ShaderFileExtension currentShaderFileExtension;
	ShaderLanguage currentShaderLanguage = m_Renderer->GetFragmentShaderFile()->GetLanguage();
	for (const auto& supportedShaderFileExtension : g_SupportedShaderFileExtensions)
	{
		if (supportedShaderFileExtension.Language == currentShaderLanguage)
		{
			currentShaderFileExtension = supportedShaderFileExtension;
		}
	}

	std::string shaderFilePath;
	if (FileExplorer::SaveShaderDialog(shaderFilePath, currentShaderFileExtension))
	{
		const std::string& textToSave = m_UserInterface->GetEditorText();
		NewShader(shaderFilePath, m_Renderer->GetFragmentShaderFile()->GetSourceCode());
	}
}

void Application::QuitMenuItem()
{
	m_Window->Close();
}

void Application::MainLoop()
{
	m_Window->Show();

	while (!m_Window->ShouldClose())
	{
		glfwPollEvents();

		m_UserInterface->ImguiNewFrame();

		m_Renderer->DrawFrame();
	}

	m_Renderer->WaitDeviceToFinish();
}

void Application::Cleanup()
{
	FileExplorer::Terminate();
	delete(m_UserInterface);
	delete(m_Renderer);
	delete(m_Window);
}

FT_END_NAMESPACE
