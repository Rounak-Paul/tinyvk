/**
 * @file main.cpp
 * @brief TinyVK comprehensive example demonstrating all features and application modes
 * 
 * This sandbox demonstrates:
 * - All three AppModes: GUI, Game, and Hybrid
 * - RenderWidget for embedded viewports
 * - Scene and ECS (Entity Component System) with EnTT
 * - Camera and CameraController
 * - Material system
 * - Mesh/geometry rendering with various primitives
 * - Graphics pipeline usage
 * - Compute pipeline usage
 * - Texture loading and ImGui integration
 * - File dialogs
 * - Input handling
 * - ImGui docking and menus
 */

#include <tinyvk/tinyvk.h>
#include <imgui.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>
#include <cstring>
#include "tinyvk/renderer/pipeline.h"
#include "tinyvk/renderer/shaders.h"
#include "tinyvk/renderer/buffer.h"
#include "tinyvk/renderer/renderer.h"

class GameViewport : public tvk::RenderWidget {
protected:
    void OnRenderInit() override {
        _rotation = 0.0f;
        
        _scene = tvk::CreateRef<tvk::Scene>("Demo Scene");
        
        auto camera_entity = _scene->CreateEntity("Main Camera");
        auto& camera_comp = camera_entity.AddComponent<tvk::CameraComponent>();
        camera_comp.primary = true;
        camera_comp.camera.SetPerspective(45.0f, 16.0f/9.0f, 0.1f, 100.0f);
        camera_comp.camera.SetPosition(tvk::Vec3(0.0f, 2.0f, 5.0f));
        camera_comp.camera.LookAt(tvk::Vec3(0.0f, 0.0f, 0.0f));
        _scene->SetPrimaryCamera(camera_entity);
        _cameraController.SetCamera(&camera_comp.camera);
        _cameraController.SetYaw(-90.0f);
        _cameraController.SetPitch(-20.0f);
        
        auto cube = _scene->CreateEntity("Cube");
        cube.GetComponent<tvk::TransformComponent>().position = tvk::Vec3(-2.0f, 0.0f, 0.0f);
        cube.AddComponent<tvk::MeshComponent>(tvk::Geometry::CreateCube(GetRenderer(), 1.0f));
        
        auto sphere = _scene->CreateEntity("Sphere");
        sphere.GetComponent<tvk::TransformComponent>().position = tvk::Vec3(0.0f, 0.0f, 0.0f);
        sphere.AddComponent<tvk::MeshComponent>(tvk::Geometry::CreateSphere(GetRenderer(), 0.6f, 32, 16));
        
        auto torus = _scene->CreateEntity("Torus");
        torus.GetComponent<tvk::TransformComponent>().position = tvk::Vec3(2.0f, 0.0f, 0.0f);
        torus.AddComponent<tvk::MeshComponent>(tvk::Geometry::CreateTorus(GetRenderer(), 0.5f, 0.2f, 32, 16));
        
        auto ground = _scene->CreateEntity("Ground");
        ground.GetComponent<tvk::TransformComponent>().position = tvk::Vec3(0.0f, -1.0f, 0.0f);
        ground.AddComponent<tvk::MeshComponent>(tvk::Geometry::CreatePlane(GetRenderer(), 10.0f, 10.0f, 10, 10));
        
        TVK_LOG_INFO("Scene created with {} entities", _scene->GetEntityCount());
        
        SetClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        
        _pipeline = tvk::CreateScope<tvk::Pipeline>();
        if (!_pipeline->Create(GetRenderer(), GetRenderPass(), tvk::shaders::basic_vert, tvk::shaders::basic_frag)) {
            TVK_LOG_ERROR("Failed to create graphics pipeline");
        }
    }

