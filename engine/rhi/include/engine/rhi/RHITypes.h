#pragma once

#include "engine/core/BaseTypes.h"
#include <string>

namespace Engine::RHI {

enum class Format : Core::u8 {
    Unknown = 0,
    R8_UNORM,
    RG8_UNORM,
    RGBA8_UNORM,
    RGBA8_UNORM_SRGB,
    R16_FLOAT,
    RG16_FLOAT,
    RGBA16_FLOAT,
    R32_FLOAT,
    RG32_FLOAT,
    RGB32_FLOAT,
    RGBA32_FLOAT,
    D32_FLOAT,
    D24_UNORM_S8_UINT
};

enum class ResourceState : Core::u16 {
    Common = 0,
    VertexBuffer = 1 << 0,
    IndexBuffer = 1 << 1,
    ConstantBuffer = 1 << 2,
    RenderTarget = 1 << 3,
    DepthWrite = 1 << 4,
    DepthRead = 1 << 5,
    ShaderResource = 1 << 6,
    UnorderedAccess = 1 << 7,
    CopyDest = 1 << 8,
    CopySource = 1 << 9,
    IndirectArgument = 1 << 10,
    Present = 1 << 11
};

inline ResourceState operator|(ResourceState a, ResourceState b) noexcept {
    return static_cast<ResourceState>(static_cast<Core::u16>(a) | static_cast<Core::u16>(b));
}

inline bool operator&(ResourceState a, ResourceState b) noexcept {
    return (static_cast<Core::u16>(a) & static_cast<Core::u16>(b)) != 0;
}

enum class BufferUsage : Core::u8 {
    None = 0,
    Vertex = 1 << 0,
    Index = 1 << 1,
    Constant = 1 << 2,
    Structured = 1 << 3,
    Indirect = 1 << 4,
    Upload = 1 << 5
};

inline BufferUsage operator|(BufferUsage a, BufferUsage b) noexcept {
    return static_cast<BufferUsage>(static_cast<Core::u8>(a) | static_cast<Core::u8>(b));
}

inline bool operator&(BufferUsage a, BufferUsage b) noexcept {
    return (static_cast<Core::u8>(a) & static_cast<Core::u8>(b)) != 0;
}

enum class TextureUsage : Core::u8 {
    None = 0,
    ShaderResource = 1 << 0,
    RenderTarget = 1 << 1,
    DepthStencil = 1 << 2,
    UnorderedAccess = 1 << 3
};

inline TextureUsage operator|(TextureUsage a, TextureUsage b) noexcept {
    return static_cast<TextureUsage>(static_cast<Core::u8>(a) | static_cast<Core::u8>(b));
}

inline bool operator&(TextureUsage a, TextureUsage b) noexcept {
    return (static_cast<Core::u8>(a) & static_cast<Core::u8>(b)) != 0;
}

struct BufferDesc {
    Core::u64 sizeInBytes{0};
    BufferUsage usage{BufferUsage::None};
    std::string debugName;
};

struct TextureDesc {
    Core::u32 width{1};
    Core::u32 height{1};
    Core::u32 depth{1};
    Core::u32 mipLevels{1};
    Format format{Format::RGBA8_UNORM};
    TextureUsage usage{TextureUsage::ShaderResource};
    std::string debugName;
};

struct Viewport {
    float x{0.0f};
    float y{0.0f};
    float width{1280.0f};
    float height{720.0f};
    float minDepth{0.0f};
    float maxDepth{1.0f};
};

struct ScissorRect {
    Core::i32 left{0};
    Core::i32 top{0};
    Core::i32 right{1280};
    Core::i32 bottom{720};
};

} // namespace Engine::RHI
