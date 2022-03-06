#pragma once

FT_BEGIN_NAMESPACE

class ImageFile
{
public:
	explicit ImageFile(const std::string& inPath);

public:
	~ImageFile();
	FT_DELETE_COPY_AND_MOVE(ImageFile)

public:
	int GetWidth() const { return m_Width; }
	int GetHeight() const { return m_Height; }
	unsigned char* GetPixels() const { return m_Pixels; }
	const std::string& GetPath() const { return m_Path; }

private:
	int m_Width;
	int m_Height;
	unsigned char* m_Pixels;
	std::string m_Path;
};

FT_END_NAMESPACE
