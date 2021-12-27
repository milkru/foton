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

// TODO: Next to do ImageResource and BufferResource.
// TODO: Lightweight light fast tool (foton is small and fast :))
// TODO: Find out if we can make background for all text.
// TODO: Separate Runtime and Editor?
// TODO: When starting application sometimes new row is added at the end on current file.
// TODO: Shader printf?
// TODO: How resource loading with paths is going to work if we only run exe files? It's relative to project root, not the exe.
// TODO: Use more high resolution font file for code editor.
// TODO: Smart pointers for transient objects (pipeline, shader, shader file...)
// TODO: Allow user to change shader entry in settings.

FT_BEGIN_NAMESPACE

void Application::Run()
{
	FileExplorer::Initialize();

	m_Window = new Window(this);
	m_Renderer = new Renderer();
	
	bool rendererInitialized = m_Renderer->Initialize(m_Window);
	if (rendererInitialized)
	{
		m_UserInterface = new UserInterface(this);
		MainLoop();
	}
	else
	{
		m_UserInterface = nullptr;
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

	const ShaderCompileResult compileResult = CompileShader(fragmentShaderFile->GetLanguage(), ShaderStage::Fragment, fragmentShaderSourceCode);

	if (compileResult.Status != ShaderCompileStatus::Success)
	{
		FT_LOG("Failed %s shader %s.\n", ConvertCompilationStatusToText(compileResult.Status), fragmentShaderFile->GetName().c_str());
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

	RecompileFragmentShader();

	FT_LOG("New shader file %s created.\n", inPath.c_str());
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
	m_Renderer->Terminate();
	delete(m_Renderer);
	delete(m_Window);
}

FT_END_NAMESPACE
