/**
 * @file retro_ui.cpp
 * @brief 8-bit RPG style UI controls implementation
 */

#include "tinyvk/ui/retro_ui.h"
#include "tinyvk/ui/retro_theme.h"

#include <imgui.h>
#include <imgui_internal.h>
#include <cstring>
#include <algorithm>

namespace tvk {

namespace {

ImGuiWindowFlags convert_window_flags(RetroWindowFlags flags) {
    ImGuiWindowFlags result = 0;
    if (flags & RetroWindowFlags::NoTitleBar) result |= ImGuiWindowFlags_NoTitleBar;
    if (flags & RetroWindowFlags::NoResize) result |= ImGuiWindowFlags_NoResize;
    if (flags & RetroWindowFlags::NoMove) result |= ImGuiWindowFlags_NoMove;
    if (flags & RetroWindowFlags::NoScrollbar) result |= ImGuiWindowFlags_NoScrollbar;
    if (flags & RetroWindowFlags::NoCollapse) result |= ImGuiWindowFlags_NoCollapse;
    if (flags & RetroWindowFlags::NoBackground) result |= ImGuiWindowFlags_NoBackground;
    if (flags & RetroWindowFlags::AlwaysAutoResize) result |= ImGuiWindowFlags_AlwaysAutoResize;
    return result;
}

ImU32 to_imu32(const RetroColor& c) {
    return IM_COL32(c.r, c.g, c.b, c.a);
}

ImVec2 to_imvec2(const Vec2& v) {
    return ImVec2(v.x, v.y);
}

Vec2 from_imvec2(const ImVec2& v) {
    return Vec2(v.x, v.y);
}

void draw_pixel_rect(ImDrawList* dl, const ImVec2& min, const ImVec2& max, ImU32 color) {
    dl->AddRectFilled(min, max, color);
}

void draw_pixel_border(ImDrawList* dl, const ImVec2& min, const ImVec2& max, 
                       ImU32 color_light, ImU32 color_dark, f32 thickness) {
    ImVec2 br(max.x, max.y);
    ImVec2 tr(max.x, min.y);
    ImVec2 bl(min.x, max.y);

    dl->AddLine(min, tr, color_light, thickness);
    dl->AddLine(min, bl, color_light, thickness);
    dl->AddLine(tr, br, color_dark, thickness);
    dl->AddLine(bl, br, color_dark, thickness);
}

void draw_rpg_border(ImDrawList* dl, const ImVec2& min, const ImVec2& max,
                     ImU32 outer_dark, ImU32 inner_light, ImU32 inner_dark, ImU32 outer_light,
                     f32 thickness) {
    f32 t = thickness;

    dl->AddRect(min, max, outer_dark, 0.0f, 0, t);

    ImVec2 inner_min(min.x + t, min.y + t);
    ImVec2 inner_max(max.x - t, max.y - t);
    
    draw_pixel_border(dl, inner_min, inner_max, inner_light, inner_dark, t);

    ImVec2 outer_inner_min(min.x + t * 2, min.y + t * 2);
    ImVec2 outer_inner_max(max.x - t * 2, max.y - t * 2);
    dl->AddRect(outer_inner_min, outer_inner_max, outer_light, 0.0f, 0, t);
}

} // anonymous namespace

void RetroUI::init() {
    RetroTheme::get().apply();
}

void RetroUI::shutdown() {
}

bool RetroUI::begin_window(const char* title, bool* open, RetroWindowFlags flags) {
    ImGuiWindowFlags imgui_flags = convert_window_flags(flags);

    const auto& palette = RetroTheme::get().get_palette();
    const auto& config = RetroTheme::get().get_config();

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, config.border_thickness);

    bool result = ImGui::Begin(title, open, imgui_flags);

    if (result && !(flags & RetroWindowFlags::NoBorder)) {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 pos = ImGui::GetWindowPos();
        ImVec2 size = ImGui::GetWindowSize();
        ImVec2 min = pos;
        ImVec2 max(pos.x + size.x, pos.y + size.y);

        draw_rpg_border(dl, min, max,
            to_imu32(palette.border_dark),
            to_imu32(palette.border_light),
            to_imu32(palette.border_dark.darken(20)),
            to_imu32(palette.border_dark),
            config.border_thickness);
    }

    ImGui::PopStyleVar(2);
    return result;
}

void RetroUI::end_window() {
    ImGui::End();
}

bool RetroUI::begin_child(const char* id, const Vec2& size, bool border) {
    ImGuiChildFlags child_flags = border ? ImGuiChildFlags_Borders : ImGuiChildFlags_None;
    return ImGui::BeginChild(id, to_imvec2(size), child_flags);
}

void RetroUI::end_child() {
    ImGui::EndChild();
}

void RetroUI::begin_group() {
    ImGui::BeginGroup();
}

void RetroUI::end_group() {
    ImGui::EndGroup();
}

void RetroUI::separator() {
    const auto& palette = RetroTheme::get().get_palette();
    ImGui::PushStyleColor(ImGuiCol_Separator, to_imu32(palette.border_dark));
    ImGui::Separator();
    ImGui::PopStyleColor();
}

