#include "ShaderFile.h"

FT_BEGIN_NAMESPACE

static std::string ReadFile(const std::string& inPath)
{
	FILE* file = fopen(inPath.c_str(), "r");

	if (file == nullptr)
	{
		file = fopen(inPath.c_str(), "w");
	}

	FT_CHECK(file != nullptr, "Failed opening/creating a file %s.", inPath.c_str());

	fseek(file, 0L, SEEK_END);
	const size_t fileByteCount = static_cast<size_t>(ftell(file));
	fseek(file, 0L, SEEK_SET);

	char* buffer = static_cast<char*>(alloca(fileByteCount + 1));
	const size_t bytesToRead = fread(buffer, sizeof(char), fileByteCount, file);
	fclose(file);

	buffer[bytesToRead] = '\0';

	return std::string(buffer);
}

static ShaderLanguage ExtractShaderLanguage(const std::string inFileName)
{
	const std::string fileExtension = ExtractFileExtension(inFileName);

	for (const auto& possibleFileExtension : g_SupportedShaderFileExtensions)
	{
		if (!fileExtension.compare(possibleFileExtension.Extension))
		{
			return possibleFileExtension.Language;
		}
	}

	FT_FAIL("Unsupported shader file extension.");
}

ShaderFile::ShaderFile(const std::string& inPath)
	: m_Path(inPath)
	, m_SourceCode(ReadFile(inPath))
	, m_Name(ExtractFileName(inPath))
	, m_Language(ExtractShaderLanguage(inPath)) {}

static void WriteFile(const std::string& inPath, const std::string& inBuffer)
{
	FILE* file = fopen(inPath.c_str(), "w");
	FT_CHECK(file != nullptr, "Failed opening a file %s.", inPath.c_str());

	const size_t fileByteCount = inBuffer.size();
	const size_t bytesToWrite = fwrite(inBuffer.c_str(), sizeof(char), fileByteCount, file);
	FT_CHECK(bytesToWrite == fileByteCount, "Failed writing to a file $s.", inPath.c_str());

	fclose(file);
}

void ShaderFile::UpdateSourceCode(const std::string& inSourceCode)
{
	m_SourceCode = inSourceCode;
	WriteFile(m_Path, m_SourceCode);
}

FT_END_NAMESPACE
