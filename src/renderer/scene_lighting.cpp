#include <tinyvk/renderer/scene_lighting.h>
#include <tinyvk/renderer/renderer.h>
#include <tinyvk/renderer/context.h>
#include <tinyvk/renderer/shader_compiler.h>
#include <tinyvk/renderer/shaders.h>
#include <tinyvk/renderer/vertex.h>
#include <tinyvk/core/log.h>
#include <glm/gtc/matrix_transform.hpp>
#include <cstring>
#include <shaderc/shaderc.hpp>

namespace tvk {

SceneLighting::~SceneLighting() {
    Destroy();
}

bool SceneLighting::Create(Renderer* renderer, VkRenderPass mainRenderPass) {
    (void)mainRenderPass;
    _renderer = renderer;
    _context = &renderer->GetContext();
    
    _uboBuffer = Buffer::Create(renderer, sizeof(SceneLightingUBO), BufferUsage::Uniform, nullptr);
    if (!_uboBuffer) {
        return false;
    }
    
    CreateDescriptorResources();
    CreateShadowResources();
    CreateShadowPipeline();
    
    return true;
}

void SceneLighting::Destroy() {
    if (!_context) return;
    
    VkDevice device = _context->GetDevice();
    
    vkDeviceWaitIdle(device);
    
    if (_shadowPipeline) {
        vkDestroyPipeline(device, _shadowPipeline, nullptr);
        _shadowPipeline = VK_NULL_HANDLE;
    }
    if (_shadowPipelineLayout) {
        vkDestroyPipelineLayout(device, _shadowPipelineLayout, nullptr);
        _shadowPipelineLayout = VK_NULL_HANDLE;
    }
    
    CleanupShadowResources();
    
    if (_descriptorPool) {
        vkDestroyDescriptorPool(device, _descriptorPool, nullptr);
        _descriptorPool = VK_NULL_HANDLE;
    }
    if (_descriptorSetLayout) {
        vkDestroyDescriptorSetLayout(device, _descriptorSetLayout, nullptr);
        _descriptorSetLayout = VK_NULL_HANDLE;
    }
    
    _uboBuffer.reset();
    
    _context = nullptr;
    _renderer = nullptr;
}

void SceneLighting::CreateDescriptorResources() {
    VkDevice device = _context->GetDevice();
    
    VkDescriptorSetLayoutBinding bindings[2] = {};
    
    bindings[0].binding = 0;
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    bindings[0].descriptorCount = 1;
    bindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    
    bindings[1].binding = 1;
    bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[1].descriptorCount = 1;
    bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    
    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 2;
    layoutInfo.pBindings = bindings;
    
    vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &_descriptorSetLayout);
    
    VkDescriptorPoolSize poolSizes[2] = {};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = 1;
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = 1;
    
    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.maxSets = 1;
    poolInfo.poolSizeCount = 2;
    poolInfo.pPoolSizes = poolSizes;
    
    vkCreateDescriptorPool(device, &poolInfo, nullptr, &_descriptorPool);
    
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = _descriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &_descriptorSetLayout;
    
    vkAllocateDescriptorSets(device, &allocInfo, &_descriptorSet);
}

