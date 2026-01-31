/**
 * @file light.h
 * @brief Light system for the renderer
 */

#pragma once

#include "../core/types.h"
#include <vector>

namespace tvk {

enum class LightType {
    Directional,
    Point,
    Spot,
    Ambient
};

struct Light {
    LightType type = LightType::Point;
    Vec3 position = Vec3(0.0f);
    Vec3 direction = Vec3(0.0f, -1.0f, 0.0f);
    Vec3 color = Vec3(1.0f);
    f32 intensity = 1.0f;
    f32 range = 10.0f;
    f32 inner_cone_angle = 30.0f;
    f32 outer_cone_angle = 45.0f;
    bool cast_shadows = false;
    bool enabled = true;
};

struct LightData {
    alignas(16) Vec4 position_type;
    alignas(16) Vec4 direction_range;
    alignas(16) Vec4 color_intensity;
    alignas(16) Vec4 cone_angles;
};

static constexpr u32 MAX_LIGHTS = 16;

struct LightingUBO {
    alignas(16) Vec4 ambient_color;
    alignas(16) Vec4 camera_position;
    alignas(4) u32 num_lights;
    alignas(4) u32 _padding[3];
    LightData lights[MAX_LIGHTS];
};

class LightManager {
public:
    static LightManager& Get();

    u32 AddLight(const Light& light);
    void RemoveLight(u32 id);
    void UpdateLight(u32 id, const Light& light);
    Light* GetLight(u32 id);

    void SetAmbientColor(const Vec3& color) { _ambient_color = color; }
    Vec3 GetAmbientColor() const { return _ambient_color; }

    void SetAmbientIntensity(f32 intensity) { _ambient_intensity = intensity; }
    f32 GetAmbientIntensity() const { return _ambient_intensity; }

    void Clear();

    const std::vector<Light>& GetAllLights() const { return _lights; }
    u32 GetLightCount() const { return static_cast<u32>(_lights.size()); }

    LightingUBO BuildUBO(const Vec3& camera_position) const;

private:
    LightManager() = default;

    std::vector<Light> _lights;
    Vec3 _ambient_color = Vec3(0.03f);
    f32 _ambient_intensity = 1.0f;
};

} // namespace tvk
