#pragma once

FT_BEGIN_NAMESPACE

class Device;
class Swapchain;
class DescriptorSet;
class Shader;

class Pipeline
{
public:
	Pipeline(const Device* inDevice, const Swapchain* inSwapchain, const DescriptorSet* inDescriptorSet, const Shader* inVertexShader, const Shader* inFragmentShader);
	~Pipeline();
	FT_DELETE_COPY_AND_MOVE(Pipeline)

public:
	VkPipelineLayout GetPipelineLayout() const { return m_PipelineLayout; }
	VkPipeline GetGraphicsPipeline() const { return m_GraphicsPipeline; }

private:
	const Device* m_Device;
	VkPipelineLayout m_PipelineLayout;
	VkPipeline m_GraphicsPipeline;
};

FT_END_NAMESPACE
