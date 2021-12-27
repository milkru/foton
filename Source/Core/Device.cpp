#include "Device.h"
#include "Window.h"

FT_BEGIN_NAMESPACE

#ifdef FT_DEBUG
	static const bool enableValidationLayers = true;
#else
	static const bool enableValidationLayers = false;
#endif // FT_DEBUG

static const std::vector<const char*> deviceExtensions =
{
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

static const std::vector<const char*> validationLayers =
{
	"VK_LAYER_KHRONOS_validation"
};

static bool CheckValidationLayerSupport()
{
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

	for (const char* layerName : validationLayers)
	{
		bool layerFound = false;

		for (const auto& layerProperties : availableLayers)
		{
			if (strcmp(layerName, layerProperties.layerName) == 0)
			{
				layerFound = true;
				break;
			}
		}

		if (!layerFound)
		{
			return false;
		}
	}

	return true;
}

static std::vector<const char*> GetRequiredExtensions()
{
	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

	if (enableValidationLayers)
	{
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}

	return extensions;
}

VKAPI_ATTR VkBool32 VKAPI_CALL DebugMessageCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData)
{
	FT_LOG("Validation layer: %s\n", pCallbackData->pMessage);
	return VK_FALSE;
}

static void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& debugMessangerCreateInfo)
{
	debugMessangerCreateInfo = {};
	debugMessangerCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	debugMessangerCreateInfo.messageSeverity =
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	debugMessangerCreateInfo.messageType =
		VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	debugMessangerCreateInfo.pfnUserCallback = DebugMessageCallback;
}

static void CreateInstance(VkInstance& outInstance)
{
	if (enableValidationLayers)
	{
		FT_CHECK(CheckValidationLayerSupport(), "Validation layers requested, but not available.");
	}

	VkApplicationInfo applicationInfo{};
	applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	applicationInfo.pApplicationName = FT_APPLICATION_NAME;
	applicationInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	applicationInfo.pEngineName = FT_APPLICATION_NAME;
	applicationInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	applicationInfo.apiVersion = VK_API_VERSION_1_0;

	VkInstanceCreateInfo instanceCreateInfo{};
	instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instanceCreateInfo.pApplicationInfo = &applicationInfo;

	const std::vector<const char*> requiredExtensions = GetRequiredExtensions();
	instanceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size());
	instanceCreateInfo.ppEnabledExtensionNames = requiredExtensions.data();

	if (enableValidationLayers)
	{
		instanceCreateInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		instanceCreateInfo.ppEnabledLayerNames = validationLayers.data();

		VkDebugUtilsMessengerCreateInfoEXT debugMessangerCreateInfo{};
		PopulateDebugMessengerCreateInfo(debugMessangerCreateInfo);
		instanceCreateInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugMessangerCreateInfo;
	}
	else
	{
		instanceCreateInfo.enabledLayerCount = 0;
		instanceCreateInfo.pNext = nullptr;
	}

	FT_VK_CALL(vkCreateInstance(&instanceCreateInfo, nullptr, &outInstance));
}

static void SetupDebugMessenger(const VkInstance inInstance, VkDebugUtilsMessengerEXT& outDebugMessanger)
{
	if (!enableValidationLayers)
	{
		return;
	}

	VkDebugUtilsMessengerCreateInfoEXT debugMessangerCreateInfo;
	PopulateDebugMessengerCreateInfo(debugMessangerCreateInfo);

	const auto vkCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(inInstance, "vkCreateDebugUtilsMessengerEXT");
	FT_CHECK(vkCreateDebugUtilsMessengerEXT != nullptr, "Unable to get vkCreateDebugUtilsMessengerEXT extension function.");

	FT_VK_CALL(vkCreateDebugUtilsMessengerEXT(inInstance, &debugMessangerCreateInfo, nullptr, &outDebugMessanger));
}

static void CreateSurface(const VkInstance inInstance, GLFWwindow* inWindow, VkSurfaceKHR& outSurface)
{
	FT_VK_CALL(glfwCreateWindowSurface(inInstance, inWindow, nullptr, &outSurface));
}

