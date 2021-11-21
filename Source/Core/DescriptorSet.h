#pragma once

namespace FT
{
	struct Descriptor
	{
		//Binding Binding
		//Resource Resource;
	};

	class Device;

	class DescriptorSet
	{
	public:
		DescriptorSet(const Device* inDevice);
		~DescriptorSet();

	private:
		DescriptorSet(DescriptorSet const&) = delete;
		DescriptorSet& operator=(DescriptorSet const&) = delete;
	};
}
