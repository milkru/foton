#include "ShaderReflect.h"
#include <spirv_reflect.h>

namespace FT
{
#	define FT_SPV_REFLECT_CALL(call) \
		do { \
			SpvReflectResult result = call; \
			FT_CHECK_MSG(result == SPV_REFLECT_RESULT_SUCCESS, "SpirVReflect API call failed."); \
		} \
		while (0)

	VkDescriptorType GetVkDescriptorTypeFrom(const SpvReflectDescriptorType inDescriptorType)
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

	std::vector<VkDescriptorSetLayoutBinding> ReflectShader(const std::vector<uint32_t>& inSpvCode, const VkShaderStageFlags inShaderStage)
	{
		SpvReflectShaderModule spvModule;

		size_t spvCodeSize = sizeof(uint32_t) * inSpvCode.size();
		const void* spvCode = reinterpret_cast<const void*>(inSpvCode.data());
		FT_SPV_REFLECT_CALL(spvReflectCreateShaderModule(spvCodeSize, spvCode, &spvModule));

		uint32_t bindingCount = 0;
		FT_SPV_REFLECT_CALL(spvReflectEnumerateDescriptorBindings(&spvModule, &bindingCount, nullptr));

		std::vector<SpvReflectDescriptorBinding*> bindings(bindingCount);
		FT_SPV_REFLECT_CALL(spvReflectEnumerateDescriptorBindings(&spvModule, &bindingCount, bindings.data()));

		std::vector<VkDescriptorSetLayoutBinding> descriptorSetLayoutBindings(bindings.size());
		for (uint32_t i = 0; i < bindingCount; ++i)
		{
			VkDescriptorSetLayoutBinding& descriptorSetLayoutBinding = descriptorSetLayoutBindings[i];
			descriptorSetLayoutBinding.binding = bindings[i]->binding;
			descriptorSetLayoutBinding.descriptorCount = 1;
			descriptorSetLayoutBinding.descriptorType = GetVkDescriptorTypeFrom(bindings[i]->descriptor_type);
			descriptorSetLayoutBinding.stageFlags = inShaderStage;
			descriptorSetLayoutBinding.pImmutableSamplers = nullptr;
		}

		spvReflectDestroyShaderModule(&spvModule);

		return descriptorSetLayoutBindings;
	}
}
