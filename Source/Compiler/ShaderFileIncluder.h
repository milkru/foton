#pragma once

#include <glslang/SPIRV/GlslangToSpv.h>

FT_BEGIN_NAMESPACE

class ShaderFileIncluder : public glslang::TShader::Includer
{
	virtual void releaseInclude(IncludeResult*) override {};
};

FT_END_NAMESPACE