void RetroUI::spacing() {
    ImGui::Spacing();
}

void RetroUI::same_line(f32 offset, f32 spacing) {
    ImGui::SameLine(offset, spacing);
}

void RetroUI::new_line() {
    ImGui::NewLine();
}

void RetroUI::indent(f32 width) {
    ImGui::Indent(width);
}

void RetroUI::unindent(f32 width) {
    ImGui::Unindent(width);
}

void RetroUI::text(const char* text) {
    ImGui::TextUnformatted(text);
}

void RetroUI::text_colored(const char* text, const RetroColor& color) {
    ImGui::PushStyleColor(ImGuiCol_Text, to_imu32(color));
    ImGui::TextUnformatted(text);
    ImGui::PopStyleColor();
}

void RetroUI::text_wrapped(const char* text) {
    ImGui::TextWrapped("%s", text);
}

void RetroUI::label(const char* label, const char* text) {
    ImGui::LabelText(label, "%s", text);
}

bool RetroUI::button(const char* label, const Vec2& size, RetroButtonStyle style) {
    const auto& palette = RetroTheme::get().get_palette();
    const auto& config = RetroTheme::get().get_config();

    RetroColor face, face_hover, face_pressed, text_color;

    switch (style) {
        case RetroButtonStyle::Primary:
            face = palette.accent_primary;
            face_hover = palette.accent_secondary;
            face_pressed = palette.accent_primary.darken(30);
            text_color = retro_colors::white;
            break;
        case RetroButtonStyle::Secondary:
            face = palette.bg_light;
            face_hover = palette.bg_light.lighten(20);
            face_pressed = palette.bg_medium;
            text_color = palette.text_primary;
            break;
        case RetroButtonStyle::Danger:
            face = retro_colors::red;
            face_hover = retro_colors::light_red;
            face_pressed = retro_colors::dark_red;
            text_color = retro_colors::white;
            break;
        case RetroButtonStyle::Success:
            face = retro_colors::green;
            face_hover = retro_colors::light_green;
            face_pressed = retro_colors::dark_green;
            text_color = retro_colors::white;
            break;
        case RetroButtonStyle::Ghost:
            face = RetroColor(0, 0, 0, 0);
            face_hover = palette.bg_light;
            face_pressed = palette.bg_medium;
            text_color = palette.text_primary;
            break;
        default:
            face = palette.button_normal;
            face_hover = palette.button_hover;
            face_pressed = palette.button_pressed;
            text_color = palette.text_primary;
            break;
    }

    ImGui::PushStyleColor(ImGuiCol_Button, to_imu32(face));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, to_imu32(face_hover));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, to_imu32(face_pressed));
    ImGui::PushStyleColor(ImGuiCol_Text, to_imu32(text_color));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, config.border_thickness);

    bool clicked = ImGui::Button(label, to_imvec2(size));

    if (ImGui::IsItemVisible()) {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 min = ImGui::GetItemRectMin();
        ImVec2 max = ImGui::GetItemRectMax();

        bool hovered = ImGui::IsItemHovered();
        bool active = ImGui::IsItemActive();

        RetroColor current_face = active ? face_pressed : (hovered ? face_hover : face);

        if (style != RetroButtonStyle::Ghost || hovered || active) {
            ImU32 light = to_imu32(active ? current_face.darken(20) : current_face.lighten(40));
            ImU32 dark = to_imu32(active ? current_face.lighten(20) : current_face.darken(40));
            draw_pixel_border(dl, min, max, active ? dark : light, active ? light : dark, config.border_thickness);
        }
    }

    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(4);

    return clicked;
}

bool RetroUI::icon_button(RetroIconType icon, const Vec2& size, RetroButtonStyle style) {
    const auto& palette = RetroTheme::get().get_palette();

    char id[32];
    snprintf(id, sizeof(id), "##icon_%d", static_cast<int>(icon));

    ImGui::PushID(id);
    bool clicked = button("", size, style);
    ImGui::PopID();

    if (ImGui::IsItemVisible()) {
        ImVec2 min = ImGui::GetItemRectMin();
        ImVec2 max = ImGui::GetItemRectMax();
        Vec2 center((min.x + max.x) * 0.5f, (min.y + max.y) * 0.5f);
        f32 icon_size = std::min(size.x, size.y) * 0.6f;
        draw_icon_internal(center, icon, palette.text_primary, icon_size);
    }

    return clicked;
}

bool RetroUI::image_button(const char* id, u64 texture_id, const Vec2& size) {
    return ImGui::ImageButton(id, (ImTextureID)texture_id, to_imvec2(size));
}

bool RetroUI::checkbox(const char* label, bool* value) {
    const auto& palette = RetroTheme::get().get_palette();
    const auto& config = RetroTheme::get().get_config();

    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, config.border_thickness);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, to_imu32(palette.input_bg));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, to_imu32(palette.bg_light));
    ImGui::PushStyleColor(ImGuiCol_CheckMark, to_imu32(palette.accent_primary));

    bool result = ImGui::Checkbox(label, value);

    ImGui::PopStyleColor(3);
    ImGui::PopStyleVar(2);

    return result;
}

