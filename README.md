# TinyVK

A lightweight Vulkan rendering framework combining the best of **Qt** (for GUI tools) and **SDL3** (for games). Build desktop applications, games, or hybrid tools with ease.

![License](https://img.shields.io/badge/license-Apache%202.0-blue.svg)
![C++](https://img.shields.io/badge/C%2B%2B-20-blue.svg)
![Vulkan](https://img.shields.io/badge/Vulkan-1.2-red.svg)

## Features

### Three Application Modes
- **GUI Mode** - Qt-style desktop tools and editors (ImGui-only)
- **Game Mode** - SDL3-style direct rendering for games
- **Hybrid Mode** - Best of both worlds (level editors, modeling tools)

### Core Features
- **Simple Application Framework** - Inherit from `tvk::App` and override virtual methods
- **Vulkan Abstraction** - Handles instance, device, swapchain, and synchronization
- **ImGui Integration** - Built-in Dear ImGui support with docking
- **Custom Rendering Widgets** - `RenderWidget` for embedded 3D viewports
- **Direct Rendering API** - SDL3-style access to command buffers and rendering
- **Geometry System** - Built-in mesh support with vertex/index buffers
- **Texture Loading** - Load and display images with `LoadTexture()`
- **File Dialogs** - Cross-platform file open/save dialogs
- **Cross-Platform** - Supports macOS (MoltenVK), Windows, and Linux
- **Modern C++20** - Uses smart pointers, scoped resources, and modern patterns

See [APPLICATION_MODES.md](docs/APPLICATION_MODES.md) for detailed documentation.

## Requirements

- CMake 3.20+
- C++20 compatible compiler
- Vulkan SDK 1.2+
- Git (for submodules)

## Building

### Clone with submodules

```bash
git clone --recursive https://github.com/Rounak-Paul/tinyvk.git
cd tinyvk
```

Or if already cloned:

```bash
git submodule update --init --recursive
```

### Build

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

On Windows with Visual Studio:

```bash
mkdir build && cd build
cmake .. -G "Visual Studio 17 2022"
cmake --build . --config Release
```

### Run Sandbox

```bash
# Comprehensive example demonstrating all TinyVK features
# Includes GUI controls, 3D viewport, texture loading, and more
./bin/sandbox
```

## Quick Start

### GUI Application (Qt-style)

```cpp
#include <tinyvk/tinyvk.h>
#include <imgui.h>

class MyTool : public tvk::App {
protected:
    void OnUI() override {
        ImGui::Begin("Tool Window");
        ImGui::Text("FPS: %.1f", FPS());
        if (ImGui::Button("Process")) {
            // Do something
        }
        ImGui::End();
    }
};

int main() {
    MyTool app;
    tvk::AppConfig config;
    config.title = "My Tool";
    config.mode = tvk::AppMode::GUI;  // GUI-only mode
    app.Run(config);
    return 0;
}
```

### Game Application (SDL3-style)

```cpp
#include <tinyvk/tinyvk.h>

class MyGame : public tvk::App {
protected:
    void OnStart() override {
        SetClearColor(0.2f, 0.3f, 0.4f);
        // Load game resources
    }
    
    void OnUpdate() override {
        // Update game logic
    }
    
    void OnRender(VkCommandBuffer cmd) override {
        // Direct Vulkan rendering
        // Once pipeline is ready, draw meshes here
    }
};

int main() {
    MyGame app;
    tvk::AppConfig config;
    config.mode = tvk::AppMode::Game;  // Game-only mode
    app.Run(config);
    return 0;
}
```

### Hybrid Application (Level Editor)

```cpp
#include <tinyvk/tinyvk.h>
#include <imgui.h>

class MyViewport : public tvk::RenderWidget {
protected:
    void OnRenderFrame(VkCommandBuffer cmd) override {
        BeginRenderPass(cmd);
        // Render 3D scene
        EndRenderPass(cmd);
    }
};

class LevelEditor : public tvk::App {
protected:
    void OnStart() override {
        _viewport = tvk::CreateScope<MyViewport>();
        RegisterWidget(_viewport.get());
    }
    
    void OnUI() override {
        ImGui::Begin("Viewport");
        _viewport->RenderImage();  // Embedded 3D view
        ImGui::End();
        
        ImGui::Begin("Properties");
        // Edit properties
        ImGui::End();
    }

private:
    tvk::Scope<MyViewport> _viewport;
};

int main() {
    LevelEditor app;
    tvk::AppConfig config;
    config.mode = tvk::AppMode::Hybrid;  // Both GUI and rendering
    app.Run(config);
    return 0;
}
```

See [APPLICATION_MODES.md](docs/APPLICATION_MODES.md) for detailed API documentation.
    }
};

TVK_MAIN(MyApp, "My Application", 1280, 720)
```

## Custom Rendering with RenderWidget

Create game viewports or 3D content within your ImGui application:

```cpp
#include <tinyvk/tinyvk.h>

class GameViewport : public tvk::RenderWidget {
protected:
    void OnRenderInit() override {
        // Initialize graphics resources
    }

    void OnRenderFrame(VkCommandBuffer cmd) override {
        BeginRenderPass(cmd);
        // Record your Vulkan rendering commands
        vkCmdDraw(cmd, vertexCount, 1, 0, 0);
        EndRenderPass(cmd);
    }

    void OnRenderUpdate(float deltaTime) override {
        // Update game logic
        _rotation += deltaTime * 45.0f;
    }

private:
    float _rotation = 0.0f;
};

class MyApp : public tvk::App {
protected:
    void OnStart() override {
        _viewport = tvk::CreateScope<GameViewport>();
        RegisterWidget(_viewport.get());
    }

    void OnUI() override {
        ImGui::Begin("Game");
        _viewport->SetEnabled(true);
        ImGui::End();
    }

private:
    tvk::Scope<GameViewport> _viewport;
};
```

See [docs/render_widget.md](docs/render_widget.md) for complete documentation.

## Project Structure

```
tinyvk/
├── include/tinyvk/          # Public headers
│   ├── tinyvk.h             # Main include header
│   ├── core/                # Core functionality
│   │   ├── application.h    # Base application class
│   │   ├── window.h         # GLFW window wrapper
│   │   ├── input.h          # Input handling
│   │   ├── log.h            # Logging utilities
│   │   └── types.h          # Common type definitions
│   ├── renderer/            # Vulkan renderer
│   │   ├── renderer.h       # Main renderer class
│   │   └── context.h        # Vulkan context management
│   └── ui/                  # UI layer
│       └── imgui_layer.h    # ImGui integration
├── src/                     # Implementation files
├── sandbox/                 # Example application
├── vendors/                 # Third-party dependencies
│   ├── glfw/                # Window/input library
│   ├── glm/                 # Math library
│   ├── imgui/               # Dear ImGui
│   ├── vma/                 # Vulkan Memory Allocator
│   ├── json/                # nlohmann/json
│   └── stb/                 # stb libraries
└── CMakeLists.txt
```

## API Reference

### Application

```cpp
class App {
    // Run the application
    void Run(const std::string& title, u32 width, u32 height, bool vsync = true);
    void Run(const AppConfig& config);
    void Quit();                          // Request exit
    
    // Widget management
    void RegisterWidget(RenderWidget* widget);
    void UnregisterWidget(RenderWidget* widget);
    
    // Timing
    float DeltaTime() const;              // Frame delta time
    float ElapsedTime() const;            // Time since start
    float FPS() const;                    // Current FPS
    
    // Window
    u32 WindowWidth() const;
    u32 WindowHeight() const;
    const std::string& WindowTitle() const;
    
    // Texture loading
    Ref<Texture> LoadTexture(const std::string& path);
    
protected:
    virtual void OnStart() {}             // Called after init
    virtual void OnUpdate() {}            // Called every frame
    virtual void OnUI() {}                // ImGui rendering
    virtual void OnStop() {}              // Before cleanup
};
```

### RenderWidget

```cpp
class RenderWidget {
    void SetSize(u32 width, u32 height);
    void SetEnabled(bool enabled);
    bool IsEnabled() const;
    
    VulkanContext* GetContext();
    VkCommandBuffer GetCommandBuffer() const;
    
protected:
    virtual void OnRenderInit() {}
    virtual void OnRenderFrame(VkCommandBuffer cmd) {}
    virtual void OnRenderUpdate(float deltaTime) {}
    virtual void OnRenderResize(u32 width, u32 height) {}
    virtual void OnRenderCleanup() {}
    
    void BeginRenderPass(VkCommandBuffer cmd);
    void EndRenderPass(VkCommandBuffer cmd);
};
```

### Input

```cpp
class Input {
    static bool IsKeyPressed(Key key);
    static bool IsKeyDown(Key key);
    static bool IsMouseButtonPressed(MouseButton button);
    static bool IsMouseButtonDown(MouseButton button);
    static Vec2 GetMousePosition();
    static Vec2 GetMouseDelta();
    static Vec2 GetScrollDelta();
    static void SetCursorMode(int mode);
};
```

### File Dialogs

```cpp
// Open file dialog
auto result = tvk::OpenFileDialog("Select Image", "Image Files", "*.png;*.jpg");
if (result.has_value()) {
    std::string path = result.value();
    auto texture = LoadTexture(path);
}

// Save file dialog
auto result = tvk::SaveFileDialog("Save As", "Data Files", "*.dat");
```

### Texture

```cpp
class Texture {
    u32 GetWidth() const;
    u32 GetHeight() const;
    VkDescriptorSet GetImGuiTextureID() const;
    
    void BindToImGui();  // Must call before using in ImGui
};

// Usage
auto texture = LoadTexture("image.png");
texture->BindToImGui();
ImGui::Image(texture->GetImGuiTextureID(), ImVec2(256, 256));
```

### Profiling

```cpp
// Timer
tvk::Timer timer;
// ... do work ...
float elapsed = timer.ElapsedMillis();

// Scoped profiling
{
    TVK_PROFILE_SCOPE("MyFunction");
    // ... function code ...
}

// Manual profiling
TVK_PROFILE_BEGIN("LoadAssets");
// ... loading code ...
TVK_PROFILE_END("LoadAssets");
```

### Logging

```cpp
TVK_LOG_TRACE("Trace message: {}", value);
TVK_LOG_DEBUG("Debug message: {}", value);
TVK_LOG_INFO("Info message: {}", value);
TVK_LOG_WARN("Warning: {}", value);
TVK_LOG_ERROR("Error: {}", value);
TVK_LOG_FATAL("Fatal error: {}", value);
```

### Types

```cpp
// Integer types
using i8, i16, i32, i64;     // Signed integers
using u8, u16, u32, u64;     // Unsigned integers
using f32, f64;              // Floating point

// Math types (GLM)
using Vec2, Vec3, Vec4;      // Float vectors
using IVec2, IVec3, IVec4;   // Integer vectors
using Mat3, Mat4;            // Matrices
using Quat;                  // Quaternion

// Smart pointers
template<typename T> using Scope = std::unique_ptr<T>;
template<typename T> using Ref = std::shared_ptr<T>;

Scope<T> CreateScope<T>(args...);
Ref<T> CreateRef<T>(args...);
```

## Dependencies

| Library | Purpose | License |
|---------|---------|---------|
| [GLFW](https://www.glfw.org/) | Window/Input | Zlib |
| [GLM](https://github.com/g-truc/glm) | Math | MIT |
| [Dear ImGui](https://github.com/ocornut/imgui) | UI | MIT |
| [VMA](https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator) | Memory | MIT |
| [nlohmann/json](https://github.com/nlohmann/json) | JSON | MIT |
| [stb](https://github.com/nothings/stb) | Image loading | Public Domain |

## Examples

### Complete Application

```cpp
#include <tinyvk/tinyvk.h>
#include <imgui.h>

class ImageViewer : public tvk::App {
protected:
    void OnStart() override {
        TVK_LOG_INFO("Image Viewer started");
    }

    void OnUpdate() override {
        if (tvk::Input::IsKeyPressed(tvk::Key::Escape)) {
            Quit();
        }
    }

    void OnUI() override {
        // Menu bar
        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("Open...", "Ctrl+O")) {
                    OpenImage();
                }
                if (ImGui::MenuItem("Exit", "Esc")) {
                    Quit();
                }
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }

        // Image display
        ImGui::Begin("Image");
        if (_texture) {
            ImGui::Text("Size: %ux%u", _texture->GetWidth(), _texture->GetHeight());
            ImGui::Image(_texture->GetImGuiTextureID(), 
                        ImVec2(512, 512));
        } else {
            ImGui::TextDisabled("No image loaded");
        }
        ImGui::End();

        // Stats
        ImGui::Begin("Stats");
        ImGui::Text("FPS: %.1f", FPS());
        ImGui::Text("Frame Time: %.3f ms", DeltaTime() * 1000.0f);
        ImGui::End();
    }

