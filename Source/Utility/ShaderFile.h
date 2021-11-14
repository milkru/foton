#pragma once

namespace FT
{
	enum class ShaderLanguage : uint8_t
	{
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

	const ShaderFileExtension SupportedShaderFileExtensions[] =
	{
		{ ShaderLanguage::GLSL, "glsl", "Graphics Library Shading Language"},
		{ ShaderLanguage::HLSL, "hlsl", "High-Level Shader Language"}
	};

	class ShaderFile
	{
	public:
		ShaderFile(const std::string inPath);

	private:
		ShaderFile(ShaderFile const&) = delete;
		ShaderFile& operator=(ShaderFile const&) = delete;

	public:
		std::string GetPath() const { return m_Path; }
		std::string GetName() const { return m_Name; }
		const std::string& GetSourceCode() const { return m_SourceCode; }
		ShaderLanguage GetLanguage() const { return m_Language; }

	protected:
		void UpdateSourceCode(const std::string& inSourceCode);

	protected:
		std::string m_Path;
		std::string m_SourceCode;
		std::string m_Name;
		ShaderLanguage m_Language;
	};
}
