/**
 * @file render_widget.h
 * @brief Custom rendering widget for games and 3D content
 */

#pragma once

#include "../core/types.h"
#include "../renderer/context.h"
#include <vulkan/vulkan.h>
#include <functional>
#include <array>

namespace tvk {

// Forward declarations
class Renderer;

/**
 * @brief Widget that provides a custom Vulkan rendering surface
 * 
 * Subclass this to create custom 3D rendering, game viewports, etc.
 * Similar to Qt's QOpenGLWidget or QVulkanWidget.
 * 
 * Example:
 * @code
 * class GameViewport : public tvk::RenderWidget {
 * protected:
 *     void OnRenderInit() override {
 *         // Initialize your graphics resources
 *         // Create pipelines, buffers, textures, etc.
 *     }
 * 
 *     void OnRenderFrame(VkCommandBuffer cmd) override {
 *         // Record your rendering commands
 *         vkCmdBindPipeline(cmd, ...);
 *         vkCmdDraw(cmd, ...);
 *     }
 * 
 *     void OnRenderCleanup() override {
 *         // Clean up your resources
 *     }
 * };
 * @endcode
 */
class RenderWidget {
public:
    RenderWidget();
    virtual ~RenderWidget();

    /**
     * @brief Initialize the render widget (called by framework)
     * Do not call this manually - it's called automatically
     */
    void Initialize(Renderer* renderer);

    /**
     * @brief Render the widget (called by framework)
     * Do not call this manually - it's called automatically
     */
    void Render(float deltaTime);
    
    /**
     * @brief Display the rendered image in current ImGui context
     */
    void RenderImage();

    /**
     * @brief Clean up resources (called by framework)
     */
    void Cleanup();

    /**
     * @brief Set the widget size
     */
    void SetSize(u32 width, u32 height);
    
    /**
     * @brief Set clear color for the render target
     */
    void SetClearColor(float r, float g, float b, float a = 1.0f) {
        _clearColor = {r, g, b, a};
    }

    /**
     * @brief Get widget dimensions
     */
    u32 GetWidth() const { return _width; }
    u32 GetHeight() const { return _height; }

    /**
     * @brief Check if widget is initialized
     */
    bool IsInitialized() const { return _initialized; }

    /**
     * @brief Enable/disable the widget
     */
    void SetEnabled(bool enabled) { _enabled = enabled; }
    bool IsEnabled() const { return _enabled; }

    /**
     * @brief Get the Vulkan context for advanced usage
     */
    VulkanContext* GetContext();
    
    /**
     * @brief Get the renderer for creating resources
     */
    Renderer* GetRenderer() { return _renderer; }

    /**
     * @brief Get the current command buffer during OnRenderFrame
     */
    VkCommandBuffer GetCommandBuffer() const { return _commandBuffer; }
    
    /**
     * @brief Get the render pass for pipeline creation
     */
    VkRenderPass GetRenderPass() const { return _renderPass; }

protected:
    /**
     * @brief Override this to initialize your rendering resources
     * Called once when the widget is first created
     * 
     * Use GetContext() to access Vulkan device, physical device, etc.
     */
    virtual void OnRenderInit() {}

    /**
     * @brief Override this to record your rendering commands
     * Called every frame to render your content
     * 
     * @param cmd The command buffer to record into
     * 
     * You can also use GetCommandBuffer() to get the current command buffer
     */
    virtual void OnRenderFrame(VkCommandBuffer cmd) {}

    /**
     * @brief Override this to update your game/simulation state
     * Called every frame before rendering
     * 
     * @param deltaTime Time since last frame in seconds
     */
    virtual void OnRenderUpdate(float deltaTime) {}

    /**
     * @brief Override this to clean up your rendering resources
     * Called when the widget is destroyed
     */
    virtual void OnRenderCleanup() {}

    /**
     * @brief Override this to handle size changes
     * Called when the widget is resized
     */
    virtual void OnRenderResize(u32 width, u32 height) {}

    // Helper methods for common operations
    
    /**
     * @brief Begin a render pass for this widget
     * Call this at the start of OnRenderFrame if you want a default render pass
     */
    void BeginRenderPass(VkCommandBuffer cmd);

    /**
     * @brief End the render pass
     * Call this at the end of OnRenderFrame if you used BeginRenderPass
     */
    void EndRenderPass(VkCommandBuffer cmd);

private:
    void CreateRenderPass();
    void CreateSampler();
    void CreateSizeDependentResources();
    void CleanupSizeDependentResources();
    void CreateRenderTarget();
    void CleanupRenderTarget();
    void RecreateRenderTarget();

    Renderer* _renderer = nullptr;
    VkCommandBuffer _currentCommandBuffer = VK_NULL_HANDLE;
    
    // Render target resources
    VkImage _renderImage = VK_NULL_HANDLE;
    VkDeviceMemory _renderImageMemory = VK_NULL_HANDLE;
    VkImageView _renderImageView = VK_NULL_HANDLE;
    VkSampler _sampler = VK_NULL_HANDLE;
    VkFramebuffer _framebuffer = VK_NULL_HANDLE;
    VkRenderPass _renderPass = VK_NULL_HANDLE;
    
    // Depth buffer
    VkImage _depthImage = VK_NULL_HANDLE;
    VkDeviceMemory _depthImageMemory = VK_NULL_HANDLE;
    VkImageView _depthImageView = VK_NULL_HANDLE;
    
    // ImGui texture ID for displaying the rendered result
    VkDescriptorSet _imguiTexture = VK_NULL_HANDLE;
    
    // Command buffer for rendering
    VkCommandBuffer _commandBuffer = VK_NULL_HANDLE;
    VkCommandPool _commandPool = VK_NULL_HANDLE;
    
    // Clear color
    std::array<float, 4> _clearColor = {0.1f, 0.1f, 0.15f, 1.0f};
    
    u32 _width = 800;
    u32 _height = 600;
    bool _initialized = false;
    bool _enabled = true;
    bool _needsResize = false;
};

} // namespace tvk
