# TinyVK

A lightweight Vulkan rendering framework with ImGui integration for rapid prototyping and tool development.

![License](https://img.shields.io/badge/license-Apache%202.0-blue.svg)
![C++](https://img.shields.io/badge/C%2B%2B-20-blue.svg)
![Vulkan](https://img.shields.io/badge/Vulkan-1.2-red.svg)

## Features

- **Simple Application Framework** - Inherit from `tvk::Application` and override virtual methods
- **Vulkan Abstraction** - Handles instance, device, swapchain, and synchronization
- **ImGui Integration** - Built-in Dear ImGui support with Vulkan backend
- **Cross-Platform** - Supports macOS (MoltenVK), Windows, and Linux
- **Modern C++20** - Uses smart pointers, type aliases, and modern patterns

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
./bin/sandbox
```

## Quick Start

Create a simple application:

```cpp
#include <tinyvk/tinyvk.h>
#include <imgui.h>

class MyApp : public tvk::Application {
public:
    MyApp(const tvk::ApplicationConfig& config) : Application(config) {}

protected:
    void OnInit() override {
        TVK_LOG_INFO("App initialized!");
    }

    void OnUpdate(float dt) override {
        // Game logic here
    }

    void OnRender() override {
        // Custom Vulkan rendering here
    }

    void OnImGui() override {
        ImGui::Begin("Hello");
        ImGui::Text("FPS: %.1f", GetFPS());
        ImGui::End();
    }

    void OnShutdown() override {
        TVK_LOG_INFO("App shutting down");
    }
};

int main() {
    tvk::ApplicationConfig config;
    config.name = "My Application";
    config.windowWidth = 1280;
    config.windowHeight = 720;
    
    MyApp app(config);
    app.Run();
    
    return 0;
}
```

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
class Application {
    void Run();                           // Start main loop
    void Quit();                          // Request exit
    
    Window& GetWindow();                  // Get window reference
    Renderer& GetRenderer();              // Get renderer reference
    
    float GetDeltaTime() const;           // Frame delta time
    float GetElapsedTime() const;         // Time since start
    float GetFPS() const;                 // Current FPS
    
protected:
    virtual void OnInit() {}              // Called after init
    virtual void OnUpdate(float dt) {}    // Called every frame
    virtual void OnRender() {}            // Custom rendering
    virtual void OnImGui() {}             // ImGui rendering
    virtual void OnShutdown() {}          // Before cleanup
    virtual void OnResize(u32 w, u32 h) {}// Window resize
};
```

### Input

```cpp
class Input {
    static bool IsKeyPressed(Key key);
    static bool IsMouseButtonPressed(MouseButton button);
    static Vec2 GetMousePosition();
    static Vec2 GetMouseDelta();
    static Vec2 GetScrollDelta();
    static void SetCursorMode(int mode);
};
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

## License

This project is licensed under the Apache License 2.0 - see the [LICENSE](LICENSE) file for details.

## Author

**Rounak Paul** - [GitHub](https://github.com/Rounak-Paul)