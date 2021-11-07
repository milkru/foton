#pragma once

namespace FT
{
	extern std::vector<VkDescriptorSetLayoutBinding> ReflectShader(const std::vector<uint32_t>& inSpvCode, const VkShaderStageFlags inShaderStage);
}
