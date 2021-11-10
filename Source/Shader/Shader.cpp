#include "Shader.h"
#include "ShaderCompiler.h"
#include "ShaderReflect.h"

namespace FT
{
	const char* ConvertCompilationStatusToText(const ShaderCompileStatus inStatus)
	{
		switch (inStatus)
		{
		case ShaderCompileStatus::Success:
			return "Success";

		case ShaderCompileStatus::PreprocessingFailed:
			return "Preprocessing";

		case ShaderCompileStatus::ParsingFailed:
			return "Parsing";

		case ShaderCompileStatus::LinkingFailed:
			return "Linking";

		default:
			FT_FAIL("Unsupported ShaderCompileStatus.");
		}
	}

	VkShaderStageFlagBits GetVkShaderStageFlagFrom(const ShaderStage inStage)
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
		VkShaderModuleCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = sizeof(uint32_t) * inSpvCode.size();
		createInfo.pCode = inSpvCode.data();

		VkShaderModule shaderModule;
		FT_VK_CALL(vkCreateShaderModule(inDevice, &createInfo, nullptr, &shaderModule));

		return shaderModule;
	}

	Shader::Shader(const VkDevice inDevice, const std::string inPath, const ShaderStage inStage, const std::string& inCodeEntry)
		: ShaderFile(inPath)
		, m_Stage(inStage)
		, m_CodeEntry(inCodeEntry)
		, m_Device(inDevice)
	{
		ShaderCompileResult compileResult = CompileShaderToSpv(m_Language, m_Stage, m_SourceCode, m_CodeEntry);
		if (compileResult.Status != ShaderCompileStatus::Success)
		{
			// TODO: LOG!
			return;
		}

		const std::vector<uint32_t> spvCode = compileResult.SpvCode;
		m_Bindings = ReflectShader(spvCode, GetVkShaderStageFlagFrom(m_Stage));
		m_Module = CreateShaderModule(m_Device, spvCode);
	}

	void DestroyShaderModule(const VkDevice inDevice, const VkShaderModule inModule)
	{
		vkDestroyShaderModule(inDevice, inModule, nullptr);
	}

	Shader::~Shader()
	{
		DestroyShaderModule(m_Device, m_Module);
	}

	void Shader::Recompile(const std::string& inSourceCode)
	{
		ShaderCompileResult compileResult = CompileShaderToSpv(m_Language, m_Stage, inSourceCode, m_CodeEntry);
		if (compileResult.Status != ShaderCompileStatus::Success)
		{
			// TOOD: LOG fail!
			return;
		}

		UpdateSourceCode(inSourceCode);

		const std::vector<uint32_t> spvCode = compileResult.SpvCode;
		m_Bindings = ReflectShader(spvCode, GetVkShaderStageFlagFrom(m_Stage));

		DestroyShaderModule(m_Device, m_Module);
		m_Module = CreateShaderModule(m_Device, spvCode);

		// TODO: Log success!
	}

	VkPipelineShaderStageCreateInfo Shader::GetPipelineStageInfo() const
	{
		VkPipelineShaderStageCreateInfo pipelineStageInfo{};
		pipelineStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		pipelineStageInfo.stage = GetVkShaderStageFlagFrom(m_Stage);
		pipelineStageInfo.module = m_Module;
		pipelineStageInfo.pName = m_CodeEntry.c_str();
		return pipelineStageInfo;
	}
}
