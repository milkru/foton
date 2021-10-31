#pragma once

namespace FT
{
	enum class ShaderLanguage : uint8_t
	{
		GLSL,
		HLSL,

		Count
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
		FileLoadingFailed,
		PreprocessingFailed,
		ParsingFailed,
		LinkingFailed,
		IncorrectByteCode,

		Count
	};

	struct CompileShaderResult
	{
		CompileShaderStatus Status;
		std::string Code;
		std::vector<uint32_t> ByteCode;
		const char* Info = nullptr;
		const char* DebugInfo = nullptr;
	};

	struct ShaderCompiler
	{
		static void Initialize();
		static void Termiante();

		static CompileShaderResult Compile(const ShaderLanguage language, const ShaderType type, const std::string source);
		static CompileShaderResult Compile(const ShaderType type, const std::string fileName);
	};
}