private:
    void OpenImage() {
        auto result = tvk::OpenFileDialog(
            "Open Image",
            "Image Files",
            "*.png;*.jpg;*.jpeg;*.bmp"
        );
        
        if (result.has_value()) {
            _texture = LoadTexture(result.value());
            if (_texture) {
                _texture->BindToImGui();
            }
        }
    }

    tvk::Ref<tvk::Texture> _texture;
};

TVK_MAIN(ImageViewer, "Image Viewer", 1280, 720)
```

### Game with 3D Viewport

```cpp
#include <tinyvk/tinyvk.h>
#include <imgui.h>

class GameViewport : public tvk::RenderWidget {
protected:
    void OnRenderInit() override {
        // Create pipeline, load models, etc.
    }

    void OnRenderFrame(VkCommandBuffer cmd) override {
        BeginRenderPass(cmd);
        
        // Render your 3D scene
        // vkCmdBindPipeline(cmd, ...);
        // vkCmdDraw(cmd, ...);
        
        EndRenderPass(cmd);
    }

    void OnRenderUpdate(float deltaTime) override {
        // Update camera, physics, etc.
        if (tvk::Input::IsMouseButtonDown(tvk::MouseButton::Right)) {
            auto delta = tvk::Input::GetMouseDelta();
            _cameraRotation.x += delta.x * 0.1f;
            _cameraRotation.y += delta.y * 0.1f;
        }
    }

private:
    tvk::Vec2 _cameraRotation{0.0f, 0.0f};
};

