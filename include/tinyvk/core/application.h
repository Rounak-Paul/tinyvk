/**
 * @file application.h
 * @brief Base application class for TinyVK applications
 */

#pragma once

#include "types.h"
#include "window.h"
#include <string>
#include <chrono>

namespace tvk {

// Forward declarations
class Renderer;
class ImGuiLayer;

/**
 * @brief Application configuration
 */
struct ApplicationConfig {
    std::string name = "TinyVK Application";
    u32 windowWidth = 1280;
    u32 windowHeight = 720;
    bool enableValidation = true;
    bool vsync = true;
};

/**
 * @brief Base application class - inherit from this to create your app
 * 
 * Example:
 * @code
 * class MyApp : public tvk::Application {
 * public:
 *     MyApp(const tvk::ApplicationConfig& config) : Application(config) {}
 *     
 *     void OnInit() override { }
 *     void OnUpdate(float dt) override { }
 *     void OnRender() override { }
 *     void OnImGui() override { }
 *     void OnShutdown() override { }
 * };
 * 
 * int main() {
 *     tvk::ApplicationConfig config;
 *     config.name = "My App";
 *     MyApp app(config);
 *     app.Run();
 *     return 0;
 * }
 * @endcode
 */
class Application {
public:
    Application(const ApplicationConfig& config = ApplicationConfig{});
    virtual ~Application();

    // Non-copyable, non-movable
    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

    /**
     * @brief Run the main application loop
     */
    void Run();

    /**
     * @brief Request application to quit
     */
    void Quit();

    /**
     * @brief Get the application instance (singleton)
     */
    static Application* Get() { return s_Instance; }

    /**
     * @brief Get the window
     */
    Window& GetWindow() { return *m_Window; }

    /**
     * @brief Get the renderer
     */
    Renderer& GetRenderer();

    /**
     * @brief Get delta time in seconds
     */
    float GetDeltaTime() const { return m_DeltaTime; }

    /**
     * @brief Get elapsed time since start in seconds
     */
    float GetElapsedTime() const { return m_ElapsedTime; }

    /**
     * @brief Get current FPS
     */
    float GetFPS() const { return m_FPS; }

protected:
    /**
     * @brief Called once after initialization
     */
    virtual void OnInit() {}

    /**
     * @brief Called every frame with delta time
     */
    virtual void OnUpdate(float dt) {}

    /**
     * @brief Called every frame for rendering
     */
    virtual void OnRender() {}

    /**
     * @brief Called every frame for ImGui rendering
     */
    virtual void OnImGui() {}

    /**
     * @brief Called before shutdown
     */
    virtual void OnShutdown() {}

    /**
     * @brief Called when window is resized
     */
    virtual void OnResize(u32 width, u32 height) {}

private:
    void Initialize();
    void Shutdown();
    void MainLoop();

    static inline Application* s_Instance = nullptr;

    ApplicationConfig m_Config;
    Scope<Window> m_Window;
    Scope<Renderer> m_Renderer;
    Scope<ImGuiLayer> m_ImGuiLayer;

    bool m_Running = false;
    float m_DeltaTime = 0.0f;
    float m_ElapsedTime = 0.0f;
    float m_FPS = 0.0f;

    std::chrono::high_resolution_clock::time_point m_LastFrameTime;
    std::chrono::high_resolution_clock::time_point m_StartTime;
};

} // namespace tvk
