/**
 * @file material.h
 * @brief Material system for rendering
 */

#pragma once

#include "../core/types.h"
#include "texture.h"
#include <vulkan/vulkan.h>
#include <string>
#include <unordered_map>

namespace tvk {

class Renderer;
class Pipeline;
class Buffer;

enum class BlendMode {
    Opaque,
    AlphaBlend,
    Additive,
    Multiply
};

enum class CullMode {
    None,
    Front,
    Back
};

struct MaterialProperties {
    Vec4 base_color = Vec4(1.0f);
    f32 metallic = 0.0f;
    f32 roughness = 0.5f;
    f32 ao = 1.0f;
    f32 emission_strength = 0.0f;
    Vec3 emission_color = Vec3(0.0f);
    f32 _padding0 = 0.0f;
};

class Material {
public:
    Material() = default;
    ~Material();

    Material(const Material&) = delete;
    Material& operator=(const Material&) = delete;

    static Ref<Material> Create(Renderer* renderer, const std::string& name = "");
    static Ref<Material> CreateFromShaders(Renderer* renderer, const std::string& vertex_source, const std::string& fragment_source, const std::string& name = "");

    void Destroy();

    void SetBaseColor(const Vec4& color);
    void SetMetallic(f32 metallic);
    void SetRoughness(f32 roughness);
    void SetAO(f32 ao);
    void SetEmission(const Vec3& color, f32 strength);

    void SetAlbedoTexture(Ref<Texture> texture);
    void SetNormalTexture(Ref<Texture> texture);
    void SetMetallicRoughnessTexture(Ref<Texture> texture);
    void SetAOTexture(Ref<Texture> texture);
    void SetEmissionTexture(Ref<Texture> texture);

    void SetBlendMode(BlendMode mode);
    void SetCullMode(CullMode mode);
    void SetDoubleSided(bool double_sided);
    void SetWireframe(bool wireframe);

    void Bind(VkCommandBuffer cmd, VkRenderPass render_pass);
    void UpdateDescriptors();

    const std::string& GetName() const { return _name; }
    BlendMode GetBlendMode() const { return _blend_mode; }
    CullMode GetCullMode() const { return _cull_mode; }
    bool IsDoubleSided() const { return _double_sided; }
    bool IsWireframe() const { return _wireframe; }
    bool IsDirty() const { return _pipeline_dirty; }

    VkDescriptorSet GetDescriptorSet() const { return _descriptor_set; }
    VkPipelineLayout GetPipelineLayout() const;

private:
    void CreateDescriptorResources();
    void CreatePipeline(VkRenderPass render_pass);
    void DestroyPipeline();

    Renderer* _renderer = nullptr;
    std::string _name;

    std::string _vertex_source;
    std::string _fragment_source;

    MaterialProperties _properties;
    Ref<Buffer> _uniform_buffer;

    Ref<Texture> _albedo_texture;
    Ref<Texture> _normal_texture;
    Ref<Texture> _metallic_roughness_texture;
    Ref<Texture> _ao_texture;
    Ref<Texture> _emission_texture;

    BlendMode _blend_mode = BlendMode::Opaque;
    CullMode _cull_mode = CullMode::Back;
    bool _double_sided = false;
    bool _wireframe = false;

    VkPipeline _pipeline = VK_NULL_HANDLE;
    VkPipelineLayout _pipeline_layout = VK_NULL_HANDLE;
    VkDescriptorSetLayout _descriptor_set_layout = VK_NULL_HANDLE;
    VkDescriptorPool _descriptor_pool = VK_NULL_HANDLE;
    VkDescriptorSet _descriptor_set = VK_NULL_HANDLE;

    bool _pipeline_dirty = true;
    bool _descriptors_dirty = true;
};

class MaterialLibrary {
public:
    static MaterialLibrary& Get();

    void Add(const std::string& name, Ref<Material> material);
    Ref<Material> Get(const std::string& name);
    bool Exists(const std::string& name) const;
    void Remove(const std::string& name);
    void Clear();

    Ref<Material> GetDefault(Renderer* renderer);

private:
    MaterialLibrary() = default;

    std::unordered_map<std::string, Ref<Material>> _materials;
    Ref<Material> _default_material;
};

} // namespace tvk
