/**
 * @file scene.h
 * @brief Scene management with ECS
 */

#pragma once

#include "../core/types.h"
#include "entity.h"
#include "components.h"
#include "camera.h"
#include <entt/entt.hpp>
#include <string>
#include <vector>

namespace tvk {

class Renderer;

class Scene {
public:
    Scene() = default;
    explicit Scene(const std::string& name);
    ~Scene();

    Scene(const Scene&) = delete;
    Scene& operator=(const Scene&) = delete;

    Entity CreateEntity(const std::string& name = "Entity");
    Entity CreateChildEntity(Entity parent, const std::string& name = "Entity");
    void DestroyEntity(Entity entity);
    void DestroyEntityRecursive(Entity entity);

    Entity FindEntityByName(const std::string& name);
    std::vector<Entity> FindEntitiesByName(const std::string& name);
    Entity GetEntityByHandle(entt::entity handle);

    void SetPrimaryCamera(Entity entity);
    Entity GetPrimaryCamera();
    Camera* GetActiveCameraPtr();

    void OnUpdate(f32 delta_time);
    void OnRender(Renderer* renderer, VkCommandBuffer cmd, VkRenderPass render_pass);
    void OnViewportResize(u32 width, u32 height);

    template<typename... Components>
    auto GetAllEntitiesWith() {
        return _registry.view<Components...>();
    }

    template<typename Func>
    void ForEachEntity(Func&& func) {
        auto view = _registry.view<TagComponent>();
        for (auto entity : view) {
            Entity e(entity, this);
            func(e);
        }
    }

    entt::registry& GetRegistry() { return _registry; }
    const entt::registry& GetRegistry() const { return _registry; }

    const std::string& GetName() const { return _name; }
    void SetName(const std::string& name) { _name = name; }

    u32 GetEntityCount() const;

    void Clear();

private:
    void UpdateHierarchyTransforms();
    Mat4 GetWorldTransform(Entity entity);

    std::string _name = "Untitled Scene";
    entt::registry _registry;
    entt::entity _primary_camera = entt::null;

    u32 _viewport_width = 1;
    u32 _viewport_height = 1;
};

template<typename T, typename... Args>
T& Entity::AddComponent(Args&&... args) {
    return _scene->GetRegistry().emplace<T>(_handle, std::forward<Args>(args)...);
}

template<typename T, typename... Args>
T& Entity::AddOrReplaceComponent(Args&&... args) {
    return _scene->GetRegistry().emplace_or_replace<T>(_handle, std::forward<Args>(args)...);
}

template<typename T>
T& Entity::GetComponent() {
    return _scene->GetRegistry().get<T>(_handle);
}

template<typename T>
const T& Entity::GetComponent() const {
    return _scene->GetRegistry().get<T>(_handle);
}

template<typename T>
T* Entity::TryGetComponent() {
    return _scene->GetRegistry().try_get<T>(_handle);
}

template<typename T>
const T* Entity::TryGetComponent() const {
    return _scene->GetRegistry().try_get<T>(_handle);
}

template<typename T>
bool Entity::HasComponent() const {
    return _scene->GetRegistry().all_of<T>(_handle);
}

template<typename... T>
bool Entity::HasComponents() const {
    return _scene->GetRegistry().all_of<T...>(_handle);
}

template<typename T>
void Entity::RemoveComponent() {
    _scene->GetRegistry().remove<T>(_handle);
}

class SceneManager {
public:
    static SceneManager& Get();

    Ref<Scene> CreateScene(const std::string& name = "Untitled Scene");
    void SetActiveScene(Ref<Scene> scene);
    Ref<Scene> GetActiveScene() const { return _active_scene; }

    void OnUpdate(f32 delta_time);
    void OnRender(Renderer* renderer, VkCommandBuffer cmd, VkRenderPass render_pass);
    void OnViewportResize(u32 width, u32 height);

private:
    SceneManager() = default;

    Ref<Scene> _active_scene;
    std::vector<Ref<Scene>> _scenes;
};

} // namespace tvk
