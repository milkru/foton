#pragma once

namespace FT
{
	enum class ShaderLanguage : uint8_t;
	enum class ShaderStage : uint8_t;

	enum class ShaderCompileStatus : uint8_t
	{
		Success,
		PreprocessingFailed,
		ParsingFailed,
		LinkingFailed,

		Count
	};

	struct ShaderCompileResult
	{
		ShaderCompileStatus Status = ShaderCompileStatus::Count;
		std::vector<uint32_t> SpvCode;
	};

	extern void InitializeShaderCompiler();
	extern void FinalizeShaderCompiler();
	extern ShaderCompileResult CompileShaderToSpv(const ShaderLanguage inLanguage, const ShaderStage inStage, const std::string inSourceCode, const std::string inCodeEntry);
}
