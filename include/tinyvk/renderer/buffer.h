/**
 * @file buffer.h
 * @brief GPU buffer utilities for TinyVK
 */

#pragma once

#include "../core/types.h"
#include <vulkan/vulkan.h>
#include <vector>

namespace tvk {

// Forward declarations
class VulkanContext;
class Renderer;

/**
 * @brief Buffer usage types
 */
enum class BufferUsage {
    Vertex,
    Index,
    Uniform,
    Storage,
    StorageShared,
    Staging
};

/**
 * @brief GPU Buffer class
 */
class Buffer {
public:
    Buffer() = default;
    ~Buffer();

    // Non-copyable
    Buffer(const Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;

    // Movable
    Buffer(Buffer&& other) noexcept;
    Buffer& operator=(Buffer&& other) noexcept;

    /**
     * @brief Create a buffer
     * @param renderer The renderer
     * @param size Buffer size in bytes
     * @param usage Buffer usage type
     * @param data Optional initial data
     * @return Created buffer or nullptr
     */
    static Ref<Buffer> Create(Renderer* renderer, VkDeviceSize size, BufferUsage usage, const void* data = nullptr);

    /**
     * @brief Create a vertex buffer from data
     */
    template<typename T>
    static Ref<Buffer> CreateVertex(Renderer* renderer, const std::vector<T>& vertices) {
        return Create(renderer, sizeof(T) * vertices.size(), BufferUsage::Vertex, vertices.data());
    }

    /**
     * @brief Create an index buffer from data
     */
    template<typename T>
    static Ref<Buffer> CreateIndex(Renderer* renderer, const std::vector<T>& indices) {
        return Create(renderer, sizeof(T) * indices.size(), BufferUsage::Index, indices.data());
    }

    /**
     * @brief Create a uniform buffer
     */
    template<typename T>
    static Ref<Buffer> CreateUniform(Renderer* renderer) {
        return Create(renderer, sizeof(T), BufferUsage::Uniform, nullptr);
    }

    /**
     * @brief Update buffer data
     * @param data Pointer to data
     * @param size Size in bytes
     * @param offset Offset in buffer
     */
    void SetData(const void* data, VkDeviceSize size, VkDeviceSize offset = 0);

    /**
     * @brief Map buffer memory for direct access
     */
    void* Map();

    /**
     * @brief Unmap buffer memory
     */
    void Unmap();

    /**
     * @brief Flush mapped memory range
     */
    void Flush(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);

    // Getters
    VkBuffer GetBuffer() const { return m_Buffer; }
    VkDeviceSize GetSize() const { return m_Size; }
    BufferUsage GetUsage() const { return m_Usage; }
    bool IsMapped() const { return m_Mapped != nullptr; }

    /**
     * @brief Bind as vertex buffer
     */
    void BindAsVertex(VkCommandBuffer cmd, u32 binding = 0) const;

    /**
     * @brief Bind as index buffer
     */
    void BindAsIndex(VkCommandBuffer cmd, VkIndexType indexType = VK_INDEX_TYPE_UINT32) const;

private:
    bool Init(Renderer* renderer, VkDeviceSize size, BufferUsage usage, const void* data);
    void Cleanup();
    
    static VkBufferUsageFlags ToVkUsage(BufferUsage usage);
    static VkMemoryPropertyFlags GetMemoryProperties(BufferUsage usage);

    VulkanContext* m_Context = nullptr;
    VkBuffer m_Buffer = VK_NULL_HANDLE;
    VkDeviceMemory m_Memory = VK_NULL_HANDLE;
    VkDeviceSize m_Size = 0;
    BufferUsage m_Usage = BufferUsage::Vertex;
    void* m_Mapped = nullptr;
};

} // namespace tvk