void SceneLighting::CreateShadowResources() {
    VkDevice device = _context->GetDevice();
    
    VkAttachmentDescription depthAttachment{};
    depthAttachment.format = VK_FORMAT_D32_SFLOAT;
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
    
    VkAttachmentReference depthRef{};
    depthRef.attachment = 0;
    depthRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 0;
    subpass.pDepthStencilAttachment = &depthRef;
    
    VkSubpassDependency dependencies[2] = {};
    dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[0].dstSubpass = 0;
    dependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dependencies[0].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
    dependencies[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
    
    dependencies[1].srcSubpass = 0;
    dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[1].srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dependencies[1].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
    
    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &depthAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 2;
    renderPassInfo.pDependencies = dependencies;
    
    vkCreateRenderPass(device, &renderPassInfo, nullptr, &_shadowRenderPass);
    
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.format = VK_FORMAT_D32_SFLOAT;
    imageInfo.extent = {SHADOW_MAP_SIZE, SHADOW_MAP_SIZE, 1};
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    
    vkCreateImage(device, &imageInfo, nullptr, &_shadowMap);
    
    VkMemoryRequirements memReqs;
    vkGetImageMemoryRequirements(device, _shadowMap, &memReqs);
    
    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memReqs.size;
    allocInfo.memoryTypeIndex = _context->FindMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    
    vkAllocateMemory(device, &allocInfo, nullptr, &_shadowMapMemory);
    vkBindImageMemory(device, _shadowMap, _shadowMapMemory, 0);
    
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = _shadowMap;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = VK_FORMAT_D32_SFLOAT;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;
    
    vkCreateImageView(device, &viewInfo, nullptr, &_shadowMapView);
    
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    samplerInfo.compareEnable = VK_FALSE;
    
    vkCreateSampler(device, &samplerInfo, nullptr, &_shadowSampler);
    
    VkFramebufferCreateInfo fbInfo{};
    fbInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    fbInfo.renderPass = _shadowRenderPass;
    fbInfo.attachmentCount = 1;
    fbInfo.pAttachments = &_shadowMapView;
    fbInfo.width = SHADOW_MAP_SIZE;
    fbInfo.height = SHADOW_MAP_SIZE;
    fbInfo.layers = 1;
    
    vkCreateFramebuffer(device, &fbInfo, nullptr, &_shadowFramebuffer);
    
    VkCommandBuffer cmd = _context->BeginSingleTimeCommands();
    
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = _shadowMap;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    
    vkCmdPipelineBarrier(cmd,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        0, 0, nullptr, 0, nullptr, 1, &barrier);
    
    _context->EndSingleTimeCommands(cmd);
    
    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = _uboBuffer->GetBuffer();
    bufferInfo.offset = 0;
    bufferInfo.range = sizeof(SceneLightingUBO);
    
    VkDescriptorImageInfo imageDescInfo{};
    imageDescInfo.sampler = _shadowSampler;
    imageDescInfo.imageView = _shadowMapView;
    imageDescInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
    
    VkWriteDescriptorSet writes[2] = {};
    writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[0].dstSet = _descriptorSet;
    writes[0].dstBinding = 0;
    writes[0].descriptorCount = 1;
    writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    writes[0].pBufferInfo = &bufferInfo;
    
    writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[1].dstSet = _descriptorSet;
    writes[1].dstBinding = 1;
    writes[1].descriptorCount = 1;
    writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writes[1].pImageInfo = &imageDescInfo;
    
    vkUpdateDescriptorSets(device, 2, writes, 0, nullptr);
}

void SceneLighting::CleanupShadowResources() {
    if (!_context) return;
    
    VkDevice device = _context->GetDevice();
    
    if (_shadowFramebuffer) {
        vkDestroyFramebuffer(device, _shadowFramebuffer, nullptr);
        _shadowFramebuffer = VK_NULL_HANDLE;
    }
    if (_shadowRenderPass) {
        vkDestroyRenderPass(device, _shadowRenderPass, nullptr);
        _shadowRenderPass = VK_NULL_HANDLE;
    }
    if (_shadowSampler) {
        vkDestroySampler(device, _shadowSampler, nullptr);
        _shadowSampler = VK_NULL_HANDLE;
    }
    if (_shadowMapView) {
        vkDestroyImageView(device, _shadowMapView, nullptr);
        _shadowMapView = VK_NULL_HANDLE;
    }
    if (_shadowMap) {
        vkDestroyImage(device, _shadowMap, nullptr);
        _shadowMap = VK_NULL_HANDLE;
    }
    if (_shadowMapMemory) {
        vkFreeMemory(device, _shadowMapMemory, nullptr);
        _shadowMapMemory = VK_NULL_HANDLE;
    }
}

void SceneLighting::CreateShadowPipeline() {
    VkDevice device = _context->GetDevice();
    
    std::vector<u32> vertSpirv = ShaderCompiler::CompileGLSL(shaders::shadow_vert, ShaderStage::Vertex, "shadow_vert");
    std::vector<u32> fragSpirv = ShaderCompiler::CompileGLSL(shaders::shadow_frag, ShaderStage::Fragment, "shadow_frag");
    
    if (vertSpirv.empty() || fragSpirv.empty()) {
        TVK_LOG_ERROR("Failed to compile shadow shaders");
        return;
    }
    
    VkShaderModule vertModule = VK_NULL_HANDLE;
    VkShaderModule fragModule = VK_NULL_HANDLE;
    
    VkShaderModuleCreateInfo vertModuleInfo{};
    vertModuleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    vertModuleInfo.codeSize = vertSpirv.size() * sizeof(u32);
    vertModuleInfo.pCode = vertSpirv.data();
    vkCreateShaderModule(device, &vertModuleInfo, nullptr, &vertModule);
    
    VkShaderModuleCreateInfo fragModuleInfo{};
    fragModuleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    fragModuleInfo.codeSize = fragSpirv.size() * sizeof(u32);
    fragModuleInfo.pCode = fragSpirv.data();
    vkCreateShaderModule(device, &fragModuleInfo, nullptr, &fragModule);
    
    VkPipelineShaderStageCreateInfo shaderStages[2] = {};
    shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    shaderStages[0].module = vertModule;
    shaderStages[0].pName = "main";
    
    shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shaderStages[1].module = fragModule;
    shaderStages[1].pName = "main";
    
    auto bindingDesc = Vertex::GetBindingDescription();
    auto attributeDescs = Vertex::GetAttributeDescriptions();
    
    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDesc;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<u32>(attributeDescs.size());
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescs.data();
    
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    
    VkViewport viewport{};
    viewport.width = (float)SHADOW_MAP_SIZE;
    viewport.height = (float)SHADOW_MAP_SIZE;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    
    VkRect2D scissor{};
    scissor.extent = {SHADOW_MAP_SIZE, SHADOW_MAP_SIZE};
    
    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;
    
    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.lineWidth = 1.0f;
    rasterizer.depthBiasEnable = VK_TRUE;
    rasterizer.depthBiasConstantFactor = 1.25f;
    rasterizer.depthBiasSlopeFactor = 1.75f;
    
    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    
    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    
    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.attachmentCount = 0;
    
    VkDynamicState dynamicStates[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = 2;
    dynamicState.pDynamicStates = dynamicStates;
    
    VkPushConstantRange pushConstant{};
    pushConstant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pushConstant.offset = 0;
    pushConstant.size = sizeof(ShadowPushConstants);
    
    VkPipelineLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.pushConstantRangeCount = 1;
    layoutInfo.pPushConstantRanges = &pushConstant;
    
    if (vkCreatePipelineLayout(device, &layoutInfo, nullptr, &_shadowPipelineLayout) != VK_SUCCESS) {
        TVK_LOG_ERROR("Failed to create shadow pipeline layout");
        vkDestroyShaderModule(device, vertModule, nullptr);
        vkDestroyShaderModule(device, fragModule, nullptr);
        return;
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
    pipelineInfo.layout = _shadowPipelineLayout;
    pipelineInfo.renderPass = _shadowRenderPass;
    pipelineInfo.subpass = 0;
    
    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &_shadowPipeline) != VK_SUCCESS) {
        TVK_LOG_ERROR("Failed to create shadow pipeline");
    }
    
    vkDestroyShaderModule(device, vertModule, nullptr);
    vkDestroyShaderModule(device, fragModule, nullptr);
}

void SceneLighting::BindShadowPipeline(VkCommandBuffer cmd) {
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _shadowPipeline);
}

