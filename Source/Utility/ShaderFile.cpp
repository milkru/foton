#include "ShaderFile.h"

FT_BEGIN_NAMESPACE

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

void ShaderFile::UpdateSourceCode(const std::string& inSourceCode)
{
	m_SourceCode = inSourceCode;
	WriteFile(m_Path, m_SourceCode);
}

FT_END_NAMESPACE
