/**
 * @file mesh.cpp
 * @brief Mesh implementation
 */

#include "tinyvk/renderer/mesh.h"
#include "tinyvk/renderer/renderer.h"
#include "tinyvk/core/log.h"
#include <glm/gtc/constants.hpp>

namespace tvk {

Mesh::~Mesh() {
    Destroy();
}

bool Mesh::Create(Renderer* renderer, const std::vector<Vertex>& vertices, const std::vector<u32>& indices) {
    _renderer = renderer;
    _vertexCount = static_cast<u32>(vertices.size());
    _indexCount = static_cast<u32>(indices.size());
    
    if (_vertexCount == 0) {
        TVK_LOG_ERROR("Mesh has no vertices");
        return false;
    }
    
    _vertexBuffer = Buffer::CreateVertex(renderer, vertices);
    if (!_vertexBuffer) {
        TVK_LOG_ERROR("Failed to create vertex buffer");
        return false;
    }
    
    if (_indexCount > 0) {
        _indexBuffer = Buffer::CreateIndex(renderer, indices);
        if (!_indexBuffer) {
            TVK_LOG_ERROR("Failed to create index buffer");
            return false;
        }
    }
    
    return true;
}

void Mesh::Destroy() {
    _vertexBuffer.reset();
    _indexBuffer.reset();
    _vertexCount = 0;
    _indexCount = 0;
}

void Mesh::Draw(VkCommandBuffer cmd) {
    if (_vertexBuffer) {
        _vertexBuffer->BindAsVertex(cmd, 0);
    }
    
    if (_indexCount > 0 && _indexBuffer) {
        _indexBuffer->BindAsIndex(cmd);
        vkCmdDrawIndexed(cmd, _indexCount, 1, 0, 0, 0);
    } else {
        vkCmdDraw(cmd, _vertexCount, 1, 0, 0);
    }
}

namespace Geometry {

Scope<Mesh> CreateCube(Renderer* renderer, float size) {
    float half = size * 0.5f;
    
    std::vector<Vertex> vertices = {
        // Front face
        {{-half, -half,  half}, { 0.0f,  0.0f,  1.0f}, {0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}},
        {{ half, -half,  half}, { 0.0f,  0.0f,  1.0f}, {1.0f, 0.0f}, {1.0f, 1.0f, 1.0f}},
        {{ half,  half,  half}, { 0.0f,  0.0f,  1.0f}, {1.0f, 1.0f}, {1.0f, 1.0f, 1.0f}},
        {{-half,  half,  half}, { 0.0f,  0.0f,  1.0f}, {0.0f, 1.0f}, {1.0f, 1.0f, 1.0f}},
        
        // Back face
        {{ half, -half, -half}, { 0.0f,  0.0f, -1.0f}, {0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}},
        {{-half, -half, -half}, { 0.0f,  0.0f, -1.0f}, {1.0f, 0.0f}, {1.0f, 1.0f, 1.0f}},
        {{-half,  half, -half}, { 0.0f,  0.0f, -1.0f}, {1.0f, 1.0f}, {1.0f, 1.0f, 1.0f}},
        {{ half,  half, -half}, { 0.0f,  0.0f, -1.0f}, {0.0f, 1.0f}, {1.0f, 1.0f, 1.0f}},
        
        // Right face
        {{ half, -half,  half}, { 1.0f,  0.0f,  0.0f}, {0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}},
        {{ half, -half, -half}, { 1.0f,  0.0f,  0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f, 1.0f}},
        {{ half,  half, -half}, { 1.0f,  0.0f,  0.0f}, {1.0f, 1.0f}, {1.0f, 1.0f, 1.0f}},
        {{ half,  half,  half}, { 1.0f,  0.0f,  0.0f}, {0.0f, 1.0f}, {1.0f, 1.0f, 1.0f}},
        
        // Left face
        {{-half, -half, -half}, {-1.0f,  0.0f,  0.0f}, {0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}},
        {{-half, -half,  half}, {-1.0f,  0.0f,  0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f, 1.0f}},
        {{-half,  half,  half}, {-1.0f,  0.0f,  0.0f}, {1.0f, 1.0f}, {1.0f, 1.0f, 1.0f}},
        {{-half,  half, -half}, {-1.0f,  0.0f,  0.0f}, {0.0f, 1.0f}, {1.0f, 1.0f, 1.0f}},
        
        // Top face
        {{-half,  half,  half}, { 0.0f,  1.0f,  0.0f}, {0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}},
        {{ half,  half,  half}, { 0.0f,  1.0f,  0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f, 1.0f}},
        {{ half,  half, -half}, { 0.0f,  1.0f,  0.0f}, {1.0f, 1.0f}, {1.0f, 1.0f, 1.0f}},
        {{-half,  half, -half}, { 0.0f,  1.0f,  0.0f}, {0.0f, 1.0f}, {1.0f, 1.0f, 1.0f}},
        
        // Bottom face
        {{-half, -half, -half}, { 0.0f, -1.0f,  0.0f}, {0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}},
        {{ half, -half, -half}, { 0.0f, -1.0f,  0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f, 1.0f}},
        {{ half, -half,  half}, { 0.0f, -1.0f,  0.0f}, {1.0f, 1.0f}, {1.0f, 1.0f, 1.0f}},
        {{-half, -half,  half}, { 0.0f, -1.0f,  0.0f}, {0.0f, 1.0f}, {1.0f, 1.0f, 1.0f}},
    };
    
    std::vector<u32> indices = {
        0, 1, 2, 2, 3, 0,       // Front
        4, 5, 6, 6, 7, 4,       // Back
        8, 9, 10, 10, 11, 8,    // Right
        12, 13, 14, 14, 15, 12, // Left
        16, 17, 18, 18, 19, 16, // Top
        20, 21, 22, 22, 23, 20  // Bottom
    };
    
    auto mesh = CreateScope<Mesh>();
    if (!mesh->Create(renderer, vertices, indices)) {
        return nullptr;
    }
    
    return mesh;
}

Scope<Mesh> CreateSphere(Renderer* renderer, float radius, u32 segments, u32 rings) {
    std::vector<Vertex> vertices;
    std::vector<u32> indices;
    
    for (u32 ring = 0; ring <= rings; ++ring) {
        float phi = glm::pi<float>() * static_cast<float>(ring) / static_cast<float>(rings);
        float sinPhi = glm::sin(phi);
        float cosPhi = glm::cos(phi);
        
        for (u32 segment = 0; segment <= segments; ++segment) {
            float theta = 2.0f * glm::pi<float>() * static_cast<float>(segment) / static_cast<float>(segments);
            float sinTheta = glm::sin(theta);
            float cosTheta = glm::cos(theta);
            
            Vertex vertex;
            vertex.position = glm::vec3(
                radius * sinPhi * cosTheta,
                radius * cosPhi,
                radius * sinPhi * sinTheta
            );
            vertex.normal = glm::normalize(vertex.position);
            vertex.texCoord = glm::vec2(
                static_cast<float>(segment) / static_cast<float>(segments),
                static_cast<float>(ring) / static_cast<float>(rings)
            );
            vertex.color = glm::vec3(1.0f);
            
            vertices.push_back(vertex);
        }
    }
    
    for (u32 ring = 0; ring < rings; ++ring) {
        for (u32 segment = 0; segment < segments; ++segment) {
            u32 current = ring * (segments + 1) + segment;
            u32 next = current + segments + 1;
            
            indices.push_back(current);
            indices.push_back(next);
            indices.push_back(current + 1);
            
            indices.push_back(current + 1);
            indices.push_back(next);
            indices.push_back(next + 1);
        }
    }
    
    auto mesh = CreateScope<Mesh>();
    if (!mesh->Create(renderer, vertices, indices)) {
        return nullptr;
    }
    
    return mesh;
}

Scope<Mesh> CreatePlane(Renderer* renderer, float width, float height, u32 segmentsX, u32 segmentsY) {
    std::vector<Vertex> vertices;
    std::vector<u32> indices;
    
    float halfWidth = width * 0.5f;
    float halfHeight = height * 0.5f;
    
    for (u32 y = 0; y <= segmentsY; ++y) {
        for (u32 x = 0; x <= segmentsX; ++x) {
            float u = static_cast<float>(x) / static_cast<float>(segmentsX);
            float v = static_cast<float>(y) / static_cast<float>(segmentsY);
            
            Vertex vertex;
            vertex.position = glm::vec3(
                -halfWidth + u * width,
                0.0f,
                -halfHeight + v * height
            );
            vertex.normal = glm::vec3(0.0f, 1.0f, 0.0f);
            vertex.texCoord = glm::vec2(u, v);
            vertex.color = glm::vec3(1.0f);
            
            vertices.push_back(vertex);
        }
    }
    
    for (u32 y = 0; y < segmentsY; ++y) {
        for (u32 x = 0; x < segmentsX; ++x) {
            u32 topLeft = y * (segmentsX + 1) + x;
            u32 topRight = topLeft + 1;
            u32 bottomLeft = (y + 1) * (segmentsX + 1) + x;
            u32 bottomRight = bottomLeft + 1;
            
            indices.push_back(topLeft);
            indices.push_back(bottomLeft);
            indices.push_back(topRight);
            
            indices.push_back(topRight);
            indices.push_back(bottomLeft);
            indices.push_back(bottomRight);
        }
    }
    
    auto mesh = CreateScope<Mesh>();
    if (!mesh->Create(renderer, vertices, indices)) {
        return nullptr;
    }
    
    return mesh;
}

Scope<Mesh> CreateCylinder(Renderer* renderer, float radius, float height, u32 segments) {
    std::vector<Vertex> vertices;
    std::vector<u32> indices;
    
    float halfHeight = height * 0.5f;
    
    // Side vertices
    for (u32 i = 0; i <= segments; ++i) {
        float angle = 2.0f * glm::pi<float>() * static_cast<float>(i) / static_cast<float>(segments);
        float x = radius * glm::cos(angle);
        float z = radius * glm::sin(angle);
        float u = static_cast<float>(i) / static_cast<float>(segments);
        
        glm::vec3 normal = glm::normalize(glm::vec3(x, 0.0f, z));
        
        vertices.push_back({{x, -halfHeight, z}, normal, {u, 0.0f}, {1.0f, 1.0f, 1.0f}});
        vertices.push_back({{x,  halfHeight, z}, normal, {u, 1.0f}, {1.0f, 1.0f, 1.0f}});
    }
    
    for (u32 i = 0; i < segments; ++i) {
        u32 base = i * 2;
        indices.push_back(base);
        indices.push_back(base + 2);
        indices.push_back(base + 1);
        
        indices.push_back(base + 1);
        indices.push_back(base + 2);
        indices.push_back(base + 3);
    }
    
    u32 baseIndex = static_cast<u32>(vertices.size());
    vertices.push_back({{0.0f, -halfHeight, 0.0f}, {0.0f, -1.0f, 0.0f}, {0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}});
    vertices.push_back({{0.0f,  halfHeight, 0.0f}, {0.0f,  1.0f, 0.0f}, {0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}});
    
    for (u32 i = 0; i < segments; ++i) {
        float angle1 = 2.0f * glm::pi<float>() * static_cast<float>(i) / static_cast<float>(segments);
        float angle2 = 2.0f * glm::pi<float>() * static_cast<float>(i + 1) / static_cast<float>(segments);
        
        glm::vec3 p1(radius * glm::cos(angle1), 0.0f, radius * glm::sin(angle1));
        glm::vec3 p2(radius * glm::cos(angle2), 0.0f, radius * glm::sin(angle2));
        
        u32 idx = static_cast<u32>(vertices.size());
        vertices.push_back({{p1.x, -halfHeight, p1.z}, {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}});
        vertices.push_back({{p2.x, -halfHeight, p2.z}, {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}});
        
        indices.push_back(baseIndex);
        indices.push_back(idx);
        indices.push_back(idx + 1);
        
        idx = static_cast<u32>(vertices.size());
        vertices.push_back({{p1.x, halfHeight, p1.z}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}});
        vertices.push_back({{p2.x, halfHeight, p2.z}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}});
        
