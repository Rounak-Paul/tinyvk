#include <tinyvk/renderer/skybox.h>
#include <tinyvk/renderer/context.h>
#include <tinyvk/renderer/renderer.h>
#include <tinyvk/renderer/shaders.h>
#include <cstring>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace tvk {

static const float kSkyboxVertices[] = {
    -1.0f,  1.0f, -1.0f,
    -1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,

    -1.0f, -1.0f,  1.0f,
    -1.0f, -1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f,  1.0f,
    -1.0f, -1.0f,  1.0f,

     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,

    -1.0f, -1.0f,  1.0f,
    -1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f, -1.0f,  1.0f,
    -1.0f, -1.0f,  1.0f,

    -1.0f,  1.0f, -1.0f,
     1.0f,  1.0f, -1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
    -1.0f,  1.0f,  1.0f,
    -1.0f,  1.0f, -1.0f,

    -1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f,  1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f,  1.0f,
     1.0f, -1.0f,  1.0f
};

Skybox::~Skybox() {
    Destroy();
}

bool Skybox::Create(Renderer* renderer, VkRenderPass renderPass) {
    _renderer = renderer;
    _context = &renderer->GetContext();
    
    CreateCubeMesh();
    
    _pipeline = CreateScope<Pipeline>();
    if (!_pipeline->Create(renderer, renderPass, shaders::skybox_vert, shaders::skybox_frag)) {
        return false;
    }
    
    return true;
}

void Skybox::CreateCubeMesh() {
    std::vector<Vertex> vertices;
    std::vector<u32> indices;
    
    for (int i = 0; i < 36; ++i) {
        Vertex v;
        v.position = glm::vec3(
            kSkyboxVertices[i * 3],
            kSkyboxVertices[i * 3 + 1],
            kSkyboxVertices[i * 3 + 2]
        );
        v.normal = glm::vec3(0.0f);
        v.texCoord = glm::vec2(0.0f);
        v.color = glm::vec3(1.0f);
        vertices.push_back(v);
        indices.push_back(i);
    }
    
    _cubeMesh = CreateScope<Mesh>();
    _cubeMesh->Create(_renderer, vertices, indices);
}

bool Skybox::LoadFromFiles(const SkyboxFaces& faces) {
    if (!_context) return false;
    
    VkDevice device = _context->GetDevice();
    VmaAllocator allocator = _context->GetAllocator();
    
    const char* facePaths[6] = {
        faces.right.c_str(),
        faces.left.c_str(),
        faces.top.c_str(),
        faces.bottom.c_str(),
        faces.front.c_str(),
        faces.back.c_str()
    };
    
    int width = 0, height = 0, channels = 0;
    unsigned char* faceData[6] = {nullptr};
    
    for (int i = 0; i < 6; ++i) {
        faceData[i] = stbi_load(facePaths[i], &width, &height, &channels, 4);
        if (!faceData[i]) {
            for (int j = 0; j < i; ++j) {
                stbi_image_free(faceData[j]);
            }
            return false;
        }
    }
    
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
    imageInfo.extent = {(u32)width, (u32)height, 1};
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 6;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
    
    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    
    vmaCreateImage(allocator, &imageInfo, &allocInfo, &_cubemap, &_cubemapAllocation, nullptr);
    
    VkDeviceSize layerSize = width * height * 4;
    VkDeviceSize totalSize = layerSize * 6;
    
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = totalSize;
    bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    
    VmaAllocationCreateInfo stagingAllocInfo{};
    stagingAllocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
    
    VkBuffer stagingBuffer;
    VmaAllocation stagingAllocation;
    vmaCreateBuffer(allocator, &bufferInfo, &stagingAllocInfo, &stagingBuffer, &stagingAllocation, nullptr);
    
    void* mapped;
    vmaMapMemory(allocator, stagingAllocation, &mapped);
    for (int i = 0; i < 6; ++i) {
        memcpy((char*)mapped + i * layerSize, faceData[i], layerSize);
        stbi_image_free(faceData[i]);
    }
    vmaUnmapMemory(allocator, stagingAllocation);
    
    VkCommandBuffer cmd = _context->BeginSingleTimeCommands();
    
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = _cubemap;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 6;
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                         0, 0, nullptr, 0, nullptr, 1, &barrier);
    
    for (int i = 0; i < 6; ++i) {
        VkBufferImageCopy region{};
        region.bufferOffset = i * layerSize;
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = i;
        region.imageSubresource.layerCount = 1;
        region.imageExtent = {(u32)width, (u32)height, 1};
        
        vkCmdCopyBufferToImage(cmd, stagingBuffer, _cubemap, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
    }
    
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                         0, 0, nullptr, 0, nullptr, 1, &barrier);
    
    _context->EndSingleTimeCommands(cmd);
    
    vmaDestroyBuffer(allocator, stagingBuffer, stagingAllocation);
    
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = _cubemap;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
    viewInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 6;
    
    vkCreateImageView(device, &viewInfo, nullptr, &_cubemapView);
    
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    
    vkCreateSampler(device, &samplerInfo, nullptr, &_sampler);
    
    return true;
}

bool Skybox::LoadFromColor(const glm::vec3& top_color, const glm::vec3& bottom_color) {
    (void)top_color;
    (void)bottom_color;
    return false;
}

void Skybox::Render(VkCommandBuffer cmd, const glm::mat4& view_projection) {
    if (!_pipeline || !_cubeMesh) return;
    
    _pipeline->Bind(cmd);
    
    PushConstants push;
    push.model = glm::mat4(1.0f);
    push.view_projection = view_projection;
    _pipeline->SetPushConstants(cmd, push);
    
    _cubeMesh->Draw(cmd);
}

void Skybox::Destroy() {
    if (!_context) return;
    
    VkDevice device = _context->GetDevice();
    VmaAllocator allocator = _context->GetAllocator();
    
    if (_pipeline) {
        _pipeline->Destroy();
        _pipeline.reset();
    }
    if (_cubeMesh) {
        _cubeMesh->Destroy();
        _cubeMesh.reset();
    }
    if (_descriptorPool) {
        vkDestroyDescriptorPool(device, _descriptorPool, nullptr);
        _descriptorPool = VK_NULL_HANDLE;
    }
    if (_descriptorLayout) {
        vkDestroyDescriptorSetLayout(device, _descriptorLayout, nullptr);
        _descriptorLayout = VK_NULL_HANDLE;
    }
    if (_sampler) {
        vkDestroySampler(device, _sampler, nullptr);
        _sampler = VK_NULL_HANDLE;
    }
    if (_cubemapView) {
        vkDestroyImageView(device, _cubemapView, nullptr);
        _cubemapView = VK_NULL_HANDLE;
    }
    if (_cubemap) {
        vmaDestroyImage(allocator, _cubemap, _cubemapAllocation);
        _cubemap = VK_NULL_HANDLE;
    }
    
    _context = nullptr;
    _renderer = nullptr;
}

} // namespace tvk
