// =========================================================================
// Meshlet Rasterizer: Amplification & Mesh Shaders (DirectX 12 SM 6.6)
// =========================================================================

struct MeshletInfo {
    uint vertexOffset;
    uint triangleOffset;
    uint vertexCount;
    uint triangleCount;
    float4 boundingSphere; // Center (xyz), Radius (w)
    float3 coneNormal;
    float coneCutoff;
};

struct MeshVertexInput {
    float3 position;
    float3 normal;
    float4 tangent;
    float2 uv;
};

struct VertexOutput {
    float4 positionCS : SV_Position;
    float3 normalWS : NORMAL;
    float2 uv : TEXCOORD0;
    uint meshletID : SV_RenderTargetArrayIndex;
};

struct MeshletPayload {
    uint meshletIndices[32];
    uint meshletCount;
};

struct FrameConstants {
    float4x4 viewProj;
    float3 cameraPos;
    float pad0;
};

ConstantBuffer<FrameConstants> FrameCB : register(b0);
StructuredBuffer<MeshletInfo> Meshlets : register(t0);
StructuredBuffer<MeshVertexInput> Vertices : register(t1);
StructuredBuffer<uint> Indices : register(t2);

// -------------------------------------------------------------
// Amplification Shader (Task Shader): Culls meshlet clusters
// -------------------------------------------------------------
[numthreads(32, 1, 1)]
void ASMain(uint threadID : SV_GroupThreadID, uint groupID : SV_GroupID) {
    uint meshletID = groupID * 32 + threadID;

    MeshletPayload payload;
    payload.meshletCount = 0;

    MeshletInfo m = Meshlets[meshletID];

    // Bounding sphere distance check
    float dist = distance(m.boundingSphere.xyz, FrameCB.cameraPos);
    bool visible = (dist < 500.0f); // Fast range culling

    if (visible) {
        uint index;
        InterlockedAdd(payload.meshletCount, 1, index);
        payload.meshletIndices[index] = meshletID;
    }

    GroupMemoryBarrierWithGroupSync();
    DispatchMesh(payload.meshletCount, 1, 1, payload);
}

// -------------------------------------------------------------
// Mesh Shader: Generates micro-triangles directly on GPU
// -------------------------------------------------------------
[outputtopology("triangle")]
[numthreads(32, 1, 1)]
void MSMain(
    uint gtid : SV_GroupThreadID,
    in payload MeshletPayload s_Payload,
    out vertices VertexOutput verts[64],
    out indices uint3 tris[126]
) {
    uint meshletID = s_Payload.meshletIndices[0];
    MeshletInfo m = Meshlets[meshletID];

    SetMeshOutputCounts(m.vertexCount, m.triangleCount);

    // Output vertices
    if (gtid < m.vertexCount) {
        uint vIdx = m.vertexOffset + gtid;
        MeshVertexInput v = Vertices[vIdx];

        VertexOutput vo;
        vo.positionCS = mul(float4(v.position, 1.0f), FrameCB.viewProj);
        vo.normalWS = v.normal;
        vo.uv = v.uv;
        vo.meshletID = meshletID;
        verts[gtid] = vo;
    }

    // Output triangles
    if (gtid < m.triangleCount) {
        uint tIdx = m.triangleOffset + gtid * 3;
        tris[gtid] = uint3(
            Indices[tIdx + 0] - m.vertexOffset,
            Indices[tIdx + 1] - m.vertexOffset,
            Indices[tIdx + 2] - m.vertexOffset
        );
    }
}

// -------------------------------------------------------------
// Pixel Shader: Visualizes meshlet clustering
// -------------------------------------------------------------
float4 PSMain(VertexOutput input) : SV_Target {
    // Unique color hash per meshlet
    uint id = input.meshletID;
    float3 color = float3(
        ((id * 17) % 255) / 255.0f,
        ((id * 43) % 255) / 255.0f,
        ((id * 97) % 255) / 255.0f
    );
    return float4(color, 1.0f);
}
