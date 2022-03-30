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

// TODO: Lightweight Light-fast tool
// TODO: Find out if we can make background for all text.
// TODO: Allow user to change shader entry in settings.
// TODO: Async file loading system.
// TODO: What about vertex shader output/fragment shader input? We only have UVs. Is that enough for generalization? Tell this or allow vertex (and maybe even geometry) shader modification as well.
// Or don't. Just use reflection on fragment shader, so you can link vertex_out with fragment_in (Interpolants). Vertex shader can have #ifdefs for different outs (or just pass a macro with binding index), which you can easily control (in order to prevent OutputNotonsumed validation error (which is also maybeok to leave)).

FT_BEGIN_NAMESPACE

struct Config
{
	std::string PreviousOpenShaderFile;
	float CodeFontSize;
	bool EnableBindingsWindow;
	bool EnableOutputWindow;
	bool ShowWhiteSpaces;
};

static const std::string ConfigFilePath = GetAbsolutePath("foton.ini");

static bool LoadConfig(Config& outConfig)
{
	std::string configJson = ReadFile(ConfigFilePath);

	if (configJson.length() == 0)
	{
		FT_LOG("Config json file doesn't exist, new one will be created %s.\n", ConfigFilePath.c_str());
		return false;
	}

	rapidjson::Document documentJson;
	documentJson.Parse(configJson.c_str());
	if (!documentJson.IsObject())
	{
		FT_LOG("Failed parsing a json document config json file %s.\n", ConfigFilePath.c_str());
		return false;
	}

	const rapidjson::Value& previousOpenShaderFileJson = documentJson["PreviousOpenShaderFile"];
	if (!previousOpenShaderFileJson.IsString())
	{
		FT_LOG("Failed parsing PreviousOpenShaderFile from config json file %s.\n", ConfigFilePath.c_str());
		return false;
	}
	outConfig.PreviousOpenShaderFile = GetAbsolutePath(previousOpenShaderFileJson.GetString());

	const rapidjson::Value& codeFontSizeJson = documentJson["CodeFontSize"];
	if (!codeFontSizeJson.IsFloat())
	{
		FT_LOG("Failed parsing CodeFontSize from config json file %s.\n", ConfigFilePath.c_str());
		return false;
	}
	outConfig.CodeFontSize = codeFontSizeJson.GetFloat();

	const rapidjson::Value& enableBindingsMenuJson = documentJson["EnableBindingsWindow"];
	if (!enableBindingsMenuJson.IsBool())
	{
		FT_LOG("Failed parsing EnableBindingsWindow from config json file %s.\n", ConfigFilePath.c_str());
		return false;
	}
	outConfig.EnableBindingsWindow = enableBindingsMenuJson.GetBool();

	const rapidjson::Value& enableOutputWindowJson = documentJson["EnableOutputWindow"];
	if (!enableOutputWindowJson.IsBool())
	{
		FT_LOG("Failed parsing EnableOutputWindow from config json file %s.\n", ConfigFilePath.c_str());
		return false;
	}
	outConfig.EnableOutputWindow = enableOutputWindowJson.GetBool();

	const rapidjson::Value& showWhiteSpacesJson = documentJson["ShowWhiteSpaces"];
	if (!showWhiteSpacesJson.IsBool())
	{
		FT_LOG("Failed parsing ShowWhiteSpaces from config json file %s.\n", ConfigFilePath.c_str());
		return false;
	}
	outConfig.ShowWhiteSpaces = showWhiteSpacesJson.GetBool();

	return true;
}

static void SaveConfig(const Config& inConfig)
{
	rapidjson::Document documentJson(rapidjson::kObjectType);

	const std::string shaderRelativePath = GetRelativePath(inConfig.PreviousOpenShaderFile);
	rapidjson::Value filePathJson(shaderRelativePath.c_str(), documentJson.GetAllocator());
	documentJson.AddMember("PreviousOpenShaderFile", filePathJson, documentJson.GetAllocator());
	documentJson.AddMember("CodeFontSize", inConfig.CodeFontSize, documentJson.GetAllocator());
	documentJson.AddMember("EnableBindingsWindow", inConfig.EnableBindingsWindow, documentJson.GetAllocator());
	documentJson.AddMember("EnableOutputWindow", inConfig.EnableOutputWindow, documentJson.GetAllocator());
	documentJson.AddMember("ShowWhiteSpaces", inConfig.ShowWhiteSpaces, documentJson.GetAllocator());

	rapidjson::StringBuffer buffer;
	rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
	documentJson.Accept(writer);

	std::string configJson = buffer.GetString();

	WriteFile(ConfigFilePath, configJson);
}

