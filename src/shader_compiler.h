#pragma once

namespace FT
{
	// TOOD: Make some order here. Move some general stuff to other files.

	enum class ShaderLanguage : uint8_t
	{
		GLSL,
		HLSL,

		Count
	};

	struct ShaderFileExtension
	{
		ShaderLanguage Language;
		std::string Extension;
		std::string Name;
	};

	const ShaderFileExtension ShaderFileExtensions[] =
	{
		{ ShaderLanguage::GLSL, "glsl", "Graphics Library Shading Language"},
		{ ShaderLanguage::HLSL, "hlsl", "High-Level Shader Language"}
	};

	enum class ShaderType : uint8_t
	{
		Vertex,
		Fragment,

		Count
	};

	enum class CompileShaderStatus : uint8_t
	{
		Success,
		PreprocessingFailed,
		ParsingFailed,
		LinkingFailed,

		Count
	};

	struct CompileShaderResult
	{
		CompileShaderStatus Status;
		std::vector<uint32_t> ByteCode;
	};

	namespace ShaderCompiler
	{
		void Initialize();
		void Termiante();

		// TODO: Is this supposed to be in a ShaderCompiler???
		std::string ReadShaderFile(std::string inFileName);
		ShaderLanguage GetShaderLanguageFromFileName(const std::string inFileName);
		// ------------------------------------------------ //

		CompileShaderResult Compile(const ShaderLanguage inShaderLanguage, const ShaderType inShaderType,
			const std::string inSourceCode, const std::string& inCodeEntry);
	}
}
