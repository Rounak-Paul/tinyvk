/**
 * @file renderer.cpp
 * @brief Renderer implementation
 */

#include "tinyvk/renderer/renderer.h"
#include "tinyvk/core/window.h"
#include "tinyvk/core/log.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <algorithm>
#include <limits>
#include <array>

namespace tvk {

Renderer::~Renderer() {
    Cleanup();
}

bool Renderer::Init(Window* window, const RendererConfig& config) {
    m_Window = window;
    m_Config = config;
    m_ClearColor = config.clearColor;

    // Initialize Vulkan context
    ContextConfig contextConfig;
    contextConfig.enableValidation = config.enableValidation;
    
    if (!m_Context.Init(window->GetNativeHandle(), contextConfig)) {
        TVK_LOG_ERROR("Failed to initialize Vulkan context");
        return false;
    }

    // Create swapchain and related resources
    if (!CreateSwapchain()) {
        TVK_LOG_ERROR("Failed to create swapchain");
        return false;
    }

    if (!CreateImageViews()) {
        TVK_LOG_ERROR("Failed to create image views");
        return false;
    }

    if (!CreateDepthResources()) {
        TVK_LOG_ERROR("Failed to create depth resources");
        return false;
    }

    if (!CreateRenderPass()) {
        TVK_LOG_ERROR("Failed to create render pass");
        return false;
    }

    if (!CreateFramebuffers()) {
        TVK_LOG_ERROR("Failed to create framebuffers");
        return false;
    }

    if (!CreateCommandBuffers()) {
        TVK_LOG_ERROR("Failed to create command buffers");
        return false;
    }

    if (!CreateSyncObjects()) {
        TVK_LOG_ERROR("Failed to create sync objects");
        return false;
    }

    TVK_LOG_INFO("Renderer initialized successfully");
    return true;
}

void Renderer::Cleanup() {
    if (m_Context.GetDevice() != VK_NULL_HANDLE) {
        m_Context.WaitIdle();
    }

    CleanupSwapchain();

    // Cleanup semaphore pools
    for (auto semaphore : m_ImageAvailableSemaphores) {
        if (semaphore != VK_NULL_HANDLE) {
            vkDestroySemaphore(m_Context.GetDevice(), semaphore, nullptr);
        }
    }
    m_ImageAvailableSemaphores.clear();
    
    for (auto semaphore : m_RenderFinishedSemaphores) {
        if (semaphore != VK_NULL_HANDLE) {
            vkDestroySemaphore(m_Context.GetDevice(), semaphore, nullptr);
        }
    }
    m_RenderFinishedSemaphores.clear();

    // Cleanup per-frame fences
    for (auto& frame : m_Frames) {
        if (frame.inFlightFence != VK_NULL_HANDLE) {
            vkDestroyFence(m_Context.GetDevice(), frame.inFlightFence, nullptr);
        }
    }
    m_Frames.clear();

    m_Context.Cleanup();
}

bool Renderer::BeginFrame() {
    auto& frame = m_Frames[m_CurrentFrame];

    // Wait for the current frame's fence to be signaled
    vkWaitForFences(m_Context.GetDevice(), 1, &frame.inFlightFence, VK_TRUE, UINT64_MAX);

    // Use next semaphore from the pool for acquiring
    VkSemaphore acquireSemaphore = m_ImageAvailableSemaphores[m_CurrentSemaphoreIndex];

    // Acquire next swapchain image
    VkResult result = vkAcquireNextImageKHR(
        m_Context.GetDevice(),
        m_Swapchain,
        UINT64_MAX,
        acquireSemaphore,
        VK_NULL_HANDLE,
        &m_CurrentImageIndex
    );

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        RecreateSwapchain();
        return false;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        TVK_LOG_ERROR("Failed to acquire swapchain image");
        return false;
    }

    // Store which semaphores we're using for this frame
    frame.imageAvailableSemaphore = acquireSemaphore;
    frame.renderFinishedSemaphore = m_RenderFinishedSemaphores[m_CurrentSemaphoreIndex];
    
    // Move to next semaphore pair for next frame
    m_CurrentSemaphoreIndex = (m_CurrentSemaphoreIndex + 1) % static_cast<u32>(m_ImageAvailableSemaphores.size());

    vkResetFences(m_Context.GetDevice(), 1, &frame.inFlightFence);

