#pragma once

FT_BEGIN_NAMESPACE

enum class ResourceType
{
	Image,
	UniformBuffer,

	Count
};

class Image;
class UniformBuffer;

union ResourceHandle
{
	Image* Image;
	UniformBuffer* UniformBuffer;
};

struct Resource
{
	ResourceType Type;
	ResourceHandle Handle;
};

FT_END_NAMESPACE
