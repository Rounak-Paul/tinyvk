/**
 * @file material.cpp
 * @brief Material system implementation
 */

#include "tinyvk/renderer/material.h"
#include "tinyvk/renderer/renderer.h"
#include "tinyvk/renderer/buffer.h"
#include "tinyvk/renderer/vertex.h"
#include "tinyvk/renderer/shader_compiler.h"
#include "tinyvk/renderer/shaders.h"
#include "tinyvk/core/log.h"
#include <array>

namespace tvk {

Material::~Material() {
    Destroy();
}

Ref<Material> Material::Create(Renderer* renderer, const std::string& name) {
    return CreateFromShaders(renderer, shaders::basic_vert, shaders::basic_frag, name);
}

Ref<Material> Material::CreateFromShaders(Renderer* renderer, const std::string& vertex_source, const std::string& fragment_source, const std::string& name) {
    auto material = CreateRef<Material>();
    material->_renderer = renderer;
    material->_name = name.empty() ? "Material" : name;
    material->_vertex_source = vertex_source;
    material->_fragment_source = fragment_source;

    material->_uniform_buffer = Buffer::Create(renderer, sizeof(MaterialProperties), BufferUsage::Uniform, &material->_properties);
    if (!material->_uniform_buffer) {
        TVK_LOG_ERROR("Failed to create material uniform buffer");
        return nullptr;
    }

    material->CreateDescriptorResources();

    return material;
}

void Material::Destroy() {
    if (!_renderer) return;

    VkDevice device = _renderer->GetContext().GetDevice();
    vkDeviceWaitIdle(device);

    DestroyPipeline();

    if (_descriptor_pool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(device, _descriptor_pool, nullptr);
        _descriptor_pool = VK_NULL_HANDLE;
    }
    if (_descriptor_set_layout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(device, _descriptor_set_layout, nullptr);
        _descriptor_set_layout = VK_NULL_HANDLE;
    }

    _uniform_buffer.reset();
    _renderer = nullptr;
}

void Material::SetBaseColor(const Vec4& color) {
    _properties.base_color = color;
    _uniform_buffer->SetData(&_properties, sizeof(MaterialProperties));
}

void Material::SetMetallic(f32 metallic) {
    _properties.metallic = metallic;
    _uniform_buffer->SetData(&_properties, sizeof(MaterialProperties));
}

void Material::SetRoughness(f32 roughness) {
    _properties.roughness = roughness;
    _uniform_buffer->SetData(&_properties, sizeof(MaterialProperties));
}

void Material::SetAO(f32 ao) {
    _properties.ao = ao;
    _uniform_buffer->SetData(&_properties, sizeof(MaterialProperties));
}

void Material::SetEmission(const Vec3& color, f32 strength) {
    _properties.emission_color = color;
    _properties.emission_strength = strength;
    _uniform_buffer->SetData(&_properties, sizeof(MaterialProperties));
}

void Material::SetAlbedoTexture(Ref<Texture> texture) {
    _albedo_texture = texture;
    _descriptors_dirty = true;
}

void Material::SetNormalTexture(Ref<Texture> texture) {
    _normal_texture = texture;
    _descriptors_dirty = true;
}

void Material::SetMetallicRoughnessTexture(Ref<Texture> texture) {
    _metallic_roughness_texture = texture;
    _descriptors_dirty = true;
}

void Material::SetAOTexture(Ref<Texture> texture) {
    _ao_texture = texture;
    _descriptors_dirty = true;
}

void Material::SetEmissionTexture(Ref<Texture> texture) {
    _emission_texture = texture;
    _descriptors_dirty = true;
}

void Material::SetBlendMode(BlendMode mode) {
    if (_blend_mode != mode) {
        _blend_mode = mode;
        _pipeline_dirty = true;
    }
}

void Material::SetCullMode(CullMode mode) {
    if (_cull_mode != mode) {
        _cull_mode = mode;
        _pipeline_dirty = true;
    }
}

void Material::SetDoubleSided(bool double_sided) {
    if (_double_sided != double_sided) {
        _double_sided = double_sided;
        _cull_mode = double_sided ? CullMode::None : CullMode::Back;
        _pipeline_dirty = true;
    }
}

void Material::SetWireframe(bool wireframe) {
    if (_wireframe != wireframe) {
        _wireframe = wireframe;
        _pipeline_dirty = true;
    }
}

void Material::Bind(VkCommandBuffer cmd, VkRenderPass render_pass) {
    if (_pipeline_dirty) {
        CreatePipeline(render_pass);
    }

    if (_descriptors_dirty) {
        UpdateDescriptors();
    }

    if (_pipeline != VK_NULL_HANDLE) {
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipeline);

        if (_descriptor_set != VK_NULL_HANDLE) {
            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipeline_layout, 0, 1, &_descriptor_set, 0, nullptr);
        }
    }
}

