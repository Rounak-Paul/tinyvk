/**
 * @file tinyvk.h
 * @brief Main header for TinyVK - A lightweight UI library with Vulkan compute and rendering API
 * 
 * TinyVK provides a simple API for creating GUI applications with custom compute
 * and rendering capabilities. The library handles Vulkan internals - you write ImGui code
 * and optionally use the compute/rendering APIs for custom GPU work.
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
#include "core/result.h"
#include "core/types.h"
#include "core/timer.h"
#include "core/file_dialog.h"

// Rendering API
#include "renderer/renderer.h"
#include "renderer/context.h"
#include "renderer/texture.h"
#include "renderer/buffer.h"
#include "renderer/vertex.h"
#include "renderer/pipeline.h"
#include "renderer/shader_compiler.h"
#include "renderer/shaders.h"

// Assets - Embedded fonts and icons
#include "assets/fonts.h"
#include "assets/icons_font_awesome.h"

// UI - Custom rendering widgets
#include "ui/render_widget.h"

// Version info
#define TINYVK_VERSION_MAJOR 1
#define TINYVK_VERSION_MINOR 0
#define TINYVK_VERSION_PATCH 0

namespace tvk {

inline const char* GetVersionString() {
    return "1.0.0";
}

} // namespace tvk
