/**
 * @file main.cpp
 * @brief TinyVK sandbox demonstrating UI library and compute capabilities
 */

#include <tinyvk/tinyvk.h>
#include <imgui.h>
#include <glm/glm.hpp>
#include <cmath>
#include <cstring>

class SandboxApp : public tvk::App {
protected:
    void OnStart() override {
        TVK_LOG_INFO("Sandbox application started!");

        SetClearColor(0.1f, 0.1f, 0.12f, 1.0f);

        _counter = 0;
        _textInput[0] = '\0';

        InitComputeDemo();
    }

    void OnMenuBar() override {
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
            ImGui::MenuItem("Compute Demo", nullptr, &_showComputeDemo);
            ImGui::MenuItem("Controls", nullptr, &_showControls);
            ImGui::MenuItem("About", nullptr, &_showAbout);
            ImGui::EndMenu();
        }
    }

    void OnUpdate() override {
        if (tvk::Input::IsKeyPressed(tvk::Key::Escape)) {
            Quit();
        }
    }

    void OnUI() override {
        if (_showDemoWindow) {
            ImGui::ShowDemoWindow(&_showDemoWindow);
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

        if (_showControls) {
            ImGui::Begin("UI Controls Demo", &_showControls);
            
            ImGui::TextWrapped("This demonstrates ImGui-based UI controls for building tools and editors.");
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

        if (_showAbout) {
            ImGui::Begin("About TinyVK", &_showAbout);

            if (ImGui::CollapsingHeader("About", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::Text("TinyVK Version: %s", tvk::GetVersionString());
                ImGui::Text("A lightweight Vulkan UI library with compute support");
                ImGui::Separator();
                ImGui::TextWrapped(
                    "TinyVK provides a simple API for creating Vulkan applications "
                    "with ImGui integration. Ideal for tools, editors, and utilities."
                );
            }

            if (ImGui::CollapsingHeader("Features")) {
                ImGui::BulletText("ImGui-based user interface");
                ImGui::BulletText("GPU compute pipeline for parallel computation");
                ImGui::BulletText("Buffer management for GPU data");
                ImGui::BulletText("Texture loading and display");
                ImGui::BulletText("File dialogs");
                ImGui::BulletText("Input handling (keyboard and mouse)");
                ImGui::BulletText("ImGui docking support");
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
    bool _showAbout = false;
    bool _showImageViewer = true;
    bool _showComputeDemo = true;
    bool _showControls = true;

    tvk::Ref<tvk::Texture> _loadedTexture;
    std::string _imagePath;
    
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
