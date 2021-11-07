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

	std::vector<uint32_t> Compile(const ShaderLanguage inLanguage, const ShaderStage inStage, const std::string inSourceCode, const std::string inCodeEntry)
	{
		ShaderCompileResult compileResult = CompileShaderToSpv(inLanguage, inStage, inSourceCode, inCodeEntry);
		
		const char* compileStatus = ConvertCompilationStatusToText(compileResult.Status);
		FT_CHECK_MSG(compileResult.Status == ShaderCompileStatus::Success, "Shader compilation %s failed.", compileStatus);
		
		return compileResult.ByteCodeSpv;
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

	std::vector<VkDescriptorSetLayoutBinding> Reflect(const std::vector<uint32_t>& inSpvCode, const ShaderStage inStage)
	{
		const VkShaderStageFlagBits shaderStage = GetVkShaderStageFlagFrom(inStage);
		return ReflectShader(inSpvCode, shaderStage);
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
		std::vector<uint32_t> spvCode = Compile(m_Language, m_Stage, m_SourceCode, m_CodeEntry);
		m_Bindings = Reflect(spvCode, m_Stage);
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

	void Shader::Recompile(const std::string inSourceCode)
	{
		UpdateSourceCode(inSourceCode);
		
		std::vector<uint32_t> spvCode = Compile(m_Language, m_Stage, m_SourceCode, m_CodeEntry);
		m_Bindings = Reflect(spvCode, m_Stage);

		DestroyShaderModule(m_Device, m_Module);
		m_Module = CreateShaderModule(m_Device, spvCode);
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
