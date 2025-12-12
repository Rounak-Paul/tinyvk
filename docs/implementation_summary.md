# RenderWidget Implementation Summary

## Overview

Implemented `RenderWidget` class that provides custom Vulkan rendering surfaces within ImGui applications, similar to Qt's `QOpenGLWidget` or `QVulkanWidget`. This allows users to create game viewports, 3D content, or any custom rendering within their UI.

## Files Created/Modified

### Created Files

1. **include/tinyvk/ui/render_widget.h**
   - Header file defining the `RenderWidget` class
   - Virtual methods for subclassing: `OnRenderInit()`, `OnRenderFrame()`, `OnRenderUpdate()`, `OnRenderCleanup()`, `OnRenderResize()`
   - Helper methods: `BeginRenderPass()`, `EndRenderPass()`, `GetContext()`, `GetCommandBuffer()`
   - Private members for Vulkan resources: render target, framebuffer, depth buffer, ImGui texture descriptor

2. **src/ui/render_widget.cpp**
   - Implementation of all RenderWidget methods
   - `CreateRenderTarget()` - Creates VkImage (800x600 default), VkFramebuffer, depth buffer, render pass
   - `Render()` - Calls virtual methods and displays result in ImGui
   - `Cleanup()` - Destroys all Vulkan resources
   - `RecreateRenderTarget()` - Handles resize by destroying and recreating resources

3. **docs/render_widget.md**
   - Complete documentation for RenderWidget
   - Usage examples, virtual method descriptions, helper methods
   - Common patterns and advanced examples

### Modified Files

1. **include/tinyvk/core/application.h**
   - Added forward declaration for `RenderWidget`
   - Added `RegisterWidget()` and `UnregisterWidget()` methods
   - Added `std::vector<RenderWidget*> _widgets` member

2. **src/core/application.cpp**
   - Added `#include "tinyvk/ui/render_widget.h"`
   - Implemented `RegisterWidget()` and `UnregisterWidget()`
   - Modified render loop to call `Render()` on all enabled widgets
   - Added widget cleanup in `Shutdown()`

3. **include/tinyvk/tinyvk.h**
   - Added `#include "ui/render_widget.h"` to expose RenderWidget to users

4. **CMakeLists.txt**
   - Added `src/ui/render_widget.cpp` to `TINYVK_SOURCES`

5. **sandbox/main.cpp**
   - Created `GameViewport` class extending `RenderWidget` as example
   - Added viewport toggle in View menu
   - Integrated viewport with sandbox application

6. **README.md**
   - Updated features list to include RenderWidget
   - Added RenderWidget section in Quick Start
   - Updated API reference with RenderWidget methods
   - Added comprehensive examples showing image viewer and game editor

## Architecture

### RenderWidget Class

```
RenderWidget
├── Initialization
│   ├── Initialize(Renderer*) - Called by framework
│   └── OnRenderInit() - Override for custom setup
├── Rendering
│   ├── Render(float deltaTime) - Called by framework
│   ├── OnRenderUpdate(float) - Override for logic updates
│   └── OnRenderFrame(VkCommandBuffer) - Override for rendering
├── Resize Handling
│   ├── SetSize(width, height) - Triggers resize
│   ├── OnRenderResize(width, height) - Override for resize handling
│   └── RecreateRenderTarget() - Internal recreation
└── Cleanup
    ├── Cleanup() - Called by framework
    └── OnRenderCleanup() - Override for custom cleanup
```

### Integration with Application

```
App::Run()
└── MainLoop()
    ├── OnUpdate()
    ├── BeginFrame()
    ├── ImGuiLayer::Begin()
    ├── OnUI()
    ├── For each registered widget:
    │   └── widget->Render(deltaTime)
    │       ├── OnRenderUpdate(deltaTime)
    │       ├── OnRenderFrame(commandBuffer)
    │       └── ImGui::Image(widgetTexture)
    ├── ImGuiLayer::End()
    └── EndFrame()
```

### Vulkan Resources per Widget

Each RenderWidget creates and manages:
- **VkImage** - Color render target (R8G8B8A8_UNORM)
- **VkImageView** - View for the color image
- **VkDeviceMemory** - Memory for color image
- **VkSampler** - Sampler for ImGui texture
- **VkImage** - Depth buffer (D32_SFLOAT)
- **VkImageView** - View for depth image
- **VkDeviceMemory** - Memory for depth image
- **VkRenderPass** - Custom render pass for widget
- **VkFramebuffer** - Framebuffer with color + depth attachments
- **VkDescriptorSet** - ImGui texture descriptor (from ImGui_ImplVulkan_AddTexture)

## Key Features

1. **Automatic Resource Management**
   - Widgets create their own render targets on initialization
   - Resources automatically recreated on resize
   - Cleanup handled by framework

2. **ImGui Integration**
   - Render target displayed using `ImGui::Image()`
   - Seamless integration with ImGui windows and docking

3. **Multiple Widgets**
   - Support for multiple simultaneous widgets
   - Each widget has independent resources
   - Enable/disable widgets dynamically

4. **User-Friendly API**
   - Simple virtual methods to override
   - Helper methods for common operations
   - Access to Vulkan context for advanced usage

5. **Proper Synchronization**
   - Uses existing renderer's command buffer
   - No additional synchronization primitives needed
   - Integrated with renderer's frame timing

## Usage Pattern

```cpp
// 1. Define custom widget class
class MyWidget : public tvk::RenderWidget {
protected:
    void OnRenderInit() override {
        // Create pipelines, buffers, etc.
    }
    
    void OnRenderFrame(VkCommandBuffer cmd) override {
        BeginRenderPass(cmd);
        // Record rendering commands
        EndRenderPass(cmd);
    }
    
    void OnRenderUpdate(float deltaTime) override {
        // Update logic
    }
};

// 2. Create and register in application
class MyApp : public tvk::App {
protected:
    void OnStart() override {
        _widget = tvk::CreateScope<MyWidget>();
        RegisterWidget(_widget.get());
    }
    
    void OnUI() override {
        ImGui::Begin("Widget");
        _widget->SetEnabled(true);
        ImGui::End();
    }
    
private:
    tvk::Scope<MyWidget> _widget;
};
```

## Testing

Tested successfully on macOS with:
- ✅ Build successful (no errors/warnings)
- ✅ Widget registration/unregistration
- ✅ Render target creation
- ✅ ImGui display integration
- ✅ Multiple widgets
- ✅ Enable/disable toggling

## Future Enhancements

Potential improvements:
1. **MSAA Support** - Add multisampling for anti-aliasing
2. **Custom Formats** - Allow users to specify color/depth formats
3. **Viewport Input** - Add mouse/keyboard input within widget bounds
4. **Screenshots** - Built-in screenshot/recording functionality
5. **Performance Metrics** - Per-widget profiling
6. **Multiple Render Passes** - Support for post-processing

## Performance Considerations

- Each widget allocates ~4-8 MB for 800x600 render target (depends on format)
- Render target recreation on resize causes brief stall (acceptable for UI)
- Command buffer recording is efficient (reuses renderer's command buffer)
- No additional CPU overhead from multiple widgets (just loop iteration)

## Conclusion

The RenderWidget implementation provides a powerful and flexible way to integrate custom Vulkan rendering into ImGui applications. It follows the TinyVK philosophy of hiding complexity while providing access to advanced features when needed. The API is simple enough for beginners but powerful enough for complex game engines and 3D editors.
