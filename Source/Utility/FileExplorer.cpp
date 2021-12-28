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

static nfdfilteritem_t GetShaderFileExtensionFilter(const ShaderFileExtension& inExtension)
{
	nfdfilteritem_t filterItem;
	filterItem.name = inExtension.Name.c_str();
	filterItem.spec = inExtension.Extension.c_str();

	return filterItem;
}

static std::vector<nfdfilteritem_t> GetShaderFileExtensionFilters()
{
	std::vector<nfdfilteritem_t> filterItems;

	for (const auto& shaderFileExtension : g_SupportedShaderFileExtensions)
	{
		filterItems.push_back(GetShaderFileExtensionFilter(shaderFileExtension));
	}

	return filterItems;
}

struct ImageFileExtension
{
	std::string Extension;
	std::string Name;
};

static const ImageFileExtension SupportedImageFileExtensions[] =
{
	{ "png", "PNG"},
	{ "jpg", "JPG"},
	{ "jpeg", "JPEG"},
	{ "bmp", "BMP"},
	{ "tga", "TGA"},
	{ "psd", "PSD"}
};

static std::vector<nfdfilteritem_t> GetImageFileExtensionFilter()
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

static bool OpenFileDialog(const std::vector<nfdfilteritem_t>& inFilterItems, std::string& outFilePath)
{
	FT_CHECK(FileExplorer::s_NFDHandle != nullptr, "File dialog not initialized.");

	NFD::UniquePath filePath;

	const nfdresult_t result = NFD::OpenDialog(filePath, inFilterItems.data(), inFilterItems.size());
	if (filePath && (result == NFD_OKAY || result == NFD_CANCEL))
	{
		std::string extension = ExtractFileExtension(filePath.get());
		for (const nfdfilteritem_t& filterItem : inFilterItems)
		{
			if (std::string(filterItem.spec).compare(extension) == 0)
			{
				outFilePath = filePath.get();
				return true;
			}
		}

		FT_LOG("Tried opening a file with an unsupported format.");
	}

	return false;
}

static bool SaveFileDialog(const std::vector<nfdfilteritem_t>& inFilterItems, std::string& outFilePath)
{
	FT_CHECK(FileExplorer::s_NFDHandle != nullptr, "File dialog not initialized.");

	NFD::UniquePath filePath;

	const nfdresult_t result = NFD::SaveDialog(filePath, inFilterItems.data(), inFilterItems.size());
	if (filePath && (result == NFD_OKAY || result == NFD_CANCEL))
	{
		std::string extension = ExtractFileExtension(filePath.get());
		for (const nfdfilteritem_t& filterItem : inFilterItems)
		{
			if (std::string(filterItem.spec).compare(extension) == 0)
			{
				outFilePath = filePath.get();
				return true;
			}
		}

		FT_LOG("Tried saving a file with an unsupported format.\n");
	}
	
	return false;
}

bool FileExplorer::OpenShaderDialog(std::string& outFilePath)
{
	const auto shaderFilters = GetShaderFileExtensionFilters();
	return OpenFileDialog(shaderFilters, outFilePath);
}

bool FileExplorer::SaveShaderDialog(std::string& outFilePath)
{
	const std::vector<nfdfilteritem_t> shaderFilters = GetShaderFileExtensionFilters();
	return SaveFileDialog(shaderFilters, outFilePath);
}

bool FileExplorer::SaveShaderDialog(std::string& outFilePath, const ShaderFileExtension& inExtension)
{
	const std::vector<nfdfilteritem_t> shaderFilters = { GetShaderFileExtensionFilter(inExtension) };
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
