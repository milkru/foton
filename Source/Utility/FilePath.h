#pragma once

FT_BEGIN_NAMESPACE

extern std::string ReadFile(std::string inPath);
extern void WriteFile(std::string inPath, const std::string& inBuffer);
extern std::string GetAbsolutePath(std::string inRelativePath);
extern std::string GetRelativePath(std::string inFullPath);
extern std::string ExtractFileExtension(const std::string inFileName);
extern std::string ExtractFileName(const std::string inFileName);

FT_END_NAMESPACE
