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
layout(location = 2) out vec2 fragTexCoord;

layout(push_constant) uniform PushConstants {
    mat4 modelMatrix;
    mat4 viewProjectionMatrix;
} push;

void main() {
    gl_Position = push.viewProjectionMatrix * push.modelMatrix * vec4(inPosition, 1.0);
    fragColor = inColor;
    fragNormal = mat3(push.modelMatrix) * inNormal;
    fragTexCoord = inTexCoord;
}
)";

constexpr const char* basic_frag = R"(
#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

void main() {
    vec3 lightDir = normalize(vec3(0.5, 1.0, 0.3));
    float light = max(dot(normalize(fragNormal), lightDir), 0.2);
    outColor = vec4(fragColor * light, 1.0);
}
)";

constexpr const char* array_multiply_comp = R"(
#version 450

layout(local_size_x = 256, local_size_y = 1, local_size_z = 1) in;

layout(std430, binding = 0) readonly buffer InputBuffer {
    float inputData[];
};

layout(std430, binding = 1) writeonly buffer OutputBuffer {
    float outputData[];
};

layout(push_constant) uniform PushConstants {
    uint count;
    float multiplier;
} push;

void main() {
    uint index = gl_GlobalInvocationID.x;
    
    if (index >= push.count) {
        return;
    }
    
    outputData[index] = inputData[index] * push.multiplier;
}
)";

constexpr const char* particle_update_comp = R"(
#version 450

layout(local_size_x = 256, local_size_y = 1, local_size_z = 1) in;

struct Particle {
    vec4 position;
    vec4 velocity;
    vec4 color;
    float life;
    float size;
    float pad0;
    float pad1;
};

layout(std430, binding = 0) buffer ParticleBuffer {
    Particle particles[];
};

layout(push_constant) uniform PushConstants {
    uint particleCount;
    float deltaTime;
    float gravity;
    float pad;
} push;

void main() {
    uint index = gl_GlobalInvocationID.x;
    
    if (index >= push.particleCount) {
        return;
    }
    
    Particle p = particles[index];
    
    p.velocity.y -= push.gravity * push.deltaTime;
    p.position.xyz += p.velocity.xyz * push.deltaTime;
    p.life -= push.deltaTime;
    
    particles[index] = p;
}
)";

} // namespace shaders
} // namespace tvk
