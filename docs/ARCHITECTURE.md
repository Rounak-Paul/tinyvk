# TinyVK Architecture Summary

## Design Philosophy

TinyVK combines the best aspects of two major frameworks:

- **Qt** - For GUI applications (tools, editors, desktop apps)
- **SDL3** - For games (direct rendering, performance-first)

This gives developers maximum flexibility to create:
1. Pure GUI tools
2. Pure games
3. Hybrid applications (level editors, modeling tools)

---

## Application Modes

### `AppMode::GUI` - Qt-style
- ImGui-only rendering
- No game render overhead
- Perfect for desktop tools
- Example: Image viewer, file browser, settings editor

### `AppMode::Game` - SDL3-style  
- Direct Vulkan command buffer access
- No ImGui overhead (optional)
- Full rendering control
- Example: FPS game, simulation, real-time graphics

### `AppMode::Hybrid` - Best of Both
- ImGui UI + Direct rendering
- Embedded 3D viewports via `RenderWidget`
- Flexible architecture
- Example: Unity-style level editor, Blender-like modeling tool

---

## Key Classes

### `tvk::App`
Base application class with three modes of operation:

**Common Callbacks:**
- `OnStart()` - Initialize app
- `OnUpdate()` - Update logic each frame
- `OnStop()` - Cleanup

**GUI/Hybrid Mode:**
- `OnUI()` - Draw ImGui interface

**Game/Hybrid Mode:**
- `OnPreRender()` - Pre-rendering setup
- `OnRender(VkCommandBuffer cmd)` - Direct Vulkan rendering
- `OnPostRender()` - Post-rendering cleanup

### `tvk::RenderWidget`
Embeddable 3D viewport for Hybrid mode (like Qt's QOpenGLWidget):

- `OnRenderInit()` - Initialize resources
- `OnRenderFrame(VkCommandBuffer)` - Render to widget
- `OnRenderUpdate(float)` - Update widget state
- `OnRenderCleanup()` - Cleanup resources
- `RenderImage()` - Display in ImGui window

### Helper APIs

**SDL3-style Direct Access:**
```cpp
Renderer* renderer = app.GetRenderer();
VkCommandBuffer cmd = app.GetCommandBuffer();
VulkanContext& ctx = app.GetContext();
app.SetClearColor(r, g, b, a);
```

**Qt-style Widget System:**
```cpp
auto viewport = CreateScope<MyViewport>();
app.RegisterWidget(viewport.get());
viewport->RenderImage();  // Display in ImGui
```

---

## Architecture Benefits

### Flexibility
- Choose the right mode for your project
- Start with GUI mode, add rendering later
- Or start with Game mode, add UI for debugging

### Performance
- GUI mode: No rendering overhead
- Game mode: No ImGui overhead  
- Hybrid mode: Both available when needed

### Familiar APIs
- Qt developers: Comfortable with widget system
- SDL3 developers: Comfortable with direct rendering
- Both: Easy to learn and use

---

## Use Cases

### Desktop Tools (GUI Mode)
- Image editors
- File browsers
- Configuration tools
- Data visualizers
- Log viewers

### Games (Game Mode)
- First-person shooters
- Strategy games
- Simulations
- Real-time graphics demos
- Physics visualizations

### Hybrid Applications (Hybrid Mode)
- Level editors (Unity, Unreal style)
- 3D modeling tools (Blender style)
- Animation editors
- Material editors
- Scene inspectors
- Debug viewers with 3D preview

---

## Example Structure

### GUI Application
```
MyTool/
├── main.cpp          (App with OnUI)
└── CMakeLists.txt
```

### Game Application  
```
MyGame/
├── main.cpp          (App with OnRender)
├── game_state.h
├── mesh.h
└── CMakeLists.txt
```

### Hybrid Application
```
LevelEditor/
├── main.cpp          (App with OnUI + OnRender)
├── editor_viewport.h (RenderWidget)
├── scene.h
├── properties.h
└── CMakeLists.txt
```

---

## Future Enhancements

- Graphics pipeline abstraction (in progress)
- Shader compilation system
- Material system
- Scene graph
- Physics integration
- Asset management
- Serialization

---

## Comparison

| Framework | TinyVK GUI | TinyVK Game | TinyVK Hybrid |
|-----------|------------|-------------|---------------|
| Qt | ✅ Similar | ❌ | ✅ Better |
| SDL3 | ❌ | ✅ Similar | ✅ Better |
| Dear ImGui | ✅ Built-in | Optional | ✅ Built-in |
| Vulkan | Abstracted | Direct | Both |
| Use Case | Tools | Games | Editors |
