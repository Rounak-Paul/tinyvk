/**
 * @file texture.cpp
 * @brief Texture implementation
 */

#include "tinyvk/renderer/texture.h"
#include "tinyvk/renderer/renderer.h"
#include "tinyvk/renderer/context.h"
#include "tinyvk/core/log.h"

#include <imgui_impl_vulkan.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <cmath>
#include <algorithm>

namespace tvk {

Texture::~Texture() {
    Cleanup();
}

Texture::Texture(Texture&& other) noexcept
    : m_Renderer(other.m_Renderer)
    , m_Context(other.m_Context)
    , m_Image(other.m_Image)
    , m_Allocation(other.m_Allocation)
    , m_ImageView(other.m_ImageView)
    , m_Sampler(other.m_Sampler)
    , m_Format(other.m_Format)
    , m_ImGuiDescriptorSet(other.m_ImGuiDescriptorSet)
    , m_Width(other.m_Width)
    , m_Height(other.m_Height)
    , m_MipLevels(other.m_MipLevels)
    , m_FilePath(std::move(other.m_FilePath)) {
    other.m_Image = VK_NULL_HANDLE;
    other.m_Allocation = VK_NULL_HANDLE;
    other.m_ImageView = VK_NULL_HANDLE;
    other.m_Sampler = VK_NULL_HANDLE;
    other.m_ImGuiDescriptorSet = VK_NULL_HANDLE;
}

Texture& Texture::operator=(Texture&& other) noexcept {
    if (this != &other) {
        Cleanup();
        m_Renderer = other.m_Renderer;
        m_Context = other.m_Context;
        m_Image = other.m_Image;
        m_Allocation = other.m_Allocation;
        m_ImageView = other.m_ImageView;
        m_Sampler = other.m_Sampler;
        m_Format = other.m_Format;
        m_ImGuiDescriptorSet = other.m_ImGuiDescriptorSet;
        m_Width = other.m_Width;
        m_Height = other.m_Height;
        m_MipLevels = other.m_MipLevels;
        m_FilePath = std::move(other.m_FilePath);

        other.m_Image = VK_NULL_HANDLE;
        other.m_Allocation = VK_NULL_HANDLE;
        other.m_ImageView = VK_NULL_HANDLE;
        other.m_Sampler = VK_NULL_HANDLE;
        other.m_ImGuiDescriptorSet = VK_NULL_HANDLE;
    }
    return *this;
}

Ref<Texture> Texture::LoadFromFile(Renderer* renderer, const std::string& filepath, const TextureSpec& spec) {
    int width, height, channels;
    stbi_uc* pixels = stbi_load(filepath.c_str(), &width, &height, &channels, STBI_rgb_alpha);
    
    if (!pixels) {
        TVK_LOG_ERROR("Failed to load texture: {}", filepath);
        return nullptr;
    }

    TextureSpec finalSpec = spec;
    finalSpec.width = static_cast<u32>(width);
    finalSpec.height = static_cast<u32>(height);

    auto texture = CreateRef<Texture>();
    texture->m_FilePath = filepath;
    
    if (!texture->Init(renderer, pixels, finalSpec)) {
        stbi_image_free(pixels);
        return nullptr;
    }

    stbi_image_free(pixels);
    
    TVK_LOG_INFO("Loaded texture: {} ({}x{})", filepath, width, height);
    return texture;
}

Ref<Texture> Texture::Create(Renderer* renderer, const void* data, u32 width, u32 height, const TextureSpec& spec) {
    TextureSpec finalSpec = spec;
    finalSpec.width = width;
    finalSpec.height = height;

    auto texture = CreateRef<Texture>();
    if (!texture->Init(renderer, data, finalSpec)) {
        return nullptr;
    }
    return texture;
}

Ref<Texture> Texture::Create(Renderer* renderer, const TextureSpec& spec) {
    // Create with null data (empty texture)
    std::vector<u8> emptyData(spec.width * spec.height * 4, 255);
    return Create(renderer, emptyData.data(), spec.width, spec.height, spec);
}

void Texture::SetData(const void* data, u32 width, u32 height) {
    if (!m_Context || !data) return;
    
    VkDeviceSize imageSize = width * height * 4;

    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = imageSize;
    bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

    VkBuffer stagingBuffer;
    VmaAllocation stagingAllocation;
    VmaAllocationInfo stagingAllocInfo;

    vmaCreateBuffer(m_Context->GetAllocator(), &bufferInfo, &allocInfo, 
                   &stagingBuffer, &stagingAllocation, &stagingAllocInfo);

    memcpy(stagingAllocInfo.pMappedData, data, static_cast<size_t>(imageSize));

    TransitionImageLayout(m_Image, m_Format, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, m_MipLevels);
    CopyBufferToImage(stagingBuffer, m_Image, width, height);
    TransitionImageLayout(m_Image, m_Format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                         VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, m_MipLevels);

    vmaDestroyBuffer(m_Context->GetAllocator(), stagingBuffer, stagingAllocation);
}

void Texture::BindToImGui() {
    if (m_ImGuiDescriptorSet != VK_NULL_HANDLE) return;
    if (!m_ImageView || !m_Sampler) return;

    m_ImGuiDescriptorSet = ImGui_ImplVulkan_AddTexture(
        m_Sampler, m_ImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    );
}

bool Texture::Init(Renderer* renderer, const void* data, const TextureSpec& spec) {
    m_Renderer = renderer;
    m_Context = &renderer->GetContext();
    m_Width = spec.width;
    m_Height = spec.height;
    m_Format = ToVkFormat(spec.format);
    m_StorageUsage = spec.storageUsage;

    if (spec.generateMipmaps) {
        m_MipLevels = static_cast<u32>(std::floor(std::log2(std::max(m_Width, m_Height)))) + 1;
    }

    VkDeviceSize imageSize = m_Width * m_Height * 4;

    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = imageSize;
    bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo stagingAllocCreateInfo{};
    stagingAllocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
    stagingAllocCreateInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

    VkBuffer stagingBuffer;
    VmaAllocation stagingAllocation;
    VmaAllocationInfo stagingAllocInfo;

    if (vmaCreateBuffer(m_Context->GetAllocator(), &bufferInfo, &stagingAllocCreateInfo, 
                        &stagingBuffer, &stagingAllocation, &stagingAllocInfo) != VK_SUCCESS) {
        TVK_LOG_ERROR("Failed to create staging buffer for texture");
        return false;
    }

    memcpy(stagingAllocInfo.pMappedData, data, static_cast<size_t>(imageSize));

    VkImageUsageFlags usageFlags = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    if (spec.storageUsage) {
        usageFlags |= VK_IMAGE_USAGE_STORAGE_BIT;
    }
    CreateImage(m_Width, m_Height, m_Format, VK_IMAGE_TILING_OPTIMAL, usageFlags);

    TransitionImageLayout(m_Image, m_Format, VK_IMAGE_LAYOUT_UNDEFINED, 
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, m_MipLevels);
    CopyBufferToImage(stagingBuffer, m_Image, m_Width, m_Height);

    if (spec.generateMipmaps && m_MipLevels > 1) {
        GenerateMipmaps(m_Image, m_Format, m_Width, m_Height, m_MipLevels);
    } else {
        TransitionImageLayout(m_Image, m_Format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                             VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, m_MipLevels);
    }

    vmaDestroyBuffer(m_Context->GetAllocator(), stagingBuffer, stagingAllocation);

    CreateImageView(m_Format, VK_IMAGE_ASPECT_COLOR_BIT);

    if (spec.useSampler) {
        CreateSampler(spec);
    }

    BindToImGui();

    return true;
}

void Texture::Cleanup() {
    if (!m_Context) return;
    
    m_Context->WaitIdle();

    if (m_ImGuiDescriptorSet != VK_NULL_HANDLE) {
        ImGui_ImplVulkan_RemoveTexture(m_ImGuiDescriptorSet);
        m_ImGuiDescriptorSet = VK_NULL_HANDLE;
    }

    if (m_Sampler != VK_NULL_HANDLE) {
        vkDestroySampler(m_Context->GetDevice(), m_Sampler, nullptr);
        m_Sampler = VK_NULL_HANDLE;
    }

    if (m_ImageView != VK_NULL_HANDLE) {
        vkDestroyImageView(m_Context->GetDevice(), m_ImageView, nullptr);
        m_ImageView = VK_NULL_HANDLE;
    }

    if (m_Image != VK_NULL_HANDLE && m_Allocation != VK_NULL_HANDLE) {
        vmaDestroyImage(m_Context->GetAllocator(), m_Image, m_Allocation);
        m_Image = VK_NULL_HANDLE;
        m_Allocation = VK_NULL_HANDLE;
    }
}

void Texture::CreateImage(u32 width, u32 height, VkFormat format, VkImageTiling tiling,
                          VkImageUsageFlags usage) {
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = m_MipLevels;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

    vmaCreateImage(m_Context->GetAllocator(), &imageInfo, &allocInfo, &m_Image, &m_Allocation, nullptr);
}

void Texture::CreateImageView(VkFormat format, VkImageAspectFlags aspectFlags) {
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = m_Image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = aspectFlags;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = m_MipLevels;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    vkCreateImageView(m_Context->GetDevice(), &viewInfo, nullptr, &m_ImageView);
}

void Texture::CreateSampler(const TextureSpec& spec) {
    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(m_Context->GetPhysicalDevice(), &properties);

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = ToVkFilter(spec.magFilter);
    samplerInfo.minFilter = ToVkFilter(spec.minFilter);
    samplerInfo.addressModeU = ToVkWrap(spec.wrapU);
    samplerInfo.addressModeV = ToVkWrap(spec.wrapV);
    samplerInfo.addressModeW = ToVkWrap(spec.wrapU);
    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = static_cast<float>(m_MipLevels);
    samplerInfo.mipLodBias = 0.0f;

    vkCreateSampler(m_Context->GetDevice(), &samplerInfo, nullptr, &m_Sampler);
}

void Texture::TransitionImageLayout(VkImage image, VkFormat format,
                                    VkImageLayout oldLayout, VkImageLayout newLayout, u32 mipLevels) {
    VkCommandBuffer commandBuffer = m_Context->BeginSingleTimeCommands();

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = mipLevels;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = 0;
        sourceStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
        destinationStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    }

    vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0,
                         0, nullptr, 0, nullptr, 1, &barrier);

    m_Context->EndSingleTimeCommands(commandBuffer);
}

void Texture::CopyBufferToImage(VkBuffer buffer, VkImage image, u32 width, u32 height) {
    VkCommandBuffer commandBuffer = m_Context->BeginSingleTimeCommands();

    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = {0, 0, 0};
    region.imageExtent = {width, height, 1};

    vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    m_Context->EndSingleTimeCommands(commandBuffer);
}

void Texture::GenerateMipmaps(VkImage image, VkFormat format, u32 width, u32 height, u32 mipLevels) {
    VkFormatProperties formatProperties;
    vkGetPhysicalDeviceFormatProperties(m_Context->GetPhysicalDevice(), format, &formatProperties);

    if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
        TVK_LOG_WARN("Texture format does not support linear blitting, mipmaps will not be generated");
        TransitionImageLayout(image, format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                             VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, mipLevels);
        return;
    }

    VkCommandBuffer commandBuffer = m_Context->BeginSingleTimeCommands();

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.image = image;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.subresourceRange.levelCount = 1;

    i32 mipWidth = static_cast<i32>(width);
    i32 mipHeight = static_cast<i32>(height);

    for (u32 i = 1; i < mipLevels; i++) {
        barrier.subresourceRange.baseMipLevel = i - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

        vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
                             0, nullptr, 0, nullptr, 1, &barrier);

        VkImageBlit blit{};
        blit.srcOffsets[0] = {0, 0, 0};
        blit.srcOffsets[1] = {mipWidth, mipHeight, 1};
        blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.srcSubresource.mipLevel = i - 1;
        blit.srcSubresource.baseArrayLayer = 0;
        blit.srcSubresource.layerCount = 1;
        blit.dstOffsets[0] = {0, 0, 0};
        blit.dstOffsets[1] = {mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1};
        blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.dstSubresource.mipLevel = i;
        blit.dstSubresource.baseArrayLayer = 0;
        blit.dstSubresource.layerCount = 1;

        vkCmdBlitImage(commandBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                       image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR);

        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
                             0, nullptr, 0, nullptr, 1, &barrier);

        if (mipWidth > 1) mipWidth /= 2;
        if (mipHeight > 1) mipHeight /= 2;
    }

    barrier.subresourceRange.baseMipLevel = mipLevels - 1;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
                         0, nullptr, 0, nullptr, 1, &barrier);

    m_Context->EndSingleTimeCommands(commandBuffer);
}

