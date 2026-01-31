/**
 * @file scene.cpp
 * @brief Scene implementation
 */

#include "tinyvk/scene/scene.h"
#include "tinyvk/renderer/renderer.h"
#include "tinyvk/renderer/pipeline.h"
#include "tinyvk/core/log.h"

namespace tvk {

Scene::Scene(const std::string& name) : _name(name) {
}

Scene::~Scene() {
    Clear();
}

Entity Scene::CreateEntity(const std::string& name) {
    Entity entity(_registry.create(), this);
    entity.AddComponent<TagComponent>(name);
    entity.AddComponent<TransformComponent>();
    return entity;
}

Entity Scene::CreateChildEntity(Entity parent, const std::string& name) {
    Entity entity = CreateEntity(name);

    auto& hierarchy = entity.AddComponent<HierarchyComponent>();
    hierarchy.parent = static_cast<u64>(parent.GetHandle());

    if (parent.HasComponent<HierarchyComponent>()) {
        auto& parent_hierarchy = parent.GetComponent<HierarchyComponent>();
        hierarchy.depth = parent_hierarchy.depth + 1;

        if (parent_hierarchy.first_child == 0) {
            parent_hierarchy.first_child = static_cast<u64>(entity.GetHandle());
        } else {
            Entity sibling(static_cast<entt::entity>(parent_hierarchy.first_child), this);
            auto* sibling_hierarchy = sibling.TryGetComponent<HierarchyComponent>();
            while (sibling_hierarchy && sibling_hierarchy->next_sibling != 0) {
                sibling = Entity(static_cast<entt::entity>(sibling_hierarchy->next_sibling), this);
                sibling_hierarchy = sibling.TryGetComponent<HierarchyComponent>();
            }
            if (sibling_hierarchy) {
                sibling_hierarchy->next_sibling = static_cast<u64>(entity.GetHandle());
                hierarchy.prev_sibling = static_cast<u64>(sibling.GetHandle());
            }
        }
    } else {
        auto& parent_hierarchy = parent.AddComponent<HierarchyComponent>();
        parent_hierarchy.first_child = static_cast<u64>(entity.GetHandle());
        hierarchy.depth = 1;
    }

    return entity;
}

void Scene::DestroyEntity(Entity entity) {
    if (!entity.IsValid()) return;

    if (entity.HasComponent<HierarchyComponent>()) {
        auto& hierarchy = entity.GetComponent<HierarchyComponent>();

        if (hierarchy.prev_sibling != 0) {
            Entity prev(static_cast<entt::entity>(hierarchy.prev_sibling), this);
            if (auto* h = prev.TryGetComponent<HierarchyComponent>()) {
                h->next_sibling = hierarchy.next_sibling;
            }
        }

        if (hierarchy.next_sibling != 0) {
            Entity next(static_cast<entt::entity>(hierarchy.next_sibling), this);
            if (auto* h = next.TryGetComponent<HierarchyComponent>()) {
                h->prev_sibling = hierarchy.prev_sibling;
            }
        }

        if (hierarchy.parent != 0) {
            Entity parent(static_cast<entt::entity>(hierarchy.parent), this);
            if (auto* h = parent.TryGetComponent<HierarchyComponent>()) {
                if (h->first_child == static_cast<u64>(entity.GetHandle())) {
                    h->first_child = hierarchy.next_sibling;
                }
            }
        }
    }

    if (static_cast<entt::entity>(_primary_camera) == entity.GetHandle()) {
        _primary_camera = entt::null;
    }

    _registry.destroy(entity.GetHandle());
}

void Scene::DestroyEntityRecursive(Entity entity) {
    if (!entity.IsValid()) return;

    if (entity.HasComponent<HierarchyComponent>()) {
        auto& hierarchy = entity.GetComponent<HierarchyComponent>();
        u64 child_handle = hierarchy.first_child;

        while (child_handle != 0) {
            Entity child(static_cast<entt::entity>(child_handle), this);
            u64 next_child = 0;
            if (auto* h = child.TryGetComponent<HierarchyComponent>()) {
                next_child = h->next_sibling;
            }
            DestroyEntityRecursive(child);
            child_handle = next_child;
        }
    }

    DestroyEntity(entity);
}

Entity Scene::FindEntityByName(const std::string& name) {
    auto view = _registry.view<TagComponent>();
    for (auto entity : view) {
        const auto& tag = view.get<TagComponent>(entity);
        if (tag.tag == name) {
            return Entity(entity, this);
        }
    }
    return Entity();
}

std::vector<Entity> Scene::FindEntitiesByName(const std::string& name) {
    std::vector<Entity> result;
    auto view = _registry.view<TagComponent>();
    for (auto entity : view) {
        const auto& tag = view.get<TagComponent>(entity);
        if (tag.tag == name) {
            result.emplace_back(entity, this);
        }
    }
    return result;
}

Entity Scene::GetEntityByHandle(entt::entity handle) {
    if (_registry.valid(handle)) {
        return Entity(handle, this);
    }
    return Entity();
}

void Scene::SetPrimaryCamera(Entity entity) {
    if (entity.IsValid() && entity.HasComponent<CameraComponent>()) {
        auto view = _registry.view<CameraComponent>();
        for (auto e : view) {
            view.get<CameraComponent>(e).primary = false;
        }
        entity.GetComponent<CameraComponent>().primary = true;
        _primary_camera = entity.GetHandle();
    }
}

Entity Scene::GetPrimaryCamera() {
    if (_primary_camera != entt::null && _registry.valid(_primary_camera)) {
        return Entity(_primary_camera, this);
    }

    auto view = _registry.view<CameraComponent>();
    for (auto entity : view) {
        const auto& camera_comp = view.get<CameraComponent>(entity);
        if (camera_comp.primary) {
            _primary_camera = entity;
            return Entity(entity, this);
        }
    }

    return Entity();
}

Camera* Scene::GetActiveCameraPtr() {
    Entity camera_entity = GetPrimaryCamera();
    if (camera_entity.IsValid()) {
        return &camera_entity.GetComponent<CameraComponent>().camera;
    }
    return nullptr;
}

void Scene::OnUpdate(f32 delta_time) {
    (void)delta_time;
}

void Scene::OnRender(Renderer* renderer, VkCommandBuffer cmd, VkRenderPass render_pass) {
    Camera* camera = GetActiveCameraPtr();
    if (!camera) return;

    Mat4 view_projection = camera->GetViewProjectionMatrix();

    auto view = _registry.view<TransformComponent, MeshComponent>();
    for (auto entity : view) {
        const auto& transform = view.get<TransformComponent>(entity);
        const auto& mesh_comp = view.get<MeshComponent>(entity);

        if (!mesh_comp.visible || !mesh_comp.mesh) continue;

        Mat4 world_transform = GetWorldTransform(Entity(entity, this));

        MaterialComponent* material_comp = _registry.try_get<MaterialComponent>(entity);
        if (material_comp && material_comp->material) {
            material_comp->material->Bind(cmd, render_pass);

            PushConstants push;
            push.model = world_transform;
            push.view_projection = view_projection;
            vkCmdPushConstants(cmd, material_comp->material->GetPipelineLayout(),
                             VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstants), &push);
        }

        mesh_comp.mesh->Draw(cmd);
    }
}

