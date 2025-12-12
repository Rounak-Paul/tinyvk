/**
 * @file mesh.h
 * @brief Mesh class for geometry data
 */

#pragma once

#include "../core/types.h"
#include "vertex.h"
#include "buffer.h"
#include <vulkan/vulkan.h>
#include <vector>

namespace tvk {

class Renderer;

class Mesh {
public:
    Mesh() = default;
    ~Mesh();
    
    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;
    
    bool Create(Renderer* renderer, const std::vector<Vertex>& vertices, const std::vector<u32>& indices);
    void Destroy();
    
    void Draw(VkCommandBuffer cmd);
    
    u32 GetVertexCount() const { return _vertexCount; }
    u32 GetIndexCount() const { return _indexCount; }
    
    VkBuffer GetVertexBuffer() const { return _vertexBuffer ? _vertexBuffer->GetBuffer() : VK_NULL_HANDLE; }
    VkBuffer GetIndexBuffer() const { return _indexBuffer ? _indexBuffer->GetBuffer() : VK_NULL_HANDLE; }
    
private:
    Renderer* _renderer = nullptr;
    Ref<Buffer> _vertexBuffer;
    Ref<Buffer> _indexBuffer;
    u32 _vertexCount = 0;
    u32 _indexCount = 0;
};

namespace Geometry {
    Scope<Mesh> CreateCube(Renderer* renderer, float size = 1.0f);
    Scope<Mesh> CreateSphere(Renderer* renderer, float radius = 1.0f, u32 segments = 32, u32 rings = 16);
    Scope<Mesh> CreatePlane(Renderer* renderer, float width = 1.0f, float height = 1.0f, u32 segmentsX = 1, u32 segmentsY = 1);
    Scope<Mesh> CreateCylinder(Renderer* renderer, float radius = 1.0f, float height = 2.0f, u32 segments = 32);
    Scope<Mesh> CreateCone(Renderer* renderer, float radius = 1.0f, float height = 2.0f, u32 segments = 32);
    Scope<Mesh> CreateTorus(Renderer* renderer, float majorRadius = 1.0f, float minorRadius = 0.3f, u32 majorSegments = 32, u32 minorSegments = 16);
    Scope<Mesh> CreateQuad(Renderer* renderer);
}

} // namespace tvk