    // Reset and begin command buffer
    vkResetCommandBuffer(frame.commandBuffer, 0);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0;
    beginInfo.pInheritanceInfo = nullptr;

    if (vkBeginCommandBuffer(frame.commandBuffer, &beginInfo) != VK_SUCCESS) {
        TVK_LOG_ERROR("Failed to begin recording command buffer");
        return false;
    }

    // Begin render pass
    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = m_RenderPass;
    renderPassInfo.framebuffer = m_Framebuffers[m_CurrentImageIndex];
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = m_SwapchainExtent;

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = {{m_ClearColor.r, m_ClearColor.g, m_ClearColor.b, m_ClearColor.a}};
    clearValues[1].depthStencil = {1.0f, 0};

    renderPassInfo.clearValueCount = static_cast<u32>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(frame.commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    // Set viewport and scissor
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(m_SwapchainExtent.width);
    viewport.height = static_cast<float>(m_SwapchainExtent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(frame.commandBuffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = m_SwapchainExtent;
    vkCmdSetScissor(frame.commandBuffer, 0, 1, &scissor);

    return true;
}

void Renderer::EndFrame() {
    auto& frame = m_Frames[m_CurrentFrame];

    // End render pass
    vkCmdEndRenderPass(frame.commandBuffer);

    if (vkEndCommandBuffer(frame.commandBuffer) != VK_SUCCESS) {
        TVK_LOG_ERROR("Failed to record command buffer");
        return;
    }

    // Submit command buffer
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = {frame.imageAvailableSemaphore};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &frame.commandBuffer;

    VkSemaphore signalSemaphores[] = {frame.renderFinishedSemaphore};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(m_Context.GetGraphicsQueue(), 1, &submitInfo, frame.inFlightFence) != VK_SUCCESS) {
        TVK_LOG_ERROR("Failed to submit draw command buffer");
        return;
    }

    // Present
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapchains[] = {m_Swapchain};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapchains;
    presentInfo.pImageIndices = &m_CurrentImageIndex;

    VkResult result = vkQueuePresentKHR(m_Context.GetPresentQueue(), &presentInfo);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_FramebufferResized) {
        m_FramebufferResized = false;
        RecreateSwapchain();
    } else if (result != VK_SUCCESS) {
        TVK_LOG_ERROR("Failed to present swapchain image");
    }

    m_CurrentFrame = (m_CurrentFrame + 1) % m_Config.maxFramesInFlight;
}

void Renderer::OnResize(u32 width, u32 height) {
    m_FramebufferResized = true;
}

VkCommandBuffer Renderer::GetCurrentCommandBuffer() const {
    return m_Frames[m_CurrentFrame].commandBuffer;
}

bool Renderer::CreateSwapchain() {
    SwapchainSupportDetails swapchainSupport = m_Context.QuerySwapchainSupport();

    VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(swapchainSupport.formats);
    VkPresentModeKHR presentMode = ChooseSwapPresentMode(swapchainSupport.presentModes);
    VkExtent2D extent = ChooseSwapExtent(swapchainSupport.capabilities);

    u32 imageCount = swapchainSupport.capabilities.minImageCount + 1;
    if (swapchainSupport.capabilities.maxImageCount > 0 && 
        imageCount > swapchainSupport.capabilities.maxImageCount) {
        imageCount = swapchainSupport.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = m_Context.GetSurface();
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    auto indices = m_Context.GetQueueFamilyIndices();
    u32 queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};

    if (indices.graphicsFamily != indices.presentFamily) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    createInfo.preTransform = swapchainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(m_Context.GetDevice(), &createInfo, nullptr, &m_Swapchain) != VK_SUCCESS) {
        return false;
    }

    vkGetSwapchainImagesKHR(m_Context.GetDevice(), m_Swapchain, &imageCount, nullptr);
    m_SwapchainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(m_Context.GetDevice(), m_Swapchain, &imageCount, m_SwapchainImages.data());

    m_SwapchainImageFormat = surfaceFormat.format;
    m_SwapchainExtent = extent;

    return true;
}

bool Renderer::CreateImageViews() {
    m_SwapchainImageViews.resize(m_SwapchainImages.size());

    for (size_t i = 0; i < m_SwapchainImages.size(); i++) {
        VkImageViewCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = m_SwapchainImages[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = m_SwapchainImageFormat;
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(m_Context.GetDevice(), &createInfo, nullptr, &m_SwapchainImageViews[i]) != VK_SUCCESS) {
            return false;
        }
    }

    return true;
}

