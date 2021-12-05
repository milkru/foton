#pragma once

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

namespace FT
{
	class Renderer
	{
	public:
		Renderer(GLFWwindow* inWindow)
			: m_Window(inWindow)
			, m_EnableUserInterface(true)
		{
			InitializeVulkan(m_Window);
		}

		~Renderer()
		{
			CleanupVulkan();
		}

		void DrawFramePUBLIC()
		{
			DrawFrame();
		}

		void WaitDevice()
		{
			vkDeviceWaitIdle(m_Device->GetDevice());
		}

	private:
		void InitializeVulkan(GLFWwindow* window)
		{
			m_Device = new Device(window);
			m_Swapchain = new Swapchain(m_Device, window);

			CreateShaders();

			m_ResourceContainer = new ResourceContainer(m_Device, m_Swapchain);
			m_ResourceContainer->UpdateBindings(m_FragmentShader->GetBindings());

			m_DescriptorSet = new DescriptorSet(m_Device, m_Swapchain, m_ResourceContainer->GetDescriptors());
			m_Pipeline = new Pipeline(m_Device, m_Swapchain, m_DescriptorSet, m_VertexShader, m_FragmentShader);
			m_CommandBuffer = new CommandBuffer(m_Device, m_Swapchain);
		}

		void CreateShaders()
		{
			// TODO: How to prevent loading uncompilable file??? Maybe make internal shader somehow uneditable or serialized and fallback to them if previous shader cannot be compiled.
			{
				const ShaderFile shaderFile(GetFullPath("Shaders/Internal/FullScreen.vert.glsl"));

				const ShaderCompileResult compileResult = CompileShader(shaderFile.GetLanguage(), ShaderStage::Vertex, shaderFile.GetSourceCode());
				FT_CHECK(compileResult.Status == ShaderCompileStatus::Success, "Failed %s vertex shader %s.", ConvertCompilationStatusToText(compileResult.Status), shaderFile.GetName().c_str());

				m_VertexShader = new Shader(m_Device, ShaderStage::Vertex, compileResult.SpvCode);
			}

			{
				m_FragmentShaderFile = new ShaderFile(GetFullPath("Shaders/Internal/Default.frag.glsl"));

				const ShaderCompileResult compileResult = CompileShader(m_FragmentShaderFile->GetLanguage(), ShaderStage::Fragment, m_FragmentShaderFile->GetSourceCode());
				FT_CHECK(compileResult.Status == ShaderCompileStatus::Success, "Failed %s fragment shader %s.", ConvertCompilationStatusToText(compileResult.Status), m_FragmentShaderFile->GetName().c_str());

				m_FragmentShader = new Shader(m_Device, ShaderStage::Fragment, compileResult.SpvCode);
			}
		}




		void CleanupSwapchain()
		{
			m_Swapchain->Cleanup();

			delete(m_CommandBuffer);
			delete(m_Pipeline);
			delete(m_DescriptorSet);
		}

		void CleanupVulkan()
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











		void RecreateSwapchain()
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

		void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
		{
			VkCommandBuffer commandBuffer = m_Device->BeginSingleTimeCommands();

			VkBufferCopy copyRegion{};
			copyRegion.size = size;
			vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

			m_Device->EndSingleTimeCommands(commandBuffer);
		}

		void UpdateUniformBuffer(uint32_t currentImage)
		{
			//static auto startTime = std::chrono::high_resolution_clock::now();
			//
			//auto currentTime = std::chrono::high_resolution_clock::now();
			//float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
			//
			//UniformBufferObject ubo{};
			//ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
			//ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
			//ubo.proj = glm::perspective(glm::radians(45.0f), m_Swapchain->GetExtent().width / static_cast<float>(m_Swapchain->GetExtent().height), 0.1f, 10.0f);
			//ubo.proj[1][1] *= -1;
			//
			//void* data = m_UniformBuffer->Map(currentImage);
			//memcpy(data, &ubo, sizeof(ubo));
			//m_UniformBuffer->Unmap(currentImage);
		}











		// TODO: Move to device and call it only when shader gets compiled/recompiled.
		void FillCommandBuffers(uint32_t inSwapchainImageIndex)
		{
			m_CommandBuffer->Begin(inSwapchainImageIndex, m_Swapchain);
			m_CommandBuffer->BindPipeline(m_Pipeline);
			m_CommandBuffer->BindDescriptorSet(m_DescriptorSet);
			m_CommandBuffer->Draw();

			if (m_EnableUserInterface)
			{
				ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), m_CommandBuffer->GetCommandBuffer(inSwapchainImageIndex));
			}

			m_CommandBuffer->End();
		}

		void DrawFrame()
		{
			const SwapchainImageAcquireResult imageAcquireResult = m_Swapchain->AcquireNextImage();
			if (imageAcquireResult.Status == SwapchainStatus::Recreate)
			{
				RecreateSwapchain();
				return;
			}

			FT_CHECK(imageAcquireResult.Status == SwapchainStatus::Success, "Failed acquiring swapchain image.");

			const uint32_t imageIndex = imageAcquireResult.ImageIndex;

			UpdateUniformBuffer(imageIndex);
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





	public:
		void UpdateFragmentShaderFile(ShaderFile* inFragmentShaderFile)
		{
			delete(m_FragmentShaderFile);
			m_FragmentShaderFile = inFragmentShaderFile;
		}

		void ToggleUserInterface()
		{
			// TODO: Imgui Demo Window -> Style -> Rendering -> Global alpha. You can use this to fade toggle.
			m_EnableUserInterface = !m_EnableUserInterface;
		}

		void OnFragmentShaderRecompiled(const std::vector<uint32_t>& inSpvCode)
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

	public:
		Device* GetDevice() const { return m_Device; }
		Swapchain* GetSwapchain() const { return m_Swapchain; }
		ShaderFile* GetFragmentShaderFile() const { return m_FragmentShaderFile; }
		bool IsUserInterfaceEnabled() const { return m_EnableUserInterface; }

	private:
		GLFWwindow* m_Window;

		Device* m_Device;
		Swapchain* m_Swapchain;

		Shader* m_VertexShader;
		Shader* m_FragmentShader;
		ShaderFile* m_FragmentShaderFile;

		Pipeline* m_Pipeline;
		DescriptorSet* m_DescriptorSet;
		CommandBuffer* m_CommandBuffer;
		ResourceContainer* m_ResourceContainer;

		bool m_EnableUserInterface;
	};
}
