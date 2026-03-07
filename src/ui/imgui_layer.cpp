/**
 * @file imgui_layer.cpp
 * @brief ImGui layer implementation for Vulkan
 */

#include "tinyvk/ui/imgui_layer.h"
#include "tinyvk/renderer/renderer.h"
#include "tinyvk/renderer/context.h"
#include "tinyvk/core/log.h"
#include "tinyvk/core/window.h"
#include "tinyvk/assets/fonts.h"
#include "tinyvk/assets/icons_font_awesome.h"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

#include <GLFW/glfw3.h>
#include <cstring>

namespace tvk {

static std::vector<FontInfo> s_AvailableFonts = {
    {"jetbrains_mono_nerd", "JetBrains Mono", jetbrains_mono_nerd, 0},
    {"firacode_nerd",       "Fira Code",      firacode_nerd,       0},
    {"hack_nerd",           "Hack",           hack_nerd,           0},
    {"sourcecodepro_nerd",  "Source Code Pro", sourcecodepro_nerd, 0},
    {"ubuntu_mono_nerd",    "Ubuntu Mono",    ubuntu_mono_nerd,    0},
    {"departure",           "Departure Mono", departure_mono,      0},
};

static void InitFontSizes() {
    static bool initialized = false;
    if (initialized) return;
    initialized = true;
    s_AvailableFonts[0].dataSize = static_cast<int>(jetbrains_mono_nerd_size);
    s_AvailableFonts[1].dataSize = static_cast<int>(firacode_nerd_size);
    s_AvailableFonts[2].dataSize = static_cast<int>(hack_nerd_size);
    s_AvailableFonts[3].dataSize = static_cast<int>(sourcecodepro_nerd_size);
    s_AvailableFonts[4].dataSize = static_cast<int>(ubuntu_mono_nerd_size);
    s_AvailableFonts[5].dataSize = static_cast<int>(departure_mono_size);
}

const std::vector<FontInfo>& ImGuiLayer::GetAvailableFonts() {
    InitFontSizes();
    return s_AvailableFonts;
}

static const FontInfo* FindFont(const char* fontId) {
    InitFontSizes();
    for (auto& f : s_AvailableFonts) {
        if (std::strcmp(f.id, fontId) == 0) return &f;
    }
    return nullptr;
}

ImGuiLayer::~ImGuiLayer() {
    Cleanup();
}

bool ImGuiLayer::Init(Window* window, Renderer* renderer, const ImGuiConfig& config, std::function<void()> menuBarCb) {
    m_tvkWindow = window;
    m_Window = window->GetNativeHandle();
    m_Renderer = renderer;
    m_Config = config;
    m_TitleBarMenuCb = std::move(menuBarCb);

    auto& context = renderer->GetContext();

    // Create descriptor pool for ImGui
    VkDescriptorPoolSize poolSizes[] = {
        { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
    };

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    poolInfo.maxSets = 1000;
    poolInfo.poolSizeCount = static_cast<u32>(std::size(poolSizes));
    poolInfo.pPoolSizes = poolSizes;

    if (vkCreateDescriptorPool(context.GetDevice(), &poolInfo, nullptr, &m_DescriptorPool) != VK_SUCCESS) {
        TVK_LOG_ERROR("Failed to create ImGui descriptor pool");
        return false;
    }

    // Initialize ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    if (config.enableKeyboardNav) {
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    }
    
    if (config.enableDocking) {
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    }
    
    if (config.enableViewports) {
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    }

    // Load font
    BuildFontAtlas(config.embeddedFontName, config.fontSize, config.fontScale);

    // Setup platform/renderer backends
    ImGui_ImplGlfw_InitForVulkan(m_Window, true);

    ImGui_ImplVulkan_InitInfo initInfo{};
    initInfo.Instance = context.GetInstance();
    initInfo.PhysicalDevice = context.GetPhysicalDevice();
    initInfo.Device = context.GetDevice();
    initInfo.QueueFamily = context.GetQueueFamilyIndices().graphicsFamily.value();
    initInfo.Queue = context.GetGraphicsQueue();
    initInfo.PipelineCache = VK_NULL_HANDLE;
    initInfo.DescriptorPool = m_DescriptorPool;
    initInfo.MinImageCount = 2;
    initInfo.ImageCount = renderer->GetSwapchainImageCount();
    initInfo.Allocator = nullptr;
    initInfo.PipelineInfoMain.RenderPass = renderer->GetRenderPass();
    initInfo.PipelineInfoMain.Subpass = 0;
    initInfo.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

    if (!ImGui_ImplVulkan_Init(&initInfo)) {
        TVK_LOG_ERROR("Failed to initialize ImGui Vulkan backend");
        return false;
    }

    // Setup style
    SetupStyle();

    m_Initialized = true;
    TVK_LOG_INFO("ImGui layer initialized");
    return true;
}

void ImGuiLayer::Cleanup() {
    if (!m_Initialized) return;

    auto& context = m_Renderer->GetContext();
    context.WaitIdle();

    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    if (m_DescriptorPool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(context.GetDevice(), m_DescriptorPool, nullptr);
        m_DescriptorPool = VK_NULL_HANDLE;
    }

    m_Initialized = false;
    TVK_LOG_INFO("ImGui layer cleaned up");
}

void ImGuiLayer::Begin() {
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void ImGuiLayer::End(VkCommandBuffer commandBuffer) {
    ImGui::Render();
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);

    // Update and render additional platform windows
    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
    }
}

