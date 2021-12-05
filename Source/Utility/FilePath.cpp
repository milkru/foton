#include "FilePath.h"

FT_BEGIN_NAMESPACE

std::string GetFullPath(const std::string inRelativePath)
{
	// TODO: FT_ROOT_DIR is probably wrong for .exe only?
	return std::string(FT_ROOT_DIR) + inRelativePath;
}

FT_END_NAMESPACE
