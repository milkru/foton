#pragma once

#include "Binding.hpp"

FT_BEGIN_NAMESPACE

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
	FT_DELETE_COPY_AND_MOVE(Shader)

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

FT_END_NAMESPACE
