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

project(tinyvk)

# Debug Mode -----------------------------------------------
add_compile_definitions($<$<CONFIG:Debug>:TVK_DEBUG>)
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