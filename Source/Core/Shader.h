#pragma once

namespace FT
{
	enum class ShaderStage : uint8_t
	{
		Vertex,
		Fragment,

		Count
	};

	class Device;
	class ShaderFile;

	class Shader
	{
	public:
		Shader(const Device* inDevice, const ShaderStage inStage, const std::string& inCodeEntry, const std::vector<uint32_t>& inSpvCode);
		~Shader();

	private:
		Shader(Shader const&) = delete;
		Shader& operator=(Shader const&) = delete;

	public:
		VkPipelineShaderStageCreateInfo GetVkPipelineStageInfo() const;

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
