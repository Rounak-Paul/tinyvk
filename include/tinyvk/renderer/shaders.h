/**
 * @file shaders.h
 * @brief Embedded shader source code
 */

#pragma once

namespace tvk {
namespace shaders {

constexpr const char* basic_vert = R"(
#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inColor;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 fragNormal;

layout(push_constant) uniform PushConstants {
    mat4 modelMatrix;
    mat4 viewProjectionMatrix;
} push;

void main() {
    gl_Position = push.viewProjectionMatrix * push.modelMatrix * vec4(inPosition, 1.0);
    fragColor = inColor;
    fragNormal = mat3(push.modelMatrix) * inNormal;
}
)";

constexpr const char* basic_frag = R"(
#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragNormal;

layout(location = 0) out vec4 outColor;

void main() {
    vec3 lightDir = normalize(vec3(0.5, 1.0, 0.3));
    float light = max(dot(normalize(fragNormal), lightDir), 0.2);
    outColor = vec4(fragColor * light, 1.0);
}
)";

} // namespace shaders
} // namespace tvk