void Scene::OnViewportResize(u32 width, u32 height) {
    _viewport_width = width;
    _viewport_height = height;

    auto view = _registry.view<CameraComponent>();
    for (auto entity : view) {
        auto& camera_comp = view.get<CameraComponent>(entity);
        if (!camera_comp.fixed_aspect_ratio) {
            camera_comp.camera.SetAspectRatio(static_cast<f32>(width) / static_cast<f32>(height));
        }
    }
}

u32 Scene::GetEntityCount() const {
    return static_cast<u32>(_registry.view<TagComponent>().size());
}

void Scene::Clear() {
    _registry.clear();
    _primary_camera = entt::null;
}

Mat4 Scene::GetWorldTransform(Entity entity) {
    Mat4 transform = entity.GetComponent<TransformComponent>().GetMatrix();

    if (entity.HasComponent<HierarchyComponent>()) {
        auto& hierarchy = entity.GetComponent<HierarchyComponent>();
        if (hierarchy.parent != 0) {
            Entity parent(static_cast<entt::entity>(hierarchy.parent), this);
            if (parent.IsValid()) {
                transform = GetWorldTransform(parent) * transform;
            }
        }
    }

    return transform;
}

SceneManager& SceneManager::Get() {
    static SceneManager instance;
    return instance;
}

Ref<Scene> SceneManager::CreateScene(const std::string& name) {
    auto scene = CreateRef<Scene>(name);
    _scenes.push_back(scene);
    return scene;
}

void SceneManager::SetActiveScene(Ref<Scene> scene) {
    _active_scene = scene;
}

void SceneManager::OnUpdate(f32 delta_time) {
    if (_active_scene) {
        _active_scene->OnUpdate(delta_time);
    }
}

void SceneManager::OnRender(Renderer* renderer, VkCommandBuffer cmd, VkRenderPass render_pass) {
    if (_active_scene) {
        _active_scene->OnRender(renderer, cmd, render_pass);
    }
}

void SceneManager::OnViewportResize(u32 width, u32 height) {
    if (_active_scene) {
        _active_scene->OnViewportResize(width, height);
    }
}

} // namespace tvk