static uint32_t FindGraphicsQueueFamily(const VkPhysicalDevice inDevice)
{
	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(inDevice, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(inDevice, &queueFamilyCount, queueFamilies.data());

	int i = 0;
	for (const auto& queueFamily : queueFamilies)
	{
		if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
			return i;
		}

		++i;
	}

	FT_FAIL("Could not find a graphics queue family index.");
}

static bool IsDeviceExtensionAvailable(const std::vector<VkExtensionProperties> inAvailableExtensions, const std::string& inExtensionName)
{
	for (const VkExtensionProperties& availableExtension : inAvailableExtensions)
	{
		if (strcmp(availableExtension.extensionName, inExtensionName.c_str()) == 0)
		{
			return true;
		}
	}

	return false;
}

static bool CheckDeviceExtensionSupport(const VkPhysicalDevice inDevice)
{
	uint32_t extensionCount;
	vkEnumerateDeviceExtensionProperties(inDevice, nullptr, &extensionCount, nullptr);

	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(inDevice, nullptr, &extensionCount, availableExtensions.data());

	for (const auto& extensionName : deviceExtensions)
	{
		if (!IsDeviceExtensionAvailable(availableExtensions, extensionName))
		{
			return false;
		}
	}

	return true;
}

static bool IsDeviceSuitable(const VkPhysicalDevice inDevice, const VkSurfaceKHR inSurface)
{
	uint32_t index = FindGraphicsQueueFamily(inDevice);

	bool extensionsSupported = CheckDeviceExtensionSupport(inDevice);

	bool swapChainAdequate = false;
	if (extensionsSupported)
	{
		uint32_t formatCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR(inDevice, inSurface, &formatCount, nullptr);

		uint32_t presentModeCount;
		vkGetPhysicalDeviceSurfacePresentModesKHR(inDevice, inSurface, &presentModeCount, nullptr);

		swapChainAdequate = formatCount > 0 && presentModeCount > 0;
	}

	VkPhysicalDeviceFeatures supportedFeatures;
	vkGetPhysicalDeviceFeatures(inDevice, &supportedFeatures);

	return index >= 0 && extensionsSupported && swapChainAdequate && supportedFeatures.samplerAnisotropy;
}

static void PickPhysicalDevice(const VkInstance inInstance, const VkSurfaceKHR inSurface, VkPhysicalDevice& outPhysicalDevice)
{
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(inInstance, &deviceCount, nullptr);

	FT_CHECK(deviceCount != 0, "Failed to find GPUs with Vulkan support.");

	std::vector<VkPhysicalDevice> physicalDevices(deviceCount);
	vkEnumeratePhysicalDevices(inInstance, &deviceCount, physicalDevices.data());

	for (const auto& physicalDevice : physicalDevices)
	{
		if (IsDeviceSuitable(physicalDevice, inSurface))
		{
			outPhysicalDevice = physicalDevice;
			return;
		}
	}

	FT_FAIL("Failed to find a suitable GPU.");
}

