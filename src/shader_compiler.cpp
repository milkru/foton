#include "shader_compiler.h"

#include <glslang/SPIRV/GlslangToSpv.h>

const TBuiltInResource DefaultTBuiltInResource = {
	/* .MaxLights = */ 32,
	/* .MaxClipPlanes = */ 6,
	/* .MaxTextureUnits = */ 32,
	/* .MaxTextureCoords = */ 32,
	/* .MaxVertexAttribs = */ 64,
	/* .MaxVertexUniformComponents = */ 4096,
	/* .MaxVaryingFloats = */ 64,
	/* .MaxVertexTextureImageUnits = */ 32,
	/* .MaxCombinedTextureImageUnits = */ 80,
	/* .MaxTextureImageUnits = */ 32,
	/* .MaxFragmentUniformComponents = */ 4096,
	/* .MaxDrawBuffers = */ 32,
	/* .MaxVertexUniformVectors = */ 128,
	/* .MaxVaryingVectors = */ 8,
	/* .MaxFragmentUniformVectors = */ 16,
	/* .MaxVertexOutputVectors = */ 16,
	/* .MaxFragmentInputVectors = */ 15,
	/* .MinProgramTexelOffset = */ -8,
	/* .MaxProgramTexelOffset = */ 7,
	/* .MaxClipDistances = */ 8,
	/* .MaxComputeWorkGroupCountX = */ 65535,
	/* .MaxComputeWorkGroupCountY = */ 65535,
	/* .MaxComputeWorkGroupCountZ = */ 65535,
	/* .MaxComputeWorkGroupSizeX = */ 1024,
	/* .MaxComputeWorkGroupSizeY = */ 1024,
	/* .MaxComputeWorkGroupSizeZ = */ 64,
	/* .MaxComputeUniformComponents = */ 1024,
	/* .MaxComputeTextureImageUnits = */ 16,
	/* .MaxComputeImageUniforms = */ 8,
	/* .MaxComputeAtomicCounters = */ 8,
	/* .MaxComputeAtomicCounterBuffers = */ 1,
	/* .MaxVaryingComponents = */ 60,
	/* .MaxVertexOutputComponents = */ 64,
	/* .MaxGeometryInputComponents = */ 64,
	/* .MaxGeometryOutputComponents = */ 128,
	/* .MaxFragmentInputComponents = */ 128,
	/* .MaxImageUnits = */ 8,
	/* .MaxCombinedImageUnitsAndFragmentOutputs = */ 8,
	/* .MaxCombinedShaderOutputResources = */ 8,
	/* .MaxImageSamples = */ 0,
	/* .MaxVertexImageUniforms = */ 0,
	/* .MaxTessControlImageUniforms = */ 0,
	/* .MaxTessEvaluationImageUniforms = */ 0,
	/* .MaxGeometryImageUniforms = */ 0,
	/* .MaxFragmentImageUniforms = */ 8,
	/* .MaxCombinedImageUniforms = */ 8,
	/* .MaxGeometryTextureImageUnits = */ 16,
	/* .MaxGeometryOutputVertices = */ 256,
	/* .MaxGeometryTotalOutputComponents = */ 1024,
	/* .MaxGeometryUniformComponents = */ 1024,
	/* .MaxGeometryVaryingComponents = */ 64,
	/* .MaxTessControlInputComponents = */ 128,
	/* .MaxTessControlOutputComponents = */ 128,
	/* .MaxTessControlTextureImageUnits = */ 16,
	/* .MaxTessControlUniformComponents = */ 1024,
	/* .MaxTessControlTotalOutputComponents = */ 4096,
	/* .MaxTessEvaluationInputComponents = */ 128,
	/* .MaxTessEvaluationOutputComponents = */ 128,
	/* .MaxTessEvaluationTextureImageUnits = */ 16,
	/* .MaxTessEvaluationUniformComponents = */ 1024,
	/* .MaxTessPatchComponents = */ 120,
	/* .MaxPatchVertices = */ 32,
	/* .MaxTessGenLevel = */ 64,
	/* .MaxViewports = */ 16,
	/* .MaxVertexAtomicCounters = */ 0,
	/* .MaxTessControlAtomicCounters = */ 0,
	/* .MaxTessEvaluationAtomicCounters = */ 0,
	/* .MaxGeometryAtomicCounters = */ 0,
	/* .MaxFragmentAtomicCounters = */ 8,
	/* .MaxCombinedAtomicCounters = */ 8,
	/* .MaxAtomicCounterBindings = */ 1,
	/* .MaxVertexAtomicCounterBuffers = */ 0,
	/* .MaxTessControlAtomicCounterBuffers = */ 0,
	/* .MaxTessEvaluationAtomicCounterBuffers = */ 0,
	/* .MaxGeometryAtomicCounterBuffers = */ 0,
	/* .MaxFragmentAtomicCounterBuffers = */ 1,
	/* .MaxCombinedAtomicCounterBuffers = */ 1,
	/* .MaxAtomicCounterBufferSize = */ 16384,
	/* .MaxTransformFeedbackBuffers = */ 4,
	/* .MaxTransformFeedbackInterleavedComponents = */ 64,
	/* .MaxCullDistances = */ 8,
	/* .MaxCombinedClipAndCullDistances = */ 8,
	/* .MaxSamples = */ 4,

	/* .maxMeshOutputVerticesNV = */ 256,
	/* .maxMeshOutputPrimitivesNV = */ 512,
	/* .maxMeshWorkGroupSizeX_NV = */ 32,
	/* .maxMeshWorkGroupSizeY_NV = */ 1,
	/* .maxMeshWorkGroupSizeZ_NV = */ 1,
	/* .maxTaskWorkGroupSizeX_NV = */ 32,
	/* .maxTaskWorkGroupSizeY_NV = */ 1,
	/* .maxTaskWorkGroupSizeZ_NV = */ 1,
	/* .maxMeshViewCountNV = */ 4,
	/* .maxDualSourceDrawBuffersEXT = */ 1,

	/* .limits = */ {
		/* .nonInductiveForLoops = */ 1,
		/* .whileLoops = */ 1,
		/* .doWhileLoops = */ 1,
		/* .generalUniformIndexing = */ 1,
		/* .generalAttributeMatrixVectorIndexing = */ 1,
		/* .generalVaryingIndexing = */ 1,
		/* .generalSamplerIndexing = */ 1,
		/* .generalVariableIndexing = */ 1,
		/* .generalConstantMatrixVectorIndexing = */ 1,
	}
};