    void OnRenderFrame(VkCommandBuffer cmd) override {
        BeginRenderPass(cmd);
        
        if (_pipeline && _scene && GetWidth() > 0 && GetHeight() > 0) {
            _pipeline->Bind(cmd);
            
            tvk::Camera* camera = _scene->GetActiveCameraPtr();
            if (!camera) {
                EndRenderPass(cmd);
                return;
            }
            
            tvk::Mat4 view_projection = camera->GetViewProjectionMatrix();
            
            auto view = _scene->GetAllEntitiesWith<tvk::TransformComponent, tvk::MeshComponent>();
            for (auto entity_handle : view) {
                const auto& transform = view.get<tvk::TransformComponent>(entity_handle);
                const auto& mesh_comp = view.get<tvk::MeshComponent>(entity_handle);
                
                if (!mesh_comp.visible || !mesh_comp.mesh) continue;
                
                tvk::Mat4 model = transform.GetMatrix();
                model = glm::rotate(model, glm::radians(_rotation), tvk::Vec3(0.0f, 1.0f, 0.0f));
                
                tvk::PushConstants push;
                push.model = model;
                push.view_projection = view_projection;
                _pipeline->SetPushConstants(cmd, push);
                
                mesh_comp.mesh->Draw(cmd);
            }
        }
        
        EndRenderPass(cmd);
    }

    void OnRenderUpdate(float deltaTime) override {
        _rotation += deltaTime * 20.0f;
        if (_rotation > 360.0f) _rotation -= 360.0f;
        
        if (_scene) {
            _scene->OnViewportResize(GetWidth(), GetHeight());
            
            if (_controlCamera && ImGui::IsWindowFocused()) {
                _cameraController.Update(deltaTime);
            }
        }
    }

    void OnRenderResize(tvk::u32 width, tvk::u32 height) override {
        if (_scene) {
            _scene->OnViewportResize(width, height);
        }
    }

    void OnRenderCleanup() override {
        if (_pipeline) {
            _pipeline->Destroy();
        }
        _scene.reset();
    }

public:
    tvk::Ref<tvk::Scene> GetScene() const { return _scene; }
    void SetCameraControl(bool enabled) { _controlCamera = enabled; }
    bool IsCameraControlEnabled() const { return _controlCamera; }
    tvk::Entity GetSelectedEntity() const { return _selectedEntity; }
    void SetSelectedEntity(tvk::Entity entity) { _selectedEntity = entity; }

private:
    float _rotation = 0.0f;
    tvk::Ref<tvk::Scene> _scene;
    tvk::Scope<tvk::Pipeline> _pipeline;
    tvk::CameraController _cameraController;
    tvk::Entity _selectedEntity;
    bool _controlCamera = false;
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
        
        InitComputeDemo();
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
                ImGui::MenuItem("Compute Demo", nullptr, &_showComputeDemo);
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
            
            if (_gameViewport && _gameViewport->GetScene()) {
                auto scene = _gameViewport->GetScene();
                ImGui::Text("Scene: %s", scene->GetName().c_str());
                ImGui::Text("Entities: %u", scene->GetEntityCount());
                ImGui::Separator();
                
                if (ImGui::Button("+ Add Entity")) {
                    auto entity = scene->CreateEntity("New Entity");
                    _gameViewport->SetSelectedEntity(entity);
                }
                ImGui::SameLine();
                if (ImGui::Button("+ Add Cube")) {
                    auto entity = scene->CreateEntity("Cube");
                    entity.AddComponent<tvk::MeshComponent>(tvk::Geometry::CreateCube(GetRenderer(), 1.0f));
                    _gameViewport->SetSelectedEntity(entity);
                }
                
                ImGui::Separator();
                
                scene->ForEachEntity([this](tvk::Entity entity) {
                    auto& tag = entity.GetComponent<tvk::TagComponent>();
                    
                    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
                    if (_gameViewport->GetSelectedEntity() == entity) {
                        flags |= ImGuiTreeNodeFlags_Selected;
                    }
                    
                    ImGui::TreeNodeEx((void*)(intptr_t)(tvk::u32)entity, flags, "%s", tag.tag.c_str());
                    
                    if (ImGui::IsItemClicked()) {
                        _gameViewport->SetSelectedEntity(entity);
                    }
                });
            } else {
                ImGui::TextDisabled("No scene loaded");
            }
            