bool RetroUI::radio_button(const char* label, bool active) {
    const auto& palette = RetroTheme::get().get_palette();

    ImGui::PushStyleColor(ImGuiCol_FrameBg, to_imu32(palette.input_bg));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, to_imu32(palette.bg_light));
    ImGui::PushStyleColor(ImGuiCol_CheckMark, to_imu32(palette.accent_primary));

    bool result = ImGui::RadioButton(label, active);

    ImGui::PopStyleColor(3);

    return result;
}

bool RetroUI::radio_button(const char* label, int* value, int button_value) {
    return radio_button(label, *value == button_value) ? (*value = button_value, true) : false;
}

bool RetroUI::slider_int(const char* label, i32* value, i32 min_val, i32 max_val) {
    const auto& palette = RetroTheme::get().get_palette();
    const auto& config = RetroTheme::get().get_config();

    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_GrabRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, config.border_thickness);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, to_imu32(palette.input_bg));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, to_imu32(palette.bg_light));
    ImGui::PushStyleColor(ImGuiCol_SliderGrab, to_imu32(palette.accent_primary));
    ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, to_imu32(palette.accent_highlight));

    bool result = ImGui::SliderInt(label, value, min_val, max_val);

    ImGui::PopStyleColor(4);
    ImGui::PopStyleVar(3);

    return result;
}

bool RetroUI::slider_float(const char* label, f32* value, f32 min_val, f32 max_val) {
    const auto& palette = RetroTheme::get().get_palette();
    const auto& config = RetroTheme::get().get_config();

    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_GrabRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, config.border_thickness);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, to_imu32(palette.input_bg));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, to_imu32(palette.bg_light));
    ImGui::PushStyleColor(ImGuiCol_SliderGrab, to_imu32(palette.accent_primary));
    ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, to_imu32(palette.accent_highlight));

    bool result = ImGui::SliderFloat(label, value, min_val, max_val);

    ImGui::PopStyleColor(4);
    ImGui::PopStyleVar(3);

    return result;
}

bool RetroUI::progress_bar(f32 fraction, const Vec2& size, const char* overlay) {
    const auto& palette = RetroTheme::get().get_palette();
    const auto& config = RetroTheme::get().get_config();

    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, to_imu32(palette.input_bg));
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, to_imu32(palette.accent_primary));

    ImGui::ProgressBar(fraction, to_imvec2(size), overlay);

    if (config.draw_borders) {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 min = ImGui::GetItemRectMin();
        ImVec2 max = ImGui::GetItemRectMax();
        draw_pixel_border(dl, min, max, 
            to_imu32(palette.border_light), 
            to_imu32(palette.border_dark), 
            config.border_thickness);
    }

    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar();

    return true;
}

bool RetroUI::input_text(const char* label, char* buf, size_t buf_size) {
    const auto& palette = RetroTheme::get().get_palette();
    const auto& config = RetroTheme::get().get_config();

    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, config.border_thickness);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, to_imu32(palette.input_bg));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, to_imu32(palette.bg_light));
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, to_imu32(palette.input_bg));
    ImGui::PushStyleColor(ImGuiCol_Border, to_imu32(palette.input_border));
    ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, to_imu32(palette.selection));

    bool result = ImGui::InputText(label, buf, buf_size);

    ImGui::PopStyleColor(5);
    ImGui::PopStyleVar(2);

    return result;
}

bool RetroUI::input_text_multiline(const char* label, char* buf, size_t buf_size, const Vec2& size) {
    const auto& palette = RetroTheme::get().get_palette();
    const auto& config = RetroTheme::get().get_config();

    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, config.border_thickness);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, to_imu32(palette.input_bg));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, to_imu32(palette.bg_light));
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, to_imu32(palette.input_bg));
    ImGui::PushStyleColor(ImGuiCol_Border, to_imu32(palette.input_border));
    ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, to_imu32(palette.selection));

    bool result = ImGui::InputTextMultiline(label, buf, buf_size, to_imvec2(size));

    ImGui::PopStyleColor(5);
    ImGui::PopStyleVar(2);

    return result;
}

bool RetroUI::input_int(const char* label, i32* value) {
    const auto& palette = RetroTheme::get().get_palette();
    const auto& config = RetroTheme::get().get_config();

    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, config.border_thickness);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, to_imu32(palette.input_bg));
    ImGui::PushStyleColor(ImGuiCol_Border, to_imu32(palette.input_border));

    bool result = ImGui::InputInt(label, value);

    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(2);

    return result;
}

bool RetroUI::input_float(const char* label, f32* value) {
    const auto& palette = RetroTheme::get().get_palette();
    const auto& config = RetroTheme::get().get_config();

    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, config.border_thickness);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, to_imu32(palette.input_bg));
    ImGui::PushStyleColor(ImGuiCol_Border, to_imu32(palette.input_border));

    bool result = ImGui::InputFloat(label, value);

    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(2);

    return result;
}

