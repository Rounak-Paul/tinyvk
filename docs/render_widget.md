# RenderWidget

`RenderWidget` provides custom Vulkan rendering surfaces within your ImGui application, similar to Qt's `QOpenGLWidget` or `QVulkanWidget`. This allows you to create game viewports, 3D previews, or any custom rendering within your UI.

## Basic Usage

```cpp
#include <tinyvk/tinyvk.h>

class GameViewport : public tvk::RenderWidget {
protected:
    void OnRenderInit() override {
        // Initialize your graphics resources
        // Create pipelines, buffers, textures, etc.
    }

    void OnRenderFrame(VkCommandBuffer cmd) override {
        // Begin rendering to the widget's render target
        BeginRenderPass(cmd);
        
        // Record your custom rendering commands
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _myPipeline);
        vkCmdDraw(cmd, vertexCount, 1, 0, 0);
        
        // End the render pass
        EndRenderPass(cmd);
    }

    void OnRenderUpdate(float deltaTime) override {
        // Update your game/scene logic
        _rotation += deltaTime * 45.0f;
    }

    void OnRenderResize(tvk::u32 width, tvk::u32 height) override {
        // Handle widget resize
        // Recreate any size-dependent resources
    }

    void OnRenderCleanup() override {
        // Clean up your resources
        auto* ctx = GetContext();
        vkDestroyPipeline(ctx->GetDevice(), _myPipeline, nullptr);
    }

private:
    VkPipeline _myPipeline = VK_NULL_HANDLE;
    float _rotation = 0.0f;
};
```

## Integrating with Your Application

```cpp
class MyApp : public tvk::App {
protected:
    void OnStart() override {
        // Create and register the widget
        _viewport = tvk::CreateScope<GameViewport>();
        RegisterWidget(_viewport.get());
    }

    void OnUI() override {
        // Display the widget in an ImGui window
        if (ImGui::Begin("Game")) {
            // The widget will render automatically here
            _viewport->SetEnabled(true);
        } else {
            _viewport->SetEnabled(false);
        }
        ImGui::End();
    }

private:
    tvk::Scope<GameViewport> _viewport;
};
```

## Virtual Methods

### OnRenderInit()
Called once when the widget is initialized. Use this to:
- Create Vulkan pipelines
- Allocate buffers and textures
- Load shaders and materials
- Initialize scene data

### OnRenderFrame(VkCommandBuffer cmd)
Called every frame with the current command buffer. Use this to:
- Record rendering commands
- Bind pipelines and descriptor sets
- Draw geometry
- Always wrap your commands with `BeginRenderPass()` and `EndRenderPass()`

### OnRenderUpdate(float deltaTime)
Called every frame before rendering. Use this to:
- Update game/scene logic
- Animate objects
- Handle physics
- Process input

### OnRenderResize(u32 width, u32 height)
Called when the widget size changes. Use this to:
- Recreate size-dependent resources
- Update projection matrices
- Adjust viewport/scissor

### OnRenderCleanup()
Called when the widget is destroyed. Use this to:
- Destroy pipelines
- Free buffers and textures
- Clean up Vulkan resources

## Helper Methods

### BeginRenderPass(VkCommandBuffer cmd)
Begins the widget's render pass with:
- Clear color: black (0, 0, 0, 1)
- Clear depth: 1.0
- Automatic viewport and scissor setup

### EndRenderPass(VkCommandBuffer cmd)
Ends the render pass. Always call after `BeginRenderPass()`.

### GetContext()
Returns the `VulkanContext*` for creating Vulkan resources:
```cpp
auto* ctx = GetContext();
VkDevice device = ctx->GetDevice();
```

### GetCommandBuffer()
Returns the current command buffer during `OnRenderFrame()`.

## Widget Management

### SetSize(u32 width, u32 height)
Resize the widget's render target. This will trigger `OnRenderResize()`.

### SetEnabled(bool enabled)
Enable or disable the widget. Disabled widgets don't render.

### IsEnabled()
Check if the widget is currently enabled.

## Advanced Example

```cpp
class EditorViewport : public tvk::RenderWidget {
protected:
    void OnRenderInit() override {
        auto* ctx = GetContext();
        CreatePipeline(ctx);
        CreateVertexBuffer(ctx);
    }

    void OnRenderFrame(VkCommandBuffer cmd) override {
        BeginRenderPass(cmd);
        
        // Bind pipeline
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipeline);
        
        // Bind vertex buffer
        VkBuffer vertexBuffers[] = {_vertexBuffer};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(cmd, 0, 1, vertexBuffers, offsets);
        
        // Draw
        vkCmdDraw(cmd, _vertexCount, 1, 0, 0);
        
        EndRenderPass(cmd);
    }

    void OnRenderUpdate(float deltaTime) override {
        _time += deltaTime;
        
        // Update camera based on input
        if (tvk::Input::IsMouseButtonDown(tvk::MouseButton::Right)) {
            auto delta = tvk::Input::GetMouseDelta();
            _camera.Rotate(delta.x * 0.1f, delta.y * 0.1f);
        }
    }

    void OnRenderCleanup() override {
        auto* ctx = GetContext();
        VkDevice device = ctx->GetDevice();
        
        vkDestroyPipeline(device, _pipeline, nullptr);
        vkDestroyBuffer(device, _vertexBuffer, nullptr);
        vkFreeMemory(device, _vertexBufferMemory, nullptr);
    }

private:
    VkPipeline _pipeline = VK_NULL_HANDLE;
    VkBuffer _vertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory _vertexBufferMemory = VK_NULL_HANDLE;
    u32 _vertexCount = 0;
    float _time = 0.0f;
    Camera _camera;
};
```

## Notes

- Widgets automatically create their own render targets (color + depth)
- The render target is 800x600 by default, call `SetSize()` to change
- Render targets are automatically recreated on resize
- Widgets are displayed in ImGui using `ImGui::Image()`
- Multiple widgets can exist simultaneously
- Each widget has independent Vulkan resources
- The framework handles synchronization and lifecycle

## Common Patterns

### Responsive Widget Size
```cpp
void OnUI() override {
    ImGui::Begin("Viewport");
    
    auto size = ImGui::GetContentRegionAvail();
    _viewport->SetSize(static_cast<u32>(size.x), static_cast<u32>(size.y));
    
    ImGui::End();
}
```

### Toggle Widget Visibility
```cpp
void OnUI() override {
    if (_showViewport) {
        ImGui::Begin("Viewport", &_showViewport);
        _viewport->SetEnabled(true);
        ImGui::End();
    } else {
        _viewport->SetEnabled(false);
    }
}
```

### Multiple Viewports
```cpp
class MyApp : public tvk::App {
protected:
    void OnStart() override {
        _viewportTop = tvk::CreateScope<GameViewport>();
        _viewportSide = tvk::CreateScope<GameViewport>();
        RegisterWidget(_viewportTop.get());
        RegisterWidget(_viewportSide.get());
    }

private:
    tvk::Scope<GameViewport> _viewportTop;
    tvk::Scope<GameViewport> _viewportSide;
};
```