void Application::Run()
{
	FileExplorer::Initialize();
	ShaderCompiler::Initialize();

	m_Window = new Window(this);
	m_Renderer = nullptr;
	m_UserInterface = nullptr;
	
	std::string fragmentShaderPath;

	Config loadConfig{};
	const bool configSuccessfullyLoaded = LoadConfig(loadConfig);
	fragmentShaderPath = loadConfig.PreviousOpenShaderFile;

	if (configSuccessfullyLoaded || FileExplorer::SaveShaderDialog(fragmentShaderPath))
	{
		ShaderFile* fragmentShaderFile = new ShaderFile(fragmentShaderPath);

		m_Renderer = new Renderer(m_Window, fragmentShaderFile);
		m_Renderer->TryApplyMetaData();

		m_UserInterface = new UserInterface(this);
		m_UserInterface->SetEditorText(fragmentShaderFile->GetSourceCode());
		m_UserInterface->SetEditorLanguage(fragmentShaderFile->GetLanguage());

		if (configSuccessfullyLoaded)
		{
			m_UserInterface->SetEditorText(fragmentShaderFile->GetSourceCode());
		}
		else
		{
			const char* defaultFragmentShaderCode = GetDefaultFragmentShader(fragmentShaderFile->GetLanguage());
			m_UserInterface->SetEditorText(defaultFragmentShaderCode);
		}

		if (configSuccessfullyLoaded)
		{
			m_UserInterface->SetCodeFontSize(loadConfig.CodeFontSize);
			m_UserInterface->SetShowBindings(loadConfig.EnableBindingsWindow);
			m_UserInterface->SetShowOutput(loadConfig.EnableOutputWindow);
			m_UserInterface->SetShowWhiteSpaces(loadConfig.ShowWhiteSpaces);
		}

		MainLoop();

		m_Renderer->SaveMetaData();

		Config saveConfig;
		saveConfig.PreviousOpenShaderFile = m_Renderer->GetFragmentShaderFile()->GetPath();
		saveConfig.CodeFontSize = m_UserInterface->GetCodeFontSize();
		saveConfig.EnableBindingsWindow = m_UserInterface->IsShowBindings();
		saveConfig.EnableOutputWindow = m_UserInterface->IsShowOutput();
		saveConfig.ShowWhiteSpaces = m_UserInterface->IsShowWhiteSpaces();

		SaveConfig(saveConfig);
	}

	Cleanup();
}

void Application::SaveFragmentShader()
{
	ShaderFile* fragmentShaderFile = m_Renderer->GetFragmentShaderFile();
	fragmentShaderFile->UpdateSourceCode(m_UserInterface->GetEditorText());
	m_Renderer->SaveMetaData();
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

void Application::NewShader(const std::string& inPath)
{
	ShaderFile* newShaderFile = new ShaderFile(inPath);

	const char* defaultFragmentShader = GetDefaultFragmentShader(newShaderFile->GetLanguage());
	newShaderFile->UpdateSourceCode(defaultFragmentShader);

	m_Renderer->UpdateFragmentShaderFile(newShaderFile);
	m_UserInterface->SetEditorText(newShaderFile->GetSourceCode());
	m_UserInterface->SetEditorLanguage(newShaderFile->GetLanguage());

	RecompileFragmentShader();
	m_Renderer->SaveMetaData();

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

	if (!m_Renderer->TryApplyMetaData())
	{
		FT_LOG("Failed parsing meta data.");
	}

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
		NewShader(shaderFilePath);
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
		NewShader(shaderFilePath);
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
	delete(m_UserInterface);
	delete(m_Renderer);
	delete(m_Window);
	ShaderCompiler::Finalize();
	FileExplorer::Terminate();
}

FT_END_NAMESPACE
