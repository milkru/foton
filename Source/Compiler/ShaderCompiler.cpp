#include "ShaderCompiler.h"
#include "ShaderFileIncluder.h"
#include "ShaderResourceLimits.hpp"
#include "Core/Shader.h"
#include "Utility/ShaderFile.h"

FT_BEGIN_NAMESPACE

void InitializeShaderCompiler()
{
	FT_CHECK(glslang::InitializeProcess(), "Glslang not initialized properly.");
}

void FinalizeShaderCompiler()
{
	glslang::FinalizeProcess();
}

glslang::EShSource GetGlslangShaderLanguage(const ShaderLanguage inShaderLanguage)
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

EShLanguage GetGlslangShaderStage(const ShaderStage inShaderStage)
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

ShaderCompileResult CompileShader(const ShaderLanguage inLanguage, const ShaderStage inStage, const std::string& inSourceCode, const std::string inCodeEntry)
{
	const EShLanguage shaderType = GetGlslangShaderStage(inStage);
	glslang::TShader compiledShader(shaderType);
	const char* codeEntry = inCodeEntry.c_str();
	compiledShader.setEntryPoint(codeEntry);
	compiledShader.setSourceEntryPoint(codeEntry);

	const char* sourceCode = inSourceCode.c_str();
	compiledShader.setStrings(&sourceCode, 1);

	const static glslang::EShSource shaderLanguage = GetGlslangShaderLanguage(inLanguage);
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
		FT_LOG(compiledShader.getInfoLog());

		ShaderCompileResult result{};
		result.Status = ShaderCompileStatus::PreprocessingFailed;
		return result;
	}

	const char* preprocessedShaderCode = preprocessedShader.c_str();
	compiledShader.setStrings(&preprocessedShaderCode, 1);
	compiledShader.setAutoMapLocations(true);

	if (!compiledShader.parse(&builtInResource, defaultVersion, false, messages))
	{
		FT_LOG(compiledShader.getInfoLog());

		ShaderCompileResult result{};
		result.Status = ShaderCompileStatus::ParsingFailed;
		return result;
	}

	glslang::TProgram shaderProgram;
	shaderProgram.addShader(&compiledShader);

	if (!shaderProgram.link(messages))
	{
		FT_LOG(compiledShader.getInfoLog());

		ShaderCompileResult result{};
		result.Status = ShaderCompileStatus::LinkingFailed;
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

	FT_LOG(spvBuildLogger.getAllMessages().c_str());

	result.Status = ShaderCompileStatus::Success;
	return result;
}

const char* ConvertCompilationStatusToText(const ShaderCompileStatus inStatus)
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

FT_END_NAMESPACE
