/**
 * @file tinyvk.h
 * @brief Main header for TinyVK - A lightweight application framework with ImGui
 * 
 * TinyVK provides a simple API for creating GUI applications. The engine handles
 * all Vulkan/rendering internals - you just write ImGui code.
 * 
 * Example:
 * @code
 * #include <tinyvk/tinyvk.h>
 * #include <imgui.h>
 * 
 * class MyApp : public tvk::App {
 * protected:
 *     void OnUI() override {
 *         ImGui::Begin("Hello");
 *         ImGui::Text("Hello, World!");
 *         ImGui::End();
 *     }
 * };
 * 
 * TVK_MAIN(MyApp, "My Application", 1280, 720)
 * @endcode
 */

#pragma once

// Core - User-facing API
#include "core/application.h"
#include "core/input.h"
#include "core/log.h"
#include "core/types.h"
#include "core/timer.h"
#include "core/file_dialog.h"

// Texture loading (for displaying images in ImGui)
#include "renderer/texture.h"

// Version info
#define TINYVK_VERSION_MAJOR 1
#define TINYVK_VERSION_MINOR 0
#define TINYVK_VERSION_PATCH 0

namespace tvk {

inline const char* GetVersionString() {
    return "1.0.0";
}

} // namespace tvk
