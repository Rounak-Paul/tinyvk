/**
 * @file components.h
 * @brief ECS components for the scene system
 */

#pragma once

#include "../core/types.h"
#include "../renderer/mesh.h"
#include "../renderer/material.h"
#include "camera.h"
#include <string>

namespace tvk {

struct TagComponent {
    std::string tag;

    TagComponent() = default;
    TagComponent(const std::string& tag) : tag(tag) {}
};

struct TransformComponent {
    Vec3 position = Vec3(0.0f);
    Vec3 rotation = Vec3(0.0f);
    Vec3 scale = Vec3(1.0f);

    TransformComponent() = default;
    TransformComponent(const Vec3& position) : position(position) {}

    Mat4 GetMatrix() const {
        Mat4 mat = glm::translate(Mat4(1.0f), position);
        mat = glm::rotate(mat, glm::radians(rotation.x), Vec3(1.0f, 0.0f, 0.0f));
        mat = glm::rotate(mat, glm::radians(rotation.y), Vec3(0.0f, 1.0f, 0.0f));
        mat = glm::rotate(mat, glm::radians(rotation.z), Vec3(0.0f, 0.0f, 1.0f));
        mat = glm::scale(mat, scale);
        return mat;
    }
};

struct MeshComponent {
    Ref<Mesh> mesh;
    bool visible = true;
    bool cast_shadows = true;
    bool receive_shadows = true;

    MeshComponent() = default;
    MeshComponent(Ref<Mesh> mesh) : mesh(mesh) {}
};

struct MaterialComponent {
    Ref<Material> material;
    u32 render_order = 0;

    MaterialComponent() = default;
    MaterialComponent(Ref<Material> material) : material(material) {}
};

struct CameraComponent {
    Camera camera;
    bool primary = false;
    bool fixed_aspect_ratio = false;

    CameraComponent() = default;
};

struct HierarchyComponent {
    u64 parent = 0;
    u64 first_child = 0;
    u64 next_sibling = 0;
    u64 prev_sibling = 0;
    u32 depth = 0;
};

struct DirectionalLightComponent {
    Vec3 color = Vec3(1.0f);
    f32 intensity = 1.0f;
    bool cast_shadows = true;
};

struct PointLightComponent {
    Vec3 color = Vec3(1.0f);
    f32 intensity = 1.0f;
    f32 radius = 10.0f;
    f32 falloff = 1.0f;
    bool cast_shadows = false;
};

struct SpotLightComponent {
    Vec3 color = Vec3(1.0f);
    f32 intensity = 1.0f;
    f32 range = 10.0f;
    f32 inner_angle = 30.0f;
    f32 outer_angle = 45.0f;
    bool cast_shadows = false;
};

struct AmbientLightComponent {
    Vec3 color = Vec3(1.0f);
    f32 intensity = 0.1f;
};

struct BoundingBoxComponent {
    Vec3 min = Vec3(0.0f);
    Vec3 max = Vec3(0.0f);

    Vec3 GetCenter() const { return (min + max) * 0.5f; }
    Vec3 GetSize() const { return max - min; }
};

struct ScriptComponent {
    std::string script_name;
    void* instance = nullptr;
};

} // namespace tvk
