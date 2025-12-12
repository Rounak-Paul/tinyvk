/**
 * @file pipeline.h
 * @brief Graphics pipeline wrapper
 */

#pragma once

#include "../core/types.h"
#include "vertex.h"
#include <vulkan/vulkan.h>
#include <string>
#include <vector>

namespace tvk {

class Renderer;
class VulkanContext;

struct PushConstants {
    glm::mat4 model;
    glm::mat4 view_projection;
};

class Pipeline {
public:
    Pipeline() = default;
    ~Pipeline();

    bool Create(Renderer* renderer, VkRenderPass renderPass, const std::string& vertShaderSource, const std::string& fragShaderSource);
    void Destroy();
    
    void Bind(VkCommandBuffer cmd);
    void SetPushConstants(VkCommandBuffer cmd, const PushConstants& constants);
    
    VkPipeline GetHandle() const { return _pipeline; }
    VkPipelineLayout GetLayout() const { return _layout; }

private:
    Renderer* _renderer = nullptr;
    VkPipeline _pipeline = VK_NULL_HANDLE;
    VkPipelineLayout _layout = VK_NULL_HANDLE;
};

} // namespace tvk
