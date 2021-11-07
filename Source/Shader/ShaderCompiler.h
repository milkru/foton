#pragma once

#include "ShaderCommon.hpp"

namespace FT
{
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

	extern bool InitializeShaderCompiler();
	extern void FinalizeShaderCompiler();
	extern ShaderCompileResult CompileShaderToSpv(const ShaderLanguage inLanguage, const ShaderStage inStage, const std::string inSourceCode, const std::string inCodeEntry);
}
