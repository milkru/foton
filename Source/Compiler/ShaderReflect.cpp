#include "ShaderReflect.h"
#include "Core/Binding.hpp"

FT_BEGIN_NAMESPACE

#define FT_SPV_REFLECT_CALL(call) \
	do { \
		SpvReflectResult result = call; \
		FT_CHECK(result == SPV_REFLECT_RESULT_SUCCESS, "SpirVReflect API call failed."); \
	} \
	while (0)

VkDescriptorType GetVkDescriptorType(const SpvReflectDescriptorType inDescriptorType)
{
	switch (inDescriptorType)
	{
	case SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
		return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

	case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
		return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

	case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
		return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;

	case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLER:
		return VK_DESCRIPTOR_TYPE_SAMPLER;

	default:
		FT_FAIL("Unsupported SpvReflectDescriptorType.");
	}
}

std::vector<Binding> ReflectShader(const std::vector<uint32_t>& inSpvCode, const VkShaderStageFlags inShaderStage)
{
	SpvReflectShaderModule spvModule;

	size_t spvCodeSize = sizeof(uint32_t) * inSpvCode.size();
	const void* spvCode = reinterpret_cast<const void*>(inSpvCode.data());
	FT_SPV_REFLECT_CALL(spvReflectCreateShaderModule(spvCodeSize, spvCode, &spvModule));

	uint32_t bindingCount = 0;
	FT_SPV_REFLECT_CALL(spvReflectEnumerateDescriptorBindings(&spvModule, &bindingCount, nullptr));

	std::vector<SpvReflectDescriptorBinding*> spvBindings(bindingCount);
	FT_SPV_REFLECT_CALL(spvReflectEnumerateDescriptorBindings(&spvModule, &bindingCount, spvBindings.data()));

	std::vector<Binding> bindings(spvBindings.size());
	for (uint32_t i = 0; i < bindingCount; ++i)
	{
		Binding& binding = bindings[i];

		VkDescriptorSetLayoutBinding& descriptorSetLayoutBinding = binding.DescriptorSetBinding;
		descriptorSetLayoutBinding.binding = spvBindings[i]->binding;
		descriptorSetLayoutBinding.descriptorCount = 1;
		descriptorSetLayoutBinding.descriptorType = GetVkDescriptorType(spvBindings[i]->descriptor_type);
		descriptorSetLayoutBinding.stageFlags = inShaderStage;
		descriptorSetLayoutBinding.pImmutableSamplers = nullptr;

		binding.ReflectDescriptorBinding = *spvBindings[i];
	}

	spvReflectDestroyShaderModule(&spvModule);

	return bindings;
}

FT_END_NAMESPACE
