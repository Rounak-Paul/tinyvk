/**
 * @file main.cpp
 * @brief TinyVK comprehensive example demonstrating all features and application modes
 * 
 * This sandbox demonstrates:
 * - All three AppModes: GUI, Game, and Hybrid
 * - RenderWidget for embedded viewports
 * - Mesh/geometry rendering with various primitives
 * - Graphics pipeline usage
 * - Texture loading and ImGui integration
 * - File dialogs
 * - Input handling
 * - ImGui docking and menus
 */

#include <tinyvk/tinyvk.h>
#include <imgui.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "tinyvk/renderer/pipeline.h"
#include "tinyvk/renderer/shaders.h"

class GameViewport : public tvk::RenderWidget {
protected:
    void OnRenderInit() override {
        _rotation = 0.0f;
        
        // Create various geometries
        _cubeMesh = tvk::Geometry::CreateCube(GetRenderer(), 1.0f);
        _sphereMesh = tvk::Geometry::CreateSphere(GetRenderer(), 0.5f, 32, 16);
        _planeMesh = tvk::Geometry::CreatePlane(GetRenderer(), 2.0f, 2.0f, 10, 10);
        _cylinderMesh = tvk::Geometry::CreateCylinder(GetRenderer(), 0.3f, 1.5f, 24);
        _coneMesh = tvk::Geometry::CreateCone(GetRenderer(), 0.5f, 1.0f, 24);
        _torusMesh = tvk::Geometry::CreateTorus(GetRenderer(), 0.5f, 0.2f, 32, 16);
        
        TVK_LOG_INFO("GameViewport initialized:");
        if (_cubeMesh) TVK_LOG_INFO("  Cube: {} vertices, {} indices", _cubeMesh->GetVertexCount(), _cubeMesh->GetIndexCount());
        if (_sphereMesh) TVK_LOG_INFO("  Sphere: {} vertices, {} indices", _sphereMesh->GetVertexCount(), _sphereMesh->GetIndexCount());
        if (_planeMesh) TVK_LOG_INFO("  Plane: {} vertices, {} indices", _planeMesh->GetVertexCount(), _planeMesh->GetIndexCount());
        if (_cylinderMesh) TVK_LOG_INFO("  Cylinder: {} vertices, {} indices", _cylinderMesh->GetVertexCount(), _cylinderMesh->GetIndexCount());
        if (_coneMesh) TVK_LOG_INFO("  Cone: {} vertices, {} indices", _coneMesh->GetVertexCount(), _coneMesh->GetIndexCount());
        if (_torusMesh) TVK_LOG_INFO("  Torus: {} vertices, {} indices", _torusMesh->GetVertexCount(), _torusMesh->GetIndexCount());
        
        SetClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        
        _pipeline = tvk::CreateScope<tvk::Pipeline>();
        if (!_pipeline->Create(GetRenderer(), GetRenderPass(), tvk::shaders::basic_vert, tvk::shaders::basic_frag)) {
            TVK_LOG_ERROR("Failed to create graphics pipeline");
        }
    }

    void OnRenderFrame(VkCommandBuffer cmd) override {
        BeginRenderPass(cmd);
        
        if (_pipeline && _cubeMesh && GetWidth() > 0 && GetHeight() > 0) {
            _pipeline->Bind(cmd);
            
            glm::mat4 view = glm::lookAt(
                glm::vec3(0.0f, 0.0f, 3.0f),
                glm::vec3(0.0f, 0.0f, 0.0f),
                glm::vec3(0.0f, 1.0f, 0.0f)
            );
            
            float aspect = (float)GetWidth() / (float)GetHeight();
            glm::mat4 proj = glm::perspective(
                glm::radians(45.0f),
                aspect,
                0.1f,
                100.0f
            );
            proj[1][1] *= -1;
            
            glm::mat4 model = glm::rotate(
                glm::mat4(1.0f),
                glm::radians(_rotation),
                glm::vec3(0.0f, 1.0f, 0.0f)
            );
            
            tvk::PushConstants push;
            push.model = model;
            push.view_projection = proj * view;
            
            _pipeline->SetPushConstants(cmd, push);
            _cubeMesh->Draw(cmd);
        }
        
        EndRenderPass(cmd);
    }

