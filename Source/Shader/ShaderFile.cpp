#include "ShaderFile.h"
#include "ShaderCommon.hpp"

namespace FT
{
	// TOOD: I don't think it's a good idea to do error handling in these helper functions. You should be doing it at the place of function call.
	// TODO: *_s funtions are MS compiler specific. Fix that.

	struct ReadFileResult
	{
		std::string Buffer;
		bool Success = false;
	};

	std::string ReadFile(const std::string inFileName)
	{
		FILE* file = nullptr;
		errno_t errorCode = fopen_s(&file, inFileName.c_str(), "r");
		FT_CHECK(errorCode == 0, "Failed opening a file %s.", inFileName.c_str());

		fseek(file, 0L, SEEK_END);
		const size_t fileByteCount = static_cast<size_t>(ftell(file));
		fseek(file, 0L, SEEK_SET);

		char* buffer = static_cast<char*>(alloca(fileByteCount + 1));
		const size_t bytesToRead = fread(buffer, sizeof(char), fileByteCount, file);
		fclose(file);

		buffer[bytesToRead] = '\0';

		return buffer;
	}

	std::string ExtractLastWord(const std::string inString, const char inDelimiter)
	{
		const int lastDelimiterPosition = inString.find_last_of(inDelimiter);
		return inString.substr(lastDelimiterPosition + 1);
	}

	std::string ExtractFileName(const std::string inFileName)
	{
		return ExtractLastWord(inFileName, '/');
	}

	std::string ExtractShaderExtension(const std::string inFileName)
	{
		std::string fileExtension = ExtractLastWord(inFileName, '.');

		std::for_each(fileExtension.begin(), fileExtension.end(), [](char& character)
			{
				character = ::tolower(character);
			});

		return fileExtension;
	}

	ShaderLanguage ExtractShaderLanguage(const std::string inFileName)
	{
		const std::string fileExtension = ExtractShaderExtension(inFileName);

		for (const auto& possibleFileExtension : SupportedShaderFileExtensions)
		{
			if (!fileExtension.compare(possibleFileExtension.Extension))
			{
				return possibleFileExtension.Language;
			}
		}

		FT_FAIL("Unsupported shader file extension.");
	}

	ShaderFile::ShaderFile(const std::string inPath)
		: m_Path(inPath)
		, m_SourceCode(ReadFile(inPath))
		, m_Name(ExtractFileName(inPath))
		, m_Language(ExtractShaderLanguage(inPath)) {}

	void WriteFile(const std::string inFileName, const std::string inBuffer)
	{
		FILE* file = nullptr;
		errno_t errorCode = fopen_s(&file, inFileName.c_str(), "w");
		FT_CHECK(errorCode == 0, "Failed opening a file %s.", inFileName.c_str());

		const size_t fileByteCount = inBuffer.size();
		const size_t bytesToWrite = fwrite(inBuffer.c_str(), sizeof(char), fileByteCount, file);
		FT_CHECK(bytesToWrite == fileByteCount, "Failed writing to a file $s.", inFileName.c_str());

		fclose(file);
	}

	void ShaderFile::UpdateSourceCode(const std::string inSourceCode)
	{
		m_SourceCode = inSourceCode;
		WriteFile(m_Path, m_SourceCode.c_str());
	}
}
