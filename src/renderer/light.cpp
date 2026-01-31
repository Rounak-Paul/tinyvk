/**
 * @file light.cpp
 * @brief Light system implementation
 */

#include "tinyvk/renderer/light.h"
#include <algorithm>

namespace tvk {

LightManager& LightManager::Get() {
    static LightManager instance;
    return instance;
}

u32 LightManager::AddLight(const Light& light) {
    _lights.push_back(light);
    return static_cast<u32>(_lights.size() - 1);
}

void LightManager::RemoveLight(u32 id) {
    if (id < _lights.size()) {
        _lights.erase(_lights.begin() + id);
    }
}

void LightManager::UpdateLight(u32 id, const Light& light) {
    if (id < _lights.size()) {
        _lights[id] = light;
    }
}

Light* LightManager::GetLight(u32 id) {
    if (id < _lights.size()) {
        return &_lights[id];
    }
    return nullptr;
}

void LightManager::Clear() {
    _lights.clear();
}

LightingUBO LightManager::BuildUBO(const Vec3& camera_position) const {
    LightingUBO ubo{};
    ubo.ambient_color = Vec4(_ambient_color * _ambient_intensity, 1.0f);
    ubo.camera_position = Vec4(camera_position, 1.0f);
    ubo.num_lights = std::min(static_cast<u32>(_lights.size()), MAX_LIGHTS);

    for (u32 i = 0; i < ubo.num_lights; ++i) {
        const Light& light = _lights[i];
        if (!light.enabled) continue;

        LightData& data = ubo.lights[i];
        data.position_type = Vec4(light.position, static_cast<f32>(light.type));
        data.direction_range = Vec4(glm::normalize(light.direction), light.range);
        data.color_intensity = Vec4(light.color, light.intensity);
        data.cone_angles = Vec4(
            glm::cos(glm::radians(light.inner_cone_angle)),
            glm::cos(glm::radians(light.outer_cone_angle)),
            light.cast_shadows ? 1.0f : 0.0f,
            0.0f
        );
    }

    return ubo;
}

} // namespace tvk
