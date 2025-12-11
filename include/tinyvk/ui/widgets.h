/**
 * @file widgets.h
 * @brief Custom ImGui widgets for TinyVK
 */

#pragma once

#include "../core/types.h"
#include "../renderer/texture.h"
#include <imgui.h>
#include <string>

namespace tvk {
namespace ui {

/**
 * @brief Display a texture as an ImGui image
 * @param texture The texture to display
 * @param size Display size (0,0 = use texture size)
 * @param uv0 UV coordinates for top-left corner
 * @param uv1 UV coordinates for bottom-right corner
 * @param tintColor Color tint
 * @param borderColor Border color
 */
inline void Image(const Ref<Texture>& texture, 
                  const Vec2& size = Vec2(0, 0),
                  const Vec2& uv0 = Vec2(0, 0),
                  const Vec2& uv1 = Vec2(1, 1),
                  const Vec4& tintColor = Vec4(1, 1, 1, 1),
                  const Vec4& borderColor = Vec4(0, 0, 0, 0)) {
    if (!texture || !texture->IsValid()) return;
    
    Vec2 displaySize = size;
    if (displaySize.x <= 0) displaySize.x = static_cast<float>(texture->GetWidth());
    if (displaySize.y <= 0) displaySize.y = static_cast<float>(texture->GetHeight());
    
    ImGui::Image(
        reinterpret_cast<ImTextureID>(texture->GetImGuiTextureID()),
        ImVec2(displaySize.x, displaySize.y),
        ImVec2(uv0.x, uv0.y),
        ImVec2(uv1.x, uv1.y),
        ImVec4(tintColor.x, tintColor.y, tintColor.z, tintColor.w),
        ImVec4(borderColor.x, borderColor.y, borderColor.z, borderColor.w)
    );
}

/**
 * @brief Display a texture as an ImGui image button
 * @param id Unique string ID for the button
 * @param texture The texture to display
 * @param size Display size
 * @param uv0 UV coordinates for top-left corner
 * @param uv1 UV coordinates for bottom-right corner
 * @param bgColor Background color
 * @param tintColor Color tint
 * @return true if clicked
 */
inline bool ImageButton(const char* id,
                        const Ref<Texture>& texture,
                        const Vec2& size,
                        const Vec2& uv0 = Vec2(0, 0),
                        const Vec2& uv1 = Vec2(1, 1),
                        const Vec4& bgColor = Vec4(0, 0, 0, 0),
                        const Vec4& tintColor = Vec4(1, 1, 1, 1)) {
    if (!texture || !texture->IsValid()) return false;
    
    return ImGui::ImageButton(
        id,
        reinterpret_cast<ImTextureID>(texture->GetImGuiTextureID()),
        ImVec2(size.x, size.y),
        ImVec2(uv0.x, uv0.y),
        ImVec2(uv1.x, uv1.y),
        ImVec4(bgColor.x, bgColor.y, bgColor.z, bgColor.w),
        ImVec4(tintColor.x, tintColor.y, tintColor.z, tintColor.w)
    );
}

/**
 * @brief Display a texture that fills available width, maintaining aspect ratio
 * @param texture The texture to display
 * @param maxHeight Maximum height (0 = no limit)
 */
inline void ImageFitWidth(const Ref<Texture>& texture, float maxHeight = 0) {
    if (!texture || !texture->IsValid()) return;
    
    float availWidth = ImGui::GetContentRegionAvail().x;
    float aspect = static_cast<float>(texture->GetWidth()) / static_cast<float>(texture->GetHeight());
    float height = availWidth / aspect;
    
    if (maxHeight > 0 && height > maxHeight) {
        height = maxHeight;
        availWidth = height * aspect;
    }
    
    Image(texture, Vec2(availWidth, height));
}

/**
 * @brief Display a texture centered in available space
 * @param texture The texture to display  
 * @param size Display size (0,0 = use texture size)
 */
inline void ImageCentered(const Ref<Texture>& texture, const Vec2& size = Vec2(0, 0)) {
    if (!texture || !texture->IsValid()) return;
    
    Vec2 displaySize = size;
    if (displaySize.x <= 0) displaySize.x = static_cast<float>(texture->GetWidth());
    if (displaySize.y <= 0) displaySize.y = static_cast<float>(texture->GetHeight());
    
    ImVec2 avail = ImGui::GetContentRegionAvail();
    float offsetX = (avail.x - displaySize.x) * 0.5f;
    float offsetY = (avail.y - displaySize.y) * 0.5f;
    
    if (offsetX > 0) ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offsetX);
    if (offsetY > 0) ImGui::SetCursorPosY(ImGui::GetCursorPosY() + offsetY);
    
