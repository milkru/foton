#include "FileExplorer.h"
#include "Utility/ShaderFile.h"

#define NFD_THROWS_EXCEPTIONS
#include <nfd.hpp>

namespace FT
{
	FileExplorer::FileExplorer()
	{
		try
		{
			m_NFDHandle = new NFD::Guard();
		}
		catch (const std::runtime_error& error)
		{
			FT_FAIL("Unable to initialize NativeFileDialog.");
		}
	}

	FileExplorer::~FileExplorer()
	{
		delete(m_NFDHandle);
	}

	std::vector<nfdfilteritem_t> GetShaderFileExtensionFilter()
	{
		static std::vector<nfdfilteritem_t> filterItems;
		if (filterItems.size() > 0)
		{
			return filterItems;
		}

		for (const auto& shaderFileExtension : SupportedShaderFileExtensions)
		{
			nfdfilteritem_t filterItem;
			filterItem.name = shaderFileExtension.Name.c_str();
			filterItem.spec = shaderFileExtension.Extension.c_str();
			filterItems.push_back(filterItem);
		}

		return filterItems;
	}

	bool FileExplorer::OpenShaderDialog(std::string& outFilePath) const
	{
		NFD::UniquePath filePath;
		const std::vector<nfdfilteritem_t> filterItems = GetShaderFileExtensionFilter();

		const nfdresult_t result = NFD::OpenDialog(filePath, filterItems.data(), filterItems.size());
		if (filePath && (result == NFD_OKAY || result == NFD_CANCEL))
		{
			outFilePath = filePath.get();
			return true;
		}
		else
		{
			FT_LOG("Opening shader file failed.");
			return false;
		}
	}

	bool FileExplorer::SaveShaderDialog(std::string& outFilePath) const
	{
		NFD::UniquePath filePath;
		const std::vector<nfdfilteritem_t> filterItems = GetShaderFileExtensionFilter();

		const nfdresult_t result = NFD::SaveDialog(filePath, filterItems.data(), filterItems.size());
		if (filePath && (result == NFD_OKAY || result == NFD_CANCEL))
		{
			outFilePath = filePath.get();
			return true;
		}
		else
		{
			FT_LOG("Saving shader file failed.");
			return false;
		}
	}
}
