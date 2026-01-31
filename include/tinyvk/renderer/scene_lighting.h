#pragma once

#include <tinyvk/core/types.h>
#include <tinyvk/renderer/buffer.h>
#include <tinyvk/scene/scene.h>
#include <tinyvk/scene/components.h>
#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

namespace tvk {

class Renderer;
class VulkanContext;

static constexpr u32 MAX_SCENE_LIGHTS = 8;
static constexpr u32 SHADOW_MAP_SIZE = 2048;

struct ShadowPushConstants {
    glm::mat4 model;
    glm::mat4 light_space;
};

struct alignas(16) GPULightData {
    glm::vec4 position_type;
    glm::vec4 direction_range;
    glm::vec4 color_intensity;
    glm::vec4 shadow_params;
};

struct alignas(16) SceneLightingUBO {
    glm::vec4 ambient_color;
    glm::vec4 camera_position;
    glm::mat4 shadow_matrix;
    u32 num_lights;
    u32 shadow_enabled;
    u32 _padding[2];
    GPULightData lights[MAX_SCENE_LIGHTS];
};

class SceneLighting {
public:
    SceneLighting() = default;
    ~SceneLighting();
    
    bool Create(Renderer* renderer, VkRenderPass mainRenderPass);
    void Destroy();
    
    void CollectLights(Scene* scene, const glm::vec3& camera_position);
    void UpdateUBO();
    
    void BeginShadowPass(VkCommandBuffer cmd);
    void EndShadowPass(VkCommandBuffer cmd);
    void BindShadowPipeline(VkCommandBuffer cmd);
    void SetShadowPushConstants(VkCommandBuffer cmd, const glm::mat4& model);
    
    VkDescriptorSet GetDescriptorSet() const { return _descriptorSet; }
    VkDescriptorSetLayout GetDescriptorSetLayout() const { return _descriptorSetLayout; }
    VkRenderPass GetShadowRenderPass() const { return _shadowRenderPass; }
    VkFramebuffer GetShadowFramebuffer() const { return _shadowFramebuffer; }
    
    const glm::mat4& GetShadowMatrix() const { return _uboData.shadow_matrix; }
    bool HasDirectionalLight() const { return _hasDirectionalLight; }
    
    void SetShadowsEnabled(bool enabled) { _shadowsEnabled = enabled; }
    bool GetShadowsEnabled() const { return _shadowsEnabled; }
    
private:
    void CreateDescriptorResources();
    void CreateShadowResources();
    void CreateShadowPipeline();
    void CleanupShadowResources();
    
    Renderer* _renderer = nullptr;
    VulkanContext* _context = nullptr;
    
    SceneLightingUBO _uboData{};
    Ref<Buffer> _uboBuffer;
    
    VkDescriptorSetLayout _descriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorPool _descriptorPool = VK_NULL_HANDLE;
    VkDescriptorSet _descriptorSet = VK_NULL_HANDLE;
    
    VkImage _shadowMap = VK_NULL_HANDLE;
    VkDeviceMemory _shadowMapMemory = VK_NULL_HANDLE;
    VkImageView _shadowMapView = VK_NULL_HANDLE;
    VkSampler _shadowSampler = VK_NULL_HANDLE;
    VkFramebuffer _shadowFramebuffer = VK_NULL_HANDLE;
    VkRenderPass _shadowRenderPass = VK_NULL_HANDLE;
    
    VkPipeline _shadowPipeline = VK_NULL_HANDLE;
    VkPipelineLayout _shadowPipelineLayout = VK_NULL_HANDLE;
    
    glm::vec3 _directionalLightDir = glm::vec3(0.0f, -1.0f, 0.0f);
    bool _hasDirectionalLight = false;
    bool _shadowsEnabled = true;
};

} // namespace tvk
