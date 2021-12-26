#pragma once

#include "Descriptor.hpp"

FT_BEGIN_NAMESPACE

class Window;
class Device;
class Swapchain;
class Shader;
class Shader;
class ShaderFile;
class Pipeline;
class DescriptorSet;
class CommandBuffer;
class ResourceContainer;
struct SamplerInfo;

class Renderer
{
public:
	Renderer(Window* inWindow);
	~Renderer();
	FT_DELETE_COPY_AND_MOVE(Renderer)

public:
	void DrawFrame();
	void WaitDeviceToFinish();
	void WaitQueueToFinish();
	void UpdateFragmentShaderFile(ShaderFile* inFragmentShaderFile);
	void OnFragmentShaderRecompiled(const std::vector<uint32_t>& inSpvCode);
	void UpdateImageDescriptor(const uint32_t inBindingIndex, const std::string& inPath);
	void UpdateSamplerDescriptor(const uint32_t inBindingIndex, const SamplerInfo& inSamplerInfo);
	void RecreateDescriptorSet();

public:
	Device* GetDevice() const { return m_Device; }
	Swapchain* GetSwapchain() const { return m_Swapchain; }
	ShaderFile* GetFragmentShaderFile() const { return m_FragmentShaderFile; }
	std::vector<Descriptor> GetDescriptors() const;

private:
	void CleanupSwapchain();
	void RecreateSwapchain();
	void FillCommandBuffers(uint32_t inSwapchainImageIndex);

private:
	Window* m_Window;
	Device* m_Device;
	Swapchain* m_Swapchain;
	Shader* m_VertexShader;
	Shader* m_FragmentShader;
	ShaderFile* m_FragmentShaderFile;
	Pipeline* m_Pipeline;
	DescriptorSet* m_DescriptorSet;
	CommandBuffer* m_CommandBuffer;
	ResourceContainer* m_ResourceContainer;
};

FT_END_NAMESPACE