void ImGuiLayer::BuildFontAtlas(const char* fontId, float fontSize, float fontScale) {
    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->Clear();

    const FontInfo* font = FindFont(fontId);
    if (font) {
        ImFontConfig fontConfig;
        fontConfig.FontDataOwnedByAtlas = false;
        fontConfig.PixelSnapH = true;
        io.Fonts->AddFontFromMemoryTTF(font->data, font->dataSize, fontSize * fontScale, &fontConfig);
        TVK_LOG_INFO("Loaded font: {}", font->displayName);
    } else {
        // Fallback to JetBrains Mono
        ImFontConfig fontConfig;
        fontConfig.FontDataOwnedByAtlas = false;
        fontConfig.PixelSnapH = true;
        io.Fonts->AddFontFromMemoryTTF(jetbrains_mono_nerd, static_cast<int>(jetbrains_mono_nerd_size), fontSize * fontScale, &fontConfig);
        TVK_LOG_INFO("Font '{}' not found, using JetBrains Mono", fontId);
    }

    // Merge Font Awesome icons
    static const ImWchar icons_ranges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
    ImFontConfig icons_config;
    icons_config.MergeMode = true;
    icons_config.PixelSnapH = true;
    icons_config.FontDataOwnedByAtlas = false;
    icons_config.GlyphMinAdvanceX = fontSize * fontScale;
    io.Fonts->AddFontFromMemoryTTF(fa_solid_900, fa_solid_900_size, fontSize * fontScale, &icons_config, icons_ranges);
}

void ImGuiLayer::ReloadFont(const char* fontId, float fontSize, float fontScale) {
    if (!m_Initialized) return;

    auto& context = m_Renderer->GetContext();
    context.WaitIdle();

    BuildFontAtlas(fontId, fontSize, fontScale);
    ImGui::GetIO().Fonts->Build();

    TVK_LOG_INFO("Font reloaded: {}", fontId);
}

