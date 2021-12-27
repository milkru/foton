#include "FilePath.h"

FT_BEGIN_NAMESPACE

std::string GetFullPath(const std::string inRelativePath)
{
	// TODO: FT_ROOT_DIR is probably wrong for .exe only?
	return std::string(FT_ROOT_DIR) + inRelativePath;
}

static std::string ExtractLastWord(const std::string inString, const char inDelimiter)
{
	const int lastDelimiterPosition = inString.find_last_of(inDelimiter);
	return inString.substr(lastDelimiterPosition + 1);
}

std::string ExtractFileExtension(const std::string inFileName)
{
	std::string fileExtension = ExtractLastWord(inFileName, '.');

	std::for_each(fileExtension.begin(), fileExtension.end(), [](char& character)
		{
			character = ::tolower(character);
		});

	return fileExtension;
}

std::string ExtractFileName(const std::string inFileName)
{
	return ExtractLastWord(inFileName, '\\');
}

FT_END_NAMESPACE
