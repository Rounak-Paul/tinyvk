/**
 * @file shader_compiler.h
 * @brief Runtime GLSL to SPIR-V compilation using shaderc
 */

#pragma once

#include "../core/types.h"
#include <vulkan/vulkan.h>
#include <string>
#include <vector>

namespace tvk {

class Renderer;

enum class ShaderStage {
    Vertex,
    Fragment,
    Compute,
    Geometry,
    TessControl,
    TessEvaluation
};

class ShaderCompiler {
public:
    ShaderCompiler() = default;
    ~ShaderCompiler() = default;

    static std::vector<u32> CompileGLSL(const std::string& source, ShaderStage stage, const std::string& name = "shader");
    
    static VkShaderModule CreateShaderModule(Renderer* renderer, const std::vector<u32>& spirv);
    static VkShaderModule CreateShaderModuleFromGLSL(Renderer* renderer, const std::string& glsl, ShaderStage stage, const std::string& name = "shader");
};

} // namespace tvk