static void CreateLogicalDevice(const VkPhysicalDevice inPhysicalDevice, const VkSurfaceKHR inSurface, VkDevice& outDevice, VkQueue& outGraphicsQueue, uint32_t& outGraphicsQueueFamilyIndex)
{
	outGraphicsQueueFamilyIndex = FindGraphicsQueueFamily(inPhysicalDevice);

	VkBool32 presentSupport = false;
	vkGetPhysicalDeviceSurfaceSupportKHR(inPhysicalDevice, outGraphicsQueueFamilyIndex, inSurface, &presentSupport);

	FT_CHECK(presentSupport, "Device doesn't support present.");

	float queuePriority = 1.0f;
	VkDeviceQueueCreateInfo queueCreateInfo{};
	queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueCreateInfo.queueFamilyIndex = outGraphicsQueueFamilyIndex;
	queueCreateInfo.queueCount = 1;
	queueCreateInfo.pQueuePriorities = &queuePriority;

	VkPhysicalDeviceFeatures deviceFeatures{};
	deviceFeatures.samplerAnisotropy = VK_TRUE;

	VkDeviceCreateInfo deviceCreateInfo{};
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceCreateInfo.queueCreateInfoCount = 1;
	deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
	deviceCreateInfo.pEnabledFeatures = &deviceFeatures;
	deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
	deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();

	if (enableValidationLayers)
	{
		deviceCreateInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		deviceCreateInfo.ppEnabledLayerNames = validationLayers.data();
	}
	else
	{
		deviceCreateInfo.enabledLayerCount = 0;
	}

	FT_VK_CALL(vkCreateDevice(inPhysicalDevice, &deviceCreateInfo, nullptr, &outDevice));

	vkGetDeviceQueue(outDevice, outGraphicsQueueFamilyIndex, 0, &outGraphicsQueue);
}

static void CreateCommandPool(const VkDevice inDevice, const uint32_t inQueueFamilyIndex, VkCommandPool& outCommandPool)
{
	VkCommandPoolCreateInfo commandPoolCreateInfo{};
	commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	commandPoolCreateInfo.queueFamilyIndex = inQueueFamilyIndex;
	commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

	FT_VK_CALL(vkCreateCommandPool(inDevice, &commandPoolCreateInfo, nullptr, &outCommandPool));
}

Device::Device(const Window* inWindow)
{
	CreateInstance(m_Instance);
	SetupDebugMessenger(m_Instance, m_DebugMessenger);
	CreateSurface(m_Instance, inWindow->GetWindow(), m_Surface);
	PickPhysicalDevice(m_Instance, m_Surface, m_PhysicalDevice);
	CreateLogicalDevice(m_PhysicalDevice, m_Surface, m_Device, m_GraphicsQueue, m_GraphicsQueueFamilyIndex);
	CreateCommandPool(m_Device, m_GraphicsQueueFamilyIndex, m_CommandPool);
}

Device::~Device()
{
	vkDestroyCommandPool(m_Device, m_CommandPool, nullptr);

	vkDestroyDevice(m_Device, nullptr);

	if (enableValidationLayers)
	{
		const auto vkDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_Instance, "vkDestroyDebugUtilsMessengerEXT");
		FT_CHECK(vkDestroyDebugUtilsMessengerEXT != nullptr, "Unable to get vkDestroyDebugUtilsMessengerEXT extension function.");
			
		vkDestroyDebugUtilsMessengerEXT(m_Instance, m_DebugMessenger, nullptr);
	}

	vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);
	vkDestroyInstance(m_Instance, nullptr);
}

uint32_t Device::FindMemoryType(const uint32_t inTypeFilter, const VkMemoryPropertyFlags inProperties) const
{
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(m_PhysicalDevice, &memProperties);

	for (uint32_t memoryTypeIndex = 0; memoryTypeIndex < memProperties.memoryTypeCount; ++memoryTypeIndex)
	{
		if ((inTypeFilter & (1 << memoryTypeIndex)) && (memProperties.memoryTypes[memoryTypeIndex].propertyFlags & inProperties) == inProperties)
		{
			return memoryTypeIndex;
		}
	}

	FT_FAIL("Failed to find suitable memory type.");
}

VkCommandBuffer Device::BeginSingleTimeCommands() const
{
	VkCommandBufferAllocateInfo allocateInfo{};
	allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocateInfo.commandPool = m_CommandPool;
	allocateInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(m_Device, &allocateInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	return commandBuffer;
}

void Device::EndSingleTimeCommands(VkCommandBuffer inCommandBuffer) const
{
	vkEndCommandBuffer(inCommandBuffer);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &inCommandBuffer;

	vkQueueSubmit(m_GraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(m_GraphicsQueue);

	vkFreeCommandBuffers(m_Device, m_CommandPool, 1, &inCommandBuffer);
}

FT_END_NAMESPACE
