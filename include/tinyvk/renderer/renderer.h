/**
 * @file renderer.h
 * @brief Main renderer interface for TinyVK
 */

#pragma once

#include "../core/types.h"
#include "context.h"
#include <vulkan/vulkan.h>
#include <vector>

struct GLFWwindow;

namespace tvk {

// Forward declarations
class Window;
class ImGuiLayer;

/**
 * @brief Renderer configuration
 */
struct RendererConfig {
    bool enableValidation = true;
    bool vsync = true;
    u32 maxFramesInFlight = 2;
    Color clearColor = Color::Black();
};

/**
 * @brief Frame data for per-frame resources
 */
struct FrameData {
    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
    VkSemaphore imageAvailableSemaphore = VK_NULL_HANDLE;
    VkSemaphore renderFinishedSemaphore = VK_NULL_HANDLE;
    VkFence inFlightFence = VK_NULL_HANDLE;
};

/**
 * @brief Main renderer class
 */
class Renderer {
public:
    Renderer() = default;
    ~Renderer();

    // Non-copyable
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    /**
     * @brief Initialize the renderer
     */
    bool Init(Window* window, const RendererConfig& config = RendererConfig{});

    /**
     * @brief Cleanup renderer resources
     */
    void Cleanup();

    /**
     * @brief Begin a new frame
     * @return true if frame was successfully started
     */
    bool BeginFrame();

    /**
     * @brief End the current frame and present
     */
    void EndFrame();

    /**
     * @brief Handle window resize
     */
    void OnResize(u32 width, u32 height);

    /**
     * @brief Set clear color
     */
    void SetClearColor(const Color& color) { m_ClearColor = color; }

    /**
     * @brief Get the Vulkan context
     */
    VulkanContext& GetContext() { return m_Context; }

    /**
     * @brief Get current command buffer
     */
    VkCommandBuffer GetCurrentCommandBuffer() const;

    /**
     * @brief Get current frame index
     */
    u32 GetCurrentFrameIndex() const { return m_CurrentFrame; }

    /**
     * @brief Get swapchain image count
     */
    u32 GetSwapchainImageCount() const { return static_cast<u32>(m_SwapchainImages.size()); }

    /**
     * @brief Get swapchain extent
     */
    VkExtent2D GetSwapchainExtent() const { return m_SwapchainExtent; }

    /**
     * @brief Get swapchain image format
     */
    VkFormat GetSwapchainFormat() const { return m_SwapchainImageFormat; }

    /**
     * @brief Get current swapchain image index
     */
    u32 GetCurrentImageIndex() const { return m_CurrentImageIndex; }

    /**
     * @brief Get render pass
     */
    VkRenderPass GetRenderPass() const { return m_RenderPass; }

private:
    bool CreateSwapchain();
    bool CreateImageViews();
    bool CreateRenderPass();
    bool CreateFramebuffers();
    bool CreateCommandBuffers();
    bool CreateSyncObjects();
    bool CreateDepthResources();

    void CleanupSwapchain();
    void RecreateSwapchain();

    VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
    VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
    VkFormat FindDepthFormat();
    VkFormat FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);

    Window* m_Window = nullptr;
    VulkanContext m_Context;
    RendererConfig m_Config;
    Color m_ClearColor = Color::Black();

    // Swapchain
    VkSwapchainKHR m_Swapchain = VK_NULL_HANDLE;
    std::vector<VkImage> m_SwapchainImages;
    std::vector<VkImageView> m_SwapchainImageViews;
    VkFormat m_SwapchainImageFormat = VK_FORMAT_UNDEFINED;
    VkExtent2D m_SwapchainExtent{};

    // Depth buffer
    VkImage m_DepthImage = VK_NULL_HANDLE;
    VkDeviceMemory m_DepthImageMemory = VK_NULL_HANDLE;
    VkImageView m_DepthImageView = VK_NULL_HANDLE;
    VkFormat m_DepthFormat = VK_FORMAT_UNDEFINED;

    // Render pass and framebuffers
    VkRenderPass m_RenderPass = VK_NULL_HANDLE;
    std::vector<VkFramebuffer> m_Framebuffers;

    // Per-frame data (indexed by current frame)
    std::vector<FrameData> m_Frames;
    
    // Semaphore pools - one per swapchain image + 1 for safe rotation
    std::vector<VkSemaphore> m_ImageAvailableSemaphores;
    std::vector<VkSemaphore> m_RenderFinishedSemaphores;
    u32 m_CurrentSemaphoreIndex = 0;
    
    u32 m_CurrentFrame = 0;
    u32 m_CurrentImageIndex = 0;
    bool m_FramebufferResized = false;
};

} // namespace tvk
