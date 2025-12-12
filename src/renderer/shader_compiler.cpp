/**
 * @file shader_compiler.cpp
 * @brief Runtime GLSL to SPIR-V compilation implementation
 */

#include "tinyvk/renderer/shader_compiler.h"
#include "tinyvk/renderer/renderer.h"
#include "tinyvk/renderer/context.h"
#include "tinyvk/core/log.h"

#include <shaderc/shaderc.hpp>

namespace tvk {

std::vector<u32> ShaderCompiler::CompileGLSL(const std::string& source, ShaderStage stage, const std::string& name) {
    shaderc::Compiler compiler;
    shaderc::CompileOptions options;
    
    options.SetOptimizationLevel(shaderc_optimization_level_performance);
    
    #ifdef TVK_DEBUG_BUILD
    options.SetGenerateDebugInfo();
    #endif
    
    shaderc_shader_kind kind;
    switch (stage) {
        case ShaderStage::Vertex:
            kind = shaderc_vertex_shader;
            break;
        case ShaderStage::Fragment:
            kind = shaderc_fragment_shader;
            break;
        case ShaderStage::Compute:
            kind = shaderc_compute_shader;
            break;
        case ShaderStage::Geometry:
            kind = shaderc_geometry_shader;
            break;
        case ShaderStage::TessControl:
            kind = shaderc_tess_control_shader;
            break;
        case ShaderStage::TessEvaluation:
            kind = shaderc_tess_evaluation_shader;
            break;
        default:
            TVK_LOG_ERROR("Unknown shader stage");
            return {};
    }
    
    shaderc::SpvCompilationResult result = compiler.CompileGlslToSpv(
        source, kind, name.c_str(), options
    );
    
    if (result.GetCompilationStatus() != shaderc_compilation_status_success) {
        TVK_LOG_ERROR("Shader compilation failed: {}", result.GetErrorMessage());
        return {};
    }
    
    TVK_LOG_INFO("Compiled shader '{}' successfully", name);
    
    return {result.cbegin(), result.cend()};
}

VkShaderModule ShaderCompiler::CreateShaderModule(Renderer* renderer, const std::vector<u32>& spirv) {
    if (spirv.empty()) {
        return VK_NULL_HANDLE;
    }
    
    auto& ctx = renderer->GetContext();
    VkDevice device = ctx.GetDevice();
    
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = spirv.size() * sizeof(u32);
    createInfo.pCode = spirv.data();
    
    VkShaderModule shaderModule;
    if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        TVK_LOG_ERROR("Failed to create shader module");
        return VK_NULL_HANDLE;
    }
    
    return shaderModule;
}

VkShaderModule ShaderCompiler::CreateShaderModuleFromGLSL(Renderer* renderer, const std::string& glsl, ShaderStage stage, const std::string& name) {
    auto spirv = CompileGLSL(glsl, stage, name);
    return CreateShaderModule(renderer, spirv);
}

} // namespace tvk
