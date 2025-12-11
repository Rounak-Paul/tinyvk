/**
 * @file main.cpp
 * @brief Sandbox application demonstrating TinyVK usage
 */

#include <tinyvk/tinyvk.h>
#include <imgui.h>
#include <memory>

class SandboxApp : public tvk::Application {
public:
    SandboxApp(const tvk::ApplicationConfig& config) : Application(config) {}

protected:
    void OnInit() override {
        TVK_LOG_INFO("Sandbox application initialized!");
        
        // Set a nice clear color
        GetRenderer().SetClearColor({0.1f, 0.1f, 0.12f, 1.0f});
    }

    void OnUpdate(float dt) override {
        TVK_PROFILE_FUNCTION();
        
        // Handle input
        if (tvk::Input::IsKeyPressed(tvk::Key::Escape)) {
            Quit();
        }

        // Rotate color over time
        m_Time += dt;
        
        // Update frame timer
        m_FrameTimer.beginFrame();
    }

    void OnRender() override {
        TVK_PROFILE_SCOPE("Render");
        // Custom rendering would go here
        // For now, we just clear the screen and let ImGui render
        
        m_FrameTimer.endFrame();
    }

    void OnImGui() override {
        TVK_PROFILE_SCOPE("ImGui");
        
        // Demo window
        if (m_ShowDemoWindow) {
            ImGui::ShowDemoWindow(&m_ShowDemoWindow);
        }

        // Main menu bar
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
                ImGui::MenuItem("ImGui Demo", nullptr, &m_ShowDemoWindow);
                ImGui::MenuItem("Stats", nullptr, &m_ShowStats);
                ImGui::MenuItem("Profiler", nullptr, &m_ShowProfiler);
                ImGui::MenuItem("Image Viewer", nullptr, &m_ShowImageViewer);
                ImGui::MenuItem("Settings", nullptr, &m_ShowSettings);
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }

        // Stats window
        if (m_ShowStats) {
            ImGui::Begin("Statistics", &m_ShowStats);
            ImGui::Text("FPS: %.1f", m_FrameTimer.getFPS());
            ImGui::Text("Frame Time: %.3f ms", m_FrameTimer.getDeltaTimeMs());
            ImGui::Text("Avg Frame Time: %.3f ms", m_FrameTimer.getAverageFrameTimeMs());
            
            // Frame time graph
            const auto& history = m_FrameTimer.getFrameTimeHistory();
            if (!history.empty()) {
                ImGui::PlotLines("Frame Times", history.data(), 
                    static_cast<int>(history.size()), 0, nullptr, 0.0f, 33.3f, ImVec2(0, 60));
            }
            
            ImGui::Separator();
            
            auto& renderer = GetRenderer();
            ImGui::Text("Swapchain Images: %u", renderer.GetSwapchainImageCount());
            
            auto extent = renderer.GetSwapchainExtent();
            ImGui::Text("Resolution: %ux%u", extent.width, extent.height);
            
            ImGui::Separator();
            auto mousePos = tvk::Input::GetMousePosition();
            ImGui::Text("Mouse: (%.0f, %.0f)", mousePos.x, mousePos.y);
            ImGui::End();
        }
        
        // Profiler window
        if (m_ShowProfiler) {
            ImGui::Begin("Profiler", &m_ShowProfiler);
            
            auto stats = tvk::Profiler::instance().getAllStats();
            if (stats.empty()) {
                ImGui::TextDisabled("No profile data yet...");
            } else {
                ImGui::Columns(5, "ProfileColumns");
                ImGui::Text("Name"); ImGui::NextColumn();
                ImGui::Text("Last (ms)"); ImGui::NextColumn();
                ImGui::Text("Avg (ms)"); ImGui::NextColumn();
                ImGui::Text("Min (ms)"); ImGui::NextColumn();
                ImGui::Text("Max (ms)"); ImGui::NextColumn();
                ImGui::Separator();
                
                for (const auto& stat : stats) {
                    ImGui::Text("%s", stat.name.c_str()); ImGui::NextColumn();
                    ImGui::Text("%.3f", stat.lastTime); ImGui::NextColumn();
                    ImGui::Text("%.3f", stat.avgTime); ImGui::NextColumn();
                    ImGui::Text("%.3f", stat.minTime); ImGui::NextColumn();
                    ImGui::Text("%.3f", stat.maxTime); ImGui::NextColumn();
                }
                ImGui::Columns(1);
            }
            
            if (ImGui::Button("Reset Stats")) {
                tvk::Profiler::instance().reset();
            }
            
            ImGui::End();
        }
        
