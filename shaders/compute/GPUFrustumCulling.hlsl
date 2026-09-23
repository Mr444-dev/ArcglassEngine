// =========================================================================
// GPU Frustum Culling & LOD Selection Compute Shader
// Shader Model 6.6
// =========================================================================

struct GPUInstanceData {
    float4x4 worldMatrix;
    float3 aabbMin;
    float pad0;
    float3 aabbMax;
    float pad1;
    uint meshID;
    uint materialID;
    uint indexCount;
    uint startIndexLocation;
    int baseVertexLocation;
    uint pad2[3];
};

struct IndirectDrawCommand {
    uint indexCountPerInstance;
    uint instanceCount;
    uint startIndexLocation;
    int baseVertexLocation;
    uint startInstanceLocation;
    uint materialID;
    uint pad[2];
};

struct FrustumPlanes {
    float4 planes[6]; // Left, Right, Bottom, Top, Near, Far (xyz = normal, w = distance)
    float3 cameraPos;
    uint totalInstanceCount;
    float lod0Distance;
    float lod1Distance;
    float lod2Distance;
    float pad;
};

ConstantBuffer<FrustumPlanes> CullingParams : register(b0);
StructuredBuffer<GPUInstanceData> Instances : register(t0);

RWStructuredBuffer<IndirectDrawCommand> CulledCommands : register(u0);
RWStructuredBuffer<uint> DrawCount : register(u1);

bool IntersectsFrustum(float3 worldMin, float3 worldMax) {
    for (int i = 0; i < 6; ++i) {
        float4 plane = CullingParams.planes[i];
        float3 p = float3(
            (plane.x >= 0.0f) ? worldMax.x : worldMin.x,
            (plane.y >= 0.0f) ? worldMax.y : worldMin.y,
            (plane.z >= 0.0f) ? worldMax.z : worldMin.z
        );

        if (dot(plane.xyz, p) + plane.w < 0.0f) {
            return false;
        }
    }
    return true;
}

[numthreads(64, 1, 1)]
void CSMain(uint3 dispatchThreadID : SV_DispatchThreadID) {
    uint instanceIdx = dispatchThreadID.x;
    if (instanceIdx >= CullingParams.totalInstanceCount) {
        return;
    }

    GPUInstanceData instance = Instances[instanceIdx];

    // 1. Transform AABB bounds to World Space
    float3 center = (instance.aabbMin + instance.aabbMax) * 0.5f;
    float3 extents = (instance.aabbMax - instance.aabbMin) * 0.5f;

    float3 worldCenter = mul(float4(center, 1.0f), instance.worldMatrix).xyz;

    // Conservative world bounds
    float3 worldMin = worldCenter - extents;
    float3 worldMax = worldCenter + extents;

    // 2. Frustum Culling
    if (!IntersectsFrustum(worldMin, worldMax)) {
        return; // Culled!
    }

    // 3. Distance-based LOD selection
    float dist = distance(worldCenter, CullingParams.cameraPos);
    uint indexCount = instance.indexCount;
    if (dist > CullingParams.lod2Distance) {
        indexCount = instance.indexCount / 4; // LOD 2 (25% geometry)
    } else if (dist > CullingParams.lod1Distance) {
        indexCount = instance.indexCount / 2; // LOD 1 (50% geometry)
    }

    // 4. Atomic Counter Allocation
    uint destIdx;
    InterlockedAdd(DrawCount[0], 1, destIdx);

    // 5. Emit Indirect Draw Command
    IndirectDrawCommand cmd;
    cmd.indexCountPerInstance = indexCount;
    cmd.instanceCount = 1;
    cmd.startIndexLocation = instance.startIndexLocation;
    cmd.baseVertexLocation = instance.baseVertexLocation;
    cmd.startInstanceLocation = instanceIdx;
    cmd.materialID = instance.materialID;
    cmd.pad[0] = 0;
    cmd.pad[1] = 0;

    CulledCommands[destIdx] = cmd;
}
