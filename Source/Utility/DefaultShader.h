#pragma once

FT_BEGIN_NAMESPACE

enum class ShaderLanguage : uint8_t;

extern const char* GetDefaultVertexShader(const ShaderLanguage inLanguage);
extern const char* GetDefaultFragmentShader(const ShaderLanguage inLanguage);

FT_END_NAMESPACE
