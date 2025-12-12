/**
 * @file context.cpp
 * @brief Vulkan context implementation
 */

#include "tinyvk/renderer/context.h"
#include "tinyvk/core/log.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <set>
#include <cstring>
#include <algorithm>

namespace tvk {

// Validation layers
static const std::vector<const char*> s_ValidationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

// Required device extensions
static const std::vector<const char*> s_DeviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
#ifdef TVK_PLATFORM_APPLE
    "VK_KHR_portability_subset",
#endif
};

// Debug callback
static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData) {
    
    if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
        TVK_LOG_ERROR("Vulkan: {}", pCallbackData->pMessage);
    } else if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        TVK_LOG_WARN("Vulkan: {}", pCallbackData->pMessage);
    } else if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
        TVK_LOG_DEBUG("Vulkan: {}", pCallbackData->pMessage);
    } else {
        TVK_LOG_TRACE("Vulkan: {}", pCallbackData->pMessage);
    }
    
    return VK_FALSE;
}

// Helper functions for debug messenger
static VkResult CreateDebugUtilsMessengerEXT(
    VkInstance instance,
    const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkDebugUtilsMessengerEXT* pDebugMessenger) {
    
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
        instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    }
    return VK_ERROR_EXTENSION_NOT_PRESENT;
}

static void DestroyDebugUtilsMessengerEXT(
    VkInstance instance,
    VkDebugUtilsMessengerEXT debugMessenger,
    const VkAllocationCallbacks* pAllocator) {
    
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
        instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}

VulkanContext::~VulkanContext() {
    Cleanup();
}

bool VulkanContext::Init(GLFWwindow* window, const ContextConfig& config) {
    m_ValidationEnabled = config.enableValidation;

    if (!CreateInstance(config)) {
        TVK_LOG_ERROR("Failed to create Vulkan instance");
        return false;
    }

    if (m_ValidationEnabled && !SetupDebugMessenger()) {
        TVK_LOG_WARN("Failed to set up debug messenger");
    }

    if (!CreateSurface(window)) {
        TVK_LOG_ERROR("Failed to create window surface");
        return false;
    }

    if (!PickPhysicalDevice()) {
        TVK_LOG_ERROR("Failed to find suitable GPU");
        return false;
    }

    if (!CreateLogicalDevice(config)) {
        TVK_LOG_ERROR("Failed to create logical device");
        return false;
    }

    if (!CreateCommandPool()) {
        TVK_LOG_ERROR("Failed to create command pool");
        return false;
    }

    if (!CreateDescriptorPool()) {
        TVK_LOG_ERROR("Failed to create descriptor pool");
        return false;
    }

    TVK_LOG_INFO("Vulkan context initialized successfully");
    TVK_LOG_INFO("GPU: {}", m_DeviceProperties.deviceName);
    return true;
}

void VulkanContext::Cleanup() {
    if (m_Device != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(m_Device);

        if (m_DescriptorPool != VK_NULL_HANDLE) {
            vkDestroyDescriptorPool(m_Device, m_DescriptorPool, nullptr);
            m_DescriptorPool = VK_NULL_HANDLE;
        }

        if (m_CommandPool != VK_NULL_HANDLE) {
            vkDestroyCommandPool(m_Device, m_CommandPool, nullptr);
            m_CommandPool = VK_NULL_HANDLE;
        }

        vkDestroyDevice(m_Device, nullptr);
        m_Device = VK_NULL_HANDLE;
    }

    if (m_Instance != VK_NULL_HANDLE) {
        if (m_Surface != VK_NULL_HANDLE) {
            vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);
            m_Surface = VK_NULL_HANDLE;
        }

        if (m_ValidationEnabled && m_DebugMessenger != VK_NULL_HANDLE) {
            DestroyDebugUtilsMessengerEXT(m_Instance, m_DebugMessenger, nullptr);
            m_DebugMessenger = VK_NULL_HANDLE;
        }

        vkDestroyInstance(m_Instance, nullptr);
        m_Instance = VK_NULL_HANDLE;
    }
}

void VulkanContext::WaitIdle() {
    if (m_Device != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(m_Device);
    }
}

