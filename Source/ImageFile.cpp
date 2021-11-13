#include "ImageFile.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace FT
{
	ImageFile::ImageFile(const std::string inPath)
	{
		m_Pixels = stbi_load(inPath.c_str(), &m_Width, &m_Height, nullptr, STBI_rgb_alpha);
		FT_CHECK(m_Pixels, "Failed to load %s image.", inPath.c_str());
	}

	ImageFile::~ImageFile()
	{
		stbi_image_free(m_Pixels);
	}
}