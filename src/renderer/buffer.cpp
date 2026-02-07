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
    , m_Allocation(other.m_Allocation)
    , m_Size(other.m_Size)
    , m_Usage(other.m_Usage)
    , m_Mapped(other.m_Mapped) {
    other.m_Buffer = VK_NULL_HANDLE;
    other.m_Allocation = VK_NULL_HANDLE;
    other.m_Mapped = nullptr;
}

Buffer& Buffer::operator=(Buffer&& other) noexcept {
    if (this != &other) {
        Cleanup();
        m_Context = other.m_Context;
        m_Buffer = other.m_Buffer;
        m_Allocation = other.m_Allocation;
        m_Size = other.m_Size;
        m_Usage = other.m_Usage;
        m_Mapped = other.m_Mapped;

        other.m_Buffer = VK_NULL_HANDLE;
        other.m_Allocation = VK_NULL_HANDLE;
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

    VmaAllocationInfo allocInfo;
    vmaGetAllocationInfo(m_Context->GetAllocator(), m_Allocation, &allocInfo);

    VkMemoryPropertyFlags memFlags;
    vmaGetAllocationMemoryProperties(m_Context->GetAllocator(), m_Allocation, &memFlags);

    if (memFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {
        void* mapped;
        vmaMapMemory(m_Context->GetAllocator(), m_Allocation, &mapped);
        memcpy(static_cast<char*>(mapped) + offset, data, static_cast<size_t>(size));
        vmaUnmapMemory(m_Context->GetAllocator(), m_Allocation);
    } else {
        VkBufferCreateInfo stagingBufferInfo{};
        stagingBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        stagingBufferInfo.size = size;
        stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        stagingBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo stagingAllocInfo{};
        stagingAllocInfo.usage = VMA_MEMORY_USAGE_AUTO;
        stagingAllocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

        VkBuffer stagingBuffer;
        VmaAllocation stagingAllocation;
        VmaAllocationInfo stagingInfo;

        vmaCreateBuffer(m_Context->GetAllocator(), &stagingBufferInfo, &stagingAllocInfo, 
                       &stagingBuffer, &stagingAllocation, &stagingInfo);

        memcpy(stagingInfo.pMappedData, data, static_cast<size_t>(size));

        VkCommandBuffer cmd = m_Context->BeginSingleTimeCommands();
        
        VkBufferCopy copyRegion{};
        copyRegion.srcOffset = 0;
        copyRegion.dstOffset = offset;
        copyRegion.size = size;
        vkCmdCopyBuffer(cmd, stagingBuffer, m_Buffer, 1, &copyRegion);
        
        m_Context->EndSingleTimeCommands(cmd);

        vmaDestroyBuffer(m_Context->GetAllocator(), stagingBuffer, stagingAllocation);
    }
}

void* Buffer::Map() {
    if (m_Mapped) return m_Mapped;
    
    vmaMapMemory(m_Context->GetAllocator(), m_Allocation, &m_Mapped);
    return m_Mapped;
}

void Buffer::Unmap() {
    if (!m_Mapped) return;
    
    vmaUnmapMemory(m_Context->GetAllocator(), m_Allocation);
    m_Mapped = nullptr;
}

void Buffer::Flush(VkDeviceSize size, VkDeviceSize offset) {
    vmaFlushAllocation(m_Context->GetAllocator(), m_Allocation, offset, size);
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

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = GetVmaMemoryUsage(usage);

    if (usage == BufferUsage::Uniform || usage == BufferUsage::StorageShared || usage == BufferUsage::Staging || usage == BufferUsage::VertexDynamic) {
        allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
    }

    if (vmaCreateBuffer(m_Context->GetAllocator(), &bufferInfo, &allocInfo, 
                        &m_Buffer, &m_Allocation, nullptr) != VK_SUCCESS) {
        TVK_LOG_ERROR("Failed to create buffer with VMA");
        return false;
    }

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

    if (m_Buffer != VK_NULL_HANDLE && m_Allocation != VK_NULL_HANDLE) {
        vmaDestroyBuffer(m_Context->GetAllocator(), m_Buffer, m_Allocation);
        m_Buffer = VK_NULL_HANDLE;
        m_Allocation = VK_NULL_HANDLE;
    }
}

VkBufferUsageFlags Buffer::ToVkUsage(BufferUsage usage) {
    switch (usage) {
        case BufferUsage::Vertex:
        case BufferUsage::VertexDynamic:
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

VmaMemoryUsage Buffer::GetVmaMemoryUsage(BufferUsage usage) {
    switch (usage) {
        case BufferUsage::Vertex:
        case BufferUsage::Index:
        case BufferUsage::Storage:
            return VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
        case BufferUsage::VertexDynamic:
        case BufferUsage::StorageShared:
        case BufferUsage::Uniform:
        case BufferUsage::Staging:
            return VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
        default:
            return VMA_MEMORY_USAGE_AUTO;
    }
}

} // namespace tvk
