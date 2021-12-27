#pragma once

namespace NFD
{
	class Guard;
}

FT_BEGIN_NAMESPACE

struct ShaderFileExtension;

class FileExplorer
{
public:
	static void Initialize();
	static void Terminate();
	static NFD::Guard* s_NFDHandle;

public:
	FileExplorer() = default;
	FT_DELETE_COPY_AND_MOVE(FileExplorer)

public:
	static bool OpenShaderDialog(std::string& outFilePath);
	static bool SaveShaderDialog(std::string& outFilePath);
	static bool SaveShaderDialog(std::string& outFilePath, const ShaderFileExtension& inExtension);

public:
	static bool OpenImageDialog(std::string& outFilePath);
	static bool SaveImageDialog(std::string& outFilePath);
};

FT_END_NAMESPACE
