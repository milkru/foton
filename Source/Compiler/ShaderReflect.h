#pragma once

namespace FT
{
	extern std::vector<struct Binding> ReflectShader(const std::vector<uint32_t>& inSpvCode, const VkShaderStageFlags inShaderStage);
}
