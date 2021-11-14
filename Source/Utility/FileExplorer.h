#pragma once

namespace NFD
{
	class Guard;
}

namespace FT
{
	class FileExplorer
	{
	public:
		FileExplorer();
		~FileExplorer();

	private:
		FileExplorer(FileExplorer const&) = delete;
		FileExplorer& operator=(FileExplorer const&) = delete;

	public:
		bool OpenShaderDialog(std::string& outFilePath) const;
		bool SaveShaderDialog(std::string& outFilePath) const;

	private:
		NFD::Guard* m_NFDHandle;
	};
}