    void OnRenderUpdate(float deltaTime) override {
        _rotation += deltaTime * 45.0f;
        if (_rotation > 360.0f) _rotation -= 360.0f;
    }

    void OnRenderResize(tvk::u32 width, tvk::u32 height) override {
        TVK_LOG_INFO("GameViewport resized to {}x{}", width, height);
    }

    void OnRenderCleanup() override {
        if (_pipeline) {
            _pipeline->Destroy();
        }
        _cubeMesh.reset();
        _sphereMesh.reset();
        _planeMesh.reset();
        _cylinderMesh.reset();
        _coneMesh.reset();
        _torusMesh.reset();
    }

private:
    float _rotation = 0.0f;
    tvk::Scope<tvk::Mesh> _cubeMesh;
    tvk::Scope<tvk::Mesh> _sphereMesh;
    tvk::Scope<tvk::Mesh> _planeMesh;
    tvk::Scope<tvk::Mesh> _cylinderMesh;
    tvk::Scope<tvk::Mesh> _coneMesh;
    tvk::Scope<tvk::Mesh> _torusMesh;
    tvk::Scope<tvk::Pipeline> _pipeline;
};

class SandboxApp : public tvk::App {
protected:
    void OnStart() override {
        TVK_LOG_INFO("Sandbox application started!");
        TVK_LOG_INFO("Running in {} mode", 
            GetMode() == tvk::AppMode::GUI ? "GUI" :
            GetMode() == tvk::AppMode::Game ? "Game" : "Hybrid");
        
        _gameViewport = tvk::CreateScope<GameViewport>();
        RegisterWidget(_gameViewport.get());
        
        SetClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        
        _counter = 0;
        _textInput[0] = '\0';
    }

    void OnUpdate() override {
        if (tvk::Input::IsKeyPressed(tvk::Key::Escape)) {
            Quit();
        }
        
        if (tvk::Input::IsKeyPressed(tvk::Key::Space)) {
            TVK_LOG_INFO("Space key pressed!");
        }
    }

    void OnUI() override {
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::DockSpaceOverViewport(0, viewport);

        if (_showDemoWindow) {
            ImGui::ShowDemoWindow(&_showDemoWindow);
        }

        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("Open Image...", "Ctrl+O")) {
                    OpenImageFile();
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Exit", "Esc")) {
                    Quit();
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("View")) {
                ImGui::MenuItem("ImGui Demo", nullptr, &_showDemoWindow);
                ImGui::MenuItem("Stats", nullptr, &_showStats);
                ImGui::MenuItem("Image Viewer", nullptr, &_showImageViewer);
                ImGui::MenuItem("3D Viewport", nullptr, &_showGameViewport);
                ImGui::MenuItem("Controls", nullptr, &_showControls);
                ImGui::MenuItem("Scene Hierarchy", nullptr, &_showHierarchy);
                ImGui::MenuItem("Properties", nullptr, &_showProperties);
                ImGui::MenuItem("About", nullptr, &_showSettings);
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Help")) {
                if (ImGui::MenuItem("Documentation")) {
                    TVK_LOG_INFO("Opening documentation...");
                }
                ImGui::Separator();
                if (ImGui::MenuItem("About")) {
                    _showSettings = true;
                }
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }

        if (_showStats) {
            ImGui::Begin("Statistics", &_showStats);
            ImGui::Text("FPS: %.1f", FPS());
            ImGui::Text("Frame Time: %.3f ms", DeltaTime() * 1000.0f);
            ImGui::Text("Elapsed: %.1f s", ElapsedTime());
            ImGui::Separator();
            ImGui::Text("Window: %ux%u", WindowWidth(), WindowHeight());
            ImGui::Separator();
            auto mousePos = tvk::Input::GetMousePosition();
            ImGui::Text("Mouse: (%.0f, %.0f)", mousePos.x, mousePos.y);
            ImGui::Text("LMB: %s", tvk::Input::IsMouseButtonPressed(tvk::MouseButton::Left) ? "Pressed" : "Released");
            ImGui::Text("RMB: %s", tvk::Input::IsMouseButtonPressed(tvk::MouseButton::Right) ? "Pressed" : "Released");
            ImGui::End();
        }
        