            ImGui::End();
        }

        if (_showProperties) {
            ImGui::Begin("Properties", &_showProperties);
            
            tvk::Entity selected = _gameViewport ? _gameViewport->GetSelectedEntity() : tvk::Entity();
            
            if (selected.IsValid()) {
                auto& tag = selected.GetComponent<tvk::TagComponent>();
                char buffer[256];
                strncpy(buffer, tag.tag.c_str(), sizeof(buffer));
                buffer[sizeof(buffer) - 1] = '\0';
                if (ImGui::InputText("Name", buffer, sizeof(buffer))) {
                    tag.tag = buffer;
                }
                
                ImGui::Separator();
                
                if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
                    auto& transform = selected.GetComponent<tvk::TransformComponent>();
                    ImGui::DragFloat3("Position", &transform.position.x, 0.1f);
                    ImGui::DragFloat3("Rotation", &transform.rotation.x, 1.0f);
                    ImGui::DragFloat3("Scale", &transform.scale.x, 0.1f);
                }
                
                if (selected.HasComponent<tvk::MeshComponent>()) {
                    if (ImGui::CollapsingHeader("Mesh", ImGuiTreeNodeFlags_DefaultOpen)) {
                        auto& mesh = selected.GetComponent<tvk::MeshComponent>();
                        ImGui::Checkbox("Visible", &mesh.visible);
                        ImGui::Checkbox("Cast Shadows", &mesh.cast_shadows);
                        if (mesh.mesh) {
                            ImGui::Text("Vertices: %u", mesh.mesh->GetVertexCount());
                            ImGui::Text("Indices: %u", mesh.mesh->GetIndexCount());
                        }
                    }
                }
                
                if (selected.HasComponent<tvk::CameraComponent>()) {
                    if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen)) {
                        auto& cam = selected.GetComponent<tvk::CameraComponent>();
                        ImGui::Checkbox("Primary", &cam.primary);
                        float fov = cam.camera.GetFov();
                        if (ImGui::SliderFloat("FOV", &fov, 10.0f, 120.0f)) {
                            cam.camera.SetFov(fov);
                        }
                        float near_plane = cam.camera.GetNearPlane();
                        float far_plane = cam.camera.GetFarPlane();
                        if (ImGui::DragFloat("Near", &near_plane, 0.01f, 0.001f, 10.0f)) {
                            cam.camera.SetNearPlane(near_plane);
                        }
                        if (ImGui::DragFloat("Far", &far_plane, 1.0f, 10.0f, 10000.0f)) {
                            cam.camera.SetFarPlane(far_plane);
                        }
                    }
                }
                
                ImGui::Separator();
                if (ImGui::Button("Delete Entity")) {
                    _gameViewport->GetScene()->DestroyEntity(selected);
                    _gameViewport->SetSelectedEntity(tvk::Entity());
                }
            } else {
                ImGui::TextDisabled("No entity selected");
                ImGui::TextWrapped("Select an entity from the Scene Hierarchy to edit its properties.");
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
                ImGui::BulletText("Compute pipeline for GPU computation");
                ImGui::BulletText("Texture loading and display");
                ImGui::BulletText("File dialogs");
                ImGui::BulletText("Input handling (keyboard and mouse)");
                ImGui::BulletText("ImGui docking and windows");
                ImGui::BulletText("RenderWidget for embedded viewports");
            }

            ImGui::End();
        }

        if (_showComputeDemo) {
            ImGui::Begin("Compute Pipeline Demo", &_showComputeDemo);
            
            ImGui::TextWrapped("GPU compute shader multiplying array values. Input array is multiplied by a factor on the GPU.");
            ImGui::Separator();
            
            ImGui::SliderFloat("Multiplier", &_computeMultiplier, 0.1f, 10.0f);
            
            if (ImGui::Button("Run Compute Shader")) {
                RunComputeShader();
            }
            
            ImGui::SameLine();
            ImGui::Text("Executions: %d", _computeExecutions);
            
            ImGui::Separator();
            
            if (ImGui::CollapsingHeader("Input Data", ImGuiTreeNodeFlags_DefaultOpen)) {
                for (int i = 0; i < COMPUTE_DATA_SIZE; i++) {
                    ImGui::Text("[%d] = %.2f", i, _computeInputData[i]);
                }
            }
            
            if (ImGui::CollapsingHeader("Output Data (GPU Result)", ImGuiTreeNodeFlags_DefaultOpen)) {
                for (int i = 0; i < COMPUTE_DATA_SIZE; i++) {
                    float expected = _computeInputData[i] * _computeMultiplier;
                    bool correct = std::abs(_computeOutputData[i] - expected) < 0.01f;
                    if (correct) {
                        ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "[%d] = %.2f", i, _computeOutputData[i]);
                    } else {
                        ImGui::Text("[%d] = %.2f", i, _computeOutputData[i]);
                    }
                }
            }
            
            if (ImGui::CollapsingHeader("Compute Shader Code")) {
                ImGui::TextWrapped("%s", tvk::shaders::array_multiply_comp);
            }
            
            ImGui::End();
        }
    }

    void OnStop() override {
        TVK_LOG_INFO("Sandbox application stopped");
        _loadedTexture.reset();
        CleanupComputeDemo();
    }