bool RetroUI::combo(const char* label, i32* current_item, const char* const* items, i32 item_count) {
    const auto& palette = RetroTheme::get().get_palette();
    const auto& config = RetroTheme::get().get_config();

    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, config.border_thickness);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, to_imu32(palette.input_bg));
    ImGui::PushStyleColor(ImGuiCol_PopupBg, to_imu32(palette.bg_dark));
    ImGui::PushStyleColor(ImGuiCol_Header, to_imu32(palette.accent_primary));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, to_imu32(palette.accent_secondary));

    bool result = ImGui::Combo(label, current_item, items, item_count);

    ImGui::PopStyleColor(4);
    ImGui::PopStyleVar(3);

    return result;
}

bool RetroUI::begin_combo(const char* label, const char* preview) {
    const auto& palette = RetroTheme::get().get_palette();
    const auto& config = RetroTheme::get().get_config();

    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, config.border_thickness);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, to_imu32(palette.input_bg));
    ImGui::PushStyleColor(ImGuiCol_PopupBg, to_imu32(palette.bg_dark));

    bool result = ImGui::BeginCombo(label, preview);
    
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(3);
    
    return result;
}

void RetroUI::end_combo() {
    ImGui::EndCombo();
}

bool RetroUI::selectable(const char* label, bool selected, const Vec2& size) {
    const auto& palette = RetroTheme::get().get_palette();

    ImGui::PushStyleColor(ImGuiCol_Header, to_imu32(palette.accent_primary));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, to_imu32(palette.accent_secondary));
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, to_imu32(palette.accent_highlight));

    bool result = ImGui::Selectable(label, selected, 0, to_imvec2(size));

    ImGui::PopStyleColor(3);

    return result;
}

bool RetroUI::selectable(const char* label, bool* selected, const Vec2& size) {
    bool result = selectable(label, *selected, size);
    if (result) *selected = !*selected;
    return result;
}

bool RetroUI::begin_listbox(const char* label, const Vec2& size) {
    const auto& palette = RetroTheme::get().get_palette();
    const auto& config = RetroTheme::get().get_config();

    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, config.border_thickness);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, to_imu32(palette.input_bg));

    bool result = ImGui::BeginListBox(label, to_imvec2(size));
    
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(2);
    
    return result;
}

void RetroUI::end_listbox() {
    ImGui::EndListBox();
}

bool RetroUI::begin_menu_bar() {
    const auto& palette = RetroTheme::get().get_palette();
    ImGui::PushStyleColor(ImGuiCol_MenuBarBg, to_imu32(palette.bg_dark));
    bool result = ImGui::BeginMenuBar();
    ImGui::PopStyleColor();
    return result;
}

void RetroUI::end_menu_bar() {
    ImGui::EndMenuBar();
}

bool RetroUI::begin_menu(const char* label) {
    const auto& palette = RetroTheme::get().get_palette();
    ImGui::PushStyleColor(ImGuiCol_PopupBg, to_imu32(palette.bg_dark));
    ImGui::PushStyleColor(ImGuiCol_Header, to_imu32(palette.accent_primary));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, to_imu32(palette.accent_secondary));
    bool result = ImGui::BeginMenu(label);
    ImGui::PopStyleColor(3);
    return result;
}

void RetroUI::end_menu() {
    ImGui::EndMenu();
}

bool RetroUI::menu_item(const char* label, const char* shortcut, bool selected) {
    return ImGui::MenuItem(label, shortcut, selected);
}

bool RetroUI::menu_item(const char* label, const char* shortcut, bool* selected) {
    return ImGui::MenuItem(label, shortcut, selected);
}

bool RetroUI::tree_node(const char* label) {
    const auto& palette = RetroTheme::get().get_palette();
    ImGui::PushStyleColor(ImGuiCol_Header, to_imu32(palette.bg_light));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, to_imu32(palette.button_hover));
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, to_imu32(palette.accent_primary));

    bool result = ImGui::TreeNode(label);

    ImGui::PopStyleColor(3);
    return result;
}

bool RetroUI::tree_node_ex(const char* label, bool default_open, bool leaf) {
    const auto& palette = RetroTheme::get().get_palette();
    ImGui::PushStyleColor(ImGuiCol_Header, to_imu32(palette.bg_light));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, to_imu32(palette.button_hover));
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, to_imu32(palette.accent_primary));

    ImGuiTreeNodeFlags flags = 0;
    if (default_open) flags |= ImGuiTreeNodeFlags_DefaultOpen;
    if (leaf) flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;

    bool result = ImGui::TreeNodeEx(label, flags);

    ImGui::PopStyleColor(3);
    return result;
}

void RetroUI::tree_pop() {
    ImGui::TreePop();
}

bool RetroUI::collapsing_header(const char* label, bool default_open) {
    const auto& palette = RetroTheme::get().get_palette();
    const auto& config = RetroTheme::get().get_config();

    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);
    ImGui::PushStyleColor(ImGuiCol_Header, to_imu32(palette.bg_light));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, to_imu32(palette.button_hover));
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, to_imu32(palette.accent_primary));

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_CollapsingHeader;
    if (default_open) flags |= ImGuiTreeNodeFlags_DefaultOpen;

    bool result = ImGui::CollapsingHeader(label, flags);

    ImGui::PopStyleColor(3);
    ImGui::PopStyleVar();

    return result;
}

