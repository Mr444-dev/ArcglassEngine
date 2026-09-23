#pragma once

#ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
    #define NOMINMAX
#endif
#include <windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>

#include "engine/rhi/RHITypes.h"
#include "engine/core/Assert.h"
#include "engine/core/Log.h"

using Microsoft::WRL::ComPtr;

#define DX_CHECK(hr) \
    do { \
        HRESULT _res = (hr); \
        if (FAILED(_res)) { \
            ENGINE_LOG_FATAL("DirectX 12 call failed with HRESULT: 0x{:08X} in {}:{}", static_cast<unsigned long>(_res), __FILE__, __LINE__); \
            ENGINE_DEBUG_BREAK(); \
        } \
    } while (false)

namespace Engine::RHI::D3D12 {

inline DXGI_FORMAT ConvertFormat(Format format) noexcept {
    switch (format) {
        case Format::R8_UNORM: return DXGI_FORMAT_R8_UNORM;
        case Format::RG8_UNORM: return DXGI_FORMAT_R8G8_UNORM;
        case Format::RGBA8_UNORM: return DXGI_FORMAT_R8G8B8A8_UNORM;
        case Format::RGBA8_UNORM_SRGB: return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        case Format::R16_FLOAT: return DXGI_FORMAT_R16_FLOAT;
        case Format::RG16_FLOAT: return DXGI_FORMAT_R16G16_FLOAT;
        case Format::RGBA16_FLOAT: return DXGI_FORMAT_R16G16B16A16_FLOAT;
        case Format::R32_FLOAT: return DXGI_FORMAT_R32_FLOAT;
        case Format::RG32_FLOAT: return DXGI_FORMAT_R32G32_FLOAT;
        case Format::RGB32_FLOAT: return DXGI_FORMAT_R32G32B32_FLOAT;
        case Format::RGBA32_FLOAT: return DXGI_FORMAT_R32G32B32A32_FLOAT;
        case Format::D32_FLOAT: return DXGI_FORMAT_D32_FLOAT;
        case Format::D24_UNORM_S8_UINT: return DXGI_FORMAT_D24_UNORM_S8_UINT;
        case Format::Unknown:
        default: return DXGI_FORMAT_UNKNOWN;
    }
}

inline D3D12_RESOURCE_STATES ConvertResourceState(ResourceState state) noexcept {
    D3D12_RESOURCE_STATES d3dState = D3D12_RESOURCE_STATE_COMMON;

    if (state & ResourceState::VertexBuffer)   d3dState |= D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
    if (state & ResourceState::IndexBuffer)    d3dState |= D3D12_RESOURCE_STATE_INDEX_BUFFER;
    if (state & ResourceState::ConstantBuffer) d3dState |= D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
    if (state & ResourceState::RenderTarget)   d3dState |= D3D12_RESOURCE_STATE_RENDER_TARGET;
    if (state & ResourceState::DepthWrite)     d3dState |= D3D12_RESOURCE_STATE_DEPTH_WRITE;
    if (state & ResourceState::DepthRead)      d3dState |= D3D12_RESOURCE_STATE_DEPTH_READ;
    if (state & ResourceState::ShaderResource) d3dState |= D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
    if (state & ResourceState::UnorderedAccess)d3dState |= D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    if (state & ResourceState::CopyDest)       d3dState |= D3D12_RESOURCE_STATE_COPY_DEST;
    if (state & ResourceState::CopySource)     d3dState |= D3D12_RESOURCE_STATE_COPY_SOURCE;
    if (state & ResourceState::IndirectArgument) d3dState |= D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT;
    if (state & ResourceState::Present)        d3dState |= D3D12_RESOURCE_STATE_PRESENT;

    return d3dState;
}

} // namespace Engine::RHI::D3D12
