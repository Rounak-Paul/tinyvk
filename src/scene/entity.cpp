/**
 * @file entity.cpp
 * @brief Entity implementation
 */

#include "tinyvk/scene/scene.h"

namespace tvk {

Entity::Entity(entt::entity handle, Scene* scene)
    : _handle(handle), _scene(scene) {
}

bool Entity::IsValid() const {
    return _handle != entt::null && _scene != nullptr && _scene->GetRegistry().valid(_handle);
}

} // namespace tvk
