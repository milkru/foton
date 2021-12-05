#include "Application.h"

namespace FT
{
	std::string GetFullPath(const std::string inRelativePath)
	{
		return std::string(FT_ROOT_DIR) + inRelativePath;
	}
}
