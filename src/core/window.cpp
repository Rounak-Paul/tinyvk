/**
 * @file window.cpp
 * @brief Window implementation using GLFW
 */

#include "tinyvk/core/window.h"
#include "tinyvk/core/log.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace tvk {

static u32 s_GLFWWindowCount = 0;

Window::Window(const WindowConfig& config) : m_Config(config) {
    // Initialize GLFW if this is the first window
    if (s_GLFWWindowCount == 0) {
        if (!glfwInit()) {
            TVK_LOG_FATAL("Failed to initialize GLFW");
            return;
        }
        glfwSetErrorCallback([](int error, const char* description) {
            TVK_LOG_ERROR("GLFW Error ({}): {}", error, description);
        });
    }

    // Vulkan requires no default API
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, config.resizable ? GLFW_TRUE : GLFW_FALSE);
    glfwWindowHint(GLFW_DECORATED, config.decorated ? GLFW_TRUE : GLFW_FALSE);
    glfwWindowHint(GLFW_MAXIMIZED, config.maximized ? GLFW_TRUE : GLFW_FALSE);

    GLFWmonitor* monitor = config.fullscreen ? glfwGetPrimaryMonitor() : nullptr;
    
    m_Window = glfwCreateWindow(
        static_cast<int>(config.width),
        static_cast<int>(config.height),
        config.title.c_str(),
        monitor,
        nullptr
    );

    if (!m_Window) {
        TVK_LOG_FATAL("Failed to create GLFW window");
        return;
    }

    s_GLFWWindowCount++;

    // Set user pointer for callbacks
    glfwSetWindowUserPointer(m_Window, this);

    // Set callbacks
    glfwSetFramebufferSizeCallback(m_Window, FramebufferResizeCallback);
    glfwSetWindowCloseCallback(m_Window, WindowCloseCallback);
    glfwSetWindowMaximizeCallback(m_Window, WindowMaximizeCallback);

    TVK_LOG_INFO("Window created: {} ({}x{})", config.title, config.width, config.height);
}

Window::~Window() {
    if (m_Window) {
        glfwDestroyWindow(m_Window);
        s_GLFWWindowCount--;

        if (s_GLFWWindowCount == 0) {
            glfwTerminate();
        }
    }
}

Window::Window(Window&& other) noexcept
    : m_Window(other.m_Window)
    , m_Config(std::move(other.m_Config))
    , m_ResizeCallback(std::move(other.m_ResizeCallback))
    , m_CloseCallback(std::move(other.m_CloseCallback))
    , m_MaximizeCallback(std::move(other.m_MaximizeCallback)) {
    other.m_Window = nullptr;
    if (m_Window) {
        glfwSetWindowUserPointer(m_Window, this);
    }
}

Window& Window::operator=(Window&& other) noexcept {
    if (this != &other) {
        if (m_Window) {
            glfwDestroyWindow(m_Window);
            s_GLFWWindowCount--;
        }

        m_Window = other.m_Window;
        m_Config = std::move(other.m_Config);
        m_ResizeCallback = std::move(other.m_ResizeCallback);
        m_CloseCallback = std::move(other.m_CloseCallback);
        m_MaximizeCallback = std::move(other.m_MaximizeCallback);

        other.m_Window = nullptr;
        if (m_Window) {
            glfwSetWindowUserPointer(m_Window, this);
        }
    }
    return *this;
}

void Window::PollEvents() {
    glfwPollEvents();
}

bool Window::ShouldClose() const {
    return glfwWindowShouldClose(m_Window);
}

void Window::Close() {
    glfwSetWindowShouldClose(m_Window, GLFW_TRUE);
}

Extent2D Window::GetExtent() const {
    int width, height;
    glfwGetWindowSize(m_Window, &width, &height);
    return {static_cast<u32>(width), static_cast<u32>(height)};
}

Extent2D Window::GetFramebufferExtent() const {
    int width, height;
    glfwGetFramebufferSize(m_Window, &width, &height);
    return {static_cast<u32>(width), static_cast<u32>(height)};
}

void Window::SetTitle(const std::string& title) {
    glfwSetWindowTitle(m_Window, title.c_str());
    m_Config.title = title;
}

bool Window::IsMinimized() const {
    auto extent = GetFramebufferExtent();
    return extent.width == 0 || extent.height == 0;
}

bool Window::IsMaximized() const {
    return glfwGetWindowAttrib(m_Window, GLFW_MAXIMIZED) == GLFW_TRUE;
}

void Window::WaitEvents() {
    glfwWaitEvents();
}

void Window::GetPosition(i32& x, i32& y) const {
    glfwGetWindowPos(m_Window, &x, &y);
}

void Window::SetPosition(i32 x, i32 y) {
    glfwSetWindowPos(m_Window, x, y);
}

void Window::SetSize(u32 width, u32 height) {
    glfwSetWindowSize(m_Window, static_cast<int>(width), static_cast<int>(height));
}

void Window::Iconify() {
    glfwIconifyWindow(m_Window);
}

void Window::Maximize() {
    glfwMaximizeWindow(m_Window);
}

void Window::Restore() {
    glfwRestoreWindow(m_Window);
}

void Window::FramebufferResizeCallback(GLFWwindow* window, int width, int height) {
    auto* self = static_cast<Window*>(glfwGetWindowUserPointer(window));
    if (self && self->m_ResizeCallback) {
        self->m_ResizeCallback(static_cast<u32>(width), static_cast<u32>(height));
    }
}

void Window::WindowCloseCallback(GLFWwindow* window) {
    auto* self = static_cast<Window*>(glfwGetWindowUserPointer(window));
    if (self && self->m_CloseCallback) {
        self->m_CloseCallback();
    }
}

void Window::WindowMaximizeCallback(GLFWwindow* window, int maximized) {
    auto* self = static_cast<Window*>(glfwGetWindowUserPointer(window));
    if (self && self->m_MaximizeCallback) {
        self->m_MaximizeCallback(maximized == GLFW_TRUE);
    }
}

} // namespace tvk
