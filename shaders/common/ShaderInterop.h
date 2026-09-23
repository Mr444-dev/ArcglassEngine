#pragma once

#ifdef __cplusplus
    #include "engine/core/BaseTypes.h"
    using float4x4 = struct { float m[16]; };
    using float4   = struct { float x, y, z, w; };
    using float3   = struct { float x, y, z; };
    using float2   = struct { float x, y; };
    using uint     = Engine::Core::u32;
#endif

// Shared structure between C++ Engine and HLSL Compute/Pixel Shaders
struct GPUInstanceData {
    float4x4 worldMatrix;
    float4x4 inverseWorldMatrix;
    float3   boundingCenter;
    float    boundingRadius;
    uint     materialIndex;
    uint     meshletOffset;
    uint     meshletCount;
    uint     flags;
};

struct GPULightData {
    float4 positionWS_Radius; // xyz = world pos, w = radius
    float4 color_Intensity;   // xyz = RGB color, w = intensity in lumens
    float4 direction_Cone;    // xyz = spot direction, w = inner/outer angle
    uint   lightType;         // 0 = Directional, 1 = Point, 2 = Spot
    uint   castShadows;
    uint   shadowMapIndex;
    uint   padding;
};
