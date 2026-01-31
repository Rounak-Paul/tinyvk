/**
 * @file entity.h
 * @brief Entity wrapper for EnTT registry
 */

#pragma once

#include "../core/types.h"
#include <entt/entt.hpp>

namespace tvk {

class Scene;

class Entity {
public:
    Entity() = default;
    Entity(entt::entity handle, Scene* scene);
    Entity(const Entity& other) = default;

    template<typename T, typename... Args>
    T& AddComponent(Args&&... args);

    template<typename T, typename... Args>
    T& AddOrReplaceComponent(Args&&... args);

    template<typename T>
    T& GetComponent();

    template<typename T>
    const T& GetComponent() const;

    template<typename T>
    T* TryGetComponent();

    template<typename T>
    const T* TryGetComponent() const;

    template<typename T>
    bool HasComponent() const;

    template<typename... T>
    bool HasComponents() const;

    template<typename T>
    void RemoveComponent();

    bool IsValid() const;

    operator bool() const { return IsValid(); }
    operator entt::entity() const { return _handle; }
    operator u32() const { return static_cast<u32>(_handle); }

    bool operator==(const Entity& other) const {
        return _handle == other._handle && _scene == other._scene;
    }

    bool operator!=(const Entity& other) const {
        return !(*this == other);
    }

    entt::entity GetHandle() const { return _handle; }
    Scene* GetScene() const { return _scene; }

private:
    entt::entity _handle = entt::null;
    Scene* _scene = nullptr;
};

} // namespace tvk