        // Image viewer window
        if (m_ShowImageViewer) {
            ImGui::Begin("Image Viewer", &m_ShowImageViewer);
            
            if (ImGui::Button("Open Image...")) {
                OpenImageFile();
            }
            
            ImGui::SameLine();
            if (m_LoadedTexture && ImGui::Button("Clear")) {
                m_LoadedTexture.reset();
                m_ImagePath.clear();
            }
            
            ImGui::Separator();
            
            if (m_LoadedTexture) {
                ImGui::Text("File: %s", m_ImagePath.c_str());
                ImGui::Text("Size: %ux%u", m_LoadedTexture->GetWidth(), m_LoadedTexture->GetHeight());
                ImGui::Text("Channels: %u", m_LoadedTexture->GetChannels());
                ImGui::Text("Mip Levels: %u", m_LoadedTexture->GetMipLevels());
                
                ImGui::Separator();
                
                // Display the image
                tvk::ui::ImageFitWidth(m_LoadedTexture);
            } else {
                ImGui::TextDisabled("No image loaded. Click 'Open Image...' to load one.");
            }
            
            ImGui::End();
        }

        // Settings window
        if (m_ShowSettings) {
            ImGui::Begin("Settings", &m_ShowSettings);
            
            if (ImGui::CollapsingHeader("Clear Color", ImGuiTreeNodeFlags_DefaultOpen)) {
                if (ImGui::ColorEdit4("Color", &m_ClearColor.r)) {
                    GetRenderer().SetClearColor(m_ClearColor);
                }
            }
            
            if (ImGui::CollapsingHeader("About", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::Text("TinyVK Version: %s", tvk::GetVersionString());
                ImGui::Text("A lightweight Vulkan renderer");
                ImGui::Separator();
                ImGui::TextWrapped(
                    "TinyVK provides a simple API for creating Vulkan applications "
                    "with ImGui integration. Perfect for tools, editors, and prototyping."
                );
            }
            
            ImGui::End();
        }

        // Dockspace (optional, for docking windows)
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

    void OnShutdown() override {
        TVK_LOG_INFO("Sandbox application shutting down");
        m_LoadedTexture.reset();
    }

    void OnResize(tvk::u32 width, tvk::u32 height) override {
        TVK_LOG_INFO("Window resized to {}x{}", width, height);
    }

private:
    void OpenImageFile() {
        auto result = tvk::FileDialog::OpenFile(
            {{"Image Files", "png,jpg,jpeg,bmp,tga"}},
            ""
        );
        
        if (result.has_value()) {
            LoadTexture(result.value());
        }
    }
    
    void LoadTexture(const std::string& path) {
        TVK_PROFILE_SCOPE("LoadTexture");
        
        m_LoadedTexture = tvk::Texture::LoadFromFile(&GetRenderer(), path);
        
        if (m_LoadedTexture && m_LoadedTexture->IsValid()) {
            m_LoadedTexture->BindToImGui();
            m_ImagePath = path;
            TVK_LOG_INFO("Loaded texture: {}", path);
        } else {
            TVK_LOG_ERROR("Failed to load texture: {}", path);
            m_LoadedTexture.reset();
        }
    }

    float m_Time = 0.0f;
    bool m_ShowDemoWindow = false;
    bool m_ShowStats = true;
    bool m_ShowSettings = true;
    bool m_ShowProfiler = false;
    bool m_ShowImageViewer = true;
    tvk::Color m_ClearColor = {0.1f, 0.1f, 0.12f, 1.0f};
    
    tvk::FrameTimer m_FrameTimer;
    tvk::Ref<tvk::Texture> m_LoadedTexture;
    std::string m_ImagePath;
};

int main() {
    tvk::ApplicationConfig config;
    config.name = "TinyVK Sandbox";
    config.windowWidth = 1280;
    config.windowHeight = 720;
    config.enableValidation = true;
    config.vsync = true;

    try {
        SandboxApp app(config);
        app.Run();
    } catch (const std::exception& e) {
        TVK_LOG_FATAL("Exception: {}", e.what());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
