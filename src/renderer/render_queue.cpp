#include <tinyvk/renderer/render_queue.h>
#include <algorithm>

namespace tvk {

void RenderQueue::Clear() {
    _commands.clear();
    _stats.Reset();
}

void RenderQueue::Submit(Ref<Mesh> mesh, Ref<Material> material, Pipeline* pipeline,
                         const glm::mat4& transform, RenderLayer layer) {
    RenderCommand cmd;
    cmd.mesh = mesh;
    cmd.material = material;
    cmd.pipeline = pipeline;
    cmd.transform = transform;
    cmd.layer = layer;
    cmd.distance_to_camera = 0.0f;
    cmd.sort_key = 0;
    _commands.push_back(cmd);
}

u32 RenderQueue::ComputeSortKey(const RenderCommand& cmd) const {
    u32 key = 0;
    
    key |= (static_cast<u32>(cmd.layer) & 0xFF) << 24;
    
    uintptr_t pipeline_id = reinterpret_cast<uintptr_t>(cmd.pipeline);
    key |= ((pipeline_id >> 4) & 0xFF) << 16;
    
    uintptr_t material_id = reinterpret_cast<uintptr_t>(cmd.material.get());
    key |= ((material_id >> 4) & 0xFFFF);
    
    return key;
}

void RenderQueue::Sort(const glm::vec3& camera_position) {
    _camera_position = camera_position;
    
    for (auto& cmd : _commands) {
        glm::vec3 obj_pos = glm::vec3(cmd.transform[3]);
        cmd.distance_to_camera = glm::length(obj_pos - camera_position);
        cmd.sort_key = ComputeSortKey(cmd);
    }
    
    std::sort(_commands.begin(), _commands.end(), [](const RenderCommand& a, const RenderCommand& b) {
        if (a.layer != b.layer) {
            return static_cast<u8>(a.layer) < static_cast<u8>(b.layer);
        }
        
        if (a.layer == RenderLayer::Transparent) {
            return a.distance_to_camera > b.distance_to_camera;
        }
        
        return a.sort_key < b.sort_key;
    });
}

void RenderQueue::Execute(VkCommandBuffer cmd, std::function<void(VkCommandBuffer, const RenderCommand&)> draw_fn) {
    if (_commands.empty()) return;
    
    Pipeline* last_pipeline = nullptr;
    Material* last_material = nullptr;
    
    for (const auto& render_cmd : _commands) {
        if (render_cmd.pipeline != last_pipeline) {
            last_pipeline = render_cmd.pipeline;
            _stats.pipeline_switches++;
        }
        
        if (render_cmd.material.get() != last_material) {
            last_material = render_cmd.material.get();
            _stats.material_switches++;
        }
        
        draw_fn(cmd, render_cmd);
        
        _stats.draw_calls++;
        if (render_cmd.mesh) {
            _stats.vertices += render_cmd.mesh->GetVertexCount();
            _stats.triangles += render_cmd.mesh->GetIndexCount() / 3;
        }
    }
}

void BatchedRenderQueue::Clear() {
    _commands.clear();
    _batches.clear();
}

void BatchedRenderQueue::Submit(Ref<Mesh> mesh, Ref<Material> material, Pipeline* pipeline,
                                const glm::mat4& transform, RenderLayer layer) {
    RenderCommand cmd;
    cmd.mesh = mesh;
    cmd.material = material;
    cmd.pipeline = pipeline;
    cmd.transform = transform;
    cmd.layer = layer;
    _commands.push_back(cmd);
}

void BatchedRenderQueue::BuildBatches() {
    _batches.clear();
    
    if (_commands.empty()) return;
    
    std::sort(_commands.begin(), _commands.end(), [](const RenderCommand& a, const RenderCommand& b) {
        if (a.layer != b.layer) return static_cast<u8>(a.layer) < static_cast<u8>(b.layer);
        if (a.pipeline != b.pipeline) return a.pipeline < b.pipeline;
        if (a.material != b.material) return a.material < b.material;
        return a.mesh < b.mesh;
    });
    
    Batch current_batch;
    current_batch.key.pipeline = _commands[0].pipeline;
    current_batch.key.material = _commands[0].material;
    current_batch.key.mesh = _commands[0].mesh;
    current_batch.transforms.push_back(_commands[0].transform);
    
    for (size_t i = 1; i < _commands.size(); ++i) {
        const auto& cmd = _commands[i];
        
        BatchKey key;
        key.pipeline = cmd.pipeline;
        key.material = cmd.material;
        key.mesh = cmd.mesh;
        
        if (key == current_batch.key) {
            current_batch.transforms.push_back(cmd.transform);
        } else {
            _batches.push_back(std::move(current_batch));
            current_batch = Batch{};
            current_batch.key = key;
            current_batch.transforms.push_back(cmd.transform);
        }
    }
    
    _batches.push_back(std::move(current_batch));
}

void BatchedRenderQueue::Execute(VkCommandBuffer cmd, std::function<void(VkCommandBuffer, const Batch&)> draw_fn) {
    for (const auto& batch : _batches) {
        draw_fn(cmd, batch);
    }
}

} // namespace tvk
