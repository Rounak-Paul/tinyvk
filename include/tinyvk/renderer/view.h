/**
 * @file view.h
 * @brief View class combining camera, scene, and render target (Filament-style)
 */

#pragma once

#include "../core/types.h"
#include "../scene/scene.h"
#include "../scene/camera.h"
#include <vulkan/vulkan.h>

namespace tvk {

class Renderer;

struct Viewport {
    i32 x = 0;
    i32 y = 0;
    u32 width = 0;
    u32 height = 0;
};

struct RenderTarget {
    VkFramebuffer framebuffer = VK_NULL_HANDLE;
    VkRenderPass render_pass = VK_NULL_HANDLE;
    VkExtent2D extent = {0, 0};
    VkFormat color_format = VK_FORMAT_UNDEFINED;
    VkFormat depth_format = VK_FORMAT_UNDEFINED;
};

class View {
public:
    View() = default;
    ~View() = default;

    void SetScene(Ref<Scene> scene) { _scene = scene; }
    Ref<Scene> GetScene() const { return _scene; }

    void SetCamera(Camera* camera) { _camera = camera; }
    Camera* GetCamera() const { return _camera; }

    void SetViewport(const Viewport& viewport) { _viewport = viewport; }
    const Viewport& GetViewport() const { return _viewport; }

    void SetRenderTarget(const RenderTarget& target) { _render_target = target; }
    const RenderTarget& GetRenderTarget() const { return _render_target; }

    void SetClearColor(const Color& color) { _clear_color = color; }
    const Color& GetClearColor() const { return _clear_color; }

    void SetPostProcessingEnabled(bool enabled) { _post_processing = enabled; }
    bool IsPostProcessingEnabled() const { return _post_processing; }

    void SetShadowsEnabled(bool enabled) { _shadows_enabled = enabled; }
    bool AreShadowsEnabled() const { return _shadows_enabled; }

    void SetAntiAliasing(u32 samples) { _aa_samples = samples; }
    u32 GetAntiAliasing() const { return _aa_samples; }

    void SetVisibleLayers(u32 mask) { _visible_layers = mask; }
    u32 GetVisibleLayers() const { return _visible_layers; }

private:
    Ref<Scene> _scene;
    Camera* _camera = nullptr;
    Viewport _viewport;
    RenderTarget _render_target;
    Color _clear_color = Color::Black();
    bool _post_processing = false;
    bool _shadows_enabled = true;
    u32 _aa_samples = 1;
    u32 _visible_layers = 0xFFFFFFFF;
};

} // namespace tvk
