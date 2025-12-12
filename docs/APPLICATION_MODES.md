# TinyVK Application Modes

TinyVK supports three application modes, making it versatile like a combination of **Qt** (for GUI apps) and **SDL3** (for games):

## Application Modes

### 1. **GUI Mode** (`AppMode::GUI`) - Qt-style
Perfect for desktop tools, editors, and utilities that only need UI.

**Features:**
- ImGui-only rendering
- No game render loop
- Ideal for tools and editors
- Similar to Qt desktop applications

**Example:**
```cpp
class MyTool : public tvk::App {
protected:
    void OnUI() override {
        ImGui::Begin("Tool");
        ImGui::Button("Click me!");
        ImGui::End();
    }
};

int main() {
    MyTool app;
    tvk::AppConfig config;
    config.mode = tvk::AppMode::GUI;  // GUI-only
    app.Run(config);
}
```

**Run example:**
```bash
./bin/gui_example
```

---

### 2. **Game Mode** (`AppMode::Game`) - SDL3-style
Perfect for games and simulations that need direct rendering control.

**Features:**
- Direct Vulkan rendering to swapchain
- No ImGui overhead (unless you want it)
- Full control over rendering
- Similar to SDL3 game development

**Example:**
```cpp
class MyGame : public tvk::App {
protected:
    void OnRender(VkCommandBuffer cmd) override {
        // Direct Vulkan rendering here
        // Draw your game using Vulkan commands
    }
};

int main() {
    MyGame app;
    tvk::AppConfig config;
    config.mode = tvk::AppMode::Game;  // Game-only
    app.Run(config);
}
```

**SDL3-style helpers:**
```cpp
// Get renderer for direct access
Renderer* renderer = app.GetRenderer();

// Get command buffer for rendering
VkCommandBuffer cmd = app.GetCommandBuffer();

// Set clear color
app.SetClearColor(0.2f, 0.3f, 0.4f);

// Access Vulkan context
VulkanContext& ctx = app.GetContext();
```

**Run example:**
```bash
./bin/game_example
```

---

### 3. **Hybrid Mode** (`AppMode::Hybrid`) - Best of Both
Perfect for level editors, modeling tools, or games with debug UI.

**Features:**
- Both ImGui UI AND game rendering
- Embedded 3D viewports using `RenderWidget`
- Flexible for complex applications
- Like Unity Editor or Unreal Editor

**Example:**
```cpp
class LevelEditor : public tvk::App {
protected:
    void OnRender(VkCommandBuffer cmd) override {
        // Optional: render to main window
    }
    
    void OnUI() override {
        // Draw your tool UI
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
    config.mode = tvk::AppMode::Hybrid;  // Both GUI and Game
    app.Run(config);
}
```

**Run example:**
```bash
./bin/hybrid_example
```

---

## Rendering Callbacks

### All Modes
- `OnStart()` - Initialize your app
- `OnUpdate()` - Update logic every frame
- `OnStop()` - Cleanup

### GUI/Hybrid Modes
- `OnUI()` - Draw ImGui interface

### Game/Hybrid Modes
- `OnPreRender()` - Prepare for rendering
- `OnRender(VkCommandBuffer cmd)` - Direct Vulkan rendering
- `OnPostRender()` - Post-processing / cleanup

---

## Comparison

| Feature | GUI Mode | Game Mode | Hybrid Mode |
|---------|----------|-----------|-------------|
| ImGui UI | ✅ | ❌ | ✅ |
| Direct Rendering | ❌ | ✅ | ✅ |
| RenderWidgets | ✅ | ❌ | ✅ |
| Performance | Good | Best | Good |
| Use Case | Tools/Editors | Games | Level Editors |
| Similar To | Qt | SDL3 | Unity Editor |

---

## API Comparison

### Qt-style (GUI Mode)
```cpp
// Desktop application with UI
class MyApp : public tvk::App {
    void OnUI() override {
        ImGui::Begin("Window");
        // Build your interface
        ImGui::End();
    }
};
```

### SDL3-style (Game Mode)
```cpp
// Game with direct rendering
class MyGame : public tvk::App {
    void OnRender(VkCommandBuffer cmd) override {
        // Direct Vulkan commands
        vkCmdDraw(...);
    }
};

// SDL3-style helpers
app.GetRenderer();      // Direct renderer access
app.GetCommandBuffer(); // Current command buffer
app.SetClearColor();    // Quick clear color
```

### Hybrid (Best of Both)
```cpp
// Level editor with viewports
class Editor : public tvk::App {
    void OnRender(VkCommandBuffer cmd) override {
        // Main scene rendering
    }
    
    void OnUI() override {
        // Tool UI + embedded viewports
        _viewport->RenderImage();
    }
};
```

---

## Examples

1. **GUI Example** (`examples/gui_example.cpp`)
   - Pure ImGui tool
   - Multiple windows
   - Input widgets

2. **Game Example** (`examples/game_example.cpp`)
   - Direct rendering
   - No UI overhead
   - SDL3-style

3. **Hybrid Example** (`examples/hybrid_example.cpp`)
   - Level editor
   - Viewport + Properties + Hierarchy
   - Best of both worlds

---

## Quick Start

```cpp
#include <tinyvk/tinyvk.h>

class MyApp : public tvk::App {
protected:
    void OnUI() override {
        ImGui::ShowDemoWindow();
    }
};

int main() {
    MyApp app;
    tvk::AppConfig config;
    config.title = "My Application";
    config.mode = tvk::AppMode::Hybrid;  // Choose your mode
    app.Run(config);
    return 0;
}
```

**Build and run:**
```bash
cd build
cmake ..
cmake --build .
./bin/your_app
```
