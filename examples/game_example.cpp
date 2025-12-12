/**
 * @file game_example.cpp
 * @brief SDL3-style game example - pure rendering without ImGui
 * Demonstrates AppMode::Game for full-window game rendering
 */

#include <tinyvk/tinyvk.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class GameExample : public tvk::App {
protected:
    void OnStart() override {
        TVK_LOG_INFO("Game mode example started");
        
        SetClearColor(0.2f, 0.3f, 0.4f, 1.0f);
        
        _rotation = 0.0f;
    }
    
    void OnUpdate() override {
        _rotation += DeltaTime() * 45.0f;
        if (_rotation > 360.0f) _rotation -= 360.0f;
        
        if (tvk::Input::IsKeyPressed(tvk::Key::Escape)) {
            Quit();
        }
    }
    
    void OnRender(VkCommandBuffer cmd) override {
        // In a real game, here you would:
        // 1. Begin render pass with your framebuffer
        // 2. Bind graphics pipeline
        // 3. Bind descriptor sets (MVP matrices, textures)
        // 4. Draw your meshes
        // 5. End render pass
        
        // For now, the clear color shows that game mode is working
        // Once we add pipeline support, meshes can be rendered here
    }
    
    void OnStop() override {
        TVK_LOG_INFO("Game mode example stopped");
    }

private:
    float _rotation = 0.0f;
};

int main() {
    try {
        GameExample app;
        
        tvk::AppConfig config;
        config.title = "TinyVK - Game Mode Example";
        config.width = 1280;
        config.height = 720;
        config.mode = tvk::AppMode::Game;  // Pure game mode
        config.vsync = true;
        
        app.Run(config);
    } catch (const std::exception& e) {
        TVK_LOG_FATAL("Exception: {}", e.what());
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
