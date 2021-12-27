#pragma once

FT_BEGIN_NAMESPACE

extern std::string GetFullPath(const std::string inRelativePath);
extern std::string ExtractFileExtension(const std::string inFileName);
extern std::string ExtractFileName(const std::string inFileName);

FT_END_NAMESPACE
