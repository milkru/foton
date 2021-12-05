#pragma once

#include "Binding.hpp"

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
		Shader(const Device* inDevice, const ShaderStage inStage, const std::vector<uint32_t>& inSpvCode, const std::string& inCodeEntry = "main");
		~Shader();

	private:
		// Note: Scott Meyers mentions in his Effective Modern
		// C++ book, that deleted functions should generally
		// be public as it results in better error messages
		// due to the compilers behavior to check accessibility
		// before deleted status

		// TODO: Prevent move too? Use macro?
		Shader(Shader const&) = delete;
		Shader& operator=(Shader const&) = delete;

	public:
		VkPipelineShaderStageCreateInfo GetVkPipelineStageInfo() const;

	public:
		ShaderStage GetStage() const { return m_Stage; }
		std::string GetCodeEntry() const { return m_CodeEntry; }
		VkShaderModule GetModule() const { return m_Module; }
		const std::vector<Binding>& GetBindings() const { return m_Bindings; }

	private:
		const Device* m_Device;
		ShaderStage m_Stage;
		std::string m_CodeEntry;
		VkShaderModule m_Module;
		std::vector<Binding> m_Bindings;
	};
}
