#include "tinyvk/core/application.h"
#include "tinyvk/core/input.h"
#include "tinyvk/core/log.h"
#include "tinyvk/renderer/renderer.h"
#include "tinyvk/ui/imgui_layer.h"
#include "tinyvk/ui/render_widget.h"

#ifdef TVK_PLATFORM_WINDOWS
#include <windows.h>
#endif

namespace tvk {

App::App() {
#ifdef TVK_PLATFORM_WINDOWS
    SetEnvironmentVariableA("DISABLE_LAYER_AMD_SWITCHABLE_GRAPHICS_1", "1");
    SetEnvironmentVariableA("DISABLE_RTSS_LAYER", "1");
#endif
}

App::~App() {
    _instance = nullptr;
}

void App::Run(const std::string& title, u32 width, u32 height, bool vsync) {
    AppConfig config;
    config.title = title;
    config.width = width;
    config.height = height;
    config.vsync = vsync;
    Run(config);
}

void App::Run(const AppConfig& config) {
    if (_instance != nullptr) {
        TVK_LOG_FATAL("Application already exists!");
        return;
    }
    _instance = this;

    Initialize(config);
    OnStart();

    _running = true;
    MainLoop();

    OnStop();
    Shutdown();
}

void App::Initialize(const AppConfig& config) {
    TVK_LOG_INFO("Initializing TinyVK Application: {}", config.title);
    
    _mode = config.mode;
    _enableDockspace = config.enableDockspace;

    WindowConfig windowConfig;
    windowConfig.title = config.title;
    windowConfig.width = config.width;
    windowConfig.height = config.height;
    windowConfig.vsync = config.vsync;
    windowConfig.decorated = config.decorated;

    _window = CreateScope<Window>(windowConfig);

    _window->SetResizeCallback([this](u32 width, u32 height) {
        _renderer->OnResize(width, height);
    });

    _window->SetCloseCallback([this]() {
        Quit();
    });

    Input::Init(_window->GetNativeHandle());

    RendererConfig rendererConfig;
#ifdef TVK_DEBUG_BUILD
    rendererConfig.enableValidation = true;
#else
    rendererConfig.enableValidation = false;
#endif
    rendererConfig.vsync = config.vsync;

    _renderer = CreateScope<Renderer>();
    if (!_renderer->Init(_window.get(), rendererConfig)) {
        TVK_LOG_FATAL("Failed to initialize renderer");
        return;
    }

    ImGuiConfig imguiConfig;
    imguiConfig.enableDocking = true;

    _imguiLayer = CreateScope<ImGuiLayer>();
    if (!_imguiLayer->Init(_window->GetNativeHandle(), _renderer.get(), imguiConfig)) {
        TVK_LOG_FATAL("Failed to initialize ImGui");
        return;
    }

    _startTime = std::chrono::high_resolution_clock::now();
    _lastFrameTime = _startTime;

    TVK_LOG_INFO("TinyVK initialized successfully");
}

void App::Shutdown() {
    TVK_LOG_INFO("Shutting down TinyVK Application");

    _renderer->GetContext().WaitIdle();
    
    for (auto* widget : _widgets) {
        widget->Cleanup();
    }
    _widgets.clear();

    _imguiLayer->Cleanup();
    _imguiLayer.reset();

    _renderer->Cleanup();
    _renderer.reset();

    _window.reset();

    TVK_LOG_INFO("TinyVK shutdown complete");
}

void App::MainLoop() {
    u32 frameCount = 0;
    float fpsTimer = 0.0f;

    while (_running && !_window->ShouldClose()) {
        auto currentTime = std::chrono::high_resolution_clock::now();
        _deltaTime = std::chrono::duration<float>(currentTime - _lastFrameTime).count();
        _lastFrameTime = currentTime;
        _elapsedTime = std::chrono::duration<float>(currentTime - _startTime).count();

        frameCount++;
        fpsTimer += _deltaTime;
        if (fpsTimer >= 1.0f) {
            _fps = static_cast<float>(frameCount) / fpsTimer;
            frameCount = 0;
            fpsTimer = 0.0f;
        }

        _window->PollEvents();
        Input::Update();

        if (_window->IsMinimized()) {
            _window->WaitEvents();
            continue;
        }

        OnUpdate();

        if (_renderer->BeginFrame()) {
            OnPreRender();
            
            // Game mode: render directly to swapchain
            if (_mode == AppMode::Game || _mode == AppMode::Hybrid) {
                VkCommandBuffer cmd = _renderer->GetCurrentCommandBuffer();
                OnRender(cmd);
            }
            
            // GUI mode: render ImGui interface
            if (_mode == AppMode::GUI || _mode == AppMode::Hybrid) {
                _imguiLayer->Begin();
                
                if (_enableDockspace) {
                    _imguiLayer->BeginDockspace();
                }
                
                OnUI();
                
                if (_enableDockspace) {
                    _imguiLayer->EndDockspace();
                }
                
                for (auto* widget : _widgets) {
                    if (widget->IsEnabled()) {
                        widget->Render(_deltaTime);
                    }
                }
                
                _imguiLayer->End(_renderer->GetCurrentCommandBuffer());
            }
            
            OnPostRender();
            _renderer->EndFrame();
        }
    }
}

u32 App::WindowWidth() const {
    return _window->GetExtent().width;
}

u32 App::WindowHeight() const {
    return _window->GetExtent().height;
}

const std::string& App::WindowTitle() const {
    return _window->GetTitle();
}

Ref<Texture> App::LoadTexture(const std::string& path) {
    return _renderer->CreateTexture(path);
}

void App::RegisterWidget(RenderWidget* widget) {
    if (!widget) return;
    
    for (auto* w : _widgets) {
        if (w == widget) return;
    }
    
    widget->Initialize(_renderer.get());
    _widgets.push_back(widget);
}

void App::UnregisterWidget(RenderWidget* widget) {
    if (!widget) return;
    
    auto it = std::find(_widgets.begin(), _widgets.end(), widget);
    if (it != _widgets.end()) {
        widget->Cleanup();
        _widgets.erase(it);
    }
}

Renderer* App::GetRenderer() {
    return _renderer.get();
}

Window* App::GetWindow() {
    return _window.get();
}

VkCommandBuffer App::GetCommandBuffer() {
    return _renderer->GetCurrentCommandBuffer();
}

VulkanContext& App::GetContext() {
    return _renderer->GetContext();
}

void App::SetClearColor(float r, float g, float b, float a) {
    _renderer->SetClearColor({r, g, b, a});
}

} // namespace tvk
