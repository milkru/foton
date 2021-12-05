#pragma once

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

class Renderer
{
public:
	Renderer(Window* inWindow);
	~Renderer();
	FT_DELETE_COPY_AND_MOVE(Renderer)

public:
	void DrawFrame();
	void WaitDeviceToFinish();
	void UpdateFragmentShaderFile(ShaderFile* inFragmentShaderFile);
	void ToggleUserInterface();
	void OnFragmentShaderRecompiled(const std::vector<uint32_t>& inSpvCode);

public:
	Device* GetDevice() const { return m_Device; }
	Swapchain* GetSwapchain() const { return m_Swapchain; }
	ShaderFile* GetFragmentShaderFile() const { return m_FragmentShaderFile; }
	bool IsUserInterfaceEnabled() const { return m_EnableUserInterface; }

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
	bool m_EnableUserInterface;
};

FT_END_NAMESPACE