bool VulkanContext::CreateInstance(const ContextConfig& config) {
    if (m_ValidationEnabled && !CheckValidationLayerSupport()) {
        TVK_LOG_WARN("Validation layers requested but not available");
        m_ValidationEnabled = false;
    }

    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pNext = nullptr;
    appInfo.pApplicationName = "TinyVK Application";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "TinyVK";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_2;

    auto extensions = GetRequiredExtensions(config);

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pNext = nullptr;
    createInfo.flags = 0;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = static_cast<u32>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();
    createInfo.enabledLayerCount = 0;
    createInfo.ppEnabledLayerNames = nullptr;

#ifdef TVK_PLATFORM_APPLE
    createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    if (m_ValidationEnabled) {
        createInfo.enabledLayerCount = static_cast<u32>(s_ValidationLayers.size());
        createInfo.ppEnabledLayerNames = s_ValidationLayers.data();

        debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        debugCreateInfo.messageSeverity = 
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        debugCreateInfo.messageType = 
            VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        debugCreateInfo.pfnUserCallback = DebugCallback;
        createInfo.pNext = &debugCreateInfo;

        TVK_LOG_INFO("Validation layers enabled:");
        for (u32 i = 0; i < s_ValidationLayers.size(); i++) {
            TVK_LOG_INFO("  {}", s_ValidationLayers[i]);
        }
    } else {
        createInfo.enabledLayerCount = 0;
        createInfo.pNext = nullptr;
    }

    VkResult result = vkCreateInstance(&createInfo, nullptr, &m_Instance);
    
    if (result != VK_SUCCESS) {
        TVK_LOG_ERROR("Failed to create Vulkan instance (error code: {})", static_cast<int>(result));
        TVK_LOG_ERROR("Try setting environment variables: DISABLE_LAYER_AMD_SWITCHABLE_GRAPHICS_1=1 and DISABLE_RTSS_LAYER=1");
        return false;
    }

    return true;
}

bool VulkanContext::SetupDebugMessenger() {
    VkDebugUtilsMessengerCreateInfoEXT createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = 
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = 
        VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = DebugCallback;

    return CreateDebugUtilsMessengerEXT(m_Instance, &createInfo, nullptr, &m_DebugMessenger) == VK_SUCCESS;
}

bool VulkanContext::CreateSurface(GLFWwindow* window) {
    return glfwCreateWindowSurface(m_Instance, window, nullptr, &m_Surface) == VK_SUCCESS;
}

bool VulkanContext::PickPhysicalDevice() {
    u32 deviceCount = 0;
    vkEnumeratePhysicalDevices(m_Instance, &deviceCount, nullptr);

    if (deviceCount == 0) {
        TVK_LOG_ERROR("Failed to find GPUs with Vulkan support");
        return false;
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(m_Instance, &deviceCount, devices.data());

    // Pick the best suitable device (prefer discrete GPU)
    VkPhysicalDevice bestDevice = VK_NULL_HANDLE;
    int bestScore = -1;

    for (const auto& device : devices) {
        ContextConfig defaultConfig;
        if (!IsDeviceSuitable(device, defaultConfig)) continue;

        VkPhysicalDeviceProperties props;
        vkGetPhysicalDeviceProperties(device, &props);

        int score = 0;
        if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            score += 1000;
        }
        score += props.limits.maxImageDimension2D;

        if (score > bestScore) {
            bestScore = score;
            bestDevice = device;
        }
    }

    if (bestDevice == VK_NULL_HANDLE) {
        return false;
    }

    m_PhysicalDevice = bestDevice;
    vkGetPhysicalDeviceProperties(m_PhysicalDevice, &m_DeviceProperties);
    vkGetPhysicalDeviceMemoryProperties(m_PhysicalDevice, &m_MemoryProperties);
    m_QueueFamilyIndices = FindQueueFamilies(m_PhysicalDevice);

    return true;
}