bool RetroUI::begin_tab_bar(const char* id) {
    const auto& palette = RetroTheme::get().get_palette();
    ImGui::PushStyleVar(ImGuiStyleVar_TabRounding, 0.0f);
    ImGui::PushStyleColor(ImGuiCol_Tab, to_imu32(palette.bg_dark));
    ImGui::PushStyleColor(ImGuiCol_TabHovered, to_imu32(palette.button_hover));
    ImGui::PushStyleColor(ImGuiCol_TabActive, to_imu32(palette.accent_primary));
    bool result = ImGui::BeginTabBar(id);
    ImGui::PopStyleColor(3);
    ImGui::PopStyleVar();
    return result;
}

void RetroUI::end_tab_bar() {
    ImGui::EndTabBar();
}

bool RetroUI::begin_tab_item(const char* label, bool* open) {
    return ImGui::BeginTabItem(label, open);
}

void RetroUI::end_tab_item() {
    ImGui::EndTabItem();
}

bool RetroUI::begin_table(const char* id, i32 columns) {
    const auto& palette = RetroTheme::get().get_palette();
    ImGui::PushStyleColor(ImGuiCol_TableHeaderBg, to_imu32(palette.bg_dark));
    ImGui::PushStyleColor(ImGuiCol_TableBorderStrong, to_imu32(palette.border_dark));
    ImGui::PushStyleColor(ImGuiCol_TableBorderLight, to_imu32(palette.border_light));
    bool result = ImGui::BeginTable(id, columns, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg);
    ImGui::PopStyleColor(3);
    return result;
}

void RetroUI::end_table() {
    ImGui::EndTable();
}

void RetroUI::table_next_row() {
    ImGui::TableNextRow();
}

void RetroUI::table_next_column() {
    ImGui::TableNextColumn();
}

void RetroUI::table_set_column_index(i32 index) {
    ImGui::TableSetColumnIndex(index);
}

void RetroUI::table_headers_row() {
    ImGui::TableHeadersRow();
}

void RetroUI::table_setup_column(const char* label, f32 width) {
    ImGuiTableColumnFlags flags = 0;
    if (width > 0) flags |= ImGuiTableColumnFlags_WidthFixed;
    ImGui::TableSetupColumn(label, flags, width);
}

bool RetroUI::begin_popup(const char* id) {
    const auto& palette = RetroTheme::get().get_palette();
    ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, 0.0f);
    ImGui::PushStyleColor(ImGuiCol_PopupBg, to_imu32(palette.bg_dark));
    bool result = ImGui::BeginPopup(id);
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
    return result;
}

bool RetroUI::begin_popup_modal(const char* name, bool* open) {
    const auto& palette = RetroTheme::get().get_palette();
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, 0.0f);
    ImGui::PushStyleColor(ImGuiCol_PopupBg, to_imu32(palette.bg_medium));
    ImGui::PushStyleColor(ImGuiCol_TitleBgActive, to_imu32(palette.accent_primary));
    bool result = ImGui::BeginPopupModal(name, open);
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(2);
    return result;
}

void RetroUI::end_popup() {
    ImGui::EndPopup();
}

void RetroUI::open_popup(const char* id) {
    ImGui::OpenPopup(id);
}

void RetroUI::close_popup() {
    ImGui::CloseCurrentPopup();
}

bool RetroUI::begin_popup_context_item(const char* id) {
    const auto& palette = RetroTheme::get().get_palette();
    ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, 0.0f);
    ImGui::PushStyleColor(ImGuiCol_PopupBg, to_imu32(palette.bg_dark));
    bool result = ImGui::BeginPopupContextItem(id);
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
    return result;
}

bool RetroUI::begin_popup_context_window(const char* id) {
    const auto& palette = RetroTheme::get().get_palette();
    ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, 0.0f);
    ImGui::PushStyleColor(ImGuiCol_PopupBg, to_imu32(palette.bg_dark));
    bool result = ImGui::BeginPopupContextWindow(id);
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
    return result;
}

bool RetroUI::begin_tooltip() {
    const auto& palette = RetroTheme::get().get_palette();
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleColor(ImGuiCol_PopupBg, to_imu32(palette.bg_dark));
    bool result = ImGui::BeginTooltip();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
    return result;
}

void RetroUI::end_tooltip() {
    ImGui::EndTooltip();
}

void RetroUI::set_tooltip(const char* text) {
    const auto& palette = RetroTheme::get().get_palette();
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleColor(ImGuiCol_PopupBg, to_imu32(palette.bg_dark));
    ImGui::SetTooltip("%s", text);
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
}

void RetroUI::draw_rect(const RetroRect& rect, const RetroColor& color, f32 border_thickness) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 min(rect.x, rect.y);
    ImVec2 max(rect.x + rect.w, rect.y + rect.h);
    dl->AddRect(min, max, to_imu32(color), 0.0f, 0, border_thickness > 0 ? border_thickness : 1.0f);
}

void RetroUI::draw_rect_filled(const RetroRect& rect, const RetroColor& color) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 min(rect.x, rect.y);
    ImVec2 max(rect.x + rect.w, rect.y + rect.h);
    dl->AddRectFilled(min, max, to_imu32(color));
}

