#pragma once

#include "Utility/ShaderFile.h"

namespace FT
{
	enum class ShaderStage : uint8_t
	{
		Vertex,
		Fragment,

		Count
	};

	class Device;

	class Shader : public ShaderFile
	{
	public:
		Shader(const Device* inDevice, const std::string inPath, const ShaderStage inStage, const std::string& inCodeEntry);
		~Shader();

	private:
		Shader(Shader const&) = delete;
		Shader& operator=(Shader const&) = delete;

	public:
		void Recompile(const std::string& inSourceCode);
		VkPipelineShaderStageCreateInfo GetPipelineStageInfo() const;

	public:
		ShaderStage GetStage() const { return m_Stage; }
		std::string GetCodeEntry() const { return m_CodeEntry; }
		VkShaderModule GetModule() const { return m_Module; }
		const std::vector<VkDescriptorSetLayoutBinding>& GetBindings() const { return m_Bindings; }

	private:
		const Device* m_Device;
		ShaderStage m_Stage;
		std::string m_CodeEntry;
		VkShaderModule m_Module;
		std::vector<VkDescriptorSetLayoutBinding> m_Bindings;
	};
}
