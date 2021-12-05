#pragma once

FT_BEGIN_NAMESPACE

extern std::vector<struct Binding> ReflectShader(const std::vector<uint32_t>& inSpvCode, const VkShaderStageFlags inShaderStage);

FT_END_NAMESPACE
