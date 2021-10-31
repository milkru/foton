#include "shader_compiler.h"

#include <glslang/Public/ShaderLang.h>

#include <glslang_c_interface.h>
#include <StandAlone/ResourceLimits.h>

std::string ReadShaderFile(std::string fileName)
{
	FILE* file = nullptr;
	errno_t errorCode = fopen_s(&file, fileName.c_str(), "r");
	if (errorCode || file == nullptr)
	{
		return "";
	}

	fseek(file, 0L, SEEK_END);
	const auto fileBytes = ftell(file);
	fseek(file, 0L, SEEK_SET);

	char* buffer = (char*)alloca(fileBytes + 1);
	const size_t bytesToRead = fread(buffer, 1, fileBytes, file);
	fclose(file);

	buffer[bytesToRead] = '\0';

	return buffer;
}

glslang_source_t GetGlslangSourceFrom(const FT::ShaderLanguage language)
{

	switch (language)
	{
	case FT::ShaderLanguage::GLSL:
		return GLSLANG_SOURCE_GLSL;

	case FT::ShaderLanguage::HLSL:
		return GLSLANG_SOURCE_HLSL;

	default:
		FT_FAIL("Unsupported ShaderLanguage.");
		return GLSLANG_SOURCE_COUNT;
	}
}

glslang_stage_t GetGlslangStageFrom(const FT::ShaderType type)
{
	switch (type)
	{
	case FT::ShaderType::Vertex:
		return GLSLANG_STAGE_VERTEX;

	case FT::ShaderType::Fragment:
		return GLSLANG_STAGE_FRAGMENT;

	default:
		FT_FAIL("Unsupported ShaderStage.");
		return GLSLANG_STAGE_COUNT;
	}
}

FT::ShaderLanguage GetShaderLanguageFrom(std::string fileName)
{
	static const uint32_t ShaderFileExtensionSize = 5;

	std::string extension = fileName.substr(fileName.size() - ShaderFileExtensionSize);
	std::for_each(extension.begin(), extension.end(), [](char& c)
		{
			c = ::tolower(c);
		});

	if (!extension.compare(".glsl"))
	{
		return FT::ShaderLanguage::GLSL;
	}

	if (!extension.compare(".hlsl"))
	{
		return FT::ShaderLanguage::HLSL;
	}

	FT_FAIL("Unsupported %s shader file extension.", extension.c_str());
	return FT::ShaderLanguage::Count;
}

void FT::ShaderCompiler::Initialize()
{
	glslang_initialize_process();
}

void FT::ShaderCompiler::Termiante()
{
	glslang_finalize_process();
}

FT::CompileShaderResult FT::ShaderCompiler::Compile(const ShaderLanguage language, const ShaderType type, const std::string source)
{
	glslang_stage_t stage = GetGlslangStageFrom(type);
	static const int DefaultVersion = 330;

	glslang_input_t input{};
	input.language = GetGlslangSourceFrom(language);
	input.stage = stage;
	input.client = GLSLANG_CLIENT_VULKAN;
	input.client_version = GLSLANG_TARGET_VULKAN_1_0;
	input.target_language = GLSLANG_TARGET_SPV;
	input.target_language_version = GLSLANG_TARGET_SPV_1_0;
	input.code = source.c_str();
	input.default_version = DefaultVersion;
	input.default_profile = GLSLANG_NO_PROFILE;
	input.force_default_version_and_profile = false;
	input.forward_compatible = false;
	input.messages = GLSLANG_MSG_DEFAULT_BIT;
	input.resource = (const glslang_resource_t*)&glslang::DefaultTBuiltInResource;

	glslang_shader_t* shader = glslang_shader_create(&input);

	if (!glslang_shader_preprocess(shader, &input))
	{
		CompileShaderResult result{};
		result.Status = CompileShaderStatus::PreprocessingFailed;
		result.Info = glslang_shader_get_info_log(shader);
		result.DebugInfo = glslang_shader_get_info_debug_log(shader);
		return result;
	}

	if (!glslang_shader_parse(shader, &input))
	{
		CompileShaderResult result{};
		result.Status = CompileShaderStatus::ParsingFailed;
		result.Info = glslang_shader_get_info_log(shader);
		result.DebugInfo = glslang_shader_get_info_debug_log(shader);
		return result;
	}

	glslang_program_t* program = glslang_program_create();
	glslang_program_add_shader(program, shader);

	if (!glslang_program_link(program, GLSLANG_MSG_SPV_RULES_BIT | GLSLANG_MSG_VULKAN_RULES_BIT))
	{
		CompileShaderResult result{};
		result.Status = CompileShaderStatus::LinkingFailed;
		result.Info = glslang_shader_get_info_log(shader);
		result.DebugInfo = glslang_shader_get_info_debug_log(shader);
		return result;
	}

	glslang_program_SPIRV_generate(program, stage);

	CompileShaderResult result{};
	result.Code = source;

	const size_t programSize = glslang_program_SPIRV_get_size(program);
	result.ByteCode.resize(programSize);
	glslang_program_SPIRV_get(program, result.ByteCode.data());

	result.Info = glslang_program_SPIRV_get_messages(program);

	glslang_program_delete(program);
	glslang_shader_delete(shader);

	return result;
}

FT::CompileShaderResult FT::ShaderCompiler::Compile(const ShaderType type, const std::string fileName)
{
	ShaderLanguage language = GetShaderLanguageFrom(fileName);

	const std::string source = ReadShaderFile(fileName);
	if (source.empty())
	{
		CompileShaderResult result{};
		result.Status = CompileShaderStatus::FileLoadingFailed;
		return result;
	}

	return Compile(language, type, source);
}
