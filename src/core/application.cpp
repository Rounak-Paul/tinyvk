/**
 * @file application.cpp
 * @brief Application implementation
 */

#include "tinyvk/core/application.h"
#include "tinyvk/core/input.h"
#include "tinyvk/core/log.h"
#include "tinyvk/renderer/renderer.h"
#include "tinyvk/ui/imgui_layer.h"

namespace tvk {

Application::Application(const ApplicationConfig& config) : m_Config(config) {
    if (s_Instance != nullptr) {
        TVK_LOG_FATAL("Application already exists!");
        return;
    }
    s_Instance = this;
}

Application::~Application() {
    s_Instance = nullptr;
}

void Application::Initialize() {
    TVK_LOG_INFO("Initializing TinyVK Application: {}", m_Config.name);

    // Create window
    WindowConfig windowConfig;
    windowConfig.title = m_Config.name;
    windowConfig.width = m_Config.windowWidth;
    windowConfig.height = m_Config.windowHeight;
    windowConfig.vsync = m_Config.vsync;

    m_Window = CreateScope<Window>(windowConfig);
    
    // Set window callbacks
    m_Window->SetResizeCallback([this](u32 width, u32 height) {
        m_Renderer->OnResize(width, height);
        OnResize(width, height);
    });

    m_Window->SetCloseCallback([this]() {
        Quit();
    });

    // Initialize input
    Input::Init(m_Window->GetNativeHandle());

    // Create renderer
    RendererConfig rendererConfig;
    rendererConfig.enableValidation = m_Config.enableValidation;
    rendererConfig.vsync = m_Config.vsync;

    m_Renderer = CreateScope<Renderer>();
    if (!m_Renderer->Init(m_Window.get(), rendererConfig)) {
        TVK_LOG_FATAL("Failed to initialize renderer");
        return;
    }

    // Create ImGui layer
    ImGuiConfig imguiConfig;
    imguiConfig.enableDocking = true;

    m_ImGuiLayer = CreateScope<ImGuiLayer>();
    if (!m_ImGuiLayer->Init(m_Window->GetNativeHandle(), m_Renderer.get(), imguiConfig)) {
        TVK_LOG_FATAL("Failed to initialize ImGui");
        return;
    }

    // Initialize timing
    m_StartTime = std::chrono::high_resolution_clock::now();
    m_LastFrameTime = m_StartTime;

    TVK_LOG_INFO("TinyVK initialized successfully");
}

void Application::Shutdown() {
    TVK_LOG_INFO("Shutting down TinyVK Application");

    // Wait for GPU to finish
    m_Renderer->GetContext().WaitIdle();

    // Cleanup in reverse order
    m_ImGuiLayer->Cleanup();
    m_ImGuiLayer.reset();

    m_Renderer->Cleanup();
    m_Renderer.reset();

    m_Window.reset();

    TVK_LOG_INFO("TinyVK shutdown complete");
}

void Application::Run() {
    Initialize();
    OnInit();
    
    m_Running = true;
    MainLoop();
    
    OnShutdown();
    Shutdown();
}

void Application::Quit() {
    m_Running = false;
    m_Window->Close();
}

void Application::MainLoop() {
    u32 frameCount = 0;
    float fpsTimer = 0.0f;

    while (m_Running && !m_Window->ShouldClose()) {
        // Calculate delta time
        auto currentTime = std::chrono::high_resolution_clock::now();
        m_DeltaTime = std::chrono::duration<float>(currentTime - m_LastFrameTime).count();
        m_LastFrameTime = currentTime;
        m_ElapsedTime = std::chrono::duration<float>(currentTime - m_StartTime).count();

        // Calculate FPS
        frameCount++;
        fpsTimer += m_DeltaTime;
        if (fpsTimer >= 1.0f) {
            m_FPS = static_cast<float>(frameCount) / fpsTimer;
            frameCount = 0;
            fpsTimer = 0.0f;
        }

        // Poll events
        m_Window->PollEvents();
        Input::Update();

        // Handle minimized window
        if (m_Window->IsMinimized()) {
            m_Window->WaitEvents();
            continue;
        }

        // Update
        OnUpdate(m_DeltaTime);

        // Render
        if (m_Renderer->BeginFrame()) {
            OnRender();

            // ImGui rendering
            m_ImGuiLayer->Begin();
            OnImGui();
            m_ImGuiLayer->End(m_Renderer->GetCurrentCommandBuffer());

            m_Renderer->EndFrame();
        }
    }
}

Renderer& Application::GetRenderer() {
    return *m_Renderer;
}

} // namespace tvk
