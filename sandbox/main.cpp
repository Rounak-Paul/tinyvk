/**
 * @file main.cpp
 * @brief Sandbox application demonstrating TinyVK usage
 */

#include <tinyvk/tinyvk.h>
#include <imgui.h>

class SandboxApp : public tvk::App {
protected:
    void OnStart() override {
        TVK_LOG_INFO("Sandbox application started!");
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
                ImGui::MenuItem("Settings", nullptr, &_showSettings);
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
            }

            ImGui::End();
        }

        if (_showSettings) {
            ImGui::Begin("Settings", &_showSettings);

            if (ImGui::CollapsingHeader("About", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::Text("TinyVK Version: %s", tvk::GetVersionString());
                ImGui::Text("A lightweight Vulkan renderer with ImGui");
                ImGui::Separator();
                ImGui::TextWrapped(
                    "TinyVK provides a simple API for creating Vulkan applications "
                    "with ImGui integration. Perfect for tools, editors, and prototyping."
                );
            }

            ImGui::End();
        }

        SetupDockspace();
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

    void SetupDockspace() {
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGui::SetNextWindowViewport(viewport->ID);

        ImGuiWindowFlags windowFlags =
            ImGuiWindowFlags_NoDocking |
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoBringToFrontOnFocus |
            ImGuiWindowFlags_NoNavFocus |
            ImGuiWindowFlags_NoBackground;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

        ImGui::Begin("DockSpace", nullptr, windowFlags);
        ImGui::PopStyleVar(3);

        ImGuiID dockspaceId = ImGui::GetID("MainDockSpace");
        ImGui::DockSpace(dockspaceId, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);

        ImGui::End();
    }

    bool _showDemoWindow = false;
    bool _showStats = true;
    bool _showSettings = true;
    bool _showImageViewer = true;

    tvk::Ref<tvk::Texture> _loadedTexture;
    std::string _imagePath;
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
