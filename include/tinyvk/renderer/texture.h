/**
 * @file texture.h
 * @brief Texture loading and management for TinyVK
 */

#pragma once

#include "../core/types.h"
#include <vulkan/vulkan.h>
#include <string>

namespace tvk {

// Forward declarations
class VulkanContext;
class Renderer;

/**
 * @brief Texture format options
 */
enum class TextureFormat {
    RGBA8,
    RGBA8_SRGB,
    BGRA8,
    BGRA8_SRGB,
    R8,
    RG8,
    RGB8,
    RGBA16F,
    RGBA32F,
    Depth24Stencil8,
    Depth32F
};

/**
 * @brief Texture filtering options
 */
enum class TextureFilter {
    Nearest,
    Linear
};

/**
 * @brief Texture wrap mode
 */
enum class TextureWrap {
    Repeat,
    MirroredRepeat,
    ClampToEdge,
    ClampToBorder
};

/**
 * @brief Texture creation specification
 */
struct TextureSpec {
    u32 width = 1;
    u32 height = 1;
    TextureFormat format = TextureFormat::RGBA8;
    TextureFilter minFilter = TextureFilter::Linear;
    TextureFilter magFilter = TextureFilter::Linear;
    TextureWrap wrapU = TextureWrap::Repeat;
    TextureWrap wrapV = TextureWrap::Repeat;
    bool generateMipmaps = true;
    bool useSampler = true;
};

/**
 * @brief 2D Texture class with Vulkan resources
 */
class Texture {
public:
    Texture() = default;
    ~Texture();

    // Non-copyable
    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;

    // Movable
    Texture(Texture&& other) noexcept;
    Texture& operator=(Texture&& other) noexcept;

    /**
     * @brief Create texture from file
     * @param renderer The renderer to use
     * @param filepath Path to image file (PNG, JPG, BMP, TGA, etc.)
     * @param spec Texture specification (filter, wrap mode, etc.)
     * @return Created texture or nullptr on failure
     */
    static Ref<Texture> LoadFromFile(Renderer* renderer, const std::string& filepath, const TextureSpec& spec = TextureSpec{});

    /**
     * @brief Create texture from memory
     * @param renderer The renderer to use  
     * @param data Pointer to RGBA pixel data
     * @param width Image width
     * @param height Image height
     * @param spec Texture specification
     * @return Created texture or nullptr on failure
     */
    static Ref<Texture> Create(Renderer* renderer, const void* data, u32 width, u32 height, const TextureSpec& spec = TextureSpec{});

    /**
     * @brief Create an empty texture
     * @param renderer The renderer to use
     * @param spec Texture specification with width/height
     * @return Created texture or nullptr on failure
     */
    static Ref<Texture> Create(Renderer* renderer, const TextureSpec& spec);

    /**
     * @brief Update texture data
     * @param data Pointer to new pixel data
     * @param width New width (must match if not resizing)
     * @param height New height (must match if not resizing)
     */
    void SetData(const void* data, u32 width, u32 height);

    /**
     * @brief Get ImGui texture ID for displaying in ImGui::Image
     */
    VkDescriptorSet GetImGuiTextureID() const { return m_ImGuiDescriptorSet; }

    /**
     * @brief Bind texture to ImGui for rendering
     * Must be called before using GetImGuiTextureID()
     */
    void BindToImGui();

    // Getters
    u32 GetWidth() const { return m_Width; }
    u32 GetHeight() const { return m_Height; }
    u32 GetMipLevels() const { return m_MipLevels; }
    u32 GetChannels() const { return 4; } // Always RGBA after loading
    VkImage GetImage() const { return m_Image; }
    VkImageView GetImageView() const { return m_ImageView; }
    VkSampler GetSampler() const { return m_Sampler; }
    VkFormat GetFormat() const { return m_Format; }
    const std::string& GetFilePath() const { return m_FilePath; }
    bool IsValid() const { return m_Image != VK_NULL_HANDLE; }

private:
    bool Init(Renderer* renderer, const void* data, const TextureSpec& spec);
    void Cleanup();
    void CreateImage(u32 width, u32 height, VkFormat format, VkImageTiling tiling,
                     VkImageUsageFlags usage, VkMemoryPropertyFlags properties);
    void CreateImageView(VkFormat format, VkImageAspectFlags aspectFlags);
    void CreateSampler(const TextureSpec& spec);
    void TransitionImageLayout(VkImage image, VkFormat format,
                               VkImageLayout oldLayout, VkImageLayout newLayout, u32 mipLevels);
    void CopyBufferToImage(VkBuffer buffer, VkImage image, u32 width, u32 height);
    void GenerateMipmaps(VkImage image, VkFormat format, u32 width, u32 height, u32 mipLevels);

    static VkFormat ToVkFormat(TextureFormat format);
    static VkFilter ToVkFilter(TextureFilter filter);
    static VkSamplerAddressMode ToVkWrap(TextureWrap wrap);

    Renderer* m_Renderer = nullptr;
    VulkanContext* m_Context = nullptr;

    VkImage m_Image = VK_NULL_HANDLE;
    VkDeviceMemory m_ImageMemory = VK_NULL_HANDLE;
    VkImageView m_ImageView = VK_NULL_HANDLE;
    VkSampler m_Sampler = VK_NULL_HANDLE;
    VkFormat m_Format = VK_FORMAT_R8G8B8A8_UNORM;

    VkDescriptorSet m_ImGuiDescriptorSet = VK_NULL_HANDLE;

    u32 m_Width = 0;
    u32 m_Height = 0;
    u32 m_MipLevels = 1;
    std::string m_FilePath;
};

} // namespace tvk
