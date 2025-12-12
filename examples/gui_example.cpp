/**
 * @file gui_example.cpp
 * @brief Qt-style GUI example - pure ImGui interface without game rendering
 * Demonstrates AppMode::GUI for tools and editors
 */

#include <tinyvk/tinyvk.h>
#include <imgui.h>
#include <string>

class GuiExample : public tvk::App {
protected:
    void OnStart() override {
        TVK_LOG_INFO("GUI mode example started");
        _counter = 0;
        _textInput[0] = '\0';
    }
    
    void OnUpdate() override {
        if (tvk::Input::IsKeyPressed(tvk::Key::Escape)) {
            Quit();
        }
    }
    
    void OnUI() override {
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::DockSpaceOverViewport(0, viewport);
        
        ImGui::Begin("Tool Window");
        
        ImGui::Text("This is a Qt-style GUI application");
        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 
                   DeltaTime() * 1000.0f, FPS());
        ImGui::Separator();
        
        if (ImGui::Button("Click me!")) {
            _counter++;
        }
        ImGui::SameLine();
        ImGui::Text("Counter: %d", _counter);
        
        ImGui::InputText("Text input", _textInput, sizeof(_textInput));
        
        ImGui::SliderFloat("Slider", &_sliderValue, 0.0f, 100.0f);
        ImGui::ColorEdit3("Color", &_color.x);
        
        ImGui::End();
        
        ImGui::Begin("Properties");
        ImGui::Text("Window size: %ux%u", WindowWidth(), WindowHeight());
        ImGui::Text("Elapsed time: %.2f seconds", ElapsedTime());
        
        auto mousePos = tvk::Input::GetMousePosition();
        ImGui::Text("Mouse position: (%.0f, %.0f)", mousePos.x, mousePos.y);
        
        ImGui::End();
        
        ImGui::Begin("Log");
        ImGui::TextWrapped("This is a pure GUI application using AppMode::GUI. "
                          "Perfect for tools, editors, and desktop applications.");
        ImGui::End();
    }
    
    void OnStop() override {
        TVK_LOG_INFO("GUI mode example stopped");
    }

private:
    int _counter;
    char _textInput[256];
    float _sliderValue = 50.0f;
    glm::vec3 _color{1.0f, 0.5f, 0.2f};
};

int main() {
    try {
        GuiExample app;
        
        tvk::AppConfig config;
        config.title = "TinyVK - GUI Mode Example";
        config.width = 1280;
        config.height = 720;
        config.mode = tvk::AppMode::GUI;  // Pure GUI mode
        config.vsync = true;
        
        app.Run(config);
    } catch (const std::exception& e) {
        TVK_LOG_FATAL("Exception: {}", e.what());
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
