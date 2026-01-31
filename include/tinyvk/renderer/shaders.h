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

layout(location = 0) out vec3 fragWorldPos;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec2 fragTexCoord;
layout(location = 3) out vec3 fragColor;

layout(push_constant) uniform PushConstants {
    mat4 modelMatrix;
    mat4 viewProjectionMatrix;
} push;

void main() {
    vec4 worldPos = push.modelMatrix * vec4(inPosition, 1.0);
    fragWorldPos = worldPos.xyz;
    fragNormal = normalize(mat3(push.modelMatrix) * inNormal);
    fragTexCoord = inTexCoord;
    fragColor = inColor;
    gl_Position = push.viewProjectionMatrix * worldPos;
}
)";

constexpr const char* basic_frag = R"(
#version 450

layout(location = 0) in vec3 fragWorldPos;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec2 fragTexCoord;
layout(location = 3) in vec3 fragColor;

layout(location = 0) out vec4 outColor;

const int MAX_LIGHTS = 8;

struct GPULightData {
    vec4 position_type;
    vec4 direction_range;
    vec4 color_intensity;
    vec4 shadow_params;
};

layout(set = 0, binding = 0) uniform SceneLightingUBO {
    vec4 ambient_color;
    vec4 camera_position;
    mat4 shadow_matrix;
    int num_lights;
    int shadow_enabled;
    int _pad0;
    int _pad1;
    GPULightData lights[MAX_LIGHTS];
} lighting;

layout(set = 0, binding = 1) uniform sampler2D shadowMap;

float CalculateShadow(vec3 worldPos) {
    if (lighting.shadow_enabled == 0) return 1.0;
    
    vec4 lightSpacePos = lighting.shadow_matrix * vec4(worldPos, 1.0);
    vec3 projCoords = lightSpacePos.xyz / lightSpacePos.w;
    projCoords.xy = projCoords.xy * 0.5 + 0.5;
    
    if (projCoords.z > 1.0 || projCoords.x < 0.0 || projCoords.x > 1.0 || 
        projCoords.y < 0.0 || projCoords.y > 1.0) {
        return 1.0;
    }
    
    float bias = 0.005;
    float closestDepth = texture(shadowMap, projCoords.xy).r;
    float currentDepth = projCoords.z;
    float shadow = currentDepth - bias > closestDepth ? 0.3 : 1.0;
    return shadow;
}

void main() {
    vec3 N = normalize(fragNormal);
    vec3 V = normalize(lighting.camera_position.xyz - fragWorldPos);
    
    vec3 ambient = lighting.ambient_color.rgb * lighting.ambient_color.a;
    vec3 Lo = vec3(0.0);
    
    for (int i = 0; i < lighting.num_lights && i < MAX_LIGHTS; ++i) {
        GPULightData light = lighting.lights[i];
        int lightType = int(light.position_type.w);
        vec3 lightColor = light.color_intensity.rgb;
        float intensity = light.color_intensity.w;
        
        vec3 L;
        float attenuation = 1.0;
        float shadow = 1.0;
        
        if (lightType == 0) {
            L = normalize(-light.direction_range.xyz);
            if (light.shadow_params.x > 0.5) {
                shadow = CalculateShadow(fragWorldPos);
            }
        } else if (lightType == 1) {
            vec3 lightPos = light.position_type.xyz;
            vec3 toLight = lightPos - fragWorldPos;
            float dist = length(toLight);
            L = toLight / dist;
            float range = light.direction_range.w;
            float falloff = light.shadow_params.y;
            attenuation = pow(clamp(1.0 - (dist / range), 0.0, 1.0), falloff);
        } else if (lightType == 2) {
            vec3 lightPos = light.position_type.xyz;
            vec3 toLight = lightPos - fragWorldPos;
            float dist = length(toLight);
            L = toLight / dist;
            float range = light.direction_range.w;
            attenuation = clamp(1.0 - (dist / range), 0.0, 1.0);
            attenuation *= attenuation;
            
            float theta = dot(L, normalize(-light.direction_range.xyz));
            float innerCos = light.shadow_params.y;
            float outerCos = light.shadow_params.z;
            float epsilon = innerCos - outerCos;
            float spotIntensity = clamp((theta - outerCos) / max(epsilon, 0.001), 0.0, 1.0);
            attenuation *= spotIntensity;
        }
        
        float NdotL = max(dot(N, L), 0.0);
        vec3 diffuse = lightColor * intensity * NdotL * attenuation * shadow;
        
        vec3 H = normalize(V + L);
        float spec = pow(max(dot(N, H), 0.0), 32.0);
        vec3 specular = lightColor * intensity * spec * attenuation * shadow * 0.5;
        
        Lo += diffuse + specular;
    }
    
    vec3 color = fragColor * (ambient + Lo);
    color = color / (color + vec3(1.0));
    
    outColor = vec4(color, 1.0);
}
)";

constexpr const char* shadow_vert = R"(
#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inColor;

layout(push_constant) uniform PushConstants {
    mat4 modelMatrix;
    mat4 lightSpaceMatrix;
} push;

void main() {
    gl_Position = push.lightSpaceMatrix * push.modelMatrix * vec4(inPosition, 1.0);
}
)";

