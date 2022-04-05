#include "Renderer.h"
#include "Window.h"
#include "Device.h"
#include "Swapchain.h"
#include "Buffer.h"
#include "Image.h"
#include "Sampler.h"
#include "CombinedImageSampler.h"
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

FT_BEGIN_NAMESPACE

Renderer::Renderer(Window* inWindow, ShaderFile* inFragmentShaderFile)
	: m_Window(inWindow)
	, m_FragmentShaderFile(inFragmentShaderFile)
{
	m_Device = new Device(m_Window);
	m_Swapchain = new Swapchain(m_Device, m_Window);

	{
		const char* defaultVertexShader = GetDefaultVertexShader(ShaderLanguage::GLSL);
		const ShaderCompileResult compileResult = ShaderCompiler::Compile(ShaderLanguage::GLSL, ShaderStage::Vertex, defaultVertexShader);
		const char* status = ShaderCompiler::GetStatusText(compileResult.Status);
		FT_CHECK(compileResult.Status == ShaderCompileStatus::Success, "Failed %s default vertex shader.", status);

		m_VertexShader = new Shader(m_Device, ShaderStage::Vertex, compileResult.SpvCode);
	}

	{
		ShaderCompileResult compileResult = ShaderCompiler::Compile(m_FragmentShaderFile->GetLanguage(), ShaderStage::Fragment, m_FragmentShaderFile->GetSourceCode());
		const char* status = ShaderCompiler::GetStatusText(compileResult.Status);
		if (!compileResult.InfoLog.empty())
		{
			FT_LOG(compileResult.InfoLog.c_str());
		}

		if (compileResult.Status != ShaderCompileStatus::Success)
		{
			compileResult = ShaderCompiler::Compile(m_FragmentShaderFile->GetLanguage(), ShaderStage::Fragment, GetDefaultFragmentShader(m_FragmentShaderFile->GetLanguage()));
			FT_LOG("Failed %s fragment shader %s, default shader will be used instead.\n", status, m_FragmentShaderFile->GetName().c_str());

		}

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

	UpdateUniformBuffersDeviceMemory(imageIndex);
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

bool Renderer::TryApplyMetaData()
{
	std::string metaDataFilePath = m_FragmentShaderFile->GetPath() + ".meta";
	std::string metaDataJson = ReadFile(metaDataFilePath);
	
	if (metaDataJson.length() == 0)
	{
		FT_LOG("Meta data json file doesn't exist, new one will be created %s.\n", metaDataFilePath.c_str());
		return false;
	}

	rapidjson::Document documentJson;
	documentJson.Parse(metaDataJson.c_str());
	if (!documentJson.IsObject())
	{
		FT_LOG("Failed parsing a json document from json file %s.\n", metaDataFilePath.c_str());
		return false;
	}

	const rapidjson::Value& descriptorsJson = documentJson["Descriptors"];
	if (!descriptorsJson.IsArray())
	{
		FT_LOG("Failed parsing descriptors from json file %s.\n", metaDataFilePath.c_str());
		return false;
	}

	WaitQueueToFinish();

	uint32_t descriptorIndex = 0;
	for (const auto& descriptorJson : descriptorsJson.GetArray())
	{
		if (descriptorIndex >= m_ResourceContainer->GetDescriptors().size())
		{
			FT_LOG("Failed parsing descriptorIndex indices from json file %s.\n", metaDataFilePath.c_str());
			return false;
		}

		const rapidjson::Value& resourceTypeJson = descriptorJson["Type"];
		if (!resourceTypeJson.IsInt())
		{
			FT_LOG("Failed parsing resource type from a json file %s.\n", metaDataFilePath.c_str());
			return false;
		}

		const rapidjson::Value& resourceJson = descriptorJson["Resource"];
		if (!resourceJson.IsObject())
		{
			FT_LOG("Failed parsing resource from a json file %s.\n", metaDataFilePath.c_str());
			return false;
		}

		const ResourceType resourceType = ResourceType(resourceTypeJson.GetInt());
		switch (resourceType)
		{
		case ResourceType::CombinedImageSampler:
		{
			std::string imagePath;
			SamplerInfo samplerInfo;
			if (!DeserializeCombinedImageSampler(resourceJson, imagePath, samplerInfo))
			{
				FT_LOG("Failed deserializing CombinedImageSampler from a json file %s.\n", metaDataFilePath.c_str());
				return false;
			}

			// TODO: Validate ImagePath and Sampler.
			m_ResourceContainer->UpdateImage(descriptorIndex, imagePath);
			m_ResourceContainer->UpdateSampler(descriptorIndex, samplerInfo);
			break;
		}

		case ResourceType::Image:
		{
			std::string imagePath;
			if (!DeserializeImage(resourceJson, imagePath))
			{
				FT_LOG("Failed deserializing Image from a json file %s.\n", metaDataFilePath.c_str());
				return false;
			}

			// TODO: Validate ImagePath.
			m_ResourceContainer->UpdateImage(descriptorIndex, imagePath);
			break;
		}

		case ResourceType::Sampler:
		{
			SamplerInfo samplerInfo;
			if (!DeserializeSampler(resourceJson, samplerInfo))
			{
				FT_LOG("Failed deserializing Sampler from a json file %s.\n", metaDataFilePath.c_str());
				return false;
			}

			// TODO: Validate Sampler.
			m_ResourceContainer->UpdateSampler(descriptorIndex, samplerInfo);
			break;
		}

		case ResourceType::UniformBuffer:
		{
			size_t size;
			unsigned char* proxyMemory;
			unsigned char* vectorState;
			if (!DeserializeUniformBuffer(resourceJson, size, proxyMemory, vectorState))
			{
				FT_LOG("Failed deserializing UniformBuffer from a json file %s.\n", metaDataFilePath.c_str());
				return false;
			}

			m_ResourceContainer->UpdateUniformBuffer(descriptorIndex, size, proxyMemory, vectorState);
			break;
		}

		default:
			FT_LOG("Failed parsing ResourceType from a json file %s.\n", metaDataFilePath.c_str());
			return false;
		}

		++descriptorIndex;
	}

	RecreateDescriptorSet();

	return true;
}

void Renderer::SaveMetaData()
{
	rapidjson::Document documentJson(rapidjson::kObjectType);

	rapidjson::Value descriptorsJson = m_ResourceContainer->Serialize(documentJson.GetAllocator());
	documentJson.AddMember("Descriptors", descriptorsJson, documentJson.GetAllocator());

	rapidjson::StringBuffer buffer;
	rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
	documentJson.Accept(writer);

	std::string metaDataFilePath = m_FragmentShaderFile->GetPath() + ".meta";
	std::string metaDataJson = buffer.GetString();

	WriteFile(metaDataFilePath, metaDataJson);
}

void Renderer::UpdateImageDescriptor(const uint32_t inDescriptorIndex, const std::string& inPath)
{
	m_ResourceContainer->UpdateImage(inDescriptorIndex, inPath);
}

void Renderer::UpdateSamplerDescriptor(const uint32_t inDescriptorIndex, const SamplerInfo& inSamplerInfo)
{
	m_ResourceContainer->UpdateSampler(inDescriptorIndex, inSamplerInfo);
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

void Renderer::UpdateUniformBuffersDeviceMemory(uint32_t inCurrentImage)
{
	for (const auto& descriptor : m_ResourceContainer->GetDescriptors())
	{
		if (descriptor.Resource.Type == ResourceType::UniformBuffer)
		{
			descriptor.Resource.Handle.UniformBuffer->UpdateDeviceMemory(inCurrentImage);
		}
	}
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