void SceneLighting::SetShadowPushConstants(VkCommandBuffer cmd, const glm::mat4& model) {
    ShadowPushConstants push;
    push.model = model;
    push.light_space = _uboData.shadow_matrix;
    vkCmdPushConstants(cmd, _shadowPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(ShadowPushConstants), &push);
}

void SceneLighting::CollectLights(Scene* scene, const glm::vec3& camera_position) {
    memset(&_uboData, 0, sizeof(SceneLightingUBO));
    _uboData.camera_position = glm::vec4(camera_position, 1.0f);
    _uboData.ambient_color = glm::vec4(0.1f, 0.1f, 0.15f, 0.3f);
    _uboData.shadow_enabled = _shadowsEnabled ? 1 : 0;
    _hasDirectionalLight = false;
    
    u32 lightIndex = 0;
    
    auto ambientView = scene->GetAllEntitiesWith<AmbientLightComponent>();
    for (auto entity : ambientView) {
        const auto& ambient = ambientView.get<AmbientLightComponent>(entity);
        _uboData.ambient_color = glm::vec4(ambient.color, ambient.intensity);
        break;
    }
    
    auto dirView = scene->GetAllEntitiesWith<TransformComponent, DirectionalLightComponent>();
    for (auto entity : dirView) {
        if (lightIndex >= MAX_SCENE_LIGHTS) break;
        
        const auto& transform = dirView.get<TransformComponent>(entity);
        const auto& light = dirView.get<DirectionalLightComponent>(entity);
        
        glm::vec3 forward = glm::vec3(0.0f, 0.0f, -1.0f);
        glm::mat4 rotMat = glm::mat4(1.0f);
        rotMat = glm::rotate(rotMat, glm::radians(transform.rotation.y), glm::vec3(0, 1, 0));
        rotMat = glm::rotate(rotMat, glm::radians(transform.rotation.x), glm::vec3(1, 0, 0));
        glm::vec3 direction = glm::normalize(glm::vec3(rotMat * glm::vec4(forward, 0.0f)));
        
        _uboData.lights[lightIndex].position_type = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
        _uboData.lights[lightIndex].direction_range = glm::vec4(direction, 0.0f);
        _uboData.lights[lightIndex].color_intensity = glm::vec4(light.color, light.intensity);
        _uboData.lights[lightIndex].shadow_params = glm::vec4(light.cast_shadows ? 1.0f : 0.0f, 0.0f, 0.0f, 0.0f);
        
        if (!_hasDirectionalLight && light.cast_shadows) {
            _hasDirectionalLight = true;
            _directionalLightDir = direction;
            
            glm::vec3 lightPos = -direction * 50.0f;
            glm::mat4 lightView = glm::lookAt(lightPos, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            glm::mat4 lightProj = glm::ortho(-20.0f, 20.0f, -20.0f, 20.0f, 0.1f, 100.0f);
            _uboData.shadow_matrix = lightProj * lightView;
        }
        
        lightIndex++;
    }
    
    auto pointView = scene->GetAllEntitiesWith<TransformComponent, PointLightComponent>();
    for (auto entity : pointView) {
        if (lightIndex >= MAX_SCENE_LIGHTS) break;
        
        const auto& transform = pointView.get<TransformComponent>(entity);
        const auto& light = pointView.get<PointLightComponent>(entity);
        
        _uboData.lights[lightIndex].position_type = glm::vec4(transform.position, 1.0f);
        _uboData.lights[lightIndex].direction_range = glm::vec4(0.0f, 0.0f, 0.0f, light.radius);
        _uboData.lights[lightIndex].color_intensity = glm::vec4(light.color, light.intensity);
        _uboData.lights[lightIndex].shadow_params = glm::vec4(0.0f, light.falloff, 0.0f, 0.0f);
        
        lightIndex++;
    }
    
    auto spotView = scene->GetAllEntitiesWith<TransformComponent, SpotLightComponent>();
    for (auto entity : spotView) {
        if (lightIndex >= MAX_SCENE_LIGHTS) break;
        
        const auto& transform = spotView.get<TransformComponent>(entity);
        const auto& light = spotView.get<SpotLightComponent>(entity);
        
        glm::vec3 forward = glm::vec3(0.0f, 0.0f, -1.0f);
        glm::mat4 rotMat = glm::mat4(1.0f);
        rotMat = glm::rotate(rotMat, glm::radians(transform.rotation.y), glm::vec3(0, 1, 0));
        rotMat = glm::rotate(rotMat, glm::radians(transform.rotation.x), glm::vec3(1, 0, 0));
        glm::vec3 direction = glm::normalize(glm::vec3(rotMat * glm::vec4(forward, 0.0f)));
        
        _uboData.lights[lightIndex].position_type = glm::vec4(transform.position, 2.0f);
        _uboData.lights[lightIndex].direction_range = glm::vec4(direction, light.range);
        _uboData.lights[lightIndex].color_intensity = glm::vec4(light.color, light.intensity);
        float innerCos = glm::cos(glm::radians(light.inner_angle));
        float outerCos = glm::cos(glm::radians(light.outer_angle));
        _uboData.lights[lightIndex].shadow_params = glm::vec4(light.cast_shadows ? 1.0f : 0.0f, innerCos, outerCos, 0.0f);
        
        lightIndex++;
    }
    
    _uboData.num_lights = lightIndex;
}

void SceneLighting::UpdateUBO() {
    if (_uboBuffer) {
        void* data = _uboBuffer->Map();
        memcpy(data, &_uboData, sizeof(SceneLightingUBO));
        _uboBuffer->Unmap();
    }
}

void SceneLighting::BeginShadowPass(VkCommandBuffer cmd) {
    VkClearValue clearValue{};
    clearValue.depthStencil = {1.0f, 0};
    
    VkRenderPassBeginInfo rpInfo{};
    rpInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rpInfo.renderPass = _shadowRenderPass;
    rpInfo.framebuffer = _shadowFramebuffer;
    rpInfo.renderArea.extent = {SHADOW_MAP_SIZE, SHADOW_MAP_SIZE};
    rpInfo.clearValueCount = 1;
    rpInfo.pClearValues = &clearValue;
    
    vkCmdBeginRenderPass(cmd, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);
    
    VkViewport viewport{};
    viewport.width = (float)SHADOW_MAP_SIZE;
    viewport.height = (float)SHADOW_MAP_SIZE;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(cmd, 0, 1, &viewport);
    
    VkRect2D scissor{};
    scissor.extent = {SHADOW_MAP_SIZE, SHADOW_MAP_SIZE};
    vkCmdSetScissor(cmd, 0, 1, &scissor);
}

void SceneLighting::EndShadowPass(VkCommandBuffer cmd) {
    vkCmdEndRenderPass(cmd);
}

} // namespace tvk
