#pragma once

#include <glslang/SPIRV/GlslangToSpv.h>

namespace FT
{
	class ShaderFileIncluder : public glslang::TShader::Includer
	{
		virtual void releaseInclude(IncludeResult*) override {};
	};
}