void RetroUI::draw_rect_border(const RetroRect& rect, const RetroColor& color, f32 thickness) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 min(rect.x, rect.y);
    ImVec2 max(rect.x + rect.w, rect.y + rect.h);
    dl->AddRect(min, max, to_imu32(color), 0.0f, 0, thickness);
}

void RetroUI::draw_line(const Vec2& a, const Vec2& b, const RetroColor& color, f32 thickness) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddLine(to_imvec2(a), to_imvec2(b), to_imu32(color), thickness);
}

void RetroUI::draw_text(const Vec2& pos, const char* text, const RetroColor& color, RetroTextAlign align) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 text_size = ImGui::CalcTextSize(text);
    ImVec2 draw_pos = to_imvec2(pos);

    switch (align) {
        case RetroTextAlign::Center:
            draw_pos.x -= text_size.x * 0.5f;
            break;
        case RetroTextAlign::Right:
            draw_pos.x -= text_size.x;
            break;
        default:
            break;
    }

    dl->AddText(draw_pos, to_imu32(color), text);
}

void RetroUI::draw_icon(const Vec2& pos, RetroIconType icon, const RetroColor& color, f32 size) {
    draw_icon_internal(pos, icon, color, size);
}

void RetroUI::draw_icon_internal(const Vec2& pos, RetroIconType icon, const RetroColor& color, f32 size) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImU32 col = to_imu32(color);
    f32 hs = size * 0.5f;

    ImVec2 center(pos.x, pos.y);

    switch (icon) {
        case RetroIconType::ArrowLeft: {
            ImVec2 points[3] = {
                ImVec2(center.x + hs, center.y - hs),
                ImVec2(center.x - hs, center.y),
                ImVec2(center.x + hs, center.y + hs)
            };
            dl->AddTriangleFilled(points[0], points[1], points[2], col);
            break;
        }
        case RetroIconType::ArrowRight: {
            ImVec2 points[3] = {
                ImVec2(center.x - hs, center.y - hs),
                ImVec2(center.x + hs, center.y),
                ImVec2(center.x - hs, center.y + hs)
            };
            dl->AddTriangleFilled(points[0], points[1], points[2], col);
            break;
        }
        case RetroIconType::ArrowUp: {
            ImVec2 points[3] = {
                ImVec2(center.x, center.y - hs),
                ImVec2(center.x + hs, center.y + hs),
                ImVec2(center.x - hs, center.y + hs)
            };
            dl->AddTriangleFilled(points[0], points[1], points[2], col);
            break;
        }
        case RetroIconType::ArrowDown: {
            ImVec2 points[3] = {
                ImVec2(center.x - hs, center.y - hs),
                ImVec2(center.x + hs, center.y - hs),
                ImVec2(center.x, center.y + hs)
            };
            dl->AddTriangleFilled(points[0], points[1], points[2], col);
            break;
        }
        case RetroIconType::Check: {
            f32 t = size * 0.15f;
            dl->AddLine(ImVec2(center.x - hs, center.y), ImVec2(center.x - hs * 0.3f, center.y + hs * 0.7f), col, t);
            dl->AddLine(ImVec2(center.x - hs * 0.3f, center.y + hs * 0.7f), ImVec2(center.x + hs, center.y - hs * 0.5f), col, t);
            break;
        }
        case RetroIconType::Cross: {
            f32 t = size * 0.15f;
            dl->AddLine(ImVec2(center.x - hs, center.y - hs), ImVec2(center.x + hs, center.y + hs), col, t);
            dl->AddLine(ImVec2(center.x + hs, center.y - hs), ImVec2(center.x - hs, center.y + hs), col, t);
            break;
        }
        case RetroIconType::Plus: {
            f32 t = size * 0.15f;
            dl->AddLine(ImVec2(center.x, center.y - hs), ImVec2(center.x, center.y + hs), col, t);
            dl->AddLine(ImVec2(center.x - hs, center.y), ImVec2(center.x + hs, center.y), col, t);
            break;
        }
        case RetroIconType::Minus: {
            f32 t = size * 0.15f;
            dl->AddLine(ImVec2(center.x - hs, center.y), ImVec2(center.x + hs, center.y), col, t);
            break;
        }
        case RetroIconType::Menu: {
            f32 t = size * 0.12f;
            f32 gap = size * 0.25f;
            dl->AddLine(ImVec2(center.x - hs, center.y - gap), ImVec2(center.x + hs, center.y - gap), col, t);
            dl->AddLine(ImVec2(center.x - hs, center.y), ImVec2(center.x + hs, center.y), col, t);
            dl->AddLine(ImVec2(center.x - hs, center.y + gap), ImVec2(center.x + hs, center.y + gap), col, t);
            break;
        }
        case RetroIconType::Heart: {
            f32 qhs = hs * 0.5f;
            dl->AddCircleFilled(ImVec2(center.x - qhs, center.y - qhs * 0.3f), qhs, col, 8);
            dl->AddCircleFilled(ImVec2(center.x + qhs, center.y - qhs * 0.3f), qhs, col, 8);
            ImVec2 points[3] = {
                ImVec2(center.x - hs, center.y),
                ImVec2(center.x, center.y + hs),
                ImVec2(center.x + hs, center.y)
            };
            dl->AddTriangleFilled(points[0], points[1], points[2], col);
            break;
        }
        case RetroIconType::Star: {
            f32 inner = hs * 0.4f;
            ImVec2 points[10];
            for (int i = 0; i < 10; i++) {
                f32 r = (i % 2 == 0) ? hs : inner;
                f32 angle = (i * 36.0f - 90.0f) * 3.14159f / 180.0f;
                points[i] = ImVec2(center.x + r * cosf(angle), center.y + r * sinf(angle));
            }
            dl->AddConvexPolyFilled(points, 10, col);
            break;
        }
        case RetroIconType::Sword: {
            f32 t = size * 0.12f;
            dl->AddLine(ImVec2(center.x - hs, center.y + hs), ImVec2(center.x + hs, center.y - hs), col, t);
            dl->AddLine(ImVec2(center.x + hs * 0.5f, center.y - hs * 0.1f), ImVec2(center.x + hs * 0.1f, center.y - hs * 0.5f), col, t);
            dl->AddRectFilled(ImVec2(center.x - hs * 0.6f, center.y + hs * 0.3f), ImVec2(center.x - hs * 0.3f, center.y + hs * 0.6f), col);
            break;
        }
        case RetroIconType::Shield: {
            ImVec2 points[5] = {
                ImVec2(center.x, center.y - hs),
                ImVec2(center.x + hs, center.y - hs * 0.5f),
                ImVec2(center.x + hs * 0.7f, center.y + hs * 0.5f),
                ImVec2(center.x, center.y + hs),
                ImVec2(center.x - hs * 0.7f, center.y + hs * 0.5f)
            };
            dl->AddConvexPolyFilled(points, 5, col);
            break;
        }
        case RetroIconType::Coin: {
            dl->AddCircleFilled(center, hs, col, 12);
            dl->AddCircle(center, hs * 0.7f, to_imu32(color.darken(40)), 12, size * 0.1f);
            break;
        }
        case RetroIconType::Key: {
            f32 t = size * 0.15f;
            dl->AddCircle(ImVec2(center.x - hs * 0.4f, center.y - hs * 0.2f), hs * 0.4f, col, 8, t);
            dl->AddLine(ImVec2(center.x, center.y), ImVec2(center.x + hs, center.y + hs * 0.3f), col, t);
            dl->AddLine(ImVec2(center.x + hs * 0.6f, center.y + hs * 0.18f), ImVec2(center.x + hs * 0.6f, center.y + hs * 0.5f), col, t);
            break;
        }
        case RetroIconType::Chest: {
            dl->AddRectFilled(ImVec2(center.x - hs, center.y - hs * 0.2f), ImVec2(center.x + hs, center.y + hs), col);
            dl->AddRectFilled(ImVec2(center.x - hs, center.y - hs * 0.6f), ImVec2(center.x + hs, center.y - hs * 0.2f), to_imu32(color.lighten(30)));
            dl->AddRectFilled(ImVec2(center.x - hs * 0.2f, center.y), ImVec2(center.x + hs * 0.2f, center.y + hs * 0.3f), to_imu32(color.darken(50)));
            break;
        }
        default:
            dl->AddRectFilled(ImVec2(center.x - hs, center.y - hs), ImVec2(center.x + hs, center.y + hs), col);
            break;
    }
}

