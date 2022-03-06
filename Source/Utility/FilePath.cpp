#include "FilePath.h"

FT_BEGIN_NAMESPACE

static void ConvertToWindowsPath(std::string& outPath)
{
	std::for_each(outPath.begin(), outPath.end(), [](char& character)
		{
			if (character == '/')
			{
				character = '\\';
			}
		});
}

static void ConvertToLinuxPath(std::string& outPath)
{
	std::for_each(outPath.begin(), outPath.end(), [](char& character)
		{
			if (character == '\\')
			{
				character = '/';
			}
		});
}

static void ConvertToPlatformPath(std::string& outPath)
{
#ifdef _WIN32                                                                              
	ConvertToWindowsPath(outPath);
#else
	ConvertToLinuxPath(outPath);
#endif // _WIN32
}

std::string ReadFile(std::string inPath)
{
	ConvertToPlatformPath(inPath);

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

void WriteFile(std::string inPath, const std::string& inBuffer)
{
	ConvertToPlatformPath(inPath);

	FILE* file = fopen(inPath.c_str(), "w");
	FT_CHECK(file != nullptr, "Failed opening a file %s.", inPath.c_str());

	const size_t fileByteCount = inBuffer.size();
	const size_t bytesToWrite = fwrite(inBuffer.c_str(), sizeof(char), fileByteCount, file);
	FT_CHECK(bytesToWrite == fileByteCount, "Failed writing to a file $s.", inPath.c_str());

	fclose(file);
}

#define STR_TOKEN "\\"
#define LAST_FOLDER "..\\"
#define FOLDER_SEP "\\"

static void TokenizePath(const char* inPath, std::vector<std::string>& outTokens)
{
	char* token = strtok(const_cast<char*>(inPath), STR_TOKEN);
	while (token != NULL)
	{
		outTokens.push_back(token);
		token = strtok(NULL, STR_TOKEN);
	}
}

static const std::string& GetRootPath()
{
	// TODO: FT_ROOT_DIR is probably wrong for .exe only?
	static std::string rootPath = std::string(FT_ROOT_DIR);
	static bool initialized = false;

	if (!initialized)
	{
		ConvertToWindowsPath(rootPath);
		initialized = true;
	}

	return rootPath;
}

static std::string GetAbsolutePath(std::string inRootPath, std::string inRelativePath)
{
	std::vector<std::string> rootPathTokens;
	std::vector<std::string> relativePathTokens;
	TokenizePath(inRootPath.c_str(), rootPathTokens);
	TokenizePath(inRelativePath.c_str(), relativePathTokens);

	auto& absolutePathTokens = rootPathTokens;

	for (const auto& token : relativePathTokens)
	{
		if (token.compare("..") == 0)
		{
			absolutePathTokens.pop_back();
		}
		else
		{
			absolutePathTokens.push_back(token);
		}
	}

	std::string absolutePath = "";
	for (uint32_t i = 0; i < absolutePathTokens.size(); ++i)
	{
		absolutePath += absolutePathTokens[i];
		if (i < (absolutePathTokens.size() - 1))
		{
			absolutePath += FOLDER_SEP;
		}
	}

	return absolutePath;
}

std::string GetAbsolutePath(std::string inRelativePath)
{
	ConvertToWindowsPath(inRelativePath);
	return GetAbsolutePath(GetRootPath(), inRelativePath);
}

static std::string GetRelativePath(std::string inRootPath, std::string inAbsolutePath)
{
	std::vector<std::string> rootPathTokens;
	std::vector<std::string> absolutePathTokens;
	TokenizePath(inRootPath.c_str(), rootPathTokens);
	TokenizePath(inAbsolutePath.c_str(), absolutePathTokens);
	
	size_t minTokenCount = (rootPathTokens.size() < absolutePathTokens.size()) ? rootPathTokens.size() : absolutePathTokens.size();
	uint32_t mutualTokenCount = 0;
	for (uint32_t i = 0; i < minTokenCount; ++i)
	{
		if (rootPathTokens[i] != absolutePathTokens[i])
		{
			mutualTokenCount = i;
			break;
		}
	}

	std::string relativePath = "";
	for (uint32_t i = 0; i < (rootPathTokens.size() - mutualTokenCount); ++i)
	{
		relativePath += LAST_FOLDER;
	}

	for (uint32_t i = mutualTokenCount; i < absolutePathTokens.size(); ++i)
	{
		relativePath += absolutePathTokens[i];
		if (i < (absolutePathTokens.size() - 1))
		{
			relativePath += FOLDER_SEP;
		}
	}

	return relativePath;
}

std::string GetRelativePath(std::string inAbsolutePath)
{
	ConvertToWindowsPath(inAbsolutePath);
	return GetRelativePath(GetRootPath(), inAbsolutePath);
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