        if (_showGameViewport && _gameViewport) {
            ImGui::Begin("3D Viewport", &_showGameViewport);
            _gameViewport->SetEnabled(true);
            _gameViewport->RenderImage();
            ImGui::End();
        } else if (_gameViewport) {
            _gameViewport->SetEnabled(false);
        }

        if (_showControls) {
            ImGui::Begin("GUI Controls Demo", &_showControls);
            
            ImGui::TextWrapped("This demonstrates Qt-style GUI controls for building tools and editors.");
            ImGui::Separator();
            
            if (ImGui::Button("Click me!")) {
                _counter++;
                TVK_LOG_INFO("Button clicked {} times", _counter);
            }
            ImGui::SameLine();
            ImGui::Text("Counter: %d", _counter);
            
            ImGui::InputText("Text input", _textInput, sizeof(_textInput));
            
            ImGui::SliderFloat("Slider", &_sliderValue, 0.0f, 100.0f);
            ImGui::ColorEdit3("Color", &_color.x);
            
            ImGui::Separator();
            
            if (ImGui::TreeNode("Advanced Controls")) {
                static int selectedItem = 0;
                const char* items[] = {"Item 1", "Item 2", "Item 3", "Item 4"};
                ImGui::Combo("Combo", &selectedItem, items, 4);
                
                static bool checkbox1 = true;
                static bool checkbox2 = false;
                ImGui::Checkbox("Option 1", &checkbox1);
                ImGui::Checkbox("Option 2", &checkbox2);
                
                static int radioButton = 0;
                ImGui::RadioButton("Radio A", &radioButton, 0); ImGui::SameLine();
                ImGui::RadioButton("Radio B", &radioButton, 1); ImGui::SameLine();
                ImGui::RadioButton("Radio C", &radioButton, 2);
                
                ImGui::TreePop();
            }
            
            ImGui::End();
        }