void ImGuiLayer::SetDarkTheme() {
    ImGui::StyleColorsDark();
    
    auto& style = ImGui::GetStyle();
    auto& colors = style.Colors;

    ImVec4 accent = ImVec4(0.26f, 0.59f, 0.98f, 1.0f);
    ImVec4 accentHover = ImVec4(0.36f, 0.69f, 1.0f, 1.0f);
    ImVec4 accentActive = ImVec4(0.16f, 0.49f, 0.88f, 1.0f);

    colors[ImGuiCol_Text] = ImVec4(0.92f, 0.92f, 0.92f, 1.0f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.0f);
    colors[ImGuiCol_WindowBg] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.05f, 0.05f, 0.05f, 0.98f);
    colors[ImGuiCol_Border] = ImVec4(0.15f, 0.15f, 0.15f, 1.0f);
    colors[ImGuiCol_BorderShadow] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.08f, 0.08f, 0.08f, 1.0f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.12f, 0.12f, 0.12f, 1.0f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.16f, 0.16f, 0.16f, 1.0f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.06f, 0.06f, 0.06f, 1.0f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.0f, 0.0f, 0.0f, 0.75f);
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 1.0f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.20f, 0.20f, 0.20f, 1.0f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.28f, 0.28f, 0.28f, 1.0f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.36f, 0.36f, 0.36f, 1.0f);
    colors[ImGuiCol_CheckMark] = accent;
    colors[ImGuiCol_SliderGrab] = ImVec4(0.30f, 0.30f, 0.30f, 1.0f);
    colors[ImGuiCol_SliderGrabActive] = accent;
    colors[ImGuiCol_Button] = ImVec4(0.12f, 0.12f, 0.12f, 1.0f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.18f, 0.18f, 0.18f, 1.0f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.24f, 0.24f, 0.24f, 1.0f);
    colors[ImGuiCol_Header] = ImVec4(0.10f, 0.10f, 0.10f, 1.0f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.16f, 0.16f, 0.16f, 1.0f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.20f, 0.20f, 0.20f, 1.0f);
    colors[ImGuiCol_Separator] = ImVec4(0.15f, 0.15f, 0.15f, 1.0f);
    colors[ImGuiCol_SeparatorHovered] = accent;
    colors[ImGuiCol_SeparatorActive] = accentActive;
    colors[ImGuiCol_ResizeGrip] = ImVec4(0.15f, 0.15f, 0.15f, 1.0f);
    colors[ImGuiCol_ResizeGripHovered] = accent;
    colors[ImGuiCol_ResizeGripActive] = accentActive;
    colors[ImGuiCol_Tab] = ImVec4(0.06f, 0.06f, 0.06f, 1.0f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.18f, 0.18f, 0.18f, 1.0f);
    colors[ImGuiCol_TabActive] = ImVec4(0.12f, 0.12f, 0.12f, 1.0f);
    colors[ImGuiCol_TabUnfocused] = ImVec4(0.04f, 0.04f, 0.04f, 1.0f);
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.08f, 0.08f, 0.08f, 1.0f);
    colors[ImGuiCol_DockingPreview] = ImVec4(accent.x, accent.y, accent.z, 0.5f);
    colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
    colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.0f);
    colors[ImGuiCol_PlotLinesHovered] = accentHover;
    colors[ImGuiCol_PlotHistogram] = accent;
    colors[ImGuiCol_PlotHistogramHovered] = accentHover;
    colors[ImGuiCol_TableHeaderBg] = ImVec4(0.08f, 0.08f, 0.08f, 1.0f);
    colors[ImGuiCol_TableBorderStrong] = ImVec4(0.15f, 0.15f, 0.15f, 1.0f);
    colors[ImGuiCol_TableBorderLight] = ImVec4(0.10f, 0.10f, 0.10f, 1.0f);
    colors[ImGuiCol_TableRowBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    colors[ImGuiCol_TableRowBgAlt] = ImVec4(0.05f, 0.05f, 0.05f, 1.0f);
    colors[ImGuiCol_TextSelectedBg] = ImVec4(accent.x, accent.y, accent.z, 0.35f);
    colors[ImGuiCol_DragDropTarget] = accent;
    colors[ImGuiCol_NavHighlight] = accent;
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.0f, 1.0f, 1.0f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.50f);
    colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.60f);
}

void ImGuiLayer::SetLightTheme() {
    ImGui::StyleColorsLight();
}

