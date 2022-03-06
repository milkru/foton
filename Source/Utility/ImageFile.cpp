#include "ImageFile.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

FT_BEGIN_NAMESPACE

ImageFile::ImageFile(const std::string& inPath)
	: m_Path(inPath)
{
	m_Pixels = stbi_load(m_Path.c_str(), &m_Width, &m_Height, nullptr, STBI_rgb_alpha);
	FT_CHECK(m_Pixels, "Failed to load %s image.", m_Path.c_str());
}

ImageFile::~ImageFile()
{
	stbi_image_free(m_Pixels);
}

FT_END_NAMESPACE