        indices.push_back(baseIndex + 1);
        indices.push_back(idx + 1);
        indices.push_back(idx);
    }
    
    auto mesh = CreateScope<Mesh>();
    if (!mesh->Create(renderer, vertices, indices)) {
        return nullptr;
    }
    
    return mesh;
}

Scope<Mesh> CreateCone(Renderer* renderer, float radius, float height, u32 segments) {
    std::vector<Vertex> vertices;
    std::vector<u32> indices;
    
    vertices.push_back({{0.0f, height, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.5f, 1.0f}, {1.0f, 1.0f, 1.0f}});
    
    for (u32 i = 0; i <= segments; ++i) {
        float angle = 2.0f * glm::pi<float>() * static_cast<float>(i) / static_cast<float>(segments);
        float x = radius * glm::cos(angle);
        float z = radius * glm::sin(angle);
        
        glm::vec3 toTip = glm::normalize(glm::vec3(-x, height, -z));
        glm::vec3 normal = glm::normalize(glm::cross(glm::vec3(-z, 0.0f, x), toTip));
        
        vertices.push_back({{x, 0.0f, z}, normal, {static_cast<float>(i) / segments, 0.0f}, {1.0f, 1.0f, 1.0f}});
    }
    
    for (u32 i = 0; i < segments; ++i) {
        indices.push_back(0);
        indices.push_back(i + 1);
        indices.push_back(i + 2);
    }
    
    u32 baseIndex = static_cast<u32>(vertices.size());
    vertices.push_back({{0.0f, 0.0f, 0.0f}, {0.0f, -1.0f, 0.0f}, {0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}});
    
    for (u32 i = 0; i < segments; ++i) {
        float angle1 = 2.0f * glm::pi<float>() * static_cast<float>(i) / static_cast<float>(segments);
        float angle2 = 2.0f * glm::pi<float>() * static_cast<float>(i + 1) / static_cast<float>(segments);
        
        glm::vec3 p1(radius * glm::cos(angle1), 0.0f, radius * glm::sin(angle1));
        glm::vec3 p2(radius * glm::cos(angle2), 0.0f, radius * glm::sin(angle2));
        
        u32 idx = static_cast<u32>(vertices.size());
        vertices.push_back({p1, {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}});
        vertices.push_back({p2, {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}});
        
        indices.push_back(baseIndex);
        indices.push_back(idx + 1);
        indices.push_back(idx);
    }
    
    auto mesh = CreateScope<Mesh>();
    if (!mesh->Create(renderer, vertices, indices)) {
        return nullptr;
    }
    
    return mesh;
}

