#pragma once

FT_BEGIN_NAMESPACE

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
	std::string InfoLog;
};

namespace ShaderCompiler
{
	extern void Initialize();
	extern void Finalize();
	extern ShaderCompileResult Compile(const ShaderLanguage inLanguage, const ShaderStage inStage, const std::string& inSourceCode, const std::string inCodeEntry = "main");
	extern const char* GetStatusText(const ShaderCompileStatus inStatus);
}

FT_END_NAMESPACE
