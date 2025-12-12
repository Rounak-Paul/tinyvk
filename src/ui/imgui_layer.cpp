/**
 * @file imgui_layer.cpp
 * @brief ImGui layer implementation for Vulkan
 */

#include "tinyvk/ui/imgui_layer.h"
#include "tinyvk/renderer/renderer.h"
#include "tinyvk/renderer/context.h"
#include "tinyvk/core/log.h"
#include "tinyvk/assets/fonts.h"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

#include <GLFW/glfw3.h>
#include <cstring>

namespace tvk {

ImGuiLayer::~ImGuiLayer() {
    Cleanup();
}

bool ImGuiLayer::Init(GLFWwindow* window, Renderer* renderer, const ImGuiConfig& config) {
    m_Window = window;
    m_Renderer = renderer;
    m_Config = config;

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
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    
    if (config.enableDocking) {
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    }
    
    if (config.enableViewports) {
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    }

    // Load font
    if (config.fontPath != nullptr) {
        io.Fonts->AddFontFromFileTTF(config.fontPath, config.fontSize * config.fontScale);
    } else if (config.useEmbeddedFont) {
        void* fontData = roboto_medium;
        int fontDataSize = roboto_medium_size;
        
        if (std::strcmp(config.embeddedFontName, "roboto") == 0) {
            fontData = roboto_medium;
            fontDataSize = roboto_medium_size;
        } else if (std::strcmp(config.embeddedFontName, "lexend") == 0) {
            fontData = lexend_regular;
            fontDataSize = lexend_regular_size;
        } else if (std::strcmp(config.embeddedFontName, "quicksand") == 0) {
            fontData = quicksand_regular;
            fontDataSize = quicksand_regular_size;
        } else if (std::strcmp(config.embeddedFontName, "droid") == 0) {
            fontData = droid_sans;
            fontDataSize = droid_sans_size;
        }
        
        ImFontConfig fontConfig;
        fontConfig.FontDataOwnedByAtlas = false;
        io.Fonts->AddFontFromMemoryTTF(fontData, fontDataSize, config.fontSize * config.fontScale, &fontConfig);
        TVK_LOG_INFO("Loaded embedded font: {}", config.embeddedFontName);
    } else {
        io.FontGlobalScale = config.fontScale;
    }

    // Setup platform/renderer backends
    ImGui_ImplGlfw_InitForVulkan(window, true);

    ImGui_ImplVulkan_InitInfo initInfo{};
    initInfo.Instance = context.GetInstance();
    initInfo.PhysicalDevice = context.GetPhysicalDevice();
    initInfo.Device = context.GetDevice();
    initInfo.QueueFamily = context.GetQueueFamilyIndices().graphicsFamily.value();
    initInfo.Queue = context.GetGraphicsQueue();
    initInfo.PipelineCache = VK_NULL_HANDLE;
    initInfo.DescriptorPool = m_DescriptorPool;
    initInfo.Subpass = 0;
    initInfo.MinImageCount = 2;
    initInfo.ImageCount = renderer->GetSwapchainImageCount();
    initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    initInfo.Allocator = nullptr;
    initInfo.RenderPass = renderer->GetRenderPass();

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

void ImGuiLayer::BeginDockspace() {
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

    ImGui::Begin("##DockSpaceWindow", nullptr, windowFlags);
    ImGui::PopStyleVar(3);

    ImGuiID dockspaceId = ImGui::GetID("MainDockSpace");
    ImGui::DockSpace(dockspaceId, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);
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
    SetDarkTheme();

    auto& style = ImGui::GetStyle();
    style.WindowRounding = 6.0f;
    style.ChildRounding = 4.0f;
    style.FrameRounding = 4.0f;
    style.GrabRounding = 4.0f;
    style.PopupRounding = 4.0f;
    style.ScrollbarRounding = 6.0f;
    style.TabRounding = 4.0f;
    style.WindowBorderSize = 1.0f;
    style.FrameBorderSize = 0.0f;
    style.PopupBorderSize = 1.0f;
    style.WindowPadding = ImVec2(10.0f, 10.0f);
    style.FramePadding = ImVec2(6.0f, 4.0f);
    style.ItemSpacing = ImVec2(8.0f, 6.0f);
    style.ItemInnerSpacing = ImVec2(6.0f, 4.0f);
    style.IndentSpacing = 20.0f;
    style.ScrollbarSize = 14.0f;
    style.GrabMinSize = 10.0f;
    style.WindowTitleAlign = ImVec2(0.5f, 0.5f);
    style.SeparatorTextBorderSize = 2.0f;
}

} // namespace tvk
