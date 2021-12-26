#pragma once

FT_BEGIN_NAMESPACE

enum class ResourceType
{
	CombinedImageSampler,
	Image,
	Sampler,
	UniformBuffer,

	Count
};

class CombinedImageSampler;
class Image;
class Sampler;
class UniformBuffer;

union ResourceHandle
{
	CombinedImageSampler* CombinedImageSampler;
	Image* Image;
	Sampler* Sampler;
	UniformBuffer* UniformBuffer;
};

struct Resource
{
	ResourceType Type;
	ResourceHandle Handle;
};

FT_END_NAMESPACE