        if (_showHierarchy) {
            ImGui::Begin("Scene Hierarchy", &_showHierarchy);
            
            ImGui::TextWrapped("Example scene hierarchy for level editor or 3D tool.");
            ImGui::Separator();
            
            if (ImGui::TreeNode("Scene Root")) {
                if (ImGui::TreeNode("Camera")) {
                    ImGui::Text("Main Camera");
                    ImGui::TreePop();
                }
                if (ImGui::TreeNode("Objects")) {
                    ImGui::Selectable("Cube");
                    ImGui::Selectable("Sphere");
                    ImGui::Selectable("Plane");
                    ImGui::Selectable("Cylinder");
                    ImGui::Selectable("Cone");
                    ImGui::Selectable("Torus");
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

        if (_showProperties) {
            ImGui::Begin("Properties", &_showProperties);
            
            ImGui::Text("Selected: Cube");
            ImGui::Separator();
            
            if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
                static float pos[3] = {0, 0, 0};
                static float rot[3] = {0, 0, 0};
                static float scale[3] = {1, 1, 1};
                
                ImGui::DragFloat3("Position", pos, 0.1f);
                ImGui::DragFloat3("Rotation", rot, 1.0f);
                ImGui::DragFloat3("Scale", scale, 0.1f);
            }
            
            if (ImGui::CollapsingHeader("Material")) {
                ImGui::ColorEdit3("Diffuse", &_color.x);
                ImGui::SliderFloat("Roughness", &_sliderValue, 0.0f, 1.0f);
            }
            
            ImGui::End();
        }

        if (_showImageViewer) {
            ImGui::Begin("Image Viewer", &_showImageViewer);

            if (ImGui::Button("Open Image...")) {
                OpenImageFile();
            }

            ImGui::SameLine();
            if (_loadedTexture && ImGui::Button("Clear")) {
                _loadedTexture.reset();
                _imagePath.clear();
            }

            ImGui::Separator();

            if (_loadedTexture) {
                ImGui::Text("File: %s", _imagePath.c_str());
                ImGui::Text("Size: %ux%u", _loadedTexture->GetWidth(), _loadedTexture->GetHeight());
                
                float availWidth = ImGui::GetContentRegionAvail().x;
                float aspect = static_cast<float>(_loadedTexture->GetWidth()) / 
                               static_cast<float>(_loadedTexture->GetHeight());
                ImVec2 size(availWidth, availWidth / aspect);
                ImGui::Image(_loadedTexture->GetImGuiTextureID(), size);
            } else {
                ImGui::TextDisabled("No image loaded.");
                ImGui::TextWrapped("Click 'Open Image...' to load a texture file (PNG, JPG, BMP, TGA).");
            }

            ImGui::End();
        }

        if (_showSettings) {
            ImGui::Begin("About TinyVK", &_showSettings);

            if (ImGui::CollapsingHeader("About", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::Text("TinyVK Version: %s", tvk::GetVersionString());
                ImGui::Text("A lightweight Vulkan renderer with ImGui");
                ImGui::Separator();
                ImGui::TextWrapped(
                    "TinyVK provides a simple API for creating Vulkan applications "
                    "with ImGui integration. Perfect for tools, editors, and games."
                );
            }

            if (ImGui::CollapsingHeader("Application Modes")) {
                ImGui::BulletText("GUI Mode - Pure ImGui interface for tools and editors");
                ImGui::BulletText("Game Mode - Full-window rendering for games");
                ImGui::BulletText("Hybrid Mode - Combines GUI with embedded 3D viewports");
                ImGui::Separator();
                ImGui::Text("Current Mode: %s", 
                    GetMode() == tvk::AppMode::GUI ? "GUI" :
                    GetMode() == tvk::AppMode::Game ? "Game" : "Hybrid");
            }

            if (ImGui::CollapsingHeader("Features Demonstrated")) {
                ImGui::BulletText("Multiple geometry primitives (cube, sphere, torus, etc.)");
                ImGui::BulletText("Graphics pipeline with vertex/fragment shaders");
                ImGui::BulletText("Texture loading and display");
                ImGui::BulletText("File dialogs");
                ImGui::BulletText("Input handling (keyboard and mouse)");
                ImGui::BulletText("ImGui docking and windows");
                ImGui::BulletText("RenderWidget for embedded viewports");
            }

            ImGui::End();
        }
    }

    void OnStop() override {
        TVK_LOG_INFO("Sandbox application stopped");
        _loadedTexture.reset();
    }

private:
    void OpenImageFile() {
        auto result = tvk::FileDialog::OpenFile(
            {{"Image Files", "png,jpg,jpeg,bmp,tga"}},
            ""
        );

        if (result.has_value()) {
            _loadedTexture = LoadTexture(result.value());
            if (_loadedTexture && _loadedTexture->IsValid()) {
                _loadedTexture->BindToImGui();
                _imagePath = result.value();
                TVK_LOG_INFO("Loaded texture: {}", _imagePath);
            } else {
                TVK_LOG_ERROR("Failed to load texture: {}", result.value());
                _loadedTexture.reset();
            }
        }
    }

    bool _showDemoWindow = false;
    bool _showStats = true;
    bool _showSettings = false;
    bool _showImageViewer = true;
    bool _showGameViewport = true;
    bool _showControls = true;
    bool _showHierarchy = true;
    bool _showProperties = true;

    tvk::Ref<tvk::Texture> _loadedTexture;
    std::string _imagePath;
    
    tvk::Scope<GameViewport> _gameViewport;
    
    int _counter;
    char _textInput[256];
    float _sliderValue = 50.0f;
    glm::vec3 _color{1.0f, 0.5f, 0.2f};
};

int main() {
    try {
        SandboxApp app;
        app.Run("TinyVK Sandbox", 1280, 720);
    } catch (const std::exception& e) {
        TVK_LOG_FATAL("Exception: {}", e.what());
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
