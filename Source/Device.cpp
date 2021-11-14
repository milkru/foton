#include "Device.h"

namespace FT
{
#if !defined(NDEBUG) // TODO: FT_DEBUG
	const static bool enableValidationLayers = true;
#else
	const static bool enableValidationLayers = false;
#endif

	const static std::vector<const char*> deviceExtensions =
	{
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

	const static std::vector<const char*> validationLayers =
	{
		"VK_LAYER_KHRONOS_validation"
	};

	bool CheckValidationLayerSupport()
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

	std::vector<const char*> GetRequiredExtensions()
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

	void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
	{
		createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		createInfo.messageSeverity =
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		createInfo.messageType =
			VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		createInfo.pfnUserCallback = DebugMessageCallback;
	}

	void CreateInstance(VkInstance& outInstance)
	{
		if (enableValidationLayers)
		{
			FT_CHECK(CheckValidationLayerSupport(), "Validation layers requested, but not available.");
		}

		VkApplicationInfo applicationInfo{};
		applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		applicationInfo.pApplicationName = "foton";
		applicationInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		applicationInfo.pEngineName = "foton";
		applicationInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		applicationInfo.apiVersion = VK_API_VERSION_1_0;

		VkInstanceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &applicationInfo;

		const std::vector<const char*> requiredExtensions = GetRequiredExtensions();
		createInfo.enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size());
		createInfo.ppEnabledExtensionNames = requiredExtensions.data();

		if (enableValidationLayers)
		{
			createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			createInfo.ppEnabledLayerNames = validationLayers.data();

			VkDebugUtilsMessengerCreateInfoEXT debugMessangerCreateInfo{};
			PopulateDebugMessengerCreateInfo(debugMessangerCreateInfo);
			createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugMessangerCreateInfo;
		}
		else
		{
			createInfo.enabledLayerCount = 0;
			createInfo.pNext = nullptr;
		}

		FT_VK_CALL(vkCreateInstance(&createInfo, nullptr, &outInstance));
	}

	void SetupDebugMessenger(const VkInstance inInstance, VkDebugUtilsMessengerEXT& outDebugMessanger)
	{
		if (!enableValidationLayers)
		{
			return;
		}

		VkDebugUtilsMessengerCreateInfoEXT createInfo;
		PopulateDebugMessengerCreateInfo(createInfo);

		const auto vkCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(inInstance, "vkCreateDebugUtilsMessengerEXT");
		FT_CHECK(vkCreateDebugUtilsMessengerEXT != nullptr, "Unable to get vkCreateDebugUtilsMessengerEXT extension function.");

		FT_VK_CALL(vkCreateDebugUtilsMessengerEXT(inInstance, &createInfo, nullptr, &outDebugMessanger));
	}

	void CreateSurface(const VkInstance inInstance, GLFWwindow* inWindow, VkSurfaceKHR& outSurface)
	{
		FT_VK_CALL(glfwCreateWindowSurface(inInstance, inWindow, nullptr, &outSurface));
	}

	uint32_t FindGraphicsQueueFamily(const VkPhysicalDevice inDevice)
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

	bool IsDeviceExtensionAvailable(const std::vector<VkExtensionProperties> inAvailableExtensions, const std::string& inExtensionName)
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

	bool CheckDeviceExtensionSupport(const VkPhysicalDevice inDevice)
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

	bool IsDeviceSuitable(const VkPhysicalDevice inDevice, const VkSurfaceKHR inSurface)
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

	void PickPhysicalDevice(const VkInstance inInstance, const VkSurfaceKHR inSurface, VkPhysicalDevice& outPhysicalDevice)
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

	void CreateLogicalDevice(const VkPhysicalDevice inPhysicalDevice, const VkSurfaceKHR inSurface, VkDevice& outDevice, VkQueue& outGraphicsQueue, uint32_t& outGraphicsQueueFamilyIndex)
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

		VkDeviceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		createInfo.queueCreateInfoCount = 1;
		createInfo.pQueueCreateInfos = &queueCreateInfo;
		createInfo.pEnabledFeatures = &deviceFeatures;
		createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
		createInfo.ppEnabledExtensionNames = deviceExtensions.data();

		if (enableValidationLayers)
		{
			createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			createInfo.ppEnabledLayerNames = validationLayers.data();
		}
		else
		{
			createInfo.enabledLayerCount = 0;
		}

		FT_VK_CALL(vkCreateDevice(inPhysicalDevice, &createInfo, nullptr, &outDevice));

		vkGetDeviceQueue(outDevice, outGraphicsQueueFamilyIndex, 0, &outGraphicsQueue);
	}

	void CreateCommandPool(const VkDevice inDevice, const uint32_t inQueueFamilyIndex, VkCommandPool& outCommandPool)
	{
		VkCommandPoolCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		createInfo.queueFamilyIndex = inQueueFamilyIndex;
		createInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

		FT_VK_CALL(vkCreateCommandPool(inDevice, &createInfo, nullptr, &outCommandPool));
	}

	Device::Device(GLFWwindow* inWindow)
	{
		CreateInstance(m_Instance);
		SetupDebugMessenger(m_Instance, m_DebugMessenger);
		CreateSurface(m_Instance, inWindow, m_Surface);
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

	void Device::FreeCommandBuffers()
	{
		vkFreeCommandBuffers(m_Device, m_CommandPool, static_cast<uint32_t>(m_CommandBuffers.size()), m_CommandBuffers.data());
	}

	void Device::AllocateCommandBuffers(const uint32_t inSwapchainImageCount)
	{
		m_CommandBuffers.resize(inSwapchainImageCount);

		VkCommandBufferAllocateInfo allocateInfo{};
		allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocateInfo.commandPool = m_CommandPool;
		allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocateInfo.commandBufferCount = (uint32_t)m_CommandBuffers.size();

		FT_VK_CALL(vkAllocateCommandBuffers(m_Device, &allocateInfo, m_CommandBuffers.data()));
	}

	uint32_t Device::FindMemoryType(const uint32_t inTypeFilter, const VkMemoryPropertyFlags inProperties) const
	{
		VkPhysicalDeviceMemoryProperties memProperties;
		vkGetPhysicalDeviceMemoryProperties(m_PhysicalDevice, &memProperties);

		for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i)
		{
			if ((inTypeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & inProperties) == inProperties)
			{
				return i;
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
}