void FT::ShaderCompiler::Initialize()
{
	glslang::InitializeProcess();
}

void FT::ShaderCompiler::Termiante()
{
	glslang::FinalizeProcess();
}

// TODO: Implement when file shader file include feature is requested. Move to new file.
class ShaderFileIncluder : public glslang::TShader::Includer
{
public:
	ShaderFileIncluder()
	{
	}

	virtual ~ShaderFileIncluder() override
	{
	}

	virtual IncludeResult* includeLocal(const char* headerName, const char* includerName, size_t inclusionDepth) override
	{
		return nullptr;
	}

	virtual IncludeResult* includeSystem(const char* headerName, const char*, size_t) override
	{
		return nullptr;
	}

	virtual void pushExternalLocalDirectory(const std::string& dir)
	{
		return;
	}

	virtual void releaseInclude(IncludeResult* result) override
	{
		return;
	}
};

glslang::EShSource GetGlslangShaderLanguageFrom(const FT::ShaderLanguage inShaderLanguage)
{
	switch (inShaderLanguage)
	{
	case FT::ShaderLanguage::GLSL:
		return glslang::EShSourceGlsl;

	case FT::ShaderLanguage::HLSL:
		return glslang::EShSourceHlsl;

	default:
		FT_FAIL("Unsupported ShaderLanguage.");
		return glslang::EShSourceCount;
	}
}

EShLanguage GetGlslangShaderTypeFrom(const FT::ShaderType inShaderType)
{
	switch (inShaderType)
	{
	case FT::ShaderType::Vertex:
		return EShLangVertex;

	case FT::ShaderType::Fragment:
		return EShLangFragment;

	default:
		FT_FAIL("Unsupported ShaderType.");
		return EShLangCount;
	}
}

std::string FT::ShaderCompiler::ReadShaderFile(std::string inFileName)
{
	FILE* file = nullptr;
	errno_t errorCode = fopen_s(&file, inFileName.c_str(), "r");
	if (errorCode || file == nullptr)
	{
		FT_FAIL("Failed loading shader file %s.", inFileName.c_str());
		return ""; // TODO: ???
	}

	fseek(file, 0L, SEEK_END);
	const auto fileBytes = ftell(file);
	fseek(file, 0L, SEEK_SET);

	char* buffer = (char*)alloca(fileBytes + 1);
	const size_t bytesToRead = fread(buffer, 1, fileBytes, file);
	fclose(file);

	buffer[bytesToRead] = '\0';

	return buffer;
}