private:
    static constexpr int COMPUTE_DATA_SIZE = 8;
    
    struct ComputePushData {
        tvk::u32 count;
        float multiplier;
    };
    
    void InitComputeDemo() {
        _computePipeline = tvk::CreateScope<tvk::ComputePipeline>();
        if (!_computePipeline->Create(GetRenderer(), tvk::shaders::array_multiply_comp)) {
            TVK_LOG_ERROR("Failed to create compute pipeline");
            return;
        }
        
        for (int i = 0; i < COMPUTE_DATA_SIZE; i++) {
            _computeInputData[i] = static_cast<float>(i + 1);
            _computeOutputData[i] = 0.0f;
        }
        
        _computeInputBuffer = tvk::Buffer::Create(
            GetRenderer(), 
            sizeof(float) * COMPUTE_DATA_SIZE, 
            tvk::BufferUsage::StorageShared, 
            _computeInputData
        );
        
        _computeOutputBuffer = tvk::Buffer::Create(
            GetRenderer(), 
            sizeof(float) * COMPUTE_DATA_SIZE, 
            tvk::BufferUsage::StorageShared, 
            nullptr
        );
        
        if (_computeInputBuffer && _computeOutputBuffer) {
            _computePipeline->BindStorageBuffers(_computeInputBuffer.get(), _computeOutputBuffer.get());
            _computePipeline->UpdateDescriptors();
            TVK_LOG_INFO("Compute demo initialized with {} elements", COMPUTE_DATA_SIZE);
        }
    }
    
    void CleanupComputeDemo() {
        if (_computePipeline) {
            _computePipeline->Destroy();
        }
        _computeInputBuffer.reset();
        _computeOutputBuffer.reset();
    }
    
    void RunComputeShader() {
        if (!_computePipeline || !_computeInputBuffer || !_computeOutputBuffer) return;
        
        auto& ctx = GetRenderer()->GetContext();
        VkDevice device = ctx.GetDevice();
        VkQueue queue = ctx.GetGraphicsQueue();
        VkCommandPool cmdPool = ctx.GetCommandPool();
        
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = cmdPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = 1;
        
        VkCommandBuffer cmd;
        vkAllocateCommandBuffers(device, &allocInfo, &cmd);
        
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(cmd, &beginInfo);
        
        _computePipeline->Bind(cmd);
        
        ComputePushData pushData;
        pushData.count = COMPUTE_DATA_SIZE;
        pushData.multiplier = _computeMultiplier;
        _computePipeline->SetPushConstants(cmd, pushData);
        
        _computePipeline->Dispatch(cmd, (COMPUTE_DATA_SIZE + 255) / 256, 1, 1);
        
        VkMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
        barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_HOST_READ_BIT;
        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_HOST_BIT,
                             0, 1, &barrier, 0, nullptr, 0, nullptr);
        
        vkEndCommandBuffer(cmd);
        
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &cmd;
        
        vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(queue);
        
        void* mappedData = _computeOutputBuffer->Map();
        if (mappedData) {
            memcpy(_computeOutputData, mappedData, sizeof(float) * COMPUTE_DATA_SIZE);
            _computeOutputBuffer->Unmap();
        }
        
        vkFreeCommandBuffers(device, cmdPool, 1, &cmd);
        
        _computeExecutions++;
        TVK_LOG_INFO("Compute shader executed (multiplier: {})", _computeMultiplier);
    }

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
    bool _showComputeDemo = true;
    bool _showControls = true;
    bool _showHierarchy = true;
    bool _showProperties = true;

    tvk::Ref<tvk::Texture> _loadedTexture;
    std::string _imagePath;
    
    tvk::Scope<GameViewport> _gameViewport;
    
    tvk::Scope<tvk::ComputePipeline> _computePipeline;
    tvk::Ref<tvk::Buffer> _computeInputBuffer;
    tvk::Ref<tvk::Buffer> _computeOutputBuffer;
    float _computeInputData[COMPUTE_DATA_SIZE] = {};
    float _computeOutputData[COMPUTE_DATA_SIZE] = {};
    float _computeMultiplier = 2.0f;
    int _computeExecutions = 0;
    
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
