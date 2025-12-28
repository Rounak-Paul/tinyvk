/**
 * @file buffer.cpp
 * @brief Buffer implementation
 */

#include "tinyvk/renderer/buffer.h"
#include "tinyvk/renderer/renderer.h"
#include "tinyvk/renderer/context.h"
#include "tinyvk/core/log.h"

#include <cstring>

namespace tvk {

Buffer::~Buffer() {
    Cleanup();
}

Buffer::Buffer(Buffer&& other) noexcept
    : m_Context(other.m_Context)
    , m_Buffer(other.m_Buffer)
    , m_Memory(other.m_Memory)
    , m_Size(other.m_Size)
    , m_Usage(other.m_Usage)
    , m_Mapped(other.m_Mapped) {
    other.m_Buffer = VK_NULL_HANDLE;
    other.m_Memory = VK_NULL_HANDLE;
    other.m_Mapped = nullptr;
}

Buffer& Buffer::operator=(Buffer&& other) noexcept {
    if (this != &other) {
        Cleanup();
        m_Context = other.m_Context;
        m_Buffer = other.m_Buffer;
        m_Memory = other.m_Memory;
        m_Size = other.m_Size;
        m_Usage = other.m_Usage;
        m_Mapped = other.m_Mapped;

        other.m_Buffer = VK_NULL_HANDLE;
        other.m_Memory = VK_NULL_HANDLE;
        other.m_Mapped = nullptr;
    }
    return *this;
}

Ref<Buffer> Buffer::Create(Renderer* renderer, VkDeviceSize size, BufferUsage usage, const void* data) {
    auto buffer = CreateRef<Buffer>();
    if (!buffer->Init(renderer, size, usage, data)) {
        return nullptr;
    }
    return buffer;
}

void Buffer::SetData(const void* data, VkDeviceSize size, VkDeviceSize offset) {
    if (!m_Context || !data) return;

    // For device local buffers, use staging
    VkMemoryPropertyFlags props = GetMemoryProperties(m_Usage);
    if (props & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT && !(props & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)) {
        // Create staging buffer
        VkBuffer stagingBuffer;
        VkDeviceMemory stagingMemory;

        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        vkCreateBuffer(m_Context->GetDevice(), &bufferInfo, nullptr, &stagingBuffer);

        VkMemoryRequirements memReqs;
        vkGetBufferMemoryRequirements(m_Context->GetDevice(), stagingBuffer, &memReqs);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memReqs.size;
        allocInfo.memoryTypeIndex = m_Context->FindMemoryType(
            memReqs.memoryTypeBits,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        );

        vkAllocateMemory(m_Context->GetDevice(), &allocInfo, nullptr, &stagingMemory);
        vkBindBufferMemory(m_Context->GetDevice(), stagingBuffer, stagingMemory, 0);

        void* mapped;
        vkMapMemory(m_Context->GetDevice(), stagingMemory, 0, size, 0, &mapped);
        memcpy(mapped, data, static_cast<size_t>(size));
        vkUnmapMemory(m_Context->GetDevice(), stagingMemory);

        // Copy to device local buffer
        VkCommandBuffer cmd = m_Context->BeginSingleTimeCommands();
        
        VkBufferCopy copyRegion{};
        copyRegion.srcOffset = 0;
        copyRegion.dstOffset = offset;
        copyRegion.size = size;
        vkCmdCopyBuffer(cmd, stagingBuffer, m_Buffer, 1, &copyRegion);
        
        m_Context->EndSingleTimeCommands(cmd);

        vkDestroyBuffer(m_Context->GetDevice(), stagingBuffer, nullptr);
        vkFreeMemory(m_Context->GetDevice(), stagingMemory, nullptr);
    } else {
        // Direct memory mapping
        void* mapped;
        vkMapMemory(m_Context->GetDevice(), m_Memory, offset, size, 0, &mapped);
        memcpy(mapped, data, static_cast<size_t>(size));
        vkUnmapMemory(m_Context->GetDevice(), m_Memory);
    }
}

void* Buffer::Map() {
    if (m_Mapped) return m_Mapped;
    
    vkMapMemory(m_Context->GetDevice(), m_Memory, 0, m_Size, 0, &m_Mapped);
    return m_Mapped;
}

void Buffer::Unmap() {
    if (!m_Mapped) return;
    
    vkUnmapMemory(m_Context->GetDevice(), m_Memory);
    m_Mapped = nullptr;
}

void Buffer::Flush(VkDeviceSize size, VkDeviceSize offset) {
    VkMappedMemoryRange mappedRange{};
    mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    mappedRange.memory = m_Memory;
    mappedRange.offset = offset;
    mappedRange.size = size;
    vkFlushMappedMemoryRanges(m_Context->GetDevice(), 1, &mappedRange);
}

void Buffer::BindAsVertex(VkCommandBuffer cmd, u32 binding) const {
    VkBuffer buffers[] = {m_Buffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(cmd, binding, 1, buffers, offsets);
}

void Buffer::BindAsIndex(VkCommandBuffer cmd, VkIndexType indexType) const {
    vkCmdBindIndexBuffer(cmd, m_Buffer, 0, indexType);
}

bool Buffer::Init(Renderer* renderer, VkDeviceSize size, BufferUsage usage, const void* data) {
    m_Context = &renderer->GetContext();
    m_Size = size;
    m_Usage = usage;

    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = ToVkUsage(usage);
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(m_Context->GetDevice(), &bufferInfo, nullptr, &m_Buffer) != VK_SUCCESS) {
        TVK_LOG_ERROR("Failed to create buffer");
        return false;
    }

    VkMemoryRequirements memReqs;
    vkGetBufferMemoryRequirements(m_Context->GetDevice(), m_Buffer, &memReqs);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memReqs.size;
    allocInfo.memoryTypeIndex = m_Context->FindMemoryType(
        memReqs.memoryTypeBits,
        GetMemoryProperties(usage)
    );

    if (vkAllocateMemory(m_Context->GetDevice(), &allocInfo, nullptr, &m_Memory) != VK_SUCCESS) {
        TVK_LOG_ERROR("Failed to allocate buffer memory");
        return false;
    }

    vkBindBufferMemory(m_Context->GetDevice(), m_Buffer, m_Memory, 0);

    if (data) {
        SetData(data, size);
    }

    return true;
}

void Buffer::Cleanup() {
    if (!m_Context) return;

    if (m_Mapped) {
        Unmap();
    }

    if (m_Buffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(m_Context->GetDevice(), m_Buffer, nullptr);
        m_Buffer = VK_NULL_HANDLE;
    }

    if (m_Memory != VK_NULL_HANDLE) {
        vkFreeMemory(m_Context->GetDevice(), m_Memory, nullptr);
        m_Memory = VK_NULL_HANDLE;
    }
}

VkBufferUsageFlags Buffer::ToVkUsage(BufferUsage usage) {
    switch (usage) {
        case BufferUsage::Vertex:
            return VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        case BufferUsage::Index:
            return VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        case BufferUsage::Uniform:
            return VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        case BufferUsage::Storage:
            return VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        case BufferUsage::StorageShared:
            return VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        case BufferUsage::Staging:
            return VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        default:
            return VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    }
}

VkMemoryPropertyFlags Buffer::GetMemoryProperties(BufferUsage usage) {
    switch (usage) {
        case BufferUsage::Vertex:
        case BufferUsage::Index:
        case BufferUsage::Storage:
            return VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        case BufferUsage::StorageShared:
        case BufferUsage::Uniform:
        case BufferUsage::Staging:
            return VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        default:
            return VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    }
}

} // namespace tvk
