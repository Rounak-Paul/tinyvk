/**
 * @file pipeline.cpp
 * @brief Graphics and compute pipeline implementation
 */

#include "tinyvk/renderer/pipeline.h"
#include "tinyvk/renderer/renderer.h"
#include "tinyvk/renderer/context.h"
#include "tinyvk/renderer/buffer.h"
#include "tinyvk/renderer/shader_compiler.h"
#include "tinyvk/core/log.h"

namespace tvk {

Pipeline::~Pipeline() {
    Destroy();
}

bool Pipeline::Create(Renderer* renderer, VkRenderPass renderPass, const std::string& vertShaderSource, const std::string& fragShaderSource) {
    _renderer = renderer;
    auto& ctx = renderer->GetContext();
    VkDevice device = ctx.GetDevice();

    VkShaderModule vertShaderModule = ShaderCompiler::CreateShaderModuleFromGLSL(
        renderer, vertShaderSource, ShaderStage::Vertex, "basic.vert"
    );
    VkShaderModule fragShaderModule = ShaderCompiler::CreateShaderModuleFromGLSL(
        renderer, fragShaderSource, ShaderStage::Fragment, "basic.frag"
    );
    
    if (vertShaderModule == VK_NULL_HANDLE || fragShaderModule == VK_NULL_HANDLE) {
        TVK_LOG_ERROR("Failed to create shader modules");
        return false;
    }

    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

    auto bindingDescription = Vertex::GetBindingDescription();
    auto attributeDescriptions = Vertex::GetAttributeDescriptions();

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<u32>(attributeDescriptions.size());
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    std::vector<VkDynamicState> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<u32>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(PushConstants);

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

    if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &_layout) != VK_SUCCESS) {
        TVK_LOG_ERROR("Failed to create pipeline layout");
        vkDestroyShaderModule(device, vertShaderModule, nullptr);
        vkDestroyShaderModule(device, fragShaderModule, nullptr);
        return false;
    }

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = _layout;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;

    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &_pipeline) != VK_SUCCESS) {
        TVK_LOG_ERROR("Failed to create graphics pipeline");
        vkDestroyShaderModule(device, vertShaderModule, nullptr);
        vkDestroyShaderModule(device, fragShaderModule, nullptr);
        return false;
    }

    vkDestroyShaderModule(device, vertShaderModule, nullptr);
    vkDestroyShaderModule(device, fragShaderModule, nullptr);

    TVK_LOG_INFO("Graphics pipeline created successfully");
    return true;
}

void Pipeline::Destroy() {
    if (!_renderer) return;
    
    auto& ctx = _renderer->GetContext();
    VkDevice device = ctx.GetDevice();

    if (_pipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(device, _pipeline, nullptr);
        _pipeline = VK_NULL_HANDLE;
    }

    if (_layout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(device, _layout, nullptr);
        _layout = VK_NULL_HANDLE;
    }
}

void Pipeline::Bind(VkCommandBuffer cmd) {
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipeline);
}

void Pipeline::SetPushConstants(VkCommandBuffer cmd, const PushConstants& constants) {
    vkCmdPushConstants(cmd, _layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstants), &constants);
}

ComputePipeline::~ComputePipeline() {
    Destroy();
}

bool ComputePipeline::Create(Renderer* renderer, const std::string& computeShaderSource) {
    _renderer = renderer;
    auto& ctx = renderer->GetContext();
    VkDevice device = ctx.GetDevice();

    VkShaderModule computeShaderModule = ShaderCompiler::CreateShaderModuleFromGLSL(
        renderer, computeShaderSource, ShaderStage::Compute, "compute.comp"
    );
    
    if (computeShaderModule == VK_NULL_HANDLE) {
        TVK_LOG_ERROR("Failed to create compute shader module");
        return false;
    }

    if (!CreateDescriptorResources()) {
        vkDestroyShaderModule(device, computeShaderModule, nullptr);
        return false;
    }

    VkPipelineShaderStageCreateInfo computeShaderStageInfo{};
    computeShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    computeShaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    computeShaderStageInfo.module = computeShaderModule;
    computeShaderStageInfo.pName = "main";

    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = 128;

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &_descriptorSetLayout;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

    if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &_layout) != VK_SUCCESS) {
        TVK_LOG_ERROR("Failed to create compute pipeline layout");
        vkDestroyShaderModule(device, computeShaderModule, nullptr);
        return false;
    }

    VkComputePipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineInfo.stage = computeShaderStageInfo;
    pipelineInfo.layout = _layout;

    if (vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &_pipeline) != VK_SUCCESS) {
        TVK_LOG_ERROR("Failed to create compute pipeline");
        vkDestroyShaderModule(device, computeShaderModule, nullptr);
        return false;
    }

    vkDestroyShaderModule(device, computeShaderModule, nullptr);

    TVK_LOG_INFO("Compute pipeline created successfully");
    return true;
}

