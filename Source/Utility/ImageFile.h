#pragma once

FT_BEGIN_NAMESPACE

class ImageFile
{
public:
	// TODO: Since we have "All formats" thing in the file explorer, we need to filter those again here.
	explicit ImageFile(const std::string& inPath);

public:
	~ImageFile();
	FT_DELETE_COPY_AND_MOVE(ImageFile)

public:
	int GetWidth() const { return m_Width; }
	int GetHeight() const { return m_Height; }
	unsigned char* GetPixels() const { return m_Pixels; }

private:
	int m_Width;
	int m_Height;
	unsigned char* m_Pixels;
};

FT_END_NAMESPACE
