#pragma once

namespace FT
{
	enum class ShaderLanguage : uint8_t
	{
		GLSL,
		HLSL,

		Count
	};

	enum class ShaderStage : uint8_t
	{
		Vertex,
		Fragment,

		Count
	};

	struct ShaderFileExtension
	{
		ShaderLanguage Language = ShaderLanguage::Count;
		std::string Extension;
		std::string Name;
	};

	const ShaderFileExtension SupportedShaderFileExtensions[] =
	{
		{ ShaderLanguage::GLSL, "glsl", "Graphics Library Shading Language"},
		{ ShaderLanguage::HLSL, "hlsl", "High-Level Shader Language"}
	};
}
