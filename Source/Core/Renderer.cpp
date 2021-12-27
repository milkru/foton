#include "Renderer.h"
#include "Window.h"
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
#include "Utility/DefaultShader.h"
#include "Utility/FileExplorer.h"

FT_BEGIN_NAMESPACE

bool Renderer::Initialize(Window* inWindow)
{
	m_Window = inWindow;

	InitializeShaderCompiler();

	m_Device = new Device(m_Window);
	m_Swapchain = new Swapchain(m_Device, m_Window);

	{
		const char* defaultVertexShader = GetDefaultVertexShader(ShaderLanguage::GLSL);
		const ShaderCompileResult compileResult = CompileShader(ShaderLanguage::GLSL, ShaderStage::Vertex, defaultVertexShader);
		const char* status = ConvertCompilationStatusToText(compileResult.Status);
		FT_CHECK(compileResult.Status == ShaderCompileStatus::Success, "Failed %s default vertex shader.", status);

		m_VertexShader = new Shader(m_Device, ShaderStage::Vertex, compileResult.SpvCode);
	}

	{
		// TODO: Get cached fragment shader file first and if that doesn't exist, use this one.
		std::string fragmentShaderPath;
		if (FileExplorer::SaveShaderDialog(fragmentShaderPath))
		{
			m_FragmentShaderFile = new ShaderFile(fragmentShaderPath);

			const char* defaultFragmentShader = GetDefaultFragmentShader(m_FragmentShaderFile->GetLanguage());
			m_FragmentShaderFile->UpdateSourceCode(defaultFragmentShader);
		}
		else
		{
			return false;
		}

		const ShaderCompileResult compileResult = CompileShader(m_FragmentShaderFile->GetLanguage(), ShaderStage::Fragment, m_FragmentShaderFile->GetSourceCode());
		const char* status = ConvertCompilationStatusToText(compileResult.Status);

		FT_CHECK(compileResult.Status == ShaderCompileStatus::Success, "Failed %s fragment shader %s.", status, m_FragmentShaderFile->GetName().c_str());

		m_FragmentShader = new Shader(m_Device, ShaderStage::Fragment, compileResult.SpvCode);
	}

	m_ResourceContainer = new ResourceContainer(m_Device, m_Swapchain);
	m_ResourceContainer->UpdateBindings(m_FragmentShader->GetBindings());

	m_DescriptorSet = new DescriptorSet(m_Device, m_Swapchain, m_ResourceContainer->GetDescriptors());
	m_Pipeline = new Pipeline(m_Device, m_Swapchain, m_DescriptorSet, m_VertexShader, m_FragmentShader);
	m_CommandBuffer = new CommandBuffer(m_Device, m_Swapchain);

	return true;
}

void Renderer::Terminate()
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

void Renderer::WaitQueueToFinish()
{
	vkQueueWaitIdle(m_Device->GetGraphicsQueue());
}

void Renderer::UpdateFragmentShaderFile(ShaderFile* inFragmentShaderFile)
{
	delete(m_FragmentShaderFile);
	m_FragmentShaderFile = inFragmentShaderFile;
}

void Renderer::OnFragmentShaderRecompiled(const std::vector<uint32_t>& inSpvCode)
{
	WaitQueueToFinish();

	delete(m_FragmentShader);
	m_FragmentShader = new Shader(m_Device, ShaderStage::Fragment, inSpvCode);

	m_ResourceContainer->UpdateBindings(m_FragmentShader->GetBindings());

	RecreateDescriptorSet();

	delete(m_Pipeline);
	m_Pipeline = new Pipeline(m_Device, m_Swapchain, m_DescriptorSet, m_VertexShader, m_FragmentShader);
}

void Renderer::UpdateImageDescriptor(const uint32_t inBindingIndex, const std::string& inPath)
{
	m_ResourceContainer->UpdateImage(inBindingIndex, inPath);
}

void Renderer::UpdateSamplerDescriptor(const uint32_t inBindingIndex, const SamplerInfo& inSamplerInfo)
{
	m_ResourceContainer->UpdateSampler(inBindingIndex, inSamplerInfo);
}

void Renderer::RecreateDescriptorSet()
{
	delete(m_DescriptorSet);
	m_DescriptorSet = new DescriptorSet(m_Device, m_Swapchain, m_ResourceContainer->GetDescriptors());
}

std::vector<Descriptor> Renderer::GetDescriptors() const
{
	return m_ResourceContainer->GetDescriptors();
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
	glfwGetFramebufferSize(m_Window->GetWindow(), &width, &height);
	while (width == 0 || height == 0)
	{
		glfwGetFramebufferSize(m_Window->GetWindow(), &width, &height);
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

	ImDrawData* drawData = ImGui::GetDrawData();
	VkCommandBuffer commandBuffer = m_CommandBuffer->GetCommandBuffer(inSwapchainImageIndex);
	ImGui_ImplVulkan_RenderDrawData(drawData, commandBuffer);

	m_CommandBuffer->End();
}

FT_END_NAMESPACE
