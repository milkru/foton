#pragma once

#include "ShaderCommon.hpp"

namespace FT
{
	class ShaderFile
	{
	public:
		ShaderFile(const std::string inPath);

	public:
		std::string GetPath() const { return m_Path; }
		std::string GetName() const { return m_Name; }
		const std::string& GetSourceCode() const { return m_SourceCode; }
		ShaderLanguage GetLanguage() const { return m_Language; }

	protected:
		void UpdateSourceCode(const std::string inSourceCode);

	protected:
		std::string m_Path;
		std::string m_SourceCode;
		std::string m_Name;
		ShaderLanguage m_Language;
	};
}
