#include "CommandBuffer.h"
#include "Device.h"
#include "Swapchain.h"
#include "Pipeline.h"
#include "DescriptorSet.h"

#define FT_ILLEGAL_COMMAND_BUFFER_INDEX -1

FT_BEGIN_NAMESPACE

CommandBuffer::CommandBuffer(const Device* inDevice, const Swapchain* inSwapchain)
	: m_Device(inDevice)
	, m_PipelineLayout(VK_NULL_HANDLE)
	, m_CurrentCommandBufferIndex(FT_ILLEGAL_COMMAND_BUFFER_INDEX)
{
	m_CommandBuffers.resize(inSwapchain->GetImageCount());

	VkCommandBufferAllocateInfo allocateInfo{};
	allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocateInfo.commandPool = m_Device->GetCommandPool();
	allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocateInfo.commandBufferCount = (uint32_t)m_CommandBuffers.size();

	FT_VK_CALL(vkAllocateCommandBuffers(m_Device->GetDevice(), &allocateInfo, m_CommandBuffers.data()));
}

CommandBuffer::~CommandBuffer()
{
	vkFreeCommandBuffers(m_Device->GetDevice(), m_Device->GetCommandPool(), static_cast<uint32_t>(m_CommandBuffers.size()), m_CommandBuffers.data());
}

void CommandBuffer::Begin(const uint32_t inCommandBufferIndex, const Swapchain* inSwapchain)
{
	m_CurrentCommandBufferIndex = inCommandBufferIndex;

	const VkCommandBuffer commandBuffer = m_CommandBuffers[m_CurrentCommandBufferIndex];

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

	FT_VK_CALL(vkBeginCommandBuffer(commandBuffer, &beginInfo));

	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = inSwapchain->GetRenderPass();
	renderPassInfo.framebuffer = inSwapchain->GetFramebuffer(m_CurrentCommandBufferIndex);
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = inSwapchain->GetExtent();

	VkClearValue clearColor = { {{0.0f, 0.0f, 0.0f, 1.0f}} };
	renderPassInfo.clearValueCount = 1;
	renderPassInfo.pClearValues = &clearColor;

	vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void CommandBuffer::End() 
{
	FT_CHECK(m_CurrentCommandBufferIndex != FT_ILLEGAL_COMMAND_BUFFER_INDEX, "Command buffer begin command needs to be called first.");

	const VkCommandBuffer commandBuffer = m_CommandBuffers[m_CurrentCommandBufferIndex];
	vkCmdEndRenderPass(commandBuffer);
	FT_VK_CALL(vkEndCommandBuffer(commandBuffer));

	m_CurrentCommandBufferIndex = FT_ILLEGAL_COMMAND_BUFFER_INDEX;
}

void CommandBuffer::Draw() const
{
	FT_CHECK(m_CurrentCommandBufferIndex != FT_ILLEGAL_COMMAND_BUFFER_INDEX, "Command buffer begin command needs to be called first.");

	const VkCommandBuffer commandBuffer = m_CommandBuffers[m_CurrentCommandBufferIndex];
	vkCmdDraw(commandBuffer, 3, 1, 0, 0);
}

void CommandBuffer::BindPipeline(const Pipeline* inPipeline)
{
	FT_CHECK(m_CurrentCommandBufferIndex != FT_ILLEGAL_COMMAND_BUFFER_INDEX, "Command buffer begin command needs to be called first.");

	const VkCommandBuffer commandBuffer = m_CommandBuffers[m_CurrentCommandBufferIndex];
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, inPipeline->GetGraphicsPipeline());
	m_PipelineLayout = inPipeline->GetPipelineLayout();
}

void CommandBuffer::BindDescriptorSet(const DescriptorSet* inDescriptorSet) const
{
	FT_CHECK(m_CurrentCommandBufferIndex != FT_ILLEGAL_COMMAND_BUFFER_INDEX, "Command buffer begin command needs to be called first.");
	FT_CHECK(m_PipelineLayout != VK_NULL_HANDLE, "Pipeline needs to be bound before descriptor set.");

	const VkCommandBuffer commandBuffer = m_CommandBuffers[m_CurrentCommandBufferIndex];
	const VkDescriptorSet& descriptorSet = inDescriptorSet->GetDescriptorSet(m_CurrentCommandBufferIndex);
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
}

FT_END_NAMESPACE