void Material::UpdateDescriptors() {
    if (!_renderer || _descriptor_set == VK_NULL_HANDLE) return;

    VkDevice device = _renderer->GetContext().GetDevice();

    VkDescriptorBufferInfo buffer_info{};
    buffer_info.buffer = _uniform_buffer->GetBuffer();
    buffer_info.offset = 0;
    buffer_info.range = sizeof(MaterialProperties);

    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = _descriptor_set;
    write.dstBinding = 0;
    write.dstArrayElement = 0;
    write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    write.descriptorCount = 1;
    write.pBufferInfo = &buffer_info;

    vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);

    _descriptors_dirty = false;
}

VkPipelineLayout Material::GetPipelineLayout() const {
    return _pipeline_layout;
}

void Material::CreateDescriptorResources() {
    VkDevice device = _renderer->GetContext().GetDevice();

    VkDescriptorSetLayoutBinding ubo_binding{};
    ubo_binding.binding = 0;
    ubo_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    ubo_binding.descriptorCount = 1;
    ubo_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    ubo_binding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutCreateInfo layout_info{};
    layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layout_info.bindingCount = 1;
    layout_info.pBindings = &ubo_binding;

    if (vkCreateDescriptorSetLayout(device, &layout_info, nullptr, &_descriptor_set_layout) != VK_SUCCESS) {
        TVK_LOG_ERROR("Failed to create material descriptor set layout");
        return;
    }

    VkDescriptorPoolSize pool_size{};
    pool_size.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    pool_size.descriptorCount = 1;

    VkDescriptorPoolCreateInfo pool_info{};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.poolSizeCount = 1;
    pool_info.pPoolSizes = &pool_size;
    pool_info.maxSets = 1;

    if (vkCreateDescriptorPool(device, &pool_info, nullptr, &_descriptor_pool) != VK_SUCCESS) {
        TVK_LOG_ERROR("Failed to create material descriptor pool");
        return;
    }

    VkDescriptorSetAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.descriptorPool = _descriptor_pool;
    alloc_info.descriptorSetCount = 1;
    alloc_info.pSetLayouts = &_descriptor_set_layout;

    if (vkAllocateDescriptorSets(device, &alloc_info, &_descriptor_set) != VK_SUCCESS) {
        TVK_LOG_ERROR("Failed to allocate material descriptor set");
        return;
    }

    UpdateDescriptors();
}