Scope<Mesh> CreateTorus(Renderer* renderer, float majorRadius, float minorRadius, u32 majorSegments, u32 minorSegments) {
    std::vector<Vertex> vertices;
    std::vector<u32> indices;
    
    for (u32 i = 0; i <= majorSegments; ++i) {
        float u = 2.0f * glm::pi<float>() * static_cast<float>(i) / static_cast<float>(majorSegments);
        float cosU = glm::cos(u);
        float sinU = glm::sin(u);
        
        for (u32 j = 0; j <= minorSegments; ++j) {
            float v = 2.0f * glm::pi<float>() * static_cast<float>(j) / static_cast<float>(minorSegments);
            float cosV = glm::cos(v);
            float sinV = glm::sin(v);
            
            Vertex vertex;
            vertex.position = glm::vec3(
                (majorRadius + minorRadius * cosV) * cosU,
                minorRadius * sinV,
                (majorRadius + minorRadius * cosV) * sinU
            );
            
            glm::vec3 center(majorRadius * cosU, 0.0f, majorRadius * sinU);
            vertex.normal = glm::normalize(vertex.position - center);
            
            vertex.texCoord = glm::vec2(
                static_cast<float>(i) / static_cast<float>(majorSegments),
                static_cast<float>(j) / static_cast<float>(minorSegments)
            );
            vertex.color = glm::vec3(1.0f);
            
            vertices.push_back(vertex);
        }
    }
    
    for (u32 i = 0; i < majorSegments; ++i) {
        for (u32 j = 0; j < minorSegments; ++j) {
            u32 current = i * (minorSegments + 1) + j;
            u32 next = current + minorSegments + 1;
            
            indices.push_back(current);
            indices.push_back(next);
            indices.push_back(current + 1);
            
            indices.push_back(current + 1);
            indices.push_back(next);
            indices.push_back(next + 1);
        }
    }
    
    auto mesh = CreateScope<Mesh>();
    if (!mesh->Create(renderer, vertices, indices)) {
        return nullptr;
    }
    
    return mesh;
}

Scope<Mesh> CreateQuad(Renderer* renderer) {
    std::vector<Vertex> vertices = {
        {{-1.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}},
        {{ 1.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}, {1.0f, 1.0f, 1.0f}},
        {{ 1.0f,  1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}, {1.0f, 1.0f, 1.0f}},
        {{-1.0f,  1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}, {1.0f, 1.0f, 1.0f}},
    };
    
    std::vector<u32> indices = {0, 1, 2, 2, 3, 0};
    
    auto mesh = CreateScope<Mesh>();
    if (!mesh->Create(renderer, vertices, indices)) {
        return nullptr;
    }
    
    return mesh;
}

} // namespace Geometry

} // namespace tvk
