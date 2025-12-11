cmake_minimum_required(VERSION 3.20)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(WIN32)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDebugDLL")
    else()
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")
    endif()
    
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS OFF)
endif()

# Enable Objective-C++ on Apple platforms for file dialogs
if(APPLE)
    project(tinyvk LANGUAGES C CXX OBJC OBJCXX)
else()
    project(tinyvk LANGUAGES C CXX)
endif()

# Debug Mode -----------------------------------------------
add_compile_definitions($<$<CONFIG:Debug>:TVK_DEBUG_BUILD>)
# ----------------------------------------------------------

# Renderer Backend -----------------------------------------
# NOTE: 0nly one of the following should be defined
add_compile_definitions(TVK_RENDERER_BACKEND_VULKAN)
# add_compile_definitions(TVK_RENDERER_BACKEND_D3DX12) # not implemented
# add_compile_definitions(TVK_RENDERER_BACKEND_METAL) # not implemented
# ----------------------------------------------------------

set(CMAKE_CXX_FLAGS_RELEASE "")
set(CMAKE_C_FLAGS_RELEASE "")
set(CMAKE_EXE_LINKER_FLAGS_RELEASE "")

if(APPLE)
    message("Building on Apple macOS or iOS")
    if(DEFINED ENV{VULKAN_SDK})
        set(VULKAN_PATH $ENV{VULKAN_SDK})
        message("Found Vulkan SDK at: ${VULKAN_PATH}")
    else()
        file(GLOB VULKAN_SDK_PATHS "$ENV{HOME}/VulkanSDK/*/macOS")
        list(SORT VULKAN_SDK_PATHS)
        list(REVERSE VULKAN_SDK_PATHS)
        list(GET VULKAN_SDK_PATHS 0 VULKAN_PATH)
        if(VULKAN_PATH)
            message("Auto-detected Vulkan SDK at: ${VULKAN_PATH}")
        else()
            message(FATAL_ERROR "Vulkan SDK not found. Please install it or set VULKAN_SDK environment variable")
        endif()
    endif()
    add_definitions(-DTVK_PLATFORM_APPLE)
    set(ENV{VK_ICD_FILENAMES} "${VULKAN_PATH}/share/vulkan/icd.d/MoltenVK_icd.json")
    set(ENV{VK_LAYER_PATH} "${VULKAN_PATH}/share/vulkan/explicit_layer.d")

elseif(WIN32)
    message(STATUS "Building on Windows")
    if(NOT DEFINED ENV{VULKAN_SDK})
        message(FATAL_ERROR "VULKAN_SDK environment variable not set. Please install Vulkan SDK.")
    endif()
    set(VULKAN_PATH $ENV{VULKAN_SDK})
    message(STATUS "Found Vulkan SDK at: ${VULKAN_PATH}")
    add_definitions(-DTVK_PLATFORM_WINDOWS)
    add_definitions(-DNOMINMAX)
    add_definitions(-DWIN32_LEAN_AND_MEAN)

elseif(UNIX)
    message("Building on Unix-like OS (including Linux)")
    add_definitions(-DTVK_PLATFORM_LINUX)

else()
set(CMAKE_CXX_FLAGS_RELEASE "")
set(CMAKE_C_FLAGS_RELEASE "")
endif()

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

# Vendors --------------------------------------------------
# GLFW
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(vendors/glfw)

# GLM
add_subdirectory(vendors/glm)

# ImGui
set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/vendors/imgui/imgui)
set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp
)
add_library(imgui STATIC ${IMGUI_SOURCES})
target_include_directories(imgui PUBLIC 
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    ${VULKAN_PATH}/include
)
target_link_directories(imgui PUBLIC ${VULKAN_PATH}/lib)
target_link_libraries(imgui PUBLIC glfw)
if(APPLE)
    target_link_libraries(imgui PUBLIC vulkan)
elseif(WIN32)
    target_link_libraries(imgui PUBLIC vulkan-1)
else()
    target_link_libraries(imgui PUBLIC vulkan)
endif()

# VMA (Header-only)
set(VMA_DIR ${CMAKE_SOURCE_DIR}/vendors/vma)

# JSON (Header-only)
set(JSON_DIR ${CMAKE_SOURCE_DIR}/vendors/json)

# STB (Header-only)
set(STB_DIR ${CMAKE_SOURCE_DIR}/vendors/stb)
# ----------------------------------------------------------

# TinyVK Library -------------------------------------------
set(TINYVK_SOURCES
    src/core/application.cpp
    src/core/window.cpp
    src/core/input.cpp
    src/core/file_dialog.cpp
    src/renderer/context.cpp
    src/renderer/renderer.cpp
    src/renderer/texture.cpp
    src/renderer/buffer.cpp
    src/ui/imgui_layer.cpp
)

add_library(tinyvk STATIC ${TINYVK_SOURCES})
target_include_directories(tinyvk PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${VULKAN_PATH}/include
    ${VMA_DIR}/include
    ${JSON_DIR}/include
    ${STB_DIR}
)
target_link_directories(tinyvk PUBLIC ${VULKAN_PATH}/lib)
target_link_libraries(tinyvk PUBLIC 
    glfw
    glm
    imgui
)
if(APPLE)
    target_link_libraries(tinyvk PUBLIC vulkan)
    # Link AppKit for file dialogs
    target_link_libraries(tinyvk PUBLIC "-framework AppKit")
    # Enable Objective-C++ for file_dialog.cpp on macOS
    set_source_files_properties(src/core/file_dialog.cpp PROPERTIES LANGUAGE OBJCXX)
elseif(WIN32)
    target_link_libraries(tinyvk PUBLIC vulkan-1)
else()
    target_link_libraries(tinyvk PUBLIC vulkan)
endif()
# ----------------------------------------------------------

# Sandbox Application --------------------------------------
add_executable(sandbox sandbox/main.cpp)
target_link_libraries(sandbox PRIVATE tinyvk)
# ----------------------------------------------------------