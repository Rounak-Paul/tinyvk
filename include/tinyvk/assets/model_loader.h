#pragma once

#include <tinyvk/core/types.h>
#include <tinyvk/core/result.h>
#include <tinyvk/renderer/mesh.h>
#include <glm/glm.hpp>
#include <string>
#include <vector>

namespace tvk {

class Renderer;

struct ModelVertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texcoord;
    glm::vec3 color;
};

struct ModelSubmesh {
    u32 vertex_offset;
    u32 index_offset;
    u32 index_count;
    i32 material_index;
};

struct LoadedModel {
    std::vector<ModelVertex> vertices;
    std::vector<u32> indices;
    std::vector<ModelSubmesh> submeshes;
    std::vector<std::string> material_names;
    glm::vec3 bounds_min;
    glm::vec3 bounds_max;
};

struct ModelLoadOptions {
    bool calculate_normals = false;
    bool flip_uvs = false;
    bool triangulate = true;
    float scale = 1.0f;
};

class ModelLoader {
public:
    static Result<LoadedModel> LoadOBJ(const std::string& path, const ModelLoadOptions& options = {});
    static Result<LoadedModel> LoadFromFile(const std::string& path, const ModelLoadOptions& options = {});
    
    static Scope<Mesh> CreateMeshFromModel(Renderer* renderer, const LoadedModel& model, u32 submesh_index = 0);
    static std::vector<Scope<Mesh>> CreateAllMeshesFromModel(Renderer* renderer, const LoadedModel& model);
    
    static void ComputeNormals(LoadedModel& model);
    static void RecalculateBounds(LoadedModel& model);
};

} // namespace tvk