bool Renderer::CreateRenderPass() {
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = m_SwapchainImageFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentDescription depthAttachment{};
    depthAttachment.format = m_DepthFormat;
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

    return vkCreateRenderPass(m_Context.GetDevice(), &renderPassInfo, nullptr, &m_RenderPass) == VK_SUCCESS;
}

bool Renderer::CreateFramebuffers() {
    m_Framebuffers.resize(m_SwapchainImageViews.size());

    for (size_t i = 0; i < m_SwapchainImageViews.size(); i++) {
        std::array<VkImageView, 2> attachments = {
            m_SwapchainImageViews[i],
            m_DepthImageView
        };

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = m_RenderPass;
        framebufferInfo.attachmentCount = static_cast<u32>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = m_SwapchainExtent.width;
        framebufferInfo.height = m_SwapchainExtent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(m_Context.GetDevice(), &framebufferInfo, nullptr, &m_Framebuffers[i]) != VK_SUCCESS) {
            return false;
        }
    }

    return true;
}

bool Renderer::CreateCommandBuffers() {
    m_Frames.resize(m_Config.maxFramesInFlight);

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = m_Context.GetCommandPool();
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    for (auto& frame : m_Frames) {
        if (vkAllocateCommandBuffers(m_Context.GetDevice(), &allocInfo, &frame.commandBuffer) != VK_SUCCESS) {
            return false;
        }
    }

    return true;
}