void RetroUI::draw_rpg_frame(const RetroRect& rect, const RetroColor& bg, const RetroColor& border_light, const RetroColor& border_dark) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 min(rect.x, rect.y);
    ImVec2 max(rect.x + rect.w, rect.y + rect.h);
    const auto& config = RetroTheme::get().get_config();

    dl->AddRectFilled(min, max, to_imu32(bg));

    draw_rpg_border(dl, min, max,
        to_imu32(border_dark.darken(20)),
        to_imu32(border_light),
        to_imu32(border_dark),
        to_imu32(border_dark.lighten(10)),
        config.border_thickness);
}

void RetroUI::draw_rpg_button(const RetroRect& rect, const RetroColor& face, bool pressed) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 min(rect.x, rect.y);
    ImVec2 max(rect.x + rect.w, rect.y + rect.h);
    const auto& config = RetroTheme::get().get_config();

    dl->AddRectFilled(min, max, to_imu32(face));

    ImU32 light = to_imu32(pressed ? face.darken(30) : face.lighten(50));
    ImU32 dark = to_imu32(pressed ? face.lighten(30) : face.darken(50));

    draw_pixel_border(dl, min, max, pressed ? dark : light, pressed ? light : dark, config.border_thickness);
}

void RetroUI::draw_scanlines(const RetroRect& rect, f32 opacity) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImU32 line_color = IM_COL32(0, 0, 0, static_cast<int>(opacity * 255));

    for (f32 y = rect.y; y < rect.y + rect.h; y += 2.0f) {
        dl->AddLine(ImVec2(rect.x, y), ImVec2(rect.x + rect.w, y), line_color, 1.0f);
    }
}

