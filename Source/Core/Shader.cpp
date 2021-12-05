#include "Shader.h"
#include "Device.h"
#include "Utility/ShaderFile.h"
#include "Compiler/ShaderReflect.h"

namespace FT
{
	VkShaderStageFlagBits GetShaderStageFlag(const ShaderStage inStage)
	{
		switch (inStage)
		{
		case ShaderStage::Vertex:
			return VK_SHADER_STAGE_VERTEX_BIT;

		case ShaderStage::Fragment:
			return VK_SHADER_STAGE_FRAGMENT_BIT;

		default:
			FT_FAIL("Unsupported ShaderStage.");
		}
	}

	VkShaderModule CreateShaderModule(const VkDevice inDevice, const std::vector<uint32_t>& inSpvCode)
	{
		VkShaderModuleCreateInfo shaderModuleCreateInfo{};
		shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		shaderModuleCreateInfo.codeSize = sizeof(uint32_t) * inSpvCode.size();
		shaderModuleCreateInfo.pCode = inSpvCode.data();

		VkShaderModule shaderModule;
		FT_VK_CALL(vkCreateShaderModule(inDevice, &shaderModuleCreateInfo, nullptr, &shaderModule));

		return shaderModule;
	}

	Shader::Shader(const Device* inDevice, const ShaderStage inStage, const std::vector<uint32_t>& inSpvCode, const std::string& inCodeEntry)
		: m_Stage(inStage)
		, m_CodeEntry(inCodeEntry)
		, m_Device(inDevice)
		, m_Bindings(ReflectShader(inSpvCode, GetShaderStageFlag(m_Stage)))
		, m_Module(CreateShaderModule(m_Device->GetDevice(), inSpvCode)) {}

	void DestroyShaderModule(const VkDevice inDevice, const VkShaderModule inModule)
	{
		vkDestroyShaderModule(inDevice, inModule, nullptr);
	}

	Shader::~Shader()
	{
		DestroyShaderModule(m_Device->GetDevice(), m_Module);
	}

	VkPipelineShaderStageCreateInfo Shader::GetVkPipelineStageInfo() const
	{
		VkPipelineShaderStageCreateInfo pipelineStageCreateInfo{};
		pipelineStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		pipelineStageCreateInfo.stage = GetShaderStageFlag(m_Stage);
		pipelineStageCreateInfo.module = m_Module;
		pipelineStageCreateInfo.pName = m_CodeEntry.c_str();
		return pipelineStageCreateInfo;
	}
}