void Material::CreatePipeline(VkRenderPass render_pass) {
    DestroyPipeline();

    VkDevice device = _renderer->GetContext().GetDevice();

    VkShaderModule vert_module = ShaderCompiler::CreateShaderModuleFromGLSL(_renderer, _vertex_source, ShaderStage::Vertex);
    VkShaderModule frag_module = ShaderCompiler::CreateShaderModuleFromGLSL(_renderer, _fragment_source, ShaderStage::Fragment);

    if (vert_module == VK_NULL_HANDLE || frag_module == VK_NULL_HANDLE) {
        TVK_LOG_ERROR("Failed to compile material shaders");
        if (vert_module) vkDestroyShaderModule(device, vert_module, nullptr);
        if (frag_module) vkDestroyShaderModule(device, frag_module, nullptr);
        return;
    }

    std::array<VkPipelineShaderStageCreateInfo, 2> shader_stages{};
    shader_stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shader_stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    shader_stages[0].module = vert_module;
    shader_stages[0].pName = "main";

    shader_stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shader_stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shader_stages[1].module = frag_module;
    shader_stages[1].pName = "main";

    auto binding = Vertex::GetBindingDescription();
    auto attributes = Vertex::GetAttributeDescriptions();

    VkPipelineVertexInputStateCreateInfo vertex_input{};
    vertex_input.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input.vertexBindingDescriptionCount = 1;
    vertex_input.pVertexBindingDescriptions = &binding;
    vertex_input.vertexAttributeDescriptionCount = static_cast<u32>(attributes.size());
    vertex_input.pVertexAttributeDescriptions = attributes.data();

    VkPipelineInputAssemblyStateCreateInfo input_assembly{};
    input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    input_assembly.primitiveRestartEnable = VK_FALSE;

    VkPipelineViewportStateCreateInfo viewport_state{};
    viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state.viewportCount = 1;
    viewport_state.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = _wireframe ? VK_POLYGON_MODE_LINE : VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;

    switch (_cull_mode) {
        case CullMode::None:  rasterizer.cullMode = VK_CULL_MODE_NONE; break;
        case CullMode::Front: rasterizer.cullMode = VK_CULL_MODE_FRONT_BIT; break;
        case CullMode::Back:  rasterizer.cullMode = VK_CULL_MODE_BACK_BIT; break;
    }
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineDepthStencilStateCreateInfo depth_stencil{};
    depth_stencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth_stencil.depthTestEnable = VK_TRUE;
    depth_stencil.depthWriteEnable = _blend_mode == BlendMode::Opaque ? VK_TRUE : VK_FALSE;
    depth_stencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depth_stencil.depthBoundsTestEnable = VK_FALSE;
    depth_stencil.stencilTestEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState color_blend_attachment{};
    color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    switch (_blend_mode) {
        case BlendMode::Opaque:
            color_blend_attachment.blendEnable = VK_FALSE;
            break;
        case BlendMode::AlphaBlend:
            color_blend_attachment.blendEnable = VK_TRUE;
            color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
            color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
            color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
            color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
            color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
            color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;
            break;
        case BlendMode::Additive:
            color_blend_attachment.blendEnable = VK_TRUE;
            color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
            color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
            color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
            color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
            color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
            color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;
            break;
        case BlendMode::Multiply:
            color_blend_attachment.blendEnable = VK_TRUE;
            color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_DST_COLOR;
            color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
            color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
            color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
            color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
            color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;
            break;
    }

    VkPipelineColorBlendStateCreateInfo color_blending{};
    color_blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blending.logicOpEnable = VK_FALSE;
    color_blending.attachmentCount = 1;
    color_blending.pAttachments = &color_blend_attachment;

    std::array<VkDynamicState, 2> dynamic_states = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamic_state{};
    dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_state.dynamicStateCount = static_cast<u32>(dynamic_states.size());
    dynamic_state.pDynamicStates = dynamic_states.data();

    VkPushConstantRange push_constant_range{};
    push_constant_range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    push_constant_range.offset = 0;
    push_constant_range.size = sizeof(glm::mat4) * 2;

    VkPipelineLayoutCreateInfo pipeline_layout_info{};
    pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_info.setLayoutCount = 1;
    pipeline_layout_info.pSetLayouts = &_descriptor_set_layout;
    pipeline_layout_info.pushConstantRangeCount = 1;
    pipeline_layout_info.pPushConstantRanges = &push_constant_range;

    if (vkCreatePipelineLayout(device, &pipeline_layout_info, nullptr, &_pipeline_layout) != VK_SUCCESS) {
        TVK_LOG_ERROR("Failed to create material pipeline layout");
        vkDestroyShaderModule(device, vert_module, nullptr);
        vkDestroyShaderModule(device, frag_module, nullptr);
        return;
    }

    VkGraphicsPipelineCreateInfo pipeline_info{};
    pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_info.stageCount = 2;
    pipeline_info.pStages = shader_stages.data();
    pipeline_info.pVertexInputState = &vertex_input;
    pipeline_info.pInputAssemblyState = &input_assembly;
    pipeline_info.pViewportState = &viewport_state;
    pipeline_info.pRasterizationState = &rasterizer;
    pipeline_info.pMultisampleState = &multisampling;
    pipeline_info.pDepthStencilState = &depth_stencil;
    pipeline_info.pColorBlendState = &color_blending;
    pipeline_info.pDynamicState = &dynamic_state;
    pipeline_info.layout = _pipeline_layout;
    pipeline_info.renderPass = render_pass;
    pipeline_info.subpass = 0;

    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &_pipeline) != VK_SUCCESS) {
        TVK_LOG_ERROR("Failed to create material graphics pipeline");
    }

    vkDestroyShaderModule(device, vert_module, nullptr);
    vkDestroyShaderModule(device, frag_module, nullptr);

    _pipeline_dirty = false;
}

void Material::DestroyPipeline() {
    if (!_renderer) return;

    VkDevice device = _renderer->GetContext().GetDevice();

    if (_pipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(device, _pipeline, nullptr);
        _pipeline = VK_NULL_HANDLE;
    }
    if (_pipeline_layout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(device, _pipeline_layout, nullptr);
        _pipeline_layout = VK_NULL_HANDLE;
    }
}

MaterialLibrary& MaterialLibrary::Get() {
    static MaterialLibrary instance;
    return instance;
}

void MaterialLibrary::Add(const std::string& name, Ref<Material> material) {
    _materials[name] = material;
}

Ref<Material> MaterialLibrary::Get(const std::string& name) {
    auto it = _materials.find(name);
    if (it != _materials.end()) {
        return it->second;
    }
    return nullptr;
}

bool MaterialLibrary::Exists(const std::string& name) const {
    return _materials.find(name) != _materials.end();
}

void MaterialLibrary::Remove(const std::string& name) {
    _materials.erase(name);
}

void MaterialLibrary::Clear() {
    _materials.clear();
    _default_material.reset();
}

Ref<Material> MaterialLibrary::GetDefault(Renderer* renderer) {
    if (!_default_material) {
        _default_material = Material::Create(renderer, "Default");
    }
    return _default_material;
}

} // namespace tvk