bool VulkanContext::CreateLogicalDevice(const ContextConfig& config) {
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<u32> uniqueQueueFamilies = {
        m_QueueFamilyIndices.graphicsFamily.value(),
        m_QueueFamilyIndices.presentFamily.value()
    };

    float queuePriority = 1.0f;
    for (u32 queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    // Query supported features
    VkPhysicalDeviceFeatures supportedFeatures;
    vkGetPhysicalDeviceFeatures(m_PhysicalDevice, &supportedFeatures);

    // Only request features that are supported
    VkPhysicalDeviceFeatures deviceFeatures{};
    if (supportedFeatures.samplerAnisotropy) {
        deviceFeatures.samplerAnisotropy = VK_TRUE;
    }
    if (supportedFeatures.fillModeNonSolid) {
        deviceFeatures.fillModeNonSolid = VK_TRUE;
    }
    if (supportedFeatures.wideLines) {
        deviceFeatures.wideLines = VK_TRUE;
    }

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount = static_cast<u32>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.pEnabledFeatures = &deviceFeatures;
    createInfo.enabledExtensionCount = static_cast<u32>(s_DeviceExtensions.size());
    createInfo.ppEnabledExtensionNames = s_DeviceExtensions.data();

    if (m_ValidationEnabled) {
        createInfo.enabledLayerCount = static_cast<u32>(s_ValidationLayers.size());
        createInfo.ppEnabledLayerNames = s_ValidationLayers.data();
    } else {
        createInfo.enabledLayerCount = 0;
    }

    if (vkCreateDevice(m_PhysicalDevice, &createInfo, nullptr, &m_Device) != VK_SUCCESS) {
        return false;
    }

    vkGetDeviceQueue(m_Device, m_QueueFamilyIndices.graphicsFamily.value(), 0, &m_GraphicsQueue);
    vkGetDeviceQueue(m_Device, m_QueueFamilyIndices.presentFamily.value(), 0, &m_PresentQueue);

    return true;
}

bool VulkanContext::CreateCommandPool() {
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = m_QueueFamilyIndices.graphicsFamily.value();

    return vkCreateCommandPool(m_Device, &poolInfo, nullptr, &m_CommandPool) == VK_SUCCESS;
}

bool VulkanContext::CreateDescriptorPool() {
    std::vector<VkDescriptorPoolSize> poolSizes = {
        { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
    };

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    poolInfo.maxSets = 1000 * static_cast<u32>(poolSizes.size());
    poolInfo.poolSizeCount = static_cast<u32>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();

    return vkCreateDescriptorPool(m_Device, &poolInfo, nullptr, &m_DescriptorPool) == VK_SUCCESS;
}

QueueFamilyIndices VulkanContext::FindQueueFamilies(VkPhysicalDevice device) const {
    QueueFamilyIndices indices;

    u32 queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    for (u32 i = 0; i < queueFamilyCount; i++) {
        if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphicsFamily = i;
        }

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_Surface, &presentSupport);
        if (presentSupport) {
            indices.presentFamily = i;
        }

        if (queueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT) {
            indices.computeFamily = i;
        }

        if (queueFamilies[i].queueFlags & VK_QUEUE_TRANSFER_BIT) {
            indices.transferFamily = i;
        }

        if (indices.IsComplete()) break;
    }

    return indices;
}

bool VulkanContext::IsDeviceSuitable(VkPhysicalDevice device, const ContextConfig& config) const {
    QueueFamilyIndices indices = FindQueueFamilies(device);
    if (!indices.IsComplete()) return false;

    if (!CheckDeviceExtensionSupport(device, s_DeviceExtensions)) return false;

    // Check swapchain support
    u32 formatCount, presentModeCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_Surface, &formatCount, nullptr);
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_Surface, &presentModeCount, nullptr);

    if (formatCount == 0 || presentModeCount == 0) return false;

    // Device is suitable if it has basic swapchain support
    return true;
}

bool VulkanContext::CheckDeviceExtensionSupport(VkPhysicalDevice device, const std::vector<const char*>& extensions) const {
    u32 extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    std::set<std::string> requiredExtensions(extensions.begin(), extensions.end());
    for (const auto& extension : availableExtensions) {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}

std::vector<const char*> VulkanContext::GetRequiredExtensions(const ContextConfig& config) const {
    u32 glfwExtensionCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    if (!glfwExtensions) {
        TVK_LOG_ERROR("Failed to get GLFW required extensions");
        return {};
    }

    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

#ifdef TVK_PLATFORM_APPLE
    extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
    extensions.push_back("VK_KHR_get_physical_device_properties2");
#endif

    if (m_ValidationEnabled) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    for (const auto* ext : config.requiredExtensions) {
        extensions.push_back(ext);
    }

    return extensions;
}

bool VulkanContext::CheckValidationLayerSupport() const {
    u32 layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const char* layerName : s_ValidationLayers) {
        bool layerFound = false;
        for (const auto& layerProperties : availableLayers) {
            if (strcmp(layerName, layerProperties.layerName) == 0) {
                layerFound = true;
                break;
            }
        }
        if (!layerFound) return false;
    }

    return true;
}

SwapchainSupportDetails VulkanContext::QuerySwapchainSupport() const {
    SwapchainSupportDetails details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_PhysicalDevice, m_Surface, &details.capabilities);

    u32 formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(m_PhysicalDevice, m_Surface, &formatCount, nullptr);
    if (formatCount != 0) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(m_PhysicalDevice, m_Surface, &formatCount, details.formats.data());
    }

    u32 presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(m_PhysicalDevice, m_Surface, &presentModeCount, nullptr);
    if (presentModeCount != 0) {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(m_PhysicalDevice, m_Surface, &presentModeCount, details.presentModes.data());
    }

    return details;
}

u32 VulkanContext::FindMemoryType(u32 typeFilter, VkMemoryPropertyFlags properties) const {
    for (u32 i = 0; i < m_MemoryProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && 
            (m_MemoryProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    TVK_LOG_ERROR("Failed to find suitable memory type");
    return 0;
}

VkCommandBuffer VulkanContext::BeginSingleTimeCommands() {
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = m_CommandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(m_Device, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
}

void VulkanContext::EndSingleTimeCommands(VkCommandBuffer commandBuffer) {
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(m_GraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(m_GraphicsQueue);

    vkFreeCommandBuffers(m_Device, m_CommandPool, 1, &commandBuffer);
}

} // namespace tvk
