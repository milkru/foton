#include "Renderer.h"
#include "Device.h"
#include "Swapchain.h"
#include "Buffer.h"
#include "Image.h"
#include "UniformBuffer.h"
#include "Binding.hpp"
#include "Resource.hpp"
#include "Shader.h"
#include "Pipeline.h"
#include "DescriptorSet.h"
#include "CommandBuffer.h"
#include "ResourceContainer.h"
#include "Compiler/ShaderCompiler.h"
#include "Utility/ShaderFile.h"

FT_BEGIN_NAMESPACE

Renderer::Renderer(GLFWwindow* inWindow)
	: m_Window(inWindow)
	, m_EnableUserInterface(true)
{
	InitializeShaderCompiler();

	m_Device = new Device(m_Window);
	m_Swapchain = new Swapchain(m_Device, m_Window);

	{
		const ShaderFile shaderFile(GetFullPath("Shaders/Internal/FullScreen.vert.glsl"));

		const ShaderCompileResult compileResult = CompileShader(shaderFile.GetLanguage(), ShaderStage::Vertex, shaderFile.GetSourceCode());
		const char* status = ConvertCompilationStatusToText(compileResult.Status);
		FT_CHECK(compileResult.Status == ShaderCompileStatus::Success, "Failed %s vertex shader %s.", status, shaderFile.GetName().c_str());

		m_VertexShader = new Shader(m_Device, ShaderStage::Vertex, compileResult.SpvCode);
	}

	{
		m_FragmentShaderFile = new ShaderFile(GetFullPath("Shaders/Internal/Default.frag.glsl"));

		const ShaderCompileResult compileResult = CompileShader(m_FragmentShaderFile->GetLanguage(), ShaderStage::Fragment, m_FragmentShaderFile->GetSourceCode());
		// TODO: Just load default shader if the compilation fails for fragment shader.
		const char* status = ConvertCompilationStatusToText(compileResult.Status);
		FT_CHECK(compileResult.Status == ShaderCompileStatus::Success, "Failed %s fragment shader %s.", status, m_FragmentShaderFile->GetName().c_str());

		m_FragmentShader = new Shader(m_Device, ShaderStage::Fragment, compileResult.SpvCode);
	}

	m_ResourceContainer = new ResourceContainer(m_Device, m_Swapchain);
	m_ResourceContainer->UpdateBindings(m_FragmentShader->GetBindings());

	m_DescriptorSet = new DescriptorSet(m_Device, m_Swapchain, m_ResourceContainer->GetDescriptors());
	m_Pipeline = new Pipeline(m_Device, m_Swapchain, m_DescriptorSet, m_VertexShader, m_FragmentShader);
	m_CommandBuffer = new CommandBuffer(m_Device, m_Swapchain);
}
	
Renderer::~Renderer()
{
	delete(m_FragmentShaderFile);
	delete(m_FragmentShader);
	delete(m_VertexShader);

	CleanupSwapchain();

	delete(m_ResourceContainer);
	delete(m_Swapchain);
	delete(m_Device);

	FinalizeShaderCompiler();
}

void Renderer::DrawFrame()
{
	const SwapchainImageAcquireResult imageAcquireResult = m_Swapchain->AcquireNextImage();
	if (imageAcquireResult.Status == SwapchainStatus::Recreate)
	{
		RecreateSwapchain();
		return;
	}

	FT_CHECK(imageAcquireResult.Status == SwapchainStatus::Success, "Failed acquiring swapchain image.");

	const uint32_t imageIndex = imageAcquireResult.ImageIndex;

	// UpdateUniformBuffer(imageIndex); // TODO:
	FillCommandBuffers(imageIndex);

	const SwapchainStatus presentStatus = m_Swapchain->Present(imageIndex, m_CommandBuffer);
	if (presentStatus == SwapchainStatus::Recreate)
	{
		RecreateSwapchain();
	}
	else
	{
		FT_CHECK(presentStatus == SwapchainStatus::Success, "Swapchain present failed.");
	}
}

void Renderer::WaitDeviceToFinish()
{
	vkDeviceWaitIdle(m_Device->GetDevice());
}

void Renderer::UpdateFragmentShaderFile(ShaderFile* inFragmentShaderFile)
{
	delete(m_FragmentShaderFile);
	m_FragmentShaderFile = inFragmentShaderFile;
}
	
void Renderer::ToggleUserInterface()
{
	// TODO: Imgui Demo Window -> Style -> Rendering -> Global alpha. You can use this to fade toggle.
	m_EnableUserInterface = !m_EnableUserInterface;
}
	
void Renderer::OnFragmentShaderRecompiled(const std::vector<uint32_t>& inSpvCode)
{
	vkQueueWaitIdle(m_Device->GetGraphicsQueue());

	delete(m_FragmentShader);
	m_FragmentShader = new Shader(m_Device, ShaderStage::Fragment, inSpvCode);

	m_ResourceContainer->UpdateBindings(m_FragmentShader->GetBindings());

	delete(m_DescriptorSet);
	m_DescriptorSet = new DescriptorSet(m_Device, m_Swapchain, m_ResourceContainer->GetDescriptors());

	delete(m_Pipeline);
	m_Pipeline = new Pipeline(m_Device, m_Swapchain, m_DescriptorSet, m_VertexShader, m_FragmentShader);
}

void Renderer::CleanupSwapchain()
{
	m_Swapchain->Cleanup();

	delete(m_CommandBuffer);
	delete(m_Pipeline);
	delete(m_DescriptorSet);
}

void Renderer::RecreateSwapchain()
{
	int width = 0, height = 0;
	glfwGetFramebufferSize(m_Window, &width, &height);
	while (width == 0 || height == 0)
	{
		glfwGetFramebufferSize(m_Window, &width, &height);
		glfwWaitEvents();
	}

	vkDeviceWaitIdle(m_Device->GetDevice());

	CleanupSwapchain();

	m_Swapchain->Recreate();

	m_ResourceContainer->RecreateUniformBuffers();

	m_DescriptorSet = new DescriptorSet(m_Device, m_Swapchain, m_ResourceContainer->GetDescriptors());
	m_Pipeline = new Pipeline(m_Device, m_Swapchain, m_DescriptorSet, m_VertexShader, m_FragmentShader);
	m_CommandBuffer = new CommandBuffer(m_Device, m_Swapchain);

	ImGui_ImplVulkan_SetMinImageCount(m_Swapchain->GetImageCount());
}

void Renderer::FillCommandBuffers(uint32_t inSwapchainImageIndex)
{
	m_CommandBuffer->Begin(inSwapchainImageIndex, m_Swapchain);
	m_CommandBuffer->BindPipeline(m_Pipeline);
	m_CommandBuffer->BindDescriptorSet(m_DescriptorSet);
	m_CommandBuffer->Draw();

	if (m_EnableUserInterface)
	{
		ImDrawData* drawData = ImGui::GetDrawData();
		VkCommandBuffer commandBuffer = m_CommandBuffer->GetCommandBuffer(inSwapchainImageIndex);
		ImGui_ImplVulkan_RenderDrawData(drawData, commandBuffer);
	}

	m_CommandBuffer->End();
}

FT_END_NAMESPACE
