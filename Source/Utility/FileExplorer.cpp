#include "FileExplorer.h"
#include "Utility/ShaderFile.h"

#define NFD_THROWS_EXCEPTIONS
#include <nfd.hpp>

FT_BEGIN_NAMESPACE

NFD::Guard* FileExplorer::s_NFDHandle = nullptr;

void FileExplorer::Initialize()
{
	try
	{
		s_NFDHandle = new NFD::Guard();
	}
	catch (const std::runtime_error& error)
	{
		FT_FAIL("Unable to initialize NativeFileDialog.");
	}
}

void FileExplorer::Terminate()
{
	delete(s_NFDHandle);
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

struct ImageFileExtension
{
	std::string Extension;
	std::string Name;
};

const ImageFileExtension SupportedImageFileExtensions[] =
{
	{ "jpg", "Joint Photographic Experts Group"}, // TODO: Check in PhotoShop how this is handled.
	{ "png", "Portable Graphics Format"}
};

std::vector<nfdfilteritem_t> GetImageFileExtensionFilter()
{
	static std::vector<nfdfilteritem_t> filterItems;
	if (filterItems.size() > 0)
	{
		return filterItems;
	}

	for (const auto& shaderFileExtension : SupportedImageFileExtensions)
	{
		nfdfilteritem_t filterItem;
		filterItem.name = shaderFileExtension.Name.c_str();
		filterItem.spec = shaderFileExtension.Extension.c_str();
		filterItems.push_back(filterItem);
	}

	return filterItems;
}

bool OpenFileDialog(const std::vector<nfdfilteritem_t>& inFilterItems, std::string& outFilePath)
{
	FT_CHECK(FileExplorer::s_NFDHandle != nullptr, "File dialog not initialized.");

	NFD::UniquePath filePath;

	const nfdresult_t result = NFD::OpenDialog(filePath, inFilterItems.data(), inFilterItems.size());
	if (filePath && (result == NFD_OKAY || result == NFD_CANCEL))
	{
		outFilePath = filePath.get();
		return true;
	}
	else
	{
		FT_LOG("Opening shader file failed.\n");
		return false;
	}
}

bool SaveFileDialog(const std::vector<nfdfilteritem_t>& inFilterItems, std::string& outFilePath)
{
	FT_CHECK(FileExplorer::s_NFDHandle != nullptr, "File dialog not initialized.");

	NFD::UniquePath filePath;

	const nfdresult_t result = NFD::SaveDialog(filePath, inFilterItems.data(), inFilterItems.size());
	if (filePath && (result == NFD_OKAY || result == NFD_CANCEL))
	{
		outFilePath = filePath.get();
		return true;
	}
	else
	{
		FT_LOG("Saving shader file failed.\n");
		return false;
	}
}

bool FileExplorer::OpenShaderDialog(std::string& outFilePath)
{
	const auto shaderFilters = GetShaderFileExtensionFilter();
	return OpenFileDialog(shaderFilters, outFilePath);
}

bool FileExplorer::SaveShaderDialog(std::string& outFilePath)
{
	const auto shaderFilters = GetShaderFileExtensionFilter();
	return SaveFileDialog(shaderFilters, outFilePath);
}

bool FileExplorer::OpenImageDialog(std::string& outFilePath)
{
	const auto imageFilters = GetImageFileExtensionFilter();
	return OpenFileDialog(imageFilters, outFilePath);
}

bool FileExplorer::SaveImageDialog(std::string& outFilePath)
{
	const auto imageFilters = GetImageFileExtensionFilter();
	return SaveFileDialog(imageFilters, outFilePath);
}

FT_END_NAMESPACE