    Image(texture, displaySize);
}

/**
 * @brief Tooltip with text
 */
inline void Tooltip(const char* text) {
    if (ImGui::IsItemHovered()) {
        ImGui::BeginTooltip();
        ImGui::TextUnformatted(text);
        ImGui::EndTooltip();
    }
}

/**
 * @brief Tooltip with texture preview
 */
inline void TooltipImage(const Ref<Texture>& texture, const Vec2& size = Vec2(128, 128)) {
    if (ImGui::IsItemHovered() && texture && texture->IsValid()) {
        ImGui::BeginTooltip();
        Image(texture, size);
        ImGui::EndTooltip();
    }
}

/**
 * @brief Separator with label
 */
inline void SeparatorText(const char* label) {
    ImGui::Separator();
    ImGui::Text("%s", label);
    ImGui::Separator();
}

/**
 * @brief Property-style row with label on left
 */
inline bool PropertyRow(const char* label, float labelWidth = 120.0f) {
    ImGui::PushID(label);
    ImGui::Columns(2, nullptr, false);
    ImGui::SetColumnWidth(0, labelWidth);
    ImGui::Text("%s", label);
    ImGui::NextColumn();
    ImGui::PushItemWidth(-1);
    return true;
}

inline void EndPropertyRow() {
    ImGui::PopItemWidth();
    ImGui::Columns(1);
    ImGui::PopID();
}

/**
 * @brief Helper for Vec3 input with color styling
 */
inline bool Vec3Control(const char* label, Vec3& values, float resetValue = 0.0f, float columnWidth = 100.0f) {
    ImGui::PushID(label);
    
    ImGui::Columns(2, nullptr, false);
    ImGui::SetColumnWidth(0, columnWidth);
    ImGui::Text("%s", label);
    ImGui::NextColumn();
    
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
    
    float lineHeight = ImGui::GetFrameHeight();
    ImVec2 buttonSize = {lineHeight + 3.0f, lineHeight};
    float dragWidth = (ImGui::CalcItemWidth() - buttonSize.x * 3 - 4.0f) / 3.0f;
    
    bool changed = false;
    
    // X
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.1f, 0.15f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.2f, 0.2f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.8f, 0.1f, 0.15f, 1.0f));
    if (ImGui::Button("X", buttonSize)) { values.x = resetValue; changed = true; }
    ImGui::PopStyleColor(3);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(dragWidth);
    if (ImGui::DragFloat("##X", &values.x, 0.1f)) changed = true;
    ImGui::SameLine();
    
    // Y
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.7f, 0.2f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.8f, 0.3f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.2f, 0.7f, 0.2f, 1.0f));
    if (ImGui::Button("Y", buttonSize)) { values.y = resetValue; changed = true; }
    ImGui::PopStyleColor(3);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(dragWidth);
    if (ImGui::DragFloat("##Y", &values.y, 0.1f)) changed = true;
    ImGui::SameLine();
    
    // Z
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.1f, 0.25f, 0.8f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.2f, 0.35f, 0.9f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.25f, 0.8f, 1.0f));
    if (ImGui::Button("Z", buttonSize)) { values.z = resetValue; changed = true; }
    ImGui::PopStyleColor(3);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(dragWidth);
    if (ImGui::DragFloat("##Z", &values.z, 0.1f)) changed = true;
    
    ImGui::PopStyleVar();
    ImGui::Columns(1);
    ImGui::PopID();
    
    return changed;
}

/**
 * @brief Toggle button
 */
inline bool ToggleButton(const char* label, bool* value) {
    bool clicked = false;
    
    ImVec4 activeColor = ImGui::GetStyle().Colors[ImGuiCol_ButtonActive];
    ImVec4 normalColor = ImGui::GetStyle().Colors[ImGuiCol_Button];
    
    if (*value) {
        ImGui::PushStyleColor(ImGuiCol_Button, activeColor);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, activeColor);
    }
    
    if (ImGui::Button(label)) {
        *value = !*value;
        clicked = true;
    }
    
    if (*value) {
        ImGui::PopStyleColor(2);
    }
    
    return clicked;
}

} // namespace ui
} // namespace tvk