bool Renderer::CreateSyncObjects() {
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    // Create per-frame fences only (semaphores come from pools)
    for (auto& frame : m_Frames) {
        if (vkCreateFence(m_Context.GetDevice(), &fenceInfo, nullptr, &frame.inFlightFence) != VK_SUCCESS) {
            return false;
        }
        frame.imageAvailableSemaphore = VK_NULL_HANDLE;
        frame.renderFinishedSemaphore = VK_NULL_HANDLE;
    }

    // Create semaphore pools
    // We need at least swapchain images + 1 to avoid reusing a semaphore that's still in use
    u32 semaphoreCount = static_cast<u32>(m_SwapchainImages.size()) + 1;
    m_ImageAvailableSemaphores.resize(semaphoreCount);
    m_RenderFinishedSemaphores.resize(semaphoreCount);
    
    for (u32 i = 0; i < semaphoreCount; i++) {
        if (vkCreateSemaphore(m_Context.GetDevice(), &semaphoreInfo, nullptr, &m_ImageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(m_Context.GetDevice(), &semaphoreInfo, nullptr, &m_RenderFinishedSemaphores[i]) != VK_SUCCESS) {
            return false;
        }
    }
    m_CurrentSemaphoreIndex = 0;

    return true;
}

bool Renderer::CreateDepthResources() {
    m_DepthFormat = FindDepthFormat();

    // Create depth image
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = m_SwapchainExtent.width;
    imageInfo.extent.height = m_SwapchainExtent.height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = m_DepthFormat;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateImage(m_Context.GetDevice(), &imageInfo, nullptr, &m_DepthImage) != VK_SUCCESS) {
        return false;
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(m_Context.GetDevice(), m_DepthImage, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = m_Context.FindMemoryType(
        memRequirements.memoryTypeBits, 
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
    );

    if (vkAllocateMemory(m_Context.GetDevice(), &allocInfo, nullptr, &m_DepthImageMemory) != VK_SUCCESS) {
        return false;
    }

    vkBindImageMemory(m_Context.GetDevice(), m_DepthImage, m_DepthImageMemory, 0);

    // Create depth image view
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = m_DepthImage;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = m_DepthFormat;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    return vkCreateImageView(m_Context.GetDevice(), &viewInfo, nullptr, &m_DepthImageView) == VK_SUCCESS;
}

void Renderer::CleanupSwapchain() {
    if (m_Context.GetDevice() == VK_NULL_HANDLE) return;

    if (m_DepthImageView != VK_NULL_HANDLE) {
        vkDestroyImageView(m_Context.GetDevice(), m_DepthImageView, nullptr);
        m_DepthImageView = VK_NULL_HANDLE;
    }

    if (m_DepthImage != VK_NULL_HANDLE) {
        vkDestroyImage(m_Context.GetDevice(), m_DepthImage, nullptr);
        m_DepthImage = VK_NULL_HANDLE;
    }

    if (m_DepthImageMemory != VK_NULL_HANDLE) {
        vkFreeMemory(m_Context.GetDevice(), m_DepthImageMemory, nullptr);
        m_DepthImageMemory = VK_NULL_HANDLE;
    }

    for (auto framebuffer : m_Framebuffers) {
        vkDestroyFramebuffer(m_Context.GetDevice(), framebuffer, nullptr);
    }
    m_Framebuffers.clear();

    if (m_RenderPass != VK_NULL_HANDLE) {
        vkDestroyRenderPass(m_Context.GetDevice(), m_RenderPass, nullptr);
        m_RenderPass = VK_NULL_HANDLE;
    }

    for (auto imageView : m_SwapchainImageViews) {
        vkDestroyImageView(m_Context.GetDevice(), imageView, nullptr);
    }
    m_SwapchainImageViews.clear();

    if (m_Swapchain != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(m_Context.GetDevice(), m_Swapchain, nullptr);
        m_Swapchain = VK_NULL_HANDLE;
    }
}

void Renderer::RecreateSwapchain() {
    // Wait for minimized window
    auto extent = m_Window->GetFramebufferExtent();
    while (extent.width == 0 || extent.height == 0) {
        extent = m_Window->GetFramebufferExtent();
        m_Window->WaitEvents();
    }

    m_Context.WaitIdle();

    CleanupSwapchain();
    
    // Destroy old semaphore pools
    for (auto semaphore : m_ImageAvailableSemaphores) {
        if (semaphore != VK_NULL_HANDLE) {
            vkDestroySemaphore(m_Context.GetDevice(), semaphore, nullptr);
        }
    }
    m_ImageAvailableSemaphores.clear();
    
    for (auto semaphore : m_RenderFinishedSemaphores) {
        if (semaphore != VK_NULL_HANDLE) {
            vkDestroySemaphore(m_Context.GetDevice(), semaphore, nullptr);
        }
    }
    m_RenderFinishedSemaphores.clear();

    CreateSwapchain();
    CreateImageViews();
    CreateDepthResources();
    CreateRenderPass();
    CreateFramebuffers();
    
    // Recreate semaphore pools for new swapchain
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    
    u32 semaphoreCount = static_cast<u32>(m_SwapchainImages.size()) + 1;
    m_ImageAvailableSemaphores.resize(semaphoreCount);
    m_RenderFinishedSemaphores.resize(semaphoreCount);
    for (u32 i = 0; i < semaphoreCount; i++) {
        vkCreateSemaphore(m_Context.GetDevice(), &semaphoreInfo, nullptr, &m_ImageAvailableSemaphores[i]);
        vkCreateSemaphore(m_Context.GetDevice(), &semaphoreInfo, nullptr, &m_RenderFinishedSemaphores[i]);
    }
    m_CurrentSemaphoreIndex = 0;
}

VkSurfaceFormatKHR Renderer::ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
    for (const auto& format : availableFormats) {
        if (format.format == VK_FORMAT_B8G8R8A8_SRGB && 
            format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return format;
        }
    }
    return availableFormats[0];
}

VkPresentModeKHR Renderer::ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
    if (!m_Config.vsync) {
        for (const auto& mode : availablePresentModes) {
            if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
                return mode;
            }
        }
        for (const auto& mode : availablePresentModes) {
            if (mode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
                return mode;
            }
        }
    }
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D Renderer::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
    if (capabilities.currentExtent.width != std::numeric_limits<u32>::max()) {
        return capabilities.currentExtent;
    }

    auto extent = m_Window->GetFramebufferExtent();
    VkExtent2D actualExtent = {extent.width, extent.height};

    actualExtent.width = std::clamp(actualExtent.width, 
        capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    actualExtent.height = std::clamp(actualExtent.height, 
        capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

    return actualExtent;
}

VkFormat Renderer::FindDepthFormat() {
    return FindSupportedFormat(
        {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
    );
}

VkFormat Renderer::FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) {
    for (VkFormat format : candidates) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(m_Context.GetPhysicalDevice(), format, &props);

        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
            return format;
        } else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
            return format;
        }
    }

    TVK_LOG_ERROR("Failed to find supported format");
    return candidates[0];
}

} // namespace tvk
