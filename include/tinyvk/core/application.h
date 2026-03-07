/**
 * @file application.h
 * @brief Base application class for TinyVK applications
 */

#pragma once

#include "types.h"
#include "window.h"
#include <string>
#include <chrono>
#include <vector>

// Forward declare Vulkan types
struct VkCommandBuffer_T;
typedef VkCommandBuffer_T* VkCommandBuffer;

namespace tvk {

// Forward declarations (internal - users don't need these)
class Renderer;
class ImGuiLayer;
class RenderWidget;
class VulkanContext;



/**
 * @brief Application configuration
 */
struct AppConfig {
    std::string title = "TinyVK Application";
    u32 width = 1280;
    u32 height = 720;
    bool vsync = true;
    bool maximized = false;
    bool enableDockspace = true;
    bool enableKeyboardNav = true;
    bool enableIdleThrottling = true;
    float idleFrameRate = 30.0f;
};

// Legacy alias
using ApplicationConfig = AppConfig;

/**
 * @brief Base application class - inherit from this to create your app
 * 
 * All engine internals (Vulkan, swapchain, render loop) are hidden.
 * Just override OnUI() to draw your ImGui interface.
 * 
 * Example:
 * @code
 * class MyApp : public tvk::App {
 * protected:
 *     void OnUI() override {
 *         ImGui::Begin("My Window");
 *         if (ImGui::Button("Click me!")) {
 *             // Handle click
 *         }
 *         ImGui::End();
 *     }
 * };
 * 
 * int main() {
 *     MyApp app;
 *     app.Run("My App", 1280, 720);
 *     return 0;
 * }
 * @endcode
 */
class App {
public:
    App();
    virtual ~App();

    App(const App&) = delete;
    App& operator=(const App&) = delete;

    /**
     * @brief Run the application
     */
    void Run(const std::string& title = "TinyVK App", u32 width = 1280, u32 height = 720, bool vsync = true);
    void Run(const AppConfig& config);

    /**
     * @brief Request application to quit
     */
    void Quit() { _running = false; }

    /**
     * @brief Get the running application instance
     */
    static App* Get() { return _instance; }

    // Timing info
    float DeltaTime() const { return _deltaTime; }
    float ElapsedTime() const { return _elapsedTime; }
    float FPS() const { return _fps; }
    
    // Window info
    u32 WindowWidth() const;
    u32 WindowHeight() const;
    const std::string& WindowTitle() const;
    
    // Texture loading helper
    Ref<class Texture> LoadTexture(const std::string& path);
    
    // Widget management
    void RegisterWidget(RenderWidget* widget);
    void UnregisterWidget(RenderWidget* widget);
    
    // Direct access to renderer, window, and imgui layer
    Renderer* GetRenderer();
    Window* GetWindow();
    ImGuiLayer* GetImGuiLayer();
    
    // SDL3-style convenience helpers
    VkCommandBuffer GetCommandBuffer();
    VulkanContext& GetContext();
    void SetClearColor(float r, float g, float b, float a = 1.0f);

protected:
    /**
     * @brief Called once at startup - override to initialize your app
     */
    virtual void OnStart() {}

    /**
     * @brief Called every frame - override to update your app logic
     */
    virtual void OnUpdate() {}

    /**
     * @brief Called every frame to populate the title bar menu
     * Override to add menus: if (ImGui::BeginMenu("File")) { ... ImGui::EndMenu(); }
     */
    virtual void OnMenuBar() {}

    /**
     * @brief Called every frame - override to draw your ImGui UI
     * This is where you write all your ImGui code
     */
    virtual void OnUI() {}
    
    /**
     * @brief Override to specify space reserved at the bottom of the dockspace
     * Useful for status bars or other fixed bottom elements
     * @return Height in pixels to reserve at the bottom (default: 0)
     */
    virtual float GetDockspaceBottomOffset() { return 0.0f; }
    
    /**
     * @brief Override to customize dockspace flags
     * @return ImGuiDockNodeFlags for the main dockspace (0 = default PassthruCentralNode)
     */
    virtual int GetDockspaceFlags() { return 0; }
    
    /**
     * @brief Called before rendering starts
     * Use this to prepare resources before render pass
     */
    virtual void OnPreRender() {}
    
    /**
     * @brief Called for custom rendering
     * Override to issue custom Vulkan commands
     * @param cmd Active command buffer for recording render commands
     */
    virtual void OnRender(VkCommandBuffer cmd) {}
    
    /**
     * @brief Called after rendering finishes
     * Use this for post-processing or cleanup
     */
    virtual void OnPostRender() {}

    /**
     * @brief Called once at shutdown - override to cleanup
     */
    virtual void OnStop() {}

private:
    void Initialize(const AppConfig& config);
    void Shutdown();
    void MainLoop();

    static inline App* _instance = nullptr;

    Scope<Window> _window;
    Scope<Renderer> _renderer;
    Scope<ImGuiLayer> _imguiLayer;
    
    std::vector<RenderWidget*> _widgets;
    
    bool _enableDockspace = true;
    bool _enableIdleThrottling = true;
    float _idleFrameInterval = 1.0f / 30.0f;  // Target frame time when idle

    bool _running = false;
    float _deltaTime = 0.0f;
    float _elapsedTime = 0.0f;
    float _fps = 0.0f;

    std::chrono::high_resolution_clock::time_point _lastFrameTime;
    std::chrono::high_resolution_clock::time_point _startTime;
};

} // namespace tvk
