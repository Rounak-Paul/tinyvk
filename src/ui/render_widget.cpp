/**
 * @file render_widget.cpp
 * @brief Custom rendering widget implementation
 */

#include "tinyvk/ui/render_widget.h"
#include "tinyvk/renderer/renderer.h"
#include "tinyvk/core/log.h"

#include <imgui.h>
#include <imgui_impl_vulkan.h>
#include <algorithm>
#include <array>

namespace tvk {

RenderWidget::RenderWidget() {
}

RenderWidget::~RenderWidget() {
    Cleanup();
}

void RenderWidget::Initialize(Renderer* renderer) {
    if (_initialized) return;
    
    _renderer = renderer;
    
    auto& ctx = _renderer->GetContext();
    VkDevice device = ctx.GetDevice();
    
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = ctx.GetQueueFamilyIndices().graphicsFamily.value();
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    
    if (vkCreateCommandPool(device, &poolInfo, nullptr, &_commandPool) != VK_SUCCESS) {
        TVK_LOG_ERROR("Failed to create RenderWidget command pool");
        return;
    }
    
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = _commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;
    
    if (vkAllocateCommandBuffers(device, &allocInfo, &_commandBuffer) != VK_SUCCESS) {
        TVK_LOG_ERROR("Failed to create RenderWidget command buffer");
        return;
    }
    
    CreateRenderTarget();
    
    OnRenderInit();
    
    _initialized = true;
}

void RenderWidget::Render(float deltaTime) {
    if (!_initialized || !_enabled) return;
    
    if (_needsResize) {
        RecreateRenderTarget();
        _needsResize = false;
    }
    
    OnRenderUpdate(deltaTime);
    
    auto& ctx = _renderer->GetContext();
    VkDevice device = ctx.GetDevice();
    
    vkResetCommandBuffer(_commandBuffer, 0);
    
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    
    if (vkBeginCommandBuffer(_commandBuffer, &beginInfo) == VK_SUCCESS) {
        OnRenderFrame(_commandBuffer);
        vkEndCommandBuffer(_commandBuffer);
        
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &_commandBuffer;
        
        vkQueueSubmit(ctx.GetGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(ctx.GetGraphicsQueue());
    }
}

void RenderWidget::RenderImage() {
    if (_imguiTexture == VK_NULL_HANDLE) return;
    
    ImVec2 contentRegion = ImGui::GetContentRegionAvail();
    
    if (contentRegion.x > 0 && contentRegion.y > 0) {
        u32 newWidth = static_cast<u32>(contentRegion.x);
        u32 newHeight = static_cast<u32>(contentRegion.y);
        
        newWidth = std::max(newWidth, 32u);
        newHeight = std::max(newHeight, 32u);
        
        if (newWidth != _width || newHeight != _height) {
            SetSize(newWidth, newHeight);
            return;
        }
        
        ImGui::Image((ImTextureID)_imguiTexture, contentRegion);
    }
}

void RenderWidget::Cleanup() {
    if (!_initialized) return;
    
    OnRenderCleanup();
    
    CleanupRenderTarget();
    
    auto& ctx = _renderer->GetContext();
    VkDevice device = ctx.GetDevice();
    
    if (_commandPool != VK_NULL_HANDLE) {
        vkDestroyCommandPool(device, _commandPool, nullptr);
        _commandPool = VK_NULL_HANDLE;
        _commandBuffer = VK_NULL_HANDLE;
    }
    
    _initialized = false;
}

void RenderWidget::SetSize(u32 width, u32 height) {
    if (_width != width || _height != height) {
        _width = width;
        _height = height;
        _needsResize = true;
        OnRenderResize(width, height);
    }
}

VulkanContext* RenderWidget::GetContext() {
    return _renderer ? &_renderer->GetContext() : nullptr;
}

void RenderWidget::BeginRenderPass(VkCommandBuffer cmd) {
    if (_renderPass == VK_NULL_HANDLE || _framebuffer == VK_NULL_HANDLE) return;
    
    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = _renderPass;
    renderPassInfo.framebuffer = _framebuffer;
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = {_width, _height};
    
    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = {{_clearColor[0], _clearColor[1], _clearColor[2], _clearColor[3]}};
    clearValues[1].depthStencil = {1.0f, 0};
    
    renderPassInfo.clearValueCount = static_cast<u32>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();
    
    vkCmdBeginRenderPass(cmd, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(_width);
    viewport.height = static_cast<float>(_height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(cmd, 0, 1, &viewport);
    
    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = {_width, _height};
    vkCmdSetScissor(cmd, 0, 1, &scissor);
}

void RenderWidget::EndRenderPass(VkCommandBuffer cmd) {
    vkCmdEndRenderPass(cmd);
}

void RenderWidget::CreateRenderPass() {
    if (!_renderer) return;
    
    auto& ctx = _renderer->GetContext();
    VkDevice device = ctx.GetDevice();
    
    VkFormat depthFormat = VK_FORMAT_D32_SFLOAT;
    
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = VK_FORMAT_R8G8B8A8_UNORM;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    
    VkAttachmentDescription depthAttachment{};
    depthAttachment.format = depthFormat;
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    
    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    
    VkAttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;
    
    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    
    std::array<VkAttachmentDescription, 2> attachments = {colorAttachment, depthAttachment};
    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<u32>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;
    
    vkCreateRenderPass(device, &renderPassInfo, nullptr, &_renderPass);
}

void RenderWidget::CreateSampler() {
    if (!_renderer) return;
    
    auto& ctx = _renderer->GetContext();
    VkDevice device = ctx.GetDevice();
    
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.maxAnisotropy = 1.0f;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    
    if (vkCreateSampler(device, &samplerInfo, nullptr, &_sampler) != VK_SUCCESS) {
        TVK_LOG_ERROR("Failed to create render widget sampler");
    }
}

void RenderWidget::CreateSizeDependentResources() {
    if (!_renderer) return;
    
    auto& ctx = _renderer->GetContext();
    VkDevice device = ctx.GetDevice();
    
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = _width;
    imageInfo.extent.height = _height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    if (vkCreateImage(device, &imageInfo, nullptr, &_renderImage) != VK_SUCCESS) {
        TVK_LOG_ERROR("Failed to create render widget image");
        return;
    }
    
    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device, _renderImage, &memRequirements);
    
    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = ctx.FindMemoryType(
        memRequirements.memoryTypeBits,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
    );
    
    if (vkAllocateMemory(device, &allocInfo, nullptr, &_renderImageMemory) != VK_SUCCESS) {
        TVK_LOG_ERROR("Failed to allocate render widget image memory");
        return;
    }
    
    vkBindImageMemory(device, _renderImage, _renderImageMemory, 0);
    
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = _renderImage;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;
    
    if (vkCreateImageView(device, &viewInfo, nullptr, &_renderImageView) != VK_SUCCESS) {
        TVK_LOG_ERROR("Failed to create render widget image view");
        return;
    }
    
    VkFormat depthFormat = VK_FORMAT_D32_SFLOAT;
    
    VkImageCreateInfo depthImageInfo{};
    depthImageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    depthImageInfo.imageType = VK_IMAGE_TYPE_2D;
    depthImageInfo.extent.width = _width;
    depthImageInfo.extent.height = _height;
    depthImageInfo.extent.depth = 1;
    depthImageInfo.mipLevels = 1;
    depthImageInfo.arrayLayers = 1;
    depthImageInfo.format = depthFormat;
    depthImageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    depthImageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthImageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    depthImageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    depthImageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    vkCreateImage(device, &depthImageInfo, nullptr, &_depthImage);
    
    VkMemoryRequirements depthMemRequirements;
    vkGetImageMemoryRequirements(device, _depthImage, &depthMemRequirements);
    
    VkMemoryAllocateInfo depthAllocInfo{};
    depthAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    depthAllocInfo.allocationSize = depthMemRequirements.size;
    depthAllocInfo.memoryTypeIndex = ctx.FindMemoryType(
        depthMemRequirements.memoryTypeBits,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
    );
    
    vkAllocateMemory(device, &depthAllocInfo, nullptr, &_depthImageMemory);
    vkBindImageMemory(device, _depthImage, _depthImageMemory, 0);
    
    VkImageViewCreateInfo depthViewInfo{};
    depthViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    depthViewInfo.image = _depthImage;
    depthViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    depthViewInfo.format = depthFormat;
    depthViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    depthViewInfo.subresourceRange.baseMipLevel = 0;
    depthViewInfo.subresourceRange.levelCount = 1;
    depthViewInfo.subresourceRange.baseArrayLayer = 0;
    depthViewInfo.subresourceRange.layerCount = 1;
    
    vkCreateImageView(device, &depthViewInfo, nullptr, &_depthImageView);
    
    std::array<VkImageView, 2> fbAttachments = {_renderImageView, _depthImageView};
    
    VkFramebufferCreateInfo framebufferInfo{};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = _renderPass;
    framebufferInfo.attachmentCount = static_cast<u32>(fbAttachments.size());
    framebufferInfo.pAttachments = fbAttachments.data();
    framebufferInfo.width = _width;
    framebufferInfo.height = _height;
    framebufferInfo.layers = 1;
    
    vkCreateFramebuffer(device, &framebufferInfo, nullptr, &_framebuffer);
    
    _imguiTexture = ImGui_ImplVulkan_AddTexture(_sampler, _renderImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}

void RenderWidget::CleanupSizeDependentResources() {
    if (!_renderer) return;
    
    VkDevice device = _renderer->GetContext().GetDevice();
    
    if (_imguiTexture != VK_NULL_HANDLE) {
        ImGui_ImplVulkan_RemoveTexture(_imguiTexture);
        _imguiTexture = VK_NULL_HANDLE;
    }
    
    if (_framebuffer != VK_NULL_HANDLE) {
        vkDestroyFramebuffer(device, _framebuffer, nullptr);
        _framebuffer = VK_NULL_HANDLE;
    }
    
    if (_depthImageView != VK_NULL_HANDLE) {
        vkDestroyImageView(device, _depthImageView, nullptr);
        _depthImageView = VK_NULL_HANDLE;
    }
    
    if (_depthImage != VK_NULL_HANDLE) {
        vkDestroyImage(device, _depthImage, nullptr);
        _depthImage = VK_NULL_HANDLE;
    }
    
    if (_depthImageMemory != VK_NULL_HANDLE) {
        vkFreeMemory(device, _depthImageMemory, nullptr);
        _depthImageMemory = VK_NULL_HANDLE;
    }
    
    if (_renderImageView != VK_NULL_HANDLE) {
        vkDestroyImageView(device, _renderImageView, nullptr);
        _renderImageView = VK_NULL_HANDLE;
    }
    
    if (_renderImage != VK_NULL_HANDLE) {
        vkDestroyImage(device, _renderImage, nullptr);
        _renderImage = VK_NULL_HANDLE;
    }
    
    if (_renderImageMemory != VK_NULL_HANDLE) {
        vkFreeMemory(device, _renderImageMemory, nullptr);
        _renderImageMemory = VK_NULL_HANDLE;
    }
}

void RenderWidget::CreateRenderTarget() {
    CreateRenderPass();
    CreateSampler();
    CreateSizeDependentResources();
}

void RenderWidget::CleanupRenderTarget() {
    if (!_renderer) return;
    
    VkDevice device = _renderer->GetContext().GetDevice();
    
    CleanupSizeDependentResources();
    
    if (_renderPass != VK_NULL_HANDLE) {
        vkDestroyRenderPass(device, _renderPass, nullptr);
        _renderPass = VK_NULL_HANDLE;
    }
    
    if (_sampler != VK_NULL_HANDLE) {
        vkDestroySampler(device, _sampler, nullptr);
        _sampler = VK_NULL_HANDLE;
    }
}

void RenderWidget::RecreateRenderTarget() {
    if (!_renderer) return;
    
    _renderer->GetContext().WaitIdle();
    
    CleanupSizeDependentResources();
    CreateSizeDependentResources();
}

} // namespace tvk
