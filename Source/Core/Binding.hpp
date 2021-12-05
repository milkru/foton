#pragma once

FT_BEGIN_NAMESPACE

struct Binding
{
	VkDescriptorSetLayoutBinding DescriptorSetBinding;
	SpvReflectDescriptorBinding ReflectDescriptorBinding;
};

FT_END_NAMESPACE