constexpr const char* shadow_frag = R"(
#version 450

void main() {
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

constexpr const char* pbr_vert = R"(
#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inColor;

layout(location = 0) out vec3 fragWorldPos;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec2 fragTexCoord;
layout(location = 3) out vec3 fragColor;

layout(push_constant) uniform PushConstants {
    mat4 model;
    mat4 viewProjection;
} push;

void main() {
    vec4 worldPos = push.model * vec4(inPosition, 1.0);
    fragWorldPos = worldPos.xyz;
    fragNormal = normalize(mat3(push.model) * inNormal);
    fragTexCoord = inTexCoord;
    fragColor = inColor;
    gl_Position = push.viewProjection * worldPos;
}
)";

constexpr const char* pbr_frag = R"(
#version 450

layout(location = 0) in vec3 fragWorldPos;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec2 fragTexCoord;
layout(location = 3) in vec3 fragColor;

layout(location = 0) out vec4 outColor;

const float PI = 3.14159265359;
const int MAX_LIGHTS = 16;

struct LightData {
    vec4 position_type;
    vec4 direction_range;
    vec4 color_intensity;
    vec4 cone_angles;
};

layout(set = 0, binding = 0) uniform MaterialUBO {
    vec4 baseColor;
    float metallic;
    float roughness;
    float ao;
    float emissionStrength;
    vec3 emissionColor;
    float _padding;
} material;

layout(set = 0, binding = 1) uniform LightingUBO {
    vec4 ambientColor;
    vec4 cameraPosition;
    uint numLights;
    uint _padding[3];
    LightData lights[MAX_LIGHTS];
} lighting;

float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    float nom = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
    return nom / max(denom, 0.0001);
}

float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;
    float nom = NdotV;
    float denom = NdotV * (1.0 - k) + k;
    return nom / max(denom, 0.0001);
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);
    return ggx1 * ggx2;
}

vec3 FresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

void main() {
    vec3 albedo = material.baseColor.rgb * fragColor;
    float metallic = material.metallic;
    float roughness = max(material.roughness, 0.04);
    float ao = material.ao;

    vec3 N = normalize(fragNormal);
    vec3 V = normalize(lighting.cameraPosition.xyz - fragWorldPos);

    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);

    vec3 Lo = vec3(0.0);

    for (uint i = 0u; i < lighting.numLights && i < MAX_LIGHTS; ++i) {
        LightData light = lighting.lights[i];
        int lightType = int(light.position_type.w);
        vec3 lightColor = light.color_intensity.rgb;
        float intensity = light.color_intensity.w;

        vec3 L;
        float attenuation = 1.0;

        if (lightType == 0) {
            L = normalize(-light.direction_range.xyz);
        } else if (lightType == 1) {
            vec3 lightPos = light.position_type.xyz;
            L = lightPos - fragWorldPos;
            float distance = length(L);
            L = normalize(L);
            float range = light.direction_range.w;
            attenuation = clamp(1.0 - (distance / range), 0.0, 1.0);
            attenuation *= attenuation;
        } else if (lightType == 2) {
            vec3 lightPos = light.position_type.xyz;
            L = lightPos - fragWorldPos;
            float distance = length(L);
            L = normalize(L);
            float range = light.direction_range.w;
            attenuation = clamp(1.0 - (distance / range), 0.0, 1.0);
            attenuation *= attenuation;
            float theta = dot(L, normalize(-light.direction_range.xyz));
            float innerCone = light.cone_angles.x;
            float outerCone = light.cone_angles.y;
            float epsilon = innerCone - outerCone;
            float spotIntensity = clamp((theta - outerCone) / max(epsilon, 0.001), 0.0, 1.0);
            attenuation *= spotIntensity;
        }

        vec3 H = normalize(V + L);
        vec3 radiance = lightColor * intensity * attenuation;

        float NDF = DistributionGGX(N, H, roughness);
        float G = GeometrySmith(N, V, L, roughness);
        vec3 F = FresnelSchlick(max(dot(H, V), 0.0), F0);

        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        kD *= 1.0 - metallic;

        vec3 numerator = NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
        vec3 specular = numerator / denominator;

        float NdotL = max(dot(N, L), 0.0);
        Lo += (kD * albedo / PI + specular) * radiance * NdotL;
    }

    vec3 ambient = lighting.ambientColor.rgb * albedo * ao;
    vec3 emission = material.emissionColor * material.emissionStrength;
    vec3 color = ambient + Lo + emission;

    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0 / 2.2));

    outColor = vec4(color, material.baseColor.a);
}
)";

constexpr const char* skybox_vert = R"(
#version 450

layout(location = 0) in vec3 inPosition;

layout(location = 0) out vec3 fragTexCoord;

layout(push_constant) uniform PushConstants {
    mat4 viewProjection;
} push;

void main() {
    fragTexCoord = inPosition;
    vec4 pos = push.viewProjection * vec4(inPosition, 1.0);
    gl_Position = pos.xyww;
}
)";

constexpr const char* skybox_frag = R"(
#version 450

layout(location = 0) in vec3 fragTexCoord;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform samplerCube skybox;

void main() {
    outColor = texture(skybox, fragTexCoord);
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
