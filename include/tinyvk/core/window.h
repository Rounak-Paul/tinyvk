/**
 * @file window.h
 * @brief Window management for TinyVK using GLFW
 */

#pragma once

#include "types.h"
#include <string>
#include <functional>

struct GLFWwindow;

namespace tvk {

/**
 * @brief Window creation configuration
 */
struct WindowConfig {
    std::string title = "TinyVK Application";
    u32 width = 1280;
    u32 height = 720;
    bool resizable = true;
    bool decorated = true;
    bool vsync = true;
    bool fullscreen = false;
    bool maximized = false;
};

/**
 * @brief Event types for window callbacks
 */
enum class WindowEventType {
    Resize,
    Close,
    Focus,
    Minimize,
    Maximize
};

struct WindowResizeEvent {
    u32 width;
    u32 height;
};

/**
 * @brief Window class wrapping GLFW window
 */
class Window {
public:
    using ResizeCallback = std::function<void(u32 width, u32 height)>;
    using CloseCallback = std::function<void()>;

    Window(const WindowConfig& config = WindowConfig{});
    ~Window();

    // Non-copyable
    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    // Movable
    Window(Window&& other) noexcept;
    Window& operator=(Window&& other) noexcept;

    /**
     * @brief Process window events
     */
    void PollEvents();

    /**
     * @brief Check if window should close
     */
    bool ShouldClose() const;

    /**
     * @brief Request window to close
     */
    void Close();

    /**
     * @brief Get window dimensions
     */
    Extent2D GetExtent() const;

    /**
     * @brief Get framebuffer dimensions (may differ from window size on HiDPI)
     */
    Extent2D GetFramebufferExtent() const;

    /**
     * @brief Get native GLFW window handle
     */
    GLFWwindow* GetNativeHandle() const { return m_Window; }

    /**
     * @brief Set window title
     */
    void SetTitle(const std::string& title);

    /**
     * @brief Get window title
     */
    const std::string& GetTitle() const { return m_Config.title; }

    /**
     * @brief Set resize callback
     */
    void SetResizeCallback(ResizeCallback callback) { m_ResizeCallback = std::move(callback); }

    /**
     * @brief Set close callback
     */
    void SetCloseCallback(CloseCallback callback) { m_CloseCallback = std::move(callback); }

    /**
     * @brief Check if window is minimized
     */
    bool IsMinimized() const;

    /**
     * @brief Wait for window events (used when minimized)
     */
    void WaitEvents();

private:
    GLFWwindow* m_Window = nullptr;
    WindowConfig m_Config;
    ResizeCallback m_ResizeCallback;
    CloseCallback m_CloseCallback;

    static void FramebufferResizeCallback(GLFWwindow* window, int width, int height);
    static void WindowCloseCallback(GLFWwindow* window);
};

} // namespace tvk
