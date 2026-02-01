/**
 * @file shaders.h
 * @brief Embedded shader source code for compute and utility operations
 */

#pragma once

namespace tvk {
namespace shaders {

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

constexpr const char* fullscreen_vert = R"(
#version 450

layout(location = 0) out vec2 fragTexCoord;

void main() {
    fragTexCoord = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
    gl_Position = vec4(fragTexCoord * 2.0 - 1.0, 0.0, 1.0);
}
)";

constexpr const char* tonemap_frag = R"(
#version 450

layout(location = 0) in vec2 fragTexCoord;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D hdrTexture;

layout(push_constant) uniform PushConstants {
    float exposure;
    float gamma;
} push;

void main() {
    vec3 hdrColor = texture(hdrTexture, fragTexCoord).rgb;
    vec3 mapped = vec3(1.0) - exp(-hdrColor * push.exposure);
    mapped = pow(mapped, vec3(1.0 / push.gamma));
    outColor = vec4(mapped, 1.0);
}
)";

} // namespace shaders
} // namespace tvk