void ImGuiLayer::BeginDockspace(float bottomOffset, int flags) {
    ImGuiViewport* viewport = ImGui::GetMainViewport();

    ImGuiWindowFlags windowFlags =
        ImGuiWindowFlags_MenuBar |
        ImGuiWindowFlags_NoDocking |
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoNavFocus |
        ImGuiWindowFlags_NoBackground;

    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, viewport->Size.y - bottomOffset));
    ImGui::SetNextWindowViewport(viewport->ID);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_BorderShadow, ImVec4(0, 0, 0, 0));

    ImGui::Begin("##DockSpaceWindow", nullptr, windowFlags);

    ImGui::PopStyleVar(5);
    ImGui::PopStyleColor(2);

    render_title_bar();

    ImGuiDockNodeFlags dockFlags = flags != 0 ? static_cast<ImGuiDockNodeFlags>(flags) : ImGuiDockNodeFlags_PassthruCentralNode;
    ImGuiID dockspaceId = ImGui::GetID("MainDockSpace");
    ImGui::DockSpace(dockspaceId, ImVec2(0.0f, 0.0f), dockFlags);
}

void ImGuiLayer::render_title_bar() {
    static bool is_dragging = false;
    static ImVec2 drag_offset;
    static bool resizing = false;
    static int resize_dir = 0;
    static ImVec2 last_mouse_pos;

    auto& colors = ImGui::GetStyle().Colors;
    ImVec4 transparent = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    ImVec4 btn_hovered = ImVec4(colors[ImGuiCol_ButtonHovered].x, colors[ImGuiCol_ButtonHovered].y, colors[ImGuiCol_ButtonHovered].z, 0.5f);

    if (ImGui::BeginMenuBar()) {
        ImVec2 menu_bar_pos = ImGui::GetWindowPos();
        float window_width = ImGui::GetWindowWidth();
        float menu_bar_height = ImGui::GetFrameHeight();
        ImVec2 mouse_pos = ImGui::GetMousePos();

        const int btn_count = m_resizable ? 3 : 2;
        const float pad = 2.0f;
        const float spacing = 4.0f;
        const float icon_size = menu_bar_height;
        float total_w = btn_count * icon_size + (btn_count - 1) * spacing;
        float buttons_start_x = menu_bar_pos.x + window_width - pad - total_w;

        bool mouse_over_buttons = (mouse_pos.x >= buttons_start_x &&
                                   mouse_pos.x <= menu_bar_pos.x + window_width &&
                                   mouse_pos.y >= menu_bar_pos.y &&
                                   mouse_pos.y <= menu_bar_pos.y + menu_bar_height);

        bool mouse_over_bar = (mouse_pos.x >= menu_bar_pos.x &&
                               mouse_pos.x <= menu_bar_pos.x + window_width &&
                               mouse_pos.y >= menu_bar_pos.y &&
                               mouse_pos.y <= menu_bar_pos.y + menu_bar_height);

        if (mouse_over_bar && !mouse_over_buttons &&
            ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
            !ImGui::IsAnyItemHovered()) {
#ifdef TVK_PLATFORM_APPLE
            // macOS: manual drag via SetPosition (Cocoa doesn't expose
            // a performWindowDragWithEvent equivalent through GLFW)
            is_dragging = true;
            double cx, cy;
            glfwGetCursorPos(m_Window, &cx, &cy);
            drag_offset.x = static_cast<float>(cx);
            drag_offset.y = static_cast<float>(cy);
#else
            // Linux/Windows: hand the move off to the compositor/WM so it
            // works correctly on Wayland (glfwSetWindowPos is a no-op there)
            m_tvkWindow->StartDrag();
#endif
        }

#ifdef TVK_PLATFORM_APPLE
        if (is_dragging) {
            if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
                double cx, cy;
                glfwGetCursorPos(m_Window, &cx, &cy);
                i32 wx, wy;
                m_tvkWindow->GetPosition(wx, wy);
                float screen_x = static_cast<float>(cx) + static_cast<float>(wx);
                float screen_y = static_cast<float>(cy) + static_cast<float>(wy);
                m_tvkWindow->SetPosition(
                    static_cast<i32>(screen_x - drag_offset.x),
                    static_cast<i32>(screen_y - drag_offset.y));
            } else {
                is_dragging = false;
            }
        }
#endif

        if (m_TitleBarMenuCb) {
            m_TitleBarMenuCb();
        }

        const std::string& title = m_tvkWindow->GetTitle();
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        ImVec2 text_size = ImGui::CalcTextSize(title.c_str());
        float text_x = menu_bar_pos.x + (window_width - text_size.x) * 0.5f;
        float text_y = menu_bar_pos.y + (menu_bar_height - text_size.y) * 0.5f;
        draw_list->AddText(ImVec2(text_x, text_y), ImGui::GetColorU32(ImGuiCol_Text), title.c_str());

        float start_x = window_width - pad - total_w;
        ImGui::SetCursorPosX(start_x);
        ImVec2 btn_size(icon_size, menu_bar_height);

        ImGui::PushStyleColor(ImGuiCol_Button, transparent);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, btn_hovered);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, colors[ImGuiCol_ButtonActive]);
        ImGui::PushStyleColor(ImGuiCol_Border, transparent);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 0.0f));

        if (ImGui::Button(ICON_FA_MINUS "##minimize", btn_size))
            m_tvkWindow->Iconify();
        ImGui::SameLine(0, spacing);

        bool is_maximized = m_tvkWindow->IsMaximized();
        if (m_resizable) {
            const char* max_icon = is_maximized ? ICON_FA_WINDOW_RESTORE "##maximize" : ICON_FA_WINDOW_MAXIMIZE "##maximize";
            if (ImGui::Button(max_icon, btn_size)) {
                if (is_maximized)
                    m_tvkWindow->Restore();
                else
                    m_tvkWindow->Maximize();
            }
            ImGui::SameLine(0, spacing);
        }

        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.80f, 0.10f, 0.10f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.60f, 0.05f, 0.05f, 1.0f));
        if (ImGui::Button(ICON_FA_XMARK "##close", btn_size))
            m_tvkWindow->Close();
        ImGui::PopStyleColor(2);

        ImGui::PopStyleVar(3);
        ImGui::PopStyleColor(4);

        ImGui::EndMenuBar();
    }

    ImVec2 win_pos = ImGui::GetWindowPos();
    ImVec2 win_size = ImGui::GetWindowSize();
    ImVec2 mouse_pos = ImGui::GetMousePos();
    const float border_thickness = 6.0f;

    ImVec2 rb_min = ImVec2(win_pos.x + win_size.x - border_thickness, win_pos.y);
    ImVec2 rb_max = ImVec2(win_pos.x + win_size.x, win_pos.y + win_size.y);
    ImVec2 bb_min = ImVec2(win_pos.x, win_pos.y + win_size.y - border_thickness);
    ImVec2 bb_max = ImVec2(win_pos.x + win_size.x, win_pos.y + win_size.y);
    ImVec2 cb_min = ImVec2(win_pos.x + win_size.x - border_thickness, win_pos.y + win_size.y - border_thickness);

    auto contains = [](ImVec2 min, ImVec2 max, ImVec2 p) {
        return p.x >= min.x && p.x <= max.x && p.y >= min.y && p.y <= max.y;
    };

    if (!resizing && m_resizable) {
        if (contains(cb_min, bb_max, mouse_pos)) {
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNWSE);
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) { resizing = true; resize_dir = 3; }
        } else if (contains(rb_min, rb_max, mouse_pos)) {
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) { resizing = true; resize_dir = 1; }
        } else if (contains(bb_min, bb_max, mouse_pos)) {
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) { resizing = true; resize_dir = 2; }
        }
    }

    if (resizing && m_resizable) {
        if      (resize_dir == 1) ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
        else if (resize_dir == 2) ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
        else if (resize_dir == 3) ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNWSE);

        if (ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
            ImVec2 delta = ImVec2(mouse_pos.x - last_mouse_pos.x, mouse_pos.y - last_mouse_pos.y);
            Extent2D ext = m_tvkWindow->GetExtent();
            u32 width = ext.width, height = ext.height;
            if (resize_dir == 1 || resize_dir == 3) {
                i32 nw = static_cast<i32>(width) + static_cast<i32>(delta.x);
                width = nw > 200 ? static_cast<u32>(nw) : 200u;
            }
            if (resize_dir == 2 || resize_dir == 3) {
                i32 nh = static_cast<i32>(height) + static_cast<i32>(delta.y);
                height = nh > 100 ? static_cast<u32>(nh) : 100u;
            }
            m_tvkWindow->SetSize(width, height);
        }

        if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
            resizing = false;
            resize_dir = 0;
        }
    }

    last_mouse_pos = mouse_pos;
}

