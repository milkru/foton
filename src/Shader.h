#pragma once

#include "ShaderFile.h"

namespace FT
{
	class Shader : public ShaderFile
	{
	public:
		Shader(const VkDevice inDevice, const std::string inPath, const ShaderStage inStage, const std::string& inCodeEntry);
		~Shader();

	public:
		void Recompile(const std::string inSourceCode);
		VkPipelineShaderStageCreateInfo GetPipelineStageInfo() const;

	public:
		ShaderStage GetStage() const { return m_Stage; }
		std::string GetCodeEntry() const { return m_CodeEntry; }
		VkShaderModule GetModule() const { return m_Module; }
		const std::vector<VkDescriptorSetLayoutBinding>& GetBindings() const { return m_Bindings; }

	private:
		ShaderStage m_Stage;
		std::string m_CodeEntry;
		VkShaderModule m_Module;
		std::vector<VkDescriptorSetLayoutBinding> m_Bindings;
		VkDevice m_Device;
	};
}
