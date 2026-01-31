#include <tinyvk/assets/model_loader.h>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <cstring>

namespace tvk {

struct OBJIndex {
    i32 vertex = -1;
    i32 texcoord = -1;
    i32 normal = -1;
    
    bool operator==(const OBJIndex& other) const {
        return vertex == other.vertex && texcoord == other.texcoord && normal == other.normal;
    }
};

struct OBJIndexHash {
    size_t operator()(const OBJIndex& idx) const {
        return ((size_t)idx.vertex * 73856093) ^ ((size_t)idx.texcoord * 19349663) ^ ((size_t)idx.normal * 83492791);
    }
};

static bool ParseFaceVertex(const std::string& token, OBJIndex& out) {
    out = OBJIndex{};
    
    size_t first_slash = token.find('/');
    if (first_slash == std::string::npos) {
        out.vertex = std::stoi(token) - 1;
        return true;
    }
    
    out.vertex = std::stoi(token.substr(0, first_slash)) - 1;
    
    size_t second_slash = token.find('/', first_slash + 1);
    if (second_slash == std::string::npos) {
        if (first_slash + 1 < token.size()) {
            out.texcoord = std::stoi(token.substr(first_slash + 1)) - 1;
        }
        return true;
    }
    
    std::string texcoord_str = token.substr(first_slash + 1, second_slash - first_slash - 1);
    if (!texcoord_str.empty()) {
        out.texcoord = std::stoi(texcoord_str) - 1;
    }
    
    if (second_slash + 1 < token.size()) {
        out.normal = std::stoi(token.substr(second_slash + 1)) - 1;
    }
    
    return true;
}

Result<LoadedModel> ModelLoader::LoadOBJ(const std::string& path, const ModelLoadOptions& options) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return Result<LoadedModel>(ErrorCode::FileNotFound, "Failed to open OBJ file: " + path);
    }
    
    std::vector<glm::vec3> positions;
    std::vector<glm::vec2> texcoords;
    std::vector<glm::vec3> normals;
    std::vector<OBJIndex> face_indices;
    std::vector<u32> submesh_face_starts;
    std::vector<std::string> material_names;
    
    std::string current_material;
    std::string line;
    
    submesh_face_starts.push_back(0);
    material_names.push_back("default");
    
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;
        
        std::istringstream iss(line);
        std::string prefix;
        iss >> prefix;
        
        if (prefix == "v") {
            glm::vec3 pos;
            iss >> pos.x >> pos.y >> pos.z;
            pos *= options.scale;
            positions.push_back(pos);
        }
        else if (prefix == "vt") {
            glm::vec2 tex;
            iss >> tex.x >> tex.y;
            if (options.flip_uvs) {
                tex.y = 1.0f - tex.y;
            }
            texcoords.push_back(tex);
        }
        else if (prefix == "vn") {
            glm::vec3 norm;
            iss >> norm.x >> norm.y >> norm.z;
            normals.push_back(norm);
        }
        else if (prefix == "usemtl") {
            std::string mtl_name;
            iss >> mtl_name;
            if (mtl_name != current_material) {
                current_material = mtl_name;
                if (face_indices.size() > submesh_face_starts.back()) {
                    submesh_face_starts.push_back((u32)face_indices.size());
                    material_names.push_back(mtl_name);
                } else {
                    material_names.back() = mtl_name;
                }
            }
        }
        else if (prefix == "f") {
            std::vector<OBJIndex> face_verts;
            std::string token;
            while (iss >> token) {
                OBJIndex idx;
                if (ParseFaceVertex(token, idx)) {
                    face_verts.push_back(idx);
                }
            }
            
            if (options.triangulate && face_verts.size() > 3) {
                for (size_t i = 1; i + 1 < face_verts.size(); ++i) {
                    face_indices.push_back(face_verts[0]);
                    face_indices.push_back(face_verts[i]);
                    face_indices.push_back(face_verts[i + 1]);
                }
            } else {
                for (auto& fv : face_verts) {
                    face_indices.push_back(fv);
                }
            }
        }
    }
    
    file.close();
    
    LoadedModel model;
    std::unordered_map<OBJIndex, u32, OBJIndexHash> unique_vertices;
    
    for (auto& fi : face_indices) {
        auto it = unique_vertices.find(fi);
        if (it != unique_vertices.end()) {
            model.indices.push_back(it->second);
        } else {
            u32 new_index = (u32)model.vertices.size();
            unique_vertices[fi] = new_index;
            
            ModelVertex v;
            v.position = (fi.vertex >= 0 && fi.vertex < (i32)positions.size()) ? positions[fi.vertex] : glm::vec3(0.0f);
            v.texcoord = (fi.texcoord >= 0 && fi.texcoord < (i32)texcoords.size()) ? texcoords[fi.texcoord] : glm::vec2(0.0f);
            v.normal = (fi.normal >= 0 && fi.normal < (i32)normals.size()) ? normals[fi.normal] : glm::vec3(0.0f, 1.0f, 0.0f);
            v.color = glm::vec3(1.0f);
            
            model.vertices.push_back(v);
            model.indices.push_back(new_index);
        }
    }
    
    submesh_face_starts.push_back((u32)face_indices.size());
    
    for (size_t i = 0; i + 1 < submesh_face_starts.size(); ++i) {
        ModelSubmesh submesh;
        submesh.vertex_offset = 0;
        submesh.index_offset = submesh_face_starts[i];
        submesh.index_count = submesh_face_starts[i + 1] - submesh_face_starts[i];
        submesh.material_index = (i32)i;
        model.submeshes.push_back(submesh);
    }
    
    model.material_names = std::move(material_names);
    
    if (options.calculate_normals || normals.empty()) {
        ComputeNormals(model);
    }
    
    RecalculateBounds(model);
    
    return Result<LoadedModel>(std::move(model));
}