std::string GetShaderExtensionFrom(const std::string inFileName)
{
	static const uint32_t ShaderFileExtensionSize = 4;
	std::string fileExtension = inFileName.substr(inFileName.size() - ShaderFileExtensionSize);

	std::for_each(fileExtension.begin(), fileExtension.end(), [](char& character)
		{
			character = ::tolower(character);
		});

	return fileExtension;
}

FT::ShaderLanguage FT::ShaderCompiler::GetShaderLanguageFromFileName(const std::string inFileName)
{
	const std::string fileExtension = GetShaderExtensionFrom(inFileName);

	for (const auto& possibleFileExtension : FT::ShaderFileExtensions)
	{
		if (!fileExtension.compare(possibleFileExtension.Extension))
		{
			return possibleFileExtension.Language;
		}
	}

	FT_FAIL("Unsupported %s shader file extension.", fileExtension.c_str());
	return FT::ShaderLanguage::Count;
}

FT::CompileShaderResult FT::ShaderCompiler::Compile(const ShaderLanguage inShaderLanguage, const ShaderType inShaderType,
	const std::string inSourceCode, const std::string& inCodeEntry)
{
	const EShLanguage shaderType = GetGlslangShaderTypeFrom(inShaderType);
	glslang::TShader shader(shaderType);
	shader.setEntryPoint(inCodeEntry.c_str());
	shader.setSourceEntryPoint(inCodeEntry.c_str());

	const char* sourceCode = inSourceCode.c_str();
	shader.setStrings(&sourceCode, 1);

	const static glslang::EShSource shaderLanguage = GetGlslangShaderLanguageFrom(inShaderLanguage);
	const static glslang::EShClient client = glslang::EShClientVulkan;
	const static int version = 330;
	shader.setEnvInput(shaderLanguage, shaderType, client, version);

	const static glslang::EShTargetClientVersion targetClientVersion = glslang::EShTargetVulkan_1_0;
	shader.setEnvClient(client, targetClientVersion);

	const static glslang::EShTargetLanguageVersion targetLanguageVersion = glslang::EShTargetSpv_1_0;
	shader.setEnvTarget(glslang::EShTargetSpv, targetLanguageVersion);

	const static TBuiltInResource buildResource = DefaultTBuiltInResource;
	const static int defaultVersion = version;

	EShMessages messages = (EShMessages)(EShMsgSpvRules | EShMsgKeepUncalled);
	std::string preprocessedShader;
	ShaderFileIncluder fileIncluder;

	if (!shader.preprocess(&buildResource, defaultVersion, ENoProfile, false, false, messages, &preprocessedShader, fileIncluder))
	{
		FT_LOG(shader.getInfoLog());

		CompileShaderResult result{};
		result.Status = CompileShaderStatus::PreprocessingFailed;
		return result;
	}

	const char* preprocessedShaderCode = preprocessedShader.c_str();
	shader.setStrings(&preprocessedShaderCode, 1);
	shader.setAutoMapLocations(true);

	if (!shader.parse(&buildResource, defaultVersion, false, messages))
	{
		FT_LOG(shader.getInfoLog());

		CompileShaderResult result{};
		result.Status = CompileShaderStatus::ParsingFailed;
		return result;
	}

	glslang::TProgram shaderProgram;
	shaderProgram.addShader(&shader);

	if (!shaderProgram.link(messages))
	{
		FT_LOG(shader.getInfoLog());

		CompileShaderResult result{};
		result.Status = CompileShaderStatus::LinkingFailed;
		return result;
	}

	spv::SpvBuildLogger spvBuildLogger;

	glslang::SpvOptions spvOptions;
	spvOptions.optimizeSize = false;
	spvOptions.disableOptimizer = true;
	spvOptions.generateDebugInfo = true;
	spvOptions.validate = true;

	CompileShaderResult result{};
	const glslang::TIntermediate* intermediate = shaderProgram.getIntermediate(shaderType);
	glslang::GlslangToSpv(*intermediate, result.ByteCode, &spvBuildLogger, &spvOptions);

	FT_LOG(spvBuildLogger.getAllMessages().c_str());

	result.Status = CompileShaderStatus::Success;
	return result;
}
