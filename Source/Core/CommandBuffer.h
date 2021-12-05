#pragma once

FT_BEGIN_NAMESPACE

class Device;
class Swapchain;
class Pipeline;
class DescriptorSet;

class CommandBuffer
{
public:
	CommandBuffer(const Device* inDevice, const Swapchain* inSwapchain);
	~CommandBuffer();
	FT_DELETE_COPY_AND_MOVE(CommandBuffer)

public:
	void Begin(const uint32_t inCommandBufferIndex, const Swapchain* inSwapchain);
	void End();
	void Draw() const;
	void BindPipeline(const Pipeline* inPipeline);
	void BindDescriptorSet(const DescriptorSet* inDescriptorSet) const;

public:
	VkCommandBuffer GetCommandBuffer(const uint32_t inIndex) const { return m_CommandBuffers[inIndex]; }

private:
	const Device* m_Device;
	std::vector<VkCommandBuffer> m_CommandBuffers;
	VkPipelineLayout m_PipelineLayout;
	uint32_t m_CurrentCommandBufferIndex;
};

FT_END_NAMESPACE
