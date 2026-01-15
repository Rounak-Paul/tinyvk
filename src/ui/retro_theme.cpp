/**
 * @file retro_theme.cpp
 * @brief 8-bit RPG style theme implementation
 */

#include "tinyvk/ui/retro_theme.h"
#include <imgui.h>

namespace tvk {

RetroPalette RetroPalette::default_dark() {
    RetroPalette p;
    p.bg_dark = RetroColor(16, 16, 24);
    p.bg_medium = RetroColor(32, 32, 48);
    p.bg_light = RetroColor(48, 48, 64);
    p.border_dark = RetroColor(64, 64, 96);
    p.border_light = RetroColor(96, 96, 128);
    p.text_primary = RetroColor(240, 240, 240);
    p.text_secondary = RetroColor(180, 180, 200);
    p.text_disabled = RetroColor(100, 100, 120);
    p.accent_primary = RetroColor(64, 128, 224);
    p.accent_secondary = RetroColor(96, 160, 255);
    p.accent_highlight = RetroColor(128, 192, 255);
    p.button_normal = RetroColor(48, 64, 96);
    p.button_hover = RetroColor(64, 80, 128);
    p.button_pressed = RetroColor(32, 48, 80);
    p.input_bg = RetroColor(24, 24, 32);
    p.input_border = RetroColor(64, 64, 96);
    p.selection = RetroColor(64, 128, 224, 128);
    p.scrollbar_bg = RetroColor(24, 24, 32);
    p.scrollbar_thumb = RetroColor(64, 64, 96);
    p.shadow = RetroColor(0, 0, 0, 128);
    return p;
}

RetroPalette RetroPalette::default_light() {
    RetroPalette p;
    p.bg_dark = RetroColor(200, 200, 216);
    p.bg_medium = RetroColor(220, 220, 232);
    p.bg_light = RetroColor(240, 240, 248);
    p.border_dark = RetroColor(128, 128, 160);
    p.border_light = RetroColor(160, 160, 192);
    p.text_primary = RetroColor(24, 24, 32);
    p.text_secondary = RetroColor(64, 64, 80);
    p.text_disabled = RetroColor(128, 128, 144);
    p.accent_primary = RetroColor(48, 96, 192);
    p.accent_secondary = RetroColor(64, 128, 224);
    p.accent_highlight = RetroColor(96, 160, 255);
    p.button_normal = RetroColor(180, 180, 200);
    p.button_hover = RetroColor(160, 160, 180);
    p.button_pressed = RetroColor(140, 140, 160);
    p.input_bg = RetroColor(248, 248, 255);
    p.input_border = RetroColor(128, 128, 160);
    p.selection = RetroColor(48, 96, 192, 128);
    p.scrollbar_bg = RetroColor(220, 220, 232);
    p.scrollbar_thumb = RetroColor(160, 160, 192);
    p.shadow = RetroColor(0, 0, 0, 64);
    return p;
}

RetroPalette RetroPalette::gameboy() {
    RetroPalette p;
    p.bg_dark = RetroColor(15, 56, 15);
    p.bg_medium = RetroColor(48, 98, 48);
    p.bg_light = RetroColor(139, 172, 15);
    p.border_dark = RetroColor(15, 56, 15);
    p.border_light = RetroColor(48, 98, 48);
    p.text_primary = RetroColor(155, 188, 15);
    p.text_secondary = RetroColor(139, 172, 15);
    p.text_disabled = RetroColor(48, 98, 48);
    p.accent_primary = RetroColor(155, 188, 15);
    p.accent_secondary = RetroColor(139, 172, 15);
    p.accent_highlight = RetroColor(155, 188, 15);
    p.button_normal = RetroColor(48, 98, 48);
    p.button_hover = RetroColor(139, 172, 15);
    p.button_pressed = RetroColor(15, 56, 15);
    p.input_bg = RetroColor(15, 56, 15);
    p.input_border = RetroColor(48, 98, 48);
    p.selection = RetroColor(155, 188, 15, 128);
    p.scrollbar_bg = RetroColor(15, 56, 15);
    p.scrollbar_thumb = RetroColor(139, 172, 15);
    p.shadow = RetroColor(15, 56, 15, 200);
    return p;
}

RetroPalette RetroPalette::nes() {
    RetroPalette p;
    p.bg_dark = RetroColor(0, 0, 0);
    p.bg_medium = RetroColor(44, 44, 44);
    p.bg_light = RetroColor(88, 88, 88);
    p.border_dark = RetroColor(0, 0, 0);
    p.border_light = RetroColor(188, 188, 188);
    p.text_primary = RetroColor(252, 252, 252);
    p.text_secondary = RetroColor(188, 188, 188);
    p.text_disabled = RetroColor(88, 88, 88);
    p.accent_primary = RetroColor(228, 92, 16);
    p.accent_secondary = RetroColor(252, 160, 68);
    p.accent_highlight = RetroColor(248, 216, 120);
    p.button_normal = RetroColor(0, 120, 248);
    p.button_hover = RetroColor(60, 188, 252);
    p.button_pressed = RetroColor(0, 88, 168);
    p.input_bg = RetroColor(0, 0, 0);
    p.input_border = RetroColor(188, 188, 188);
    p.selection = RetroColor(228, 92, 16, 160);
    p.scrollbar_bg = RetroColor(44, 44, 44);
    p.scrollbar_thumb = RetroColor(188, 188, 188);
    p.shadow = RetroColor(0, 0, 0, 200);
    return p;
}

RetroPalette RetroPalette::snes() {
    RetroPalette p;
    p.bg_dark = RetroColor(24, 24, 56);
    p.bg_medium = RetroColor(40, 40, 88);
    p.bg_light = RetroColor(64, 64, 128);
    p.border_dark = RetroColor(24, 24, 56);
    p.border_light = RetroColor(128, 128, 200);
    p.text_primary = RetroColor(248, 248, 248);
    p.text_secondary = RetroColor(200, 200, 232);
    p.text_disabled = RetroColor(96, 96, 144);
    p.accent_primary = RetroColor(248, 184, 0);
    p.accent_secondary = RetroColor(248, 216, 0);
    p.accent_highlight = RetroColor(248, 248, 128);
    p.button_normal = RetroColor(80, 80, 160);
    p.button_hover = RetroColor(112, 112, 200);
    p.button_pressed = RetroColor(48, 48, 120);
    p.input_bg = RetroColor(24, 24, 56);
    p.input_border = RetroColor(128, 128, 200);
    p.selection = RetroColor(248, 184, 0, 128);
    p.scrollbar_bg = RetroColor(24, 24, 56);
    p.scrollbar_thumb = RetroColor(128, 128, 200);
    p.shadow = RetroColor(0, 0, 24, 180);
    return p;
}

RetroPalette RetroPalette::c64() {
    RetroPalette p;
    p.bg_dark = RetroColor(53, 40, 121);
    p.bg_medium = RetroColor(68, 68, 68);
    p.bg_light = RetroColor(108, 94, 181);
    p.border_dark = RetroColor(0, 0, 0);
    p.border_light = RetroColor(154, 154, 154);
    p.text_primary = RetroColor(134, 195, 221);
    p.text_secondary = RetroColor(154, 154, 154);
    p.text_disabled = RetroColor(68, 68, 68);
    p.accent_primary = RetroColor(111, 79, 37);
    p.accent_secondary = RetroColor(161, 130, 84);
    p.accent_highlight = RetroColor(154, 154, 154);
    p.button_normal = RetroColor(53, 40, 121);
    p.button_hover = RetroColor(108, 94, 181);
    p.button_pressed = RetroColor(0, 0, 0);
    p.input_bg = RetroColor(0, 0, 0);
    p.input_border = RetroColor(134, 195, 221);
    p.selection = RetroColor(134, 195, 221, 128);
    p.scrollbar_bg = RetroColor(53, 40, 121);
    p.scrollbar_thumb = RetroColor(134, 195, 221);
    p.shadow = RetroColor(0, 0, 0, 200);
    return p;
}

RetroTheme& RetroTheme::get() {
    static RetroTheme instance;
    return instance;
}

RetroTheme::RetroTheme() {
    _config.palette = RetroPalette::default_dark();
}

void RetroTheme::set_config(const RetroThemeConfig& config) {
    _config = config;
    if (_applied) {
        apply();
    }
}

void RetroTheme::set_palette(const RetroPalette& palette) {
    _config.palette = palette;
    if (_applied) {
        apply();
    }
}

void RetroTheme::apply() {
    auto& style = ImGui::GetStyle();
    auto& colors = style.Colors;
    const auto& p = _config.palette;

    style.WindowRounding = 0.0f;
    style.ChildRounding = 0.0f;
    style.FrameRounding = 0.0f;
    style.GrabRounding = 0.0f;
    style.PopupRounding = 0.0f;
    style.ScrollbarRounding = 0.0f;
    style.TabRounding = 0.0f;

    style.WindowBorderSize = _config.draw_borders ? _config.border_thickness : 0.0f;
    style.FrameBorderSize = _config.draw_borders ? 1.0f : 0.0f;
    style.PopupBorderSize = _config.draw_borders ? _config.border_thickness : 0.0f;
    style.ChildBorderSize = _config.draw_borders ? 1.0f : 0.0f;

    style.WindowPadding = ImVec2(_config.window_padding, _config.window_padding);
    style.FramePadding = ImVec2(_config.button_padding_x, _config.button_padding_y);
    style.ItemSpacing = ImVec2(_config.item_spacing, _config.item_spacing);
    style.ItemInnerSpacing = ImVec2(_config.item_spacing, _config.item_spacing);
    style.ScrollbarSize = _config.scrollbar_width;

    style.AntiAliasedLines = !_config.pixel_perfect;
    style.AntiAliasedLinesUseTex = !_config.pixel_perfect;
    style.AntiAliasedFill = !_config.pixel_perfect;

    auto to_imvec4 = [](const RetroColor& c) {
        return ImVec4(c.rf(), c.gf(), c.bf(), c.af());
    };

    colors[ImGuiCol_Text] = to_imvec4(p.text_primary);
    colors[ImGuiCol_TextDisabled] = to_imvec4(p.text_disabled);
    colors[ImGuiCol_WindowBg] = to_imvec4(p.bg_medium);
    colors[ImGuiCol_ChildBg] = to_imvec4(p.bg_dark);
    colors[ImGuiCol_PopupBg] = to_imvec4(p.bg_dark);
    colors[ImGuiCol_Border] = to_imvec4(p.border_dark);
    colors[ImGuiCol_BorderShadow] = to_imvec4(p.shadow);
    colors[ImGuiCol_FrameBg] = to_imvec4(p.input_bg);
    colors[ImGuiCol_FrameBgHovered] = to_imvec4(p.bg_light);
    colors[ImGuiCol_FrameBgActive] = to_imvec4(p.bg_medium);
    colors[ImGuiCol_TitleBg] = to_imvec4(p.bg_dark);
    colors[ImGuiCol_TitleBgActive] = to_imvec4(p.accent_primary);
    colors[ImGuiCol_TitleBgCollapsed] = to_imvec4(p.bg_dark);
    colors[ImGuiCol_MenuBarBg] = to_imvec4(p.bg_dark);
    colors[ImGuiCol_ScrollbarBg] = to_imvec4(p.scrollbar_bg);
    colors[ImGuiCol_ScrollbarGrab] = to_imvec4(p.scrollbar_thumb);
    colors[ImGuiCol_ScrollbarGrabHovered] = to_imvec4(p.scrollbar_thumb.lighten(20));
    colors[ImGuiCol_ScrollbarGrabActive] = to_imvec4(p.accent_primary);
    colors[ImGuiCol_CheckMark] = to_imvec4(p.accent_primary);
    colors[ImGuiCol_SliderGrab] = to_imvec4(p.accent_primary);
    colors[ImGuiCol_SliderGrabActive] = to_imvec4(p.accent_highlight);
    colors[ImGuiCol_Button] = to_imvec4(p.button_normal);
    colors[ImGuiCol_ButtonHovered] = to_imvec4(p.button_hover);
    colors[ImGuiCol_ButtonActive] = to_imvec4(p.button_pressed);
    colors[ImGuiCol_Header] = to_imvec4(p.bg_light);
    colors[ImGuiCol_HeaderHovered] = to_imvec4(p.button_hover);
    colors[ImGuiCol_HeaderActive] = to_imvec4(p.accent_primary);
    colors[ImGuiCol_Separator] = to_imvec4(p.border_dark);
    colors[ImGuiCol_SeparatorHovered] = to_imvec4(p.accent_primary);
    colors[ImGuiCol_SeparatorActive] = to_imvec4(p.accent_highlight);
    colors[ImGuiCol_ResizeGrip] = to_imvec4(p.border_light);
    colors[ImGuiCol_ResizeGripHovered] = to_imvec4(p.accent_primary);
    colors[ImGuiCol_ResizeGripActive] = to_imvec4(p.accent_highlight);
    colors[ImGuiCol_Tab] = to_imvec4(p.bg_dark);
    colors[ImGuiCol_TabHovered] = to_imvec4(p.button_hover);
    colors[ImGuiCol_TabActive] = to_imvec4(p.accent_primary);
    colors[ImGuiCol_TabUnfocused] = to_imvec4(p.bg_dark);
    colors[ImGuiCol_TabUnfocusedActive] = to_imvec4(p.bg_medium);
    colors[ImGuiCol_DockingPreview] = to_imvec4(RetroColor(p.accent_primary.r, p.accent_primary.g, p.accent_primary.b, 180));
    colors[ImGuiCol_DockingEmptyBg] = to_imvec4(p.bg_dark);
    colors[ImGuiCol_PlotLines] = to_imvec4(p.accent_secondary);
    colors[ImGuiCol_PlotLinesHovered] = to_imvec4(p.accent_highlight);
    colors[ImGuiCol_PlotHistogram] = to_imvec4(p.accent_primary);
    colors[ImGuiCol_PlotHistogramHovered] = to_imvec4(p.accent_highlight);
    colors[ImGuiCol_TableHeaderBg] = to_imvec4(p.bg_dark);
    colors[ImGuiCol_TableBorderStrong] = to_imvec4(p.border_dark);
    colors[ImGuiCol_TableBorderLight] = to_imvec4(p.border_light);
    colors[ImGuiCol_TableRowBg] = ImVec4(0, 0, 0, 0);
    colors[ImGuiCol_TableRowBgAlt] = to_imvec4(RetroColor(p.bg_light.r, p.bg_light.g, p.bg_light.b, 32));
    colors[ImGuiCol_TextSelectedBg] = to_imvec4(p.selection);
    colors[ImGuiCol_DragDropTarget] = to_imvec4(p.accent_highlight);
    colors[ImGuiCol_NavHighlight] = to_imvec4(p.accent_primary);
    colors[ImGuiCol_NavWindowingHighlight] = to_imvec4(p.accent_highlight);
    colors[ImGuiCol_NavWindowingDimBg] = to_imvec4(RetroColor(0, 0, 0, 180));
    colors[ImGuiCol_ModalWindowDimBg] = to_imvec4(RetroColor(0, 0, 0, 180));

    _applied = true;
}

void RetroTheme::push_color(const RetroColor& color) {
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(color.rf(), color.gf(), color.bf(), color.af()));
}

void RetroTheme::pop_color() {
    ImGui::PopStyleColor();
}

f32 RetroTheme::snap_to_pixel(f32 value) const {
    if (_config.pixel_perfect) {
        return static_cast<f32>(static_cast<i32>(value));
    }
    return value;
}

Vec2 RetroTheme::snap_to_pixel(const Vec2& value) const {
    if (_config.pixel_perfect) {
        return Vec2(
            static_cast<f32>(static_cast<i32>(value.x)),
            static_cast<f32>(static_cast<i32>(value.y))
        );
    }
    return value;
}

} // namespace tvk
