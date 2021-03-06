#include "ShaderCompiler.h"
#include "ShaderFileIncluder.h"
#include "ShaderResourceLimits.hpp"
#include "Core/Shader.h"
#include "Utility/ShaderFile.h"

FT_BEGIN_NAMESPACE

namespace ShaderCompiler
{
	void Initialize()
	{
		FT_CHECK(glslang::InitializeProcess(), "Glslang not initialized properly.");
	}

	void Finalize()
	{
		glslang::FinalizeProcess();
	}

	static glslang::EShSource GetGlslangShaderLanguage(const ShaderLanguage inShaderLanguage)
	{
		switch (inShaderLanguage)
		{
		case ShaderLanguage::GLSL:
			return glslang::EShSourceGlsl;

		case ShaderLanguage::HLSL:
			return glslang::EShSourceHlsl;

		default:
			FT_FAIL("Unsupported ShaderLanguage.");
		}
	}

	static EShLanguage GetGlslangShaderStage(const ShaderStage inShaderStage)
	{
		switch (inShaderStage)
		{
		case ShaderStage::Vertex:
			return EShLangVertex;

		case ShaderStage::Fragment:
			return EShLangFragment;

		default:
			FT_FAIL("Unsupported ShaderType.");
		}
	}

	ShaderCompileResult Compile(const ShaderLanguage inLanguage, const ShaderStage inStage, const std::string& inSourceCode, const std::string inCodeEntry)
	{
		const EShLanguage shaderType = GetGlslangShaderStage(inStage);
		glslang::TShader compiledShader(shaderType);
		const char* codeEntry = inCodeEntry.c_str();
		compiledShader.setEntryPoint(codeEntry);
		compiledShader.setSourceEntryPoint(codeEntry);

		const char* sourceCode = inSourceCode.c_str();
		compiledShader.setStrings(&sourceCode, 1);

		const glslang::EShSource shaderLanguage = GetGlslangShaderLanguage(inLanguage);
		const static glslang::EShClient client = glslang::EShClientVulkan;
		const static int version = 330;
		compiledShader.setEnvInput(shaderLanguage, shaderType, client, version);

		const static glslang::EShTargetClientVersion targetClientVersion = glslang::EShTargetVulkan_1_0;
		compiledShader.setEnvClient(client, targetClientVersion);

		const static glslang::EShTargetLanguageVersion targetLanguageVersion = glslang::EShTargetSpv_1_0;
		compiledShader.setEnvTarget(glslang::EShTargetSpv, targetLanguageVersion);

		const static TBuiltInResource builtInResource = DefaultTBuiltInResource;
		const static int defaultVersion = version;

		EShMessages messages = (EShMessages)(EShMsgSpvRules | EShMsgKeepUncalled);
		std::string preprocessedShader;
		ShaderFileIncluder fileIncluder;

		if (!compiledShader.preprocess(&builtInResource, defaultVersion, ENoProfile, false, false, messages, &preprocessedShader, fileIncluder))
		{
			ShaderCompileResult result{};
			result.Status = ShaderCompileStatus::PreprocessingFailed;
			result.InfoLog = compiledShader.getInfoLog();
			return result;
		}

		const char* preprocessedShaderCode = preprocessedShader.c_str();
		compiledShader.setStrings(&preprocessedShaderCode, 1);
		compiledShader.setAutoMapLocations(true);

		if (!compiledShader.parse(&builtInResource, defaultVersion, false, messages))
		{
			ShaderCompileResult result{};
			result.Status = ShaderCompileStatus::ParsingFailed;
			result.InfoLog = compiledShader.getInfoLog();
			return result;
		}

		glslang::TProgram shaderProgram;
		shaderProgram.addShader(&compiledShader);

		if (!shaderProgram.link(messages))
		{
			ShaderCompileResult result{};
			result.Status = ShaderCompileStatus::LinkingFailed;
			result.InfoLog = compiledShader.getInfoLog();
			return result;
		}

		spv::SpvBuildLogger spvBuildLogger;

		glslang::SpvOptions spvOptions;
		spvOptions.optimizeSize = false;
		spvOptions.disableOptimizer = true;
		spvOptions.generateDebugInfo = true;
		spvOptions.validate = true;

		ShaderCompileResult result{};
		const glslang::TIntermediate* intermediate = shaderProgram.getIntermediate(shaderType);
		glslang::GlslangToSpv(*intermediate, result.SpvCode, &spvBuildLogger, &spvOptions);

		result.Status = ShaderCompileStatus::Success;
		result.InfoLog = spvBuildLogger.getAllMessages().c_str();
		return result;
	}

	const char* GetStatusText(const ShaderCompileStatus inStatus)
	{
		switch (inStatus)
		{
		case ShaderCompileStatus::Success:
			return "Success";

		case ShaderCompileStatus::PreprocessingFailed:
			return "Preprocessing";

		case ShaderCompileStatus::ParsingFailed:
			return "Parsing";

		case ShaderCompileStatus::LinkingFailed:
			return "Linking";

		default:
			FT_FAIL("Unsupported ShaderCompileStatus.");
		}
	}
}

FT_END_NAMESPACE