class GameEditor : public tvk::App {
protected:
    void OnStart() override {
        _viewport = tvk::CreateScope<GameViewport>();
        RegisterWidget(_viewport.get());
    }

    void OnUI() override {
        // Main viewport
        ImGui::Begin("Viewport");
        auto size = ImGui::GetContentRegionAvail();
        _viewport->SetSize(
            static_cast<tvk::u32>(size.x),
            static_cast<tvk::u32>(size.y)
        );
        _viewport->SetEnabled(true);
        ImGui::End();

        // Inspector panel
        ImGui::Begin("Inspector");
        ImGui::Text("Object Properties");
        ImGui::SliderFloat("Position X", &_objectPos.x, -10.0f, 10.0f);
        ImGui::SliderFloat("Position Y", &_objectPos.y, -10.0f, 10.0f);
        ImGui::SliderFloat("Position Z", &_objectPos.z, -10.0f, 10.0f);
        ImGui::End();

        // Stats
        ImGui::Begin("Stats");
        ImGui::Text("FPS: %.1f", FPS());
        ImGui::End();
    }

private:
    tvk::Scope<GameViewport> _viewport;
    tvk::Vec3 _objectPos{0.0f, 0.0f, 0.0f};
};

TVK_MAIN(GameEditor, "Game Editor", 1920, 1080)
```

## Documentation

- [RenderWidget Guide](docs/render_widget.md) - Complete guide to custom rendering widgets

## License

This project is licensed under the Apache License 2.0 - see the [LICENSE](LICENSE) file for details.

## Author

**Rounak Paul** - [GitHub](https://github.com/Rounak-Paul)