bool ComputePipeline::CreateDescriptorResources() {
    auto& ctx = _renderer->GetContext();
    VkDevice device = ctx.GetDevice();

    VkDescriptorSetLayoutBinding bindings[MAX_STORAGE_BINDINGS];
    for (u32 i = 0; i < MAX_STORAGE_BINDINGS; i++) {
        bindings[i].binding = i;
        bindings[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        bindings[i].descriptorCount = 1;
        bindings[i].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        bindings[i].pImmutableSamplers = nullptr;
    }

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = MAX_STORAGE_BINDINGS;
    layoutInfo.pBindings = bindings;

    if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &_descriptorSetLayout) != VK_SUCCESS) {
        TVK_LOG_ERROR("Failed to create compute descriptor set layout");
        return false;
    }

    VkDescriptorPoolSize poolSize{};
    poolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    poolSize.descriptorCount = MAX_STORAGE_BINDINGS;

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    poolInfo.maxSets = 1;

    if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &_descriptorPool) != VK_SUCCESS) {
        TVK_LOG_ERROR("Failed to create compute descriptor pool");
        return false;
    }

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = _descriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &_descriptorSetLayout;

    if (vkAllocateDescriptorSets(device, &allocInfo, &_descriptorSet) != VK_SUCCESS) {
        TVK_LOG_ERROR("Failed to allocate compute descriptor set");
        return false;
    }

    return true;
}

void ComputePipeline::Destroy() {
    if (!_renderer) return;
    
    auto& ctx = _renderer->GetContext();
    VkDevice device = ctx.GetDevice();

    if (_pipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(device, _pipeline, nullptr);
        _pipeline = VK_NULL_HANDLE;
    }

    if (_layout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(device, _layout, nullptr);
        _layout = VK_NULL_HANDLE;
    }

    if (_descriptorPool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(device, _descriptorPool, nullptr);
        _descriptorPool = VK_NULL_HANDLE;
    }

    if (_descriptorSetLayout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(device, _descriptorSetLayout, nullptr);
        _descriptorSetLayout = VK_NULL_HANDLE;
    }
}

void ComputePipeline::Bind(VkCommandBuffer cmd) {
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, _pipeline);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, _layout, 0, 1, &_descriptorSet, 0, nullptr);
}

void ComputePipeline::Dispatch(VkCommandBuffer cmd, u32 groupCountX, u32 groupCountY, u32 groupCountZ) {
    vkCmdDispatch(cmd, groupCountX, groupCountY, groupCountZ);
}

void ComputePipeline::BindStorageBuffer(u32 binding, Buffer* buffer) {
    if (binding >= MAX_STORAGE_BINDINGS) return;
    _boundBuffers[binding] = buffer;
}

void ComputePipeline::BindStorageBuffers(Buffer* buffer0, Buffer* buffer1) {
    _boundBuffers[0] = buffer0;
    _boundBuffers[1] = buffer1;
}

void ComputePipeline::UpdateDescriptors() {
    if (!_renderer) return;
    
    auto& ctx = _renderer->GetContext();
    VkDevice device = ctx.GetDevice();

    VkWriteDescriptorSet writes[MAX_STORAGE_BINDINGS];
    VkDescriptorBufferInfo bufferInfos[MAX_STORAGE_BINDINGS];
    u32 writeCount = 0;

    for (u32 i = 0; i < MAX_STORAGE_BINDINGS; i++) {
        if (_boundBuffers[i] == nullptr) continue;
        
        bufferInfos[writeCount].buffer = _boundBuffers[i]->GetBuffer();
        bufferInfos[writeCount].offset = 0;
        bufferInfos[writeCount].range = _boundBuffers[i]->GetSize();

        writes[writeCount].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[writeCount].pNext = nullptr;
        writes[writeCount].dstSet = _descriptorSet;
        writes[writeCount].dstBinding = i;
        writes[writeCount].dstArrayElement = 0;
        writes[writeCount].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        writes[writeCount].descriptorCount = 1;
        writes[writeCount].pBufferInfo = &bufferInfos[writeCount];
        writes[writeCount].pImageInfo = nullptr;
        writes[writeCount].pTexelBufferView = nullptr;
        writeCount++;
    }

    if (writeCount > 0) {
        vkUpdateDescriptorSets(device, writeCount, writes, 0, nullptr);
    }
}

} // namespace tvk
