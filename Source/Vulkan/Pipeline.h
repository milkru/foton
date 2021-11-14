#pragma once

namespace FT
{
	class Device;
	class Swapchain;
	class Shader;

	class Pipeline
	{
	public:
		// TODO: Change inDescriptorSetLayout input after it gets wrapped.
		Pipeline(const Device* inDevice, const Swapchain* inSwapchain, const VkDescriptorSetLayout inDescriptorSetLayout, const Shader* inVertexShader, const Shader* inFragmentShader);
		~Pipeline();

	private:
		Pipeline(Pipeline const&) = delete;
		Pipeline& operator=(Pipeline const&) = delete;

	public:
		VkPipelineLayout GetPipelineLayout() const { return m_PipelineLayout; }
		VkPipeline GetGraphicsPipeline() const { return m_GraphicsPipeline; }

	private:
		const Device* m_Device;
		VkPipelineLayout m_PipelineLayout;
		VkPipeline m_GraphicsPipeline;
	};
}