VkFormat Texture::ToVkFormat(TextureFormat format) {
    switch (format) {
        case TextureFormat::RGBA8: return VK_FORMAT_R8G8B8A8_UNORM;
        case TextureFormat::RGBA8_SRGB: return VK_FORMAT_R8G8B8A8_SRGB;
        case TextureFormat::BGRA8: return VK_FORMAT_B8G8R8A8_UNORM;
        case TextureFormat::BGRA8_SRGB: return VK_FORMAT_B8G8R8A8_SRGB;
        case TextureFormat::R8: return VK_FORMAT_R8_UNORM;
        case TextureFormat::RG8: return VK_FORMAT_R8G8_UNORM;
        case TextureFormat::RGB8: return VK_FORMAT_R8G8B8_UNORM;
        case TextureFormat::RGBA16F: return VK_FORMAT_R16G16B16A16_SFLOAT;
        case TextureFormat::RGBA32F: return VK_FORMAT_R32G32B32A32_SFLOAT;
        case TextureFormat::Depth24Stencil8: return VK_FORMAT_D24_UNORM_S8_UINT;
        case TextureFormat::Depth32F: return VK_FORMAT_D32_SFLOAT;
        default: return VK_FORMAT_R8G8B8A8_UNORM;
    }
}

VkFilter Texture::ToVkFilter(TextureFilter filter) {
    switch (filter) {
        case TextureFilter::Nearest: return VK_FILTER_NEAREST;
        case TextureFilter::Linear: return VK_FILTER_LINEAR;
        default: return VK_FILTER_LINEAR;
    }
}

VkSamplerAddressMode Texture::ToVkWrap(TextureWrap wrap) {
    switch (wrap) {
        case TextureWrap::Repeat: return VK_SAMPLER_ADDRESS_MODE_REPEAT;
        case TextureWrap::MirroredRepeat: return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
        case TextureWrap::ClampToEdge: return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        case TextureWrap::ClampToBorder: return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        default: return VK_SAMPLER_ADDRESS_MODE_REPEAT;
    }
}

} // namespace tvk
