#pragma once

namespace FT
{
	class ImageFile
	{
	public:
		ImageFile(const std::string inPath);
		~ImageFile();

	public:
		int GetWidth() const { return m_Width; }
		int GetHeight() const { return m_Height; }
		unsigned char* GetPixels() const { return m_Pixels; }

	private:
		int m_Width;
		int m_Height;
		unsigned char* m_Pixels;
	};
}