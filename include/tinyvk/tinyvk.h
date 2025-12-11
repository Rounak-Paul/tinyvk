/**
 * @file tinyvk.h
 * @brief Main header for TinyVK - A lightweight Vulkan renderer with ImGui and GLFW
 * 
 * Include this single header to use the TinyVK library.
 */

#pragma once

// Core
#include "core/application.h"
#include "core/window.h"
#include "core/input.h"
#include "core/log.h"
#include "core/types.h"
#include "core/timer.h"
#include "core/file_dialog.h"

// Renderer
#include "renderer/renderer.h"
#include "renderer/context.h"
#include "renderer/texture.h"
#include "renderer/buffer.h"

// UI
#include "ui/imgui_layer.h"
#include "ui/widgets.h"

// Version info
#define TINYVK_VERSION_MAJOR 1
#define TINYVK_VERSION_MINOR 0
#define TINYVK_VERSION_PATCH 0

namespace tvk {

/**
 * @brief Get the version string of TinyVK
 * @return Version string in format "major.minor.patch"
 */
inline const char* GetVersionString() {
    return "1.0.0";
}

} // namespace tvk
