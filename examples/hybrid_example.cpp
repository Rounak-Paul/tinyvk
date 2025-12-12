/**
 * @file hybrid_example.cpp
 * @brief Hybrid example - combines GUI tools with embedded rendering
 * Demonstrates AppMode::Hybrid for level editors, modeling tools, etc.
 */

#include <tinyvk/tinyvk.h>
#include <imgui.h>
#include <glm/glm.hpp>

class HybridViewport : public tvk::RenderWidget {
protected:
    void OnRenderInit() override {
        SetClearColor(0.15f, 0.15f, 0.2f, 1.0f);
        _rotation = 0.0f;
    }
    
    void OnRenderFrame(VkCommandBuffer cmd) override {
        BeginRenderPass(cmd);
        // Render 3D content here once pipeline is ready
        EndRenderPass(cmd);
    }
    
    void OnRenderUpdate(float deltaTime) override {
        _rotation += deltaTime * 30.0f;
        if (_rotation > 360.0f) _rotation -= 360.0f;
    }

private:
    float _rotation;
};

class HybridExample : public tvk::App {
protected:
    void OnStart() override {
        TVK_LOG_INFO("Hybrid mode example started");
        
        _viewport = tvk::CreateScope<HybridViewport>();
        RegisterWidget(_viewport.get());
        
        _showViewport = true;
        _showProperties = true;
        _showHierarchy = true;
    }
    
    void OnUpdate() override {
        if (tvk::Input::IsKeyPressed(tvk::Key::Escape)) {
            Quit();
        }
    }
    
    void OnUI() override {
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::DockSpaceOverViewport(0, viewport);
        
        // Menu bar
        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("New Scene")) {}
                if (ImGui::MenuItem("Open Scene...")) {}
                if (ImGui::MenuItem("Save Scene")) {}
                ImGui::Separator();
                if (ImGui::MenuItem("Exit", "Esc")) {
                    Quit();
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("View")) {
                ImGui::MenuItem("Viewport", nullptr, &_showViewport);
                ImGui::MenuItem("Properties", nullptr, &_showProperties);
                ImGui::MenuItem("Hierarchy", nullptr, &_showHierarchy);
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }
        
        // Viewport window
        if (_showViewport && _viewport) {
            ImGui::Begin("Viewport", &_showViewport);
            _viewport->SetEnabled(true);
            _viewport->RenderImage();
            ImGui::End();
        } else if (_viewport) {
            _viewport->SetEnabled(false);
        }
        
        // Properties window
        if (_showProperties) {
            ImGui::Begin("Properties", &_showProperties);
            ImGui::Text("Selected: None");
            ImGui::Separator();
            
            ImGui::Text("Transform");
            static float pos[3] = {0, 0, 0};
            static float rot[3] = {0, 0, 0};
            static float scale[3] = {1, 1, 1};
            
            ImGui::DragFloat3("Position", pos, 0.1f);
            ImGui::DragFloat3("Rotation", rot, 1.0f);
            ImGui::DragFloat3("Scale", scale, 0.1f);
            
            ImGui::End();
        }
        
        // Hierarchy window
        if (_showHierarchy) {
            ImGui::Begin("Scene Hierarchy", &_showHierarchy);
            
            if (ImGui::TreeNode("Scene Root")) {
                if (ImGui::TreeNode("Camera")) {
                    ImGui::Text("Main Camera");
                    ImGui::TreePop();
                }
                if (ImGui::TreeNode("Objects")) {
                    ImGui::Text("Cube");
                    ImGui::Text("Sphere");
                    ImGui::Text("Plane");
                    ImGui::TreePop();
                }
                if (ImGui::TreeNode("Lights")) {
                    ImGui::Text("Directional Light");
                    ImGui::TreePop();
                }
                ImGui::TreePop();
            }
            
            ImGui::End();
        }
        
        // Stats window
        ImGui::Begin("Statistics");
        ImGui::Text("FPS: %.1f", FPS());
        ImGui::Text("Frame time: %.3f ms", DeltaTime() * 1000.0f);
        ImGui::Separator();
        ImGui::Text("Mode: Hybrid (GUI + Game)");
        ImGui::TextWrapped("This demonstrates a level editor / modeling tool "
                          "with both GUI controls and embedded 3D viewport.");
        ImGui::End();
    }
    
    void OnStop() override {
        TVK_LOG_INFO("Hybrid mode example stopped");
    }

private:
    tvk::Scope<HybridViewport> _viewport;
    bool _showViewport;
    bool _showProperties;
    bool _showHierarchy;
};

int main() {
    try {
        HybridExample app;
        
        tvk::AppConfig config;
        config.title = "TinyVK - Hybrid Mode Example (Level Editor)";
        config.width = 1600;
        config.height = 900;
        config.mode = tvk::AppMode::Hybrid;  // Hybrid mode
        config.vsync = true;
        
        app.Run(config);
    } catch (const std::exception& e) {
        TVK_LOG_FATAL("Exception: {}", e.what());
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
