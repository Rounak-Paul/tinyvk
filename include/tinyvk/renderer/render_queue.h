#pragma once

#include <tinyvk/core/types.h>
#include <tinyvk/renderer/mesh.h>
#include <tinyvk/renderer/material.h>
#include <tinyvk/renderer/pipeline.h>
#include <glm/glm.hpp>
#include <functional>
#include <vector>

namespace tvk {

enum class RenderLayer : u8 {
    Background = 0,
    Opaque = 10,
    AlphaTest = 20,
    Transparent = 30,
    Overlay = 40
};

struct RenderCommand {
    Ref<Mesh> mesh;
    Ref<Material> material;
    Pipeline* pipeline = nullptr;
    glm::mat4 transform;
    float distance_to_camera;
    RenderLayer layer;
    u32 sort_key;
};

struct RenderStats {
    u32 draw_calls;
    u32 triangles;
    u32 vertices;
    u32 material_switches;
    u32 pipeline_switches;
    
    void Reset() {
        draw_calls = 0;
        triangles = 0;
        vertices = 0;
        material_switches = 0;
        pipeline_switches = 0;
    }
};

class RenderQueue {
public:
    RenderQueue() = default;
    ~RenderQueue() = default;
    
    void Clear();
    
    void Submit(Ref<Mesh> mesh, Ref<Material> material, Pipeline* pipeline, 
                const glm::mat4& transform, RenderLayer layer = RenderLayer::Opaque);
    
    void Sort(const glm::vec3& camera_position);
    
    void Execute(VkCommandBuffer cmd, std::function<void(VkCommandBuffer, const RenderCommand&)> draw_fn);
    
    const RenderStats& GetStats() const { return _stats; }
    size_t CommandCount() const { return _commands.size(); }
    
private:
    u32 ComputeSortKey(const RenderCommand& cmd) const;
    
    std::vector<RenderCommand> _commands;
    RenderStats _stats;
    glm::vec3 _camera_position;
};

class BatchedRenderQueue {
public:
    struct BatchKey {
        Pipeline* pipeline;
        Ref<Material> material;
        Ref<Mesh> mesh;
        
        bool operator==(const BatchKey& other) const {
            return pipeline == other.pipeline && material == other.material && mesh == other.mesh;
        }
    };
    
    struct Batch {
        BatchKey key;
        std::vector<glm::mat4> transforms;
    };
    
    void Clear();
    
    void Submit(Ref<Mesh> mesh, Ref<Material> material, Pipeline* pipeline,
                const glm::mat4& transform, RenderLayer layer = RenderLayer::Opaque);
    
    void BuildBatches();
    
    void Execute(VkCommandBuffer cmd, std::function<void(VkCommandBuffer, const Batch&)> draw_fn);
    
    size_t BatchCount() const { return _batches.size(); }
    
private:
    std::vector<RenderCommand> _commands;
    std::vector<Batch> _batches;
};

} // namespace tvk
