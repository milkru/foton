#pragma once

#include "Binding.hpp"
#include "Resource.hpp"

FT_BEGIN_NAMESPACE

struct Descriptor
{
	Binding Binding;
	Resource Resource;
	uint32_t Index;
};

FT_END_NAMESPACE