void RetroUI::draw_pixel_grid(const RetroRect& rect, f32 grid_size, const RetroColor& color) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImU32 col = to_imu32(color);

    for (f32 x = rect.x; x <= rect.x + rect.w; x += grid_size) {
        dl->AddLine(ImVec2(x, rect.y), ImVec2(x, rect.y + rect.h), col, 1.0f);
    }
    for (f32 y = rect.y; y <= rect.y + rect.h; y += grid_size) {
        dl->AddLine(ImVec2(rect.x, y), ImVec2(rect.x + rect.w, y), col, 1.0f);
    }
}

bool RetroUI::is_item_hovered() { return ImGui::IsItemHovered(); }
bool RetroUI::is_item_active() { return ImGui::IsItemActive(); }
bool RetroUI::is_item_clicked(i32 button) { return ImGui::IsItemClicked(button); }
bool RetroUI::is_item_focused() { return ImGui::IsItemFocused(); }
Vec2 RetroUI::get_item_rect_min() { return from_imvec2(ImGui::GetItemRectMin()); }
Vec2 RetroUI::get_item_rect_max() { return from_imvec2(ImGui::GetItemRectMax()); }
Vec2 RetroUI::get_item_rect_size() { return from_imvec2(ImGui::GetItemRectSize()); }

Vec2 RetroUI::get_cursor_pos() { return from_imvec2(ImGui::GetCursorPos()); }
void RetroUI::set_cursor_pos(const Vec2& pos) { ImGui::SetCursorPos(to_imvec2(pos)); }
Vec2 RetroUI::get_content_region_avail() { return from_imvec2(ImGui::GetContentRegionAvail()); }
Vec2 RetroUI::get_window_pos() { return from_imvec2(ImGui::GetWindowPos()); }
Vec2 RetroUI::get_window_size() { return from_imvec2(ImGui::GetWindowSize()); }

void RetroUI::set_next_item_width(f32 width) { ImGui::SetNextItemWidth(width); }
void RetroUI::push_item_width(f32 width) { ImGui::PushItemWidth(width); }
void RetroUI::pop_item_width() { ImGui::PopItemWidth(); }

void RetroUI::push_id(const char* id) { ImGui::PushID(id); }
void RetroUI::push_id(i32 id) { ImGui::PushID(id); }
void RetroUI::pop_id() { ImGui::PopID(); }

void RetroUI::push_style_color(const RetroColor& color) {
    ImGui::PushStyleColor(ImGuiCol_Text, to_imu32(color));
}

void RetroUI::pop_style_color(i32 count) {
    ImGui::PopStyleColor(count);
}

u32 RetroUI::get_id(const char* str) { return ImGui::GetID(str); }

bool RetroUI::is_window_focused() { return ImGui::IsWindowFocused(); }
bool RetroUI::is_window_hovered() { return ImGui::IsWindowHovered(); }

f32 RetroUI::get_scroll_x() { return ImGui::GetScrollX(); }
f32 RetroUI::get_scroll_y() { return ImGui::GetScrollY(); }
f32 RetroUI::get_scroll_max_x() { return ImGui::GetScrollMaxX(); }
f32 RetroUI::get_scroll_max_y() { return ImGui::GetScrollMaxY(); }
void RetroUI::set_scroll_x(f32 scroll) { ImGui::SetScrollX(scroll); }
void RetroUI::set_scroll_y(f32 scroll) { ImGui::SetScrollY(scroll); }
void RetroUI::set_scroll_here_x(f32 center_ratio) { ImGui::SetScrollHereX(center_ratio); }
void RetroUI::set_scroll_here_y(f32 center_ratio) { ImGui::SetScrollHereY(center_ratio); }

void RetroUI::set_keyboard_focus_here(i32 offset) { ImGui::SetKeyboardFocusHere(offset); }

bool RetroUI::is_key_pressed(i32 key) { return ImGui::IsKeyPressed((ImGuiKey)key); }
bool RetroUI::is_key_down(i32 key) { return ImGui::IsKeyDown((ImGuiKey)key); }
bool RetroUI::is_key_released(i32 key) { return ImGui::IsKeyReleased((ImGuiKey)key); }

bool RetroUI::is_mouse_clicked(i32 button) { return ImGui::IsMouseClicked(button); }
bool RetroUI::is_mouse_down(i32 button) { return ImGui::IsMouseDown(button); }
bool RetroUI::is_mouse_released(i32 button) { return ImGui::IsMouseReleased(button); }
bool RetroUI::is_mouse_double_clicked(i32 button) { return ImGui::IsMouseDoubleClicked(button); }
Vec2 RetroUI::get_mouse_pos() { return from_imvec2(ImGui::GetMousePos()); }
Vec2 RetroUI::get_mouse_drag_delta(i32 button, f32 lock_threshold) { return from_imvec2(ImGui::GetMouseDragDelta(button, lock_threshold)); }
void RetroUI::reset_mouse_drag_delta(i32 button) { ImGui::ResetMouseDragDelta(button); }

} // namespace tvk
