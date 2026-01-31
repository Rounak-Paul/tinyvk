#pragma once

#include <tinyvk/core/types.h>
#include <tinyvk/renderer/mesh.h>
#include <tinyvk/renderer/pipeline.h>
#include <glm/glm.hpp>
#include <string>
#include <vector>

namespace tvk {

class Renderer;
class VulkanContext;

struct SkyboxFaces {
    std::string right;
    std::string left;
    std::string top;
    std::string bottom;
    std::string front;
    std::string back;
};

class Skybox {
public:
    Skybox() = default;
    ~Skybox();
    
    bool Create(Renderer* renderer, VkRenderPass renderPass);
    bool LoadFromFiles(const SkyboxFaces& faces);
    bool LoadFromColor(const glm::vec3& top_color, const glm::vec3& bottom_color);
    void Destroy();
    
    void Render(VkCommandBuffer cmd, const glm::mat4& view_projection);
    
    bool IsValid() const { return _cubemapView != VK_NULL_HANDLE; }
    
private:
    void CreateCubeMesh();
    
    Renderer* _renderer = nullptr;
    VulkanContext* _context = nullptr;
    
    VkImage _cubemap = VK_NULL_HANDLE;
    VmaAllocation _cubemapAllocation = VK_NULL_HANDLE;
    VkImageView _cubemapView = VK_NULL_HANDLE;
    VkSampler _sampler = VK_NULL_HANDLE;
    
    Scope<Mesh> _cubeMesh;
    Scope<Pipeline> _pipeline;
    VkDescriptorSetLayout _descriptorLayout = VK_NULL_HANDLE;
    VkDescriptorPool _descriptorPool = VK_NULL_HANDLE;
    VkDescriptorSet _descriptorSet = VK_NULL_HANDLE;
};

struct SkyParams {
    glm::vec3 sun_direction = glm::normalize(glm::vec3(0.5f, 0.8f, 0.3f));
    glm::vec3 sun_color = glm::vec3(1.0f, 0.95f, 0.9f);
    float sun_intensity = 1.0f;
    glm::vec3 sky_color_zenith = glm::vec3(0.2f, 0.4f, 0.8f);
    glm::vec3 sky_color_horizon = glm::vec3(0.6f, 0.7f, 0.9f);
    glm::vec3 ground_color = glm::vec3(0.3f, 0.25f, 0.2f);
};

} // namespace tvk
