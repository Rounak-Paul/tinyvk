/**
 * @file context.h
 * @brief Vulkan context and device management for TinyVK
 */

#pragma once

#include "../core/types.h"
#include <vulkan/vulkan.h>
#include <vector>
#include <optional>

struct GLFWwindow;

namespace tvk {

/**
 * @brief Queue family indices for Vulkan
 */
struct QueueFamilyIndices {
    std::optional<u32> graphicsFamily;
    std::optional<u32> presentFamily;
    std::optional<u32> computeFamily;
    std::optional<u32> transferFamily;

    bool IsComplete() const {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

/**
 * @brief Swapchain support details
 */
struct SwapchainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

/**
 * @brief Vulkan context configuration
 */
struct ContextConfig {
    bool enableValidation = true;
    bool enableGPUDebugMarkers = true;
    std::vector<const char*> requiredExtensions;
    std::vector<const char*> requiredDeviceExtensions;
};

/**
 * @brief Vulkan context - manages instance, device, and core Vulkan resources
 */
class VulkanContext {
public:
    VulkanContext() = default;
    ~VulkanContext();

    // Non-copyable
    VulkanContext(const VulkanContext&) = delete;
    VulkanContext& operator=(const VulkanContext&) = delete;

    /**
     * @brief Initialize Vulkan context
     */
    bool Init(GLFWwindow* window, const ContextConfig& config = ContextConfig{});

    /**
     * @brief Cleanup Vulkan resources
     */
    void Cleanup();

    /**
     * @brief Wait for device to be idle
     */
    void WaitIdle();

    // Getters
    VkInstance GetInstance() const { return m_Instance; }
    VkPhysicalDevice GetPhysicalDevice() const { return m_PhysicalDevice; }
    VkDevice GetDevice() const { return m_Device; }
    VkSurfaceKHR GetSurface() const { return m_Surface; }
    VkQueue GetGraphicsQueue() const { return m_GraphicsQueue; }
    VkQueue GetPresentQueue() const { return m_PresentQueue; }
    VkCommandPool GetCommandPool() const { return m_CommandPool; }
    VkDescriptorPool GetDescriptorPool() const { return m_DescriptorPool; }
    const QueueFamilyIndices& GetQueueFamilyIndices() const { return m_QueueFamilyIndices; }
    VkPhysicalDeviceProperties GetDeviceProperties() const { return m_DeviceProperties; }
    VkPhysicalDeviceMemoryProperties GetMemoryProperties() const { return m_MemoryProperties; }

    /**
     * @brief Query swapchain support for physical device
     */
    SwapchainSupportDetails QuerySwapchainSupport() const;

    /**
     * @brief Find suitable memory type
     */
    u32 FindMemoryType(u32 typeFilter, VkMemoryPropertyFlags properties) const;

    /**
     * @brief Create a single-use command buffer
     */
    VkCommandBuffer BeginSingleTimeCommands();

    /**
     * @brief End and submit single-use command buffer
     */
    void EndSingleTimeCommands(VkCommandBuffer commandBuffer);

private:
    bool CreateInstance(const ContextConfig& config);
    bool SetupDebugMessenger();
    bool CreateSurface(GLFWwindow* window);
    bool PickPhysicalDevice();
    bool CreateLogicalDevice(const ContextConfig& config);
    bool CreateCommandPool();
    bool CreateDescriptorPool();

    QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device) const;
    bool IsDeviceSuitable(VkPhysicalDevice device, const ContextConfig& config) const;
    bool CheckDeviceExtensionSupport(VkPhysicalDevice device, const std::vector<const char*>& extensions) const;
    std::vector<const char*> GetRequiredExtensions(const ContextConfig& config) const;
    bool CheckValidationLayerSupport() const;

    VkInstance m_Instance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT m_DebugMessenger = VK_NULL_HANDLE;
    VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
    VkDevice m_Device = VK_NULL_HANDLE;
    VkSurfaceKHR m_Surface = VK_NULL_HANDLE;
    VkQueue m_GraphicsQueue = VK_NULL_HANDLE;
    VkQueue m_PresentQueue = VK_NULL_HANDLE;
    VkCommandPool m_CommandPool = VK_NULL_HANDLE;
    VkDescriptorPool m_DescriptorPool = VK_NULL_HANDLE;

    QueueFamilyIndices m_QueueFamilyIndices;
    VkPhysicalDeviceProperties m_DeviceProperties{};
    VkPhysicalDeviceMemoryProperties m_MemoryProperties{};

    bool m_ValidationEnabled = false;
};

} // namespace tvk
