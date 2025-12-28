/**
 * @file pipeline.h
 * @brief Graphics and compute pipeline wrappers
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
class Buffer;

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

class ComputePipeline {
public:
    ComputePipeline() = default;
    ~ComputePipeline();

    bool Create(Renderer* renderer, const std::string& computeShaderSource);
    void Destroy();
    
    void Bind(VkCommandBuffer cmd);
    
    template<typename T>
    void SetPushConstants(VkCommandBuffer cmd, const T& data) {
        vkCmdPushConstants(cmd, _layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(T), &data);
    }
    
    void Dispatch(VkCommandBuffer cmd, u32 groupCountX, u32 groupCountY, u32 groupCountZ);
    
    VkPipeline GetHandle() const { return _pipeline; }
    VkPipelineLayout GetLayout() const { return _layout; }
    VkDescriptorSetLayout GetDescriptorSetLayout() const { return _descriptorSetLayout; }
    VkDescriptorSet GetDescriptorSet() const { return _descriptorSet; }

    void BindStorageBuffer(u32 binding, Buffer* buffer);
    void BindStorageBuffers(Buffer* buffer0, Buffer* buffer1 = nullptr);
    void UpdateDescriptors();

private:
    bool CreateDescriptorResources();

    Renderer* _renderer = nullptr;
    VkPipeline _pipeline = VK_NULL_HANDLE;
    VkPipelineLayout _layout = VK_NULL_HANDLE;
    VkDescriptorSetLayout _descriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorPool _descriptorPool = VK_NULL_HANDLE;
    VkDescriptorSet _descriptorSet = VK_NULL_HANDLE;
    
    static constexpr u32 MAX_STORAGE_BINDINGS = 4;
    Buffer* _boundBuffers[MAX_STORAGE_BINDINGS] = {};
};

} // namespace tvk
