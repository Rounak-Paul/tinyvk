/**
 * @file imgui_layer.h
 * @brief ImGui integration layer for TinyVK
 */

#pragma once

#include "../core/types.h"
#include <vulkan/vulkan.h>
#include <string>
#include <vector>

struct GLFWwindow;

namespace tvk {

// Forward declarations
class VulkanContext;
class Renderer;

struct FontInfo {
    const char* id;
    const char* displayName;
    void* data;
    int dataSize;
};

/**
 * @brief ImGui layer configuration
 */
struct ImGuiConfig {
    bool enableDocking = true;
    bool enableViewports = false;
    float fontScale = 1.0f;
    const char* fontPath = nullptr;
    float fontSize = 16.0f;
    bool useEmbeddedFont = true;
    const char* embeddedFontName = "jetbrains_mono_nerd";
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

    bool Init(GLFWwindow* window, Renderer* renderer, const ImGuiConfig& config = ImGuiConfig{});
    void Cleanup();
    void Begin();
    void End(VkCommandBuffer commandBuffer);

    void SetDarkTheme();
    void SetLightTheme();

    bool WantsCaptureKeyboard() const;
    bool WantsCaptureMouse() const;

    void BeginDockspace(float bottomOffset = 0.0f, int flags = 0);
    void EndDockspace();

    // Font management
    void ReloadFont(const char* fontId, float fontSize, float fontScale = 1.0f);
    static const std::vector<FontInfo>& GetAvailableFonts();

private:
    void SetupStyle();
    void BuildFontAtlas(const char* fontId, float fontSize, float fontScale);

    GLFWwindow* m_Window = nullptr;
    Renderer* m_Renderer = nullptr;
    VkDescriptorPool m_DescriptorPool = VK_NULL_HANDLE;
    ImGuiConfig m_Config;
    bool m_Initialized = false;
};

} // namespace tvk
