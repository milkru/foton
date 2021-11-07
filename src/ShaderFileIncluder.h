#pragma once

#include <glslang/SPIRV/GlslangToSpv.h>

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
