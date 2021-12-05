#pragma once

namespace NFD
{
	class Guard;
}

FT_BEGIN_NAMESPACE

class FileExplorer
{
public:
	FileExplorer();
	~FileExplorer();
	FT_DELETE_COPY_AND_MOVE(FileExplorer)

public:
	bool OpenShaderDialog(std::string& outFilePath) const;
	bool SaveShaderDialog(std::string& outFilePath) const;

private:
	NFD::Guard* m_NFDHandle;
};

FT_END_NAMESPACE
