#include "DefaultShader.h"
#include "Utility/ShaderFile.h"

FT_BEGIN_NAMESPACE

static const char* DefaultVertexShaderGLSL =
	"#version 450\n"
	"\n"
	"layout (location = 0) out vec2 outUV;\n"
	"\n"
	"void main()\n"
	"{\n"
	"	outUV = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);\n"
	"	gl_Position = vec4(outUV * 2.0 - 1.0, 0.0, 1.0);\n"
	"}\n";

static const char* DefaultFragmentShaderGLSL =
	"#version 450\n"
	"\n"
	"layout (location = 0) in vec2 inUV;\n"
	"\n"
	"layout (binding = 0) uniform UBO\n"
	"{\n"
	"	int x;\n"
	"	float y;\n"
	"} inputUbo;\n"
	"\n"
	"layout (binding = 1) uniform sampler2D textureInput;\n"
	"\n"
	"layout (location = 0) out vec4 outColor;\n"
	"\n"
	"void main()\n"
	"{\n"
	"	vec4 fragCol = texture(textureInput, inUV);\n"
	"	outColor = fragCol;\n"
	"}\n";

static const char* DefaultVertexShaderHLSL =
	"struct VSOutput\n"
	"{\n"
	"	float4 Pos : SV_POSITION;\n"
	"	[[vk::location(0)]] float2 UV : TEXCOORD0;\n"
	"};\n"
	"\n"
	"VSOutput main(uint VertexIndex : SV_VertexID)\n"
	"{\n"
	"	VSOutput output = (VSOutput)0;\n"
	"	output.UV = float2((VertexIndex << 1) & 2, VertexIndex & 2);\n"
	"	output.Pos = float4(output.UV * 2.0f - 1.0f, 0.0f, 1.0f);\n"
	"	return output;\n"
	"}\n";

static const char* DefaultFragmentShaderHLSL =
	"struct UBO\n"
	"{\n"
	"	int x;\n"
	"	float y;\n"
	"};\n"
	"cbuffer ubo : register(b0) { UBO inputUbo; };\n"
	"\n"
	"Texture2D textureInput : register(t1);\n"
	"SamplerState samplerInput : register(s1);\n"
	"\n"
	"float4 main([[vk::location(0)]] float2 inUV : TEXCOORD0) : SV_TARGET\n"
	"{\n"
	"	float4 fragCol = textureInput.Sample(samplerInput, inUV);\n"
	"	return fragCol;\n"
	"}\n";

const char* GetDefaultVertexShader(const ShaderLanguage inLanguage)
{
	switch (inLanguage)
	{
	case ShaderLanguage::GLSL:
		return DefaultVertexShaderGLSL;

	case ShaderLanguage::HLSL:
		return DefaultVertexShaderHLSL;

	default:
		FT_FAIL("Unsupported ShaderLanguage.");
	}
}

const char* GetDefaultFragmentShader(const ShaderLanguage inLanguage)
{
	switch (inLanguage)
	{
	case ShaderLanguage::GLSL:
		return DefaultFragmentShaderGLSL;

	case ShaderLanguage::HLSL:
		return DefaultFragmentShaderHLSL;

	default:
		FT_FAIL("Unsupported ShaderLanguage.");
	}
}

FT_END_NAMESPACE