Result<LoadedModel> ModelLoader::LoadFromFile(const std::string& path, const ModelLoadOptions& options) {
    std::string ext = path.substr(path.find_last_of('.') + 1);
    for (auto& c : ext) c = (char)std::tolower(c);
    
    if (ext == "obj") {
        return LoadOBJ(path, options);
    }
    
    return Result<LoadedModel>(ErrorCode::InvalidArgument, "Unsupported model format: " + ext);
}

Scope<Mesh> ModelLoader::CreateMeshFromModel(Renderer* renderer, const LoadedModel& model, u32 submesh_index) {
    if (submesh_index >= model.submeshes.size()) {
        return nullptr;
    }
    
    const ModelSubmesh& submesh = model.submeshes[submesh_index];
    
    std::vector<Vertex> vertices;
    std::vector<u32> indices;
    
    u32 min_index = UINT32_MAX;
    u32 max_index = 0;
    for (u32 i = submesh.index_offset; i < submesh.index_offset + submesh.index_count; ++i) {
        u32 idx = model.indices[i];
        if (idx < min_index) min_index = idx;
        if (idx > max_index) max_index = idx;
    }
    
    for (u32 i = min_index; i <= max_index; ++i) {
        const ModelVertex& mv = model.vertices[i];
        Vertex v;
        v.position = mv.position;
        v.normal = mv.normal;
        v.texCoord = mv.texcoord;
        v.color = mv.color;
        vertices.push_back(v);
    }
    
    for (u32 i = submesh.index_offset; i < submesh.index_offset + submesh.index_count; ++i) {
        indices.push_back(model.indices[i] - min_index);
    }
    
    auto mesh = CreateScope<Mesh>();
    if (!mesh->Create(renderer, vertices, indices)) {
        return nullptr;
    }
    return mesh;
}

std::vector<Scope<Mesh>> ModelLoader::CreateAllMeshesFromModel(Renderer* renderer, const LoadedModel& model) {
    std::vector<Scope<Mesh>> meshes;
    for (u32 i = 0; i < model.submeshes.size(); ++i) {
        meshes.push_back(CreateMeshFromModel(renderer, model, i));
    }
    return meshes;
}

void ModelLoader::ComputeNormals(LoadedModel& model) {
    for (auto& v : model.vertices) {
        v.normal = glm::vec3(0.0f);
    }
    
    for (size_t i = 0; i + 2 < model.indices.size(); i += 3) {
        u32 i0 = model.indices[i];
        u32 i1 = model.indices[i + 1];
        u32 i2 = model.indices[i + 2];
        
        glm::vec3 v0 = model.vertices[i0].position;
        glm::vec3 v1 = model.vertices[i1].position;
        glm::vec3 v2 = model.vertices[i2].position;
        
        glm::vec3 edge1 = v1 - v0;
        glm::vec3 edge2 = v2 - v0;
        glm::vec3 face_normal = glm::cross(edge1, edge2);
        
        model.vertices[i0].normal += face_normal;
        model.vertices[i1].normal += face_normal;
        model.vertices[i2].normal += face_normal;
    }
    
    for (auto& v : model.vertices) {
        float len = glm::length(v.normal);
        if (len > 0.0001f) {
            v.normal /= len;
        } else {
            v.normal = glm::vec3(0.0f, 1.0f, 0.0f);
        }
    }
}

void ModelLoader::RecalculateBounds(LoadedModel& model) {
    if (model.vertices.empty()) {
        model.bounds_min = glm::vec3(0.0f);
        model.bounds_max = glm::vec3(0.0f);
        return;
    }
    
    model.bounds_min = model.vertices[0].position;
    model.bounds_max = model.vertices[0].position;
    
    for (const auto& v : model.vertices) {
        model.bounds_min = glm::min(model.bounds_min, v.position);
        model.bounds_max = glm::max(model.bounds_max, v.position);
    }
}

} // namespace tvk