void ImGuiLayer::EndDockspace() {
    ImGui::End();
}

bool ImGuiLayer::WantsCaptureKeyboard() const {
    return ImGui::GetIO().WantCaptureKeyboard;
}

bool ImGuiLayer::WantsCaptureMouse() const {
    return ImGui::GetIO().WantCaptureMouse;
}

void ImGuiLayer::SetupStyle() {
    ImGuiStyle& style = ImGui::GetStyle();
    ImGuiIO& io = ImGui::GetIO();
    
    // ========== ROUNDING ==========
    // Set all rounding to 0 for sharp edges
    style.WindowRounding = 0.0f;
    style.ChildRounding = 0.0f;
    style.FrameRounding = 0.0f;
    style.PopupRounding = 0.0f;
    style.ScrollbarRounding = 0.0f;
    style.GrabRounding = 0.0f;
    style.TabRounding = 0.0f;
    
    // ========== SIZING & SPACING ==========
    // Compact layout with smaller padding and spacing
    style.WindowPadding = ImVec2(6.0f, 6.0f);
    style.FramePadding = ImVec2(4.0f, 2.0f);
    style.CellPadding = ImVec2(4.0f, 2.0f);
    style.ItemSpacing = ImVec2(6.0f, 4.0f);
    style.ItemInnerSpacing = ImVec2(4.0f, 4.0f);
    style.TouchExtraPadding = ImVec2(0.0f, 0.0f);
    style.IndentSpacing = 16.0f;
    style.ScrollbarSize = 12.0f;
    style.GrabMinSize = 8.0f;
    
    // ========== BORDERS ==========
    style.WindowBorderSize = 1.0f;
    style.ChildBorderSize = 1.0f;
    style.PopupBorderSize = 1.0f;
    style.FrameBorderSize = 1.0f;
    style.TabBorderSize = 0.0f;
    
    // ========== FONT SCALING ==========
    // Smaller font scale for compact UI
    io.FontGlobalScale = 0.9f;
    
    // ========== SPACE BLACK THEME COLORS WITH BETTER CONTRAST ==========
    ImVec4* colors = style.Colors;
    
    // Base colors - near-black space theme
    colors[ImGuiCol_WindowBg] = ImVec4(0.05f, 0.05f, 0.06f, 1.00f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.06f, 0.06f, 0.07f, 1.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.05f, 0.05f, 0.06f, 1.00f);
    colors[ImGuiCol_Border] = ImVec4(0.20f, 0.20f, 0.22f, 1.00f);
    colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    
    // Frame colors - better contrast
    colors[ImGuiCol_FrameBg] = ImVec4(0.10f, 0.10f, 0.12f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.15f, 0.15f, 0.17f, 1.00f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.20f, 0.20f, 0.22f, 1.00f);
    
    // Title bar
    colors[ImGuiCol_TitleBg] = ImVec4(0.03f, 0.03f, 0.04f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.08f, 0.08f, 0.10f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.03f, 0.03f, 0.04f, 1.00f);
    
    // Menu bar - slightly lighter
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.08f, 0.08f, 0.10f, 1.00f);
    
    // Scrollbar - better visibility
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.05f, 0.05f, 0.06f, 1.00f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.25f, 0.25f, 0.28f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.35f, 0.35f, 0.38f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.45f, 0.45f, 0.48f, 1.00f);
    
    // Check mark - brighter
    colors[ImGuiCol_CheckMark] = ImVec4(0.80f, 0.80f, 0.85f, 1.00f);
    
    // Slider - brighter
    colors[ImGuiCol_SliderGrab] = ImVec4(0.50f, 0.50f, 0.55f, 1.00f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.70f, 0.70f, 0.75f, 1.00f);
    
    // Button - more visible
    colors[ImGuiCol_Button] = ImVec4(0.12f, 0.12f, 0.15f, 1.00f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.20f, 0.20f, 0.23f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.28f, 0.28f, 0.31f, 1.00f);
    
    // Header
    colors[ImGuiCol_Header] = ImVec4(0.12f, 0.12f, 0.15f, 1.00f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.20f, 0.20f, 0.23f, 1.00f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.28f, 0.28f, 0.31f, 1.00f);
    
    // Separator - better visibility
    colors[ImGuiCol_Separator] = ImVec4(0.20f, 0.20f, 0.22f, 1.00f);
    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.28f, 0.28f, 0.31f, 1.00f);
    colors[ImGuiCol_SeparatorActive] = ImVec4(0.36f, 0.36f, 0.39f, 1.00f);
    
    // Resize grip
    colors[ImGuiCol_ResizeGrip] = ImVec4(0.18f, 0.18f, 0.21f, 1.00f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.28f, 0.28f, 0.31f, 1.00f);
    colors[ImGuiCol_ResizeGripActive] = ImVec4(0.36f, 0.36f, 0.39f, 1.00f);
    
    // Tab
    colors[ImGuiCol_Tab] = ImVec4(0.08f, 0.08f, 0.10f, 1.00f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.18f, 0.18f, 0.21f, 1.00f);
    colors[ImGuiCol_TabActive] = ImVec4(0.12f, 0.12f, 0.15f, 1.00f);
    colors[ImGuiCol_TabUnfocused] = ImVec4(0.05f, 0.05f, 0.06f, 1.00f);
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.10f, 0.10f, 0.12f, 1.00f);
    
    // Docking
    colors[ImGuiCol_DockingPreview] = ImVec4(0.45f, 0.45f, 0.48f, 0.70f);
    colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.05f, 0.05f, 0.06f, 1.00f);
    
    // Plot - brighter
    colors[ImGuiCol_PlotLines] = ImVec4(0.75f, 0.75f, 0.78f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.85f, 0.85f, 0.88f, 1.00f);
    colors[ImGuiCol_PlotHistogram] = ImVec4(0.75f, 0.75f, 0.78f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.85f, 0.85f, 0.88f, 1.00f);
    
    // Table
    colors[ImGuiCol_TableHeaderBg] = ImVec4(0.10f, 0.10f, 0.12f, 1.00f);
    colors[ImGuiCol_TableBorderStrong] = ImVec4(0.20f, 0.20f, 0.22f, 1.00f);
    colors[ImGuiCol_TableBorderLight] = ImVec4(0.15f, 0.15f, 0.17f, 1.00f);
    colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 0.03f);
    
    // Text - brighter for better readability
    colors[ImGuiCol_Text] = ImVec4(0.95f, 0.95f, 0.97f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.53f, 1.00f);
    colors[ImGuiCol_TextSelectedBg] = ImVec4(0.28f, 0.28f, 0.31f, 1.00f);
    
    // Drag drop
    colors[ImGuiCol_DragDropTarget] = ImVec4(0.50f, 0.50f, 0.55f, 0.90f);
    
    // Nav
    colors[ImGuiCol_NavHighlight] = ImVec4(0.40f, 0.40f, 0.45f, 1.00f);
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(0.70f, 0.70f, 0.75f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.15f, 0.15f, 0.17f, 0.20f);
    
    // Modal
    colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.70f);
}

} // namespace tvk
