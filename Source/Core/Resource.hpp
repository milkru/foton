#pragma once

namespace FT
{
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
}
