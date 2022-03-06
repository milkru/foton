#pragma once

FT_BEGIN_NAMESPACE

enum class ShaderLanguage : uint8_t
{
	None,
	GLSL,
	HLSL,

	Count
};

struct ShaderFileExtension
{
	ShaderLanguage Language = ShaderLanguage::Count;
	std::string Extension;
	std::string Name;
};

const ShaderFileExtension g_SupportedShaderFileExtensions[] =
{
	{ ShaderLanguage::GLSL, "glsl", "GLSL"},
	{ ShaderLanguage::HLSL, "hlsl", "HLSL"}
};

class ResourceContainer;

class ShaderFile
{
public:
	explicit ShaderFile(const std::string& inPath);

public:
	FT_DELETE_COPY_AND_MOVE(ShaderFile)

public:
	void UpdateSourceCode(const std::string& inSourceCode);

public:
	const std::string& GetPath() const { return m_Path; }
	const std::string& GetName() const { return m_Name; }
	const std::string& GetSourceCode() const { return m_SourceCode; }
	ShaderLanguage GetLanguage() const { return m_Language; }

private:
	std::string m_Path;
	std::string m_SourceCode;
	std::string m_Name;
	ShaderLanguage m_Language;
};

FT_END_NAMESPACE
