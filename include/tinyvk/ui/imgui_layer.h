/**
 * @file imgui_layer.h
 * @brief ImGui integration layer for TinyVK
 */

#pragma once

#include "../core/types.h"
#include <vulkan/vulkan.h>

struct GLFWwindow;

namespace tvk {

// Forward declarations
class VulkanContext;
class Renderer;

/**
 * @brief ImGui layer configuration
 */
struct ImGuiConfig {
    bool enableDocking = true;
    bool enableViewports = false;  // Multi-viewport support
    float fontScale = 1.0f;
    const char* fontPath = nullptr;  // Optional custom font
    float fontSize = 16.0f;
};

/**
 * @brief ImGui integration layer
 */
class ImGuiLayer {
public:
    ImGuiLayer() = default;
    ~ImGuiLayer();

    // Non-copyable
    ImGuiLayer(const ImGuiLayer&) = delete;
    ImGuiLayer& operator=(const ImGuiLayer&) = delete;

    /**
     * @brief Initialize ImGui
     */
    bool Init(GLFWwindow* window, Renderer* renderer, const ImGuiConfig& config = ImGuiConfig{});

    /**
     * @brief Cleanup ImGui resources
     */
    void Cleanup();

    /**
     * @brief Begin ImGui frame
     */
    void Begin();

    /**
     * @brief End ImGui frame and record draw commands
     */
    void End(VkCommandBuffer commandBuffer);

    /**
     * @brief Set dark theme colors
     */
    void SetDarkTheme();

    /**
     * @brief Set light theme colors
     */
    void SetLightTheme();

    /**
     * @brief Block input events from reaching the application
     */
    bool WantsCaptureKeyboard() const;
    bool WantsCaptureMouse() const;

private:
    void SetupStyle();

    GLFWwindow* m_Window = nullptr;
    Renderer* m_Renderer = nullptr;
    VkDescriptorPool m_DescriptorPool = VK_NULL_HANDLE;
    ImGuiConfig m_Config;
    bool m_Initialized = false;
};

} // namespace tvk